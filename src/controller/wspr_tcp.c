
#include "ch.h"
#include "hal.h"

#include "wspr.h"
#include "wspr_tcp.h"
#include "datastream.h"
#include "wspr_parser.h"
#include "txn.h"
#include "common.h"
#include "linked_list.h"

#include <stdlib.h>
#include <string.h>


typedef struct {
  int32_t result;
  uint16_t handle;
} tcp_connect_response_t;

typedef struct {
  Semaphore sem_connect;
  int32_t result;
  uint16_t handle;
} tcp_stream_setup_t;


static size_t tcp_write(void *instance, const uint8_t *bp, size_t n);
static size_t tcp_read(void *instance, uint8_t *bp, size_t n);
static bool_t tcp_putwouldblock(void *instance);
static bool_t tcp_getwouldblock(void *instance);
static msg_t tcp_put(void *instance, uint8_t b, systime_t time);
static msg_t tcp_get(void *instance, systime_t time);
static size_t tcp_writet(void *instance, const uint8_t *bp, size_t n, systime_t time);
static size_t tcp_readt(void *instance, uint8_t *bp, size_t n, systime_t time);
static void tcp_stream_pump(tcp_stream_t* s);
static tcp_stream_t* find_stream(uint16_t handle);
static void tcp_connect_complete(tcp_stream_setup_t* ss, tcp_connect_response_t* response_data);
static void handle_tcp_connect_result(uint8_t* data, uint16_t data_len);
static void handle_tcp_send_result(uint8_t* data, uint16_t data_len);
static void handle_tcp_recv(uint8_t* data, uint16_t data_len);


const struct BaseChannelVMT tcp_vmt = {
    .read          = tcp_read,
    .write         = tcp_write,
    .putwouldblock = tcp_putwouldblock,
    .getwouldblock = tcp_getwouldblock,
    .put           = tcp_put,
    .get           = tcp_get,
    .writet        = tcp_writet,
    .readt         = tcp_readt
};

static txn_cache_t* tcp_txns;
static linked_list_t* conn_list;


void
wspr_tcp_init()
{
  tcp_txns = txn_cache_new();
  conn_list = linked_list_new();

//  wspr_set_handler(WSPR_OUT_TCP_LISTEN, handle_tcp_listen_result);
//  wspr_set_handler(WSPR_OUT_TCP_ACCEPTED, handle_tcp_accepted_result);
  wspr_set_handler(WSPR_OUT_TCP_CONNECT, handle_tcp_connect_result);
  wspr_set_handler(WSPR_OUT_TCP_SEND, handle_tcp_send_result);
  wspr_set_handler(WSPR_OUT_TCP_RECV, handle_tcp_recv);
//  wspr_set_handler(WSPR_OUT_TCP_CLOSE, handle_tcp_close_result);
}

void
wspr_tcp_idle()
{
  linked_list_node_t* node;
  for (node = conn_list->head; node != NULL; node = node->next) {
    tcp_stream_t* s = node->data;
    tcp_stream_pump(s);
  }
}

static void
tcp_stream_pump(tcp_stream_t* s)
{
  // send any data that has been queued up
  if (!chOQIsEmptyI(&s->out)) {
    int i;
    datastream_t* ds = ds_new(NULL, 1024);

    ds_write_u16(ds, s->handle);
    uint16_t n = MIN(chOQGetFullI(&s->out), 256);
    ds_write_u16(ds, n);
    for (i = 0; i < n; ++i) {
      uint8_t b = chOQGetI(&s->out);
      ds_write_u8(ds, b);
    }

    wspr_send(WSPR_IN_TCP_SEND, ds->buf, ds_index(ds));

    ds_free(ds);
  }

  // request more recv data if it has all been processed
  if (chIQIsEmptyI(&s->in)) {
    datastream_t* ds = ds_new(NULL, 1024);

    ds_write_u16(ds, s->handle);

    wspr_send(WSPR_IN_TCP_RECV, ds->buf, ds_index(ds));

    ds_free(ds);
  }
}

BaseChannel*
wspr_tcp_connect(uint32_t ip, uint16_t port)
{
  tcp_stream_setup_t ss;
  chSemInit(&ss.sem_connect, 0);


  txn_t* txn = txn_new(tcp_txns, (txn_callback_t)tcp_connect_complete, &ss);

  datastream_t* ds = ds_new(NULL, 1024);

  ds_write_u32(ds, txn->txn_id);
  ds_write_u32(ds, ip);
  ds_write_u16(ds, port);

  wspr_send(WSPR_IN_TCP_CONNECT, ds->buf, ds_index(ds));

  ds_free(ds);

  chSemWait(&ss.sem_connect);

  tcp_stream_t* tcp_stream = NULL;
  if (ss.result == 0) {
    tcp_stream = calloc(1, sizeof(tcp_stream_t));
    tcp_stream->channel.vmt = &tcp_vmt;
    chIQInit(&tcp_stream->in, tcp_stream->in_buf, sizeof(tcp_stream->in_buf), NULL);
    chOQInit(&tcp_stream->out, tcp_stream->out_buf, sizeof(tcp_stream->out_buf), NULL);
    linked_list_append(conn_list, tcp_stream);
  }

  return (BaseChannel*)tcp_stream;
}

static void
tcp_connect_complete(tcp_stream_setup_t* ss, tcp_connect_response_t* response)
{
  ss->handle = response->handle;
  ss->result = response->result;
  chSemSignal(&ss->sem_connect);
}

static size_t
tcp_write(void *instance, const uint8_t *bp, size_t n)
{
  return tcp_writet(instance, bp, n, TIME_INFINITE);
}

static size_t
tcp_read(void *instance, uint8_t *bp, size_t n)
{
  return tcp_readt(instance, bp, n, TIME_INFINITE);
}

static bool_t
tcp_putwouldblock(void *instance)
{
  tcp_stream_t* s = instance;
  return chOQIsFullI(&s->out);
}

static bool_t
tcp_getwouldblock(void *instance)
{
  tcp_stream_t* s = instance;
  return chIQIsEmptyI(&s->in);
}

static msg_t
tcp_put(void *instance, uint8_t b, systime_t time)
{
  tcp_stream_t* s = instance;
  return chOQPutTimeout(&s->out, b, time);
}

static msg_t
tcp_get(void *instance, systime_t time)
{
  tcp_stream_t* s = instance;
  return chIQGetTimeout(&s->in, time);
}

static size_t
tcp_writet(void *instance, const uint8_t *bp, size_t n, systime_t time)
{
  tcp_stream_t* s = instance;
  return chOQWriteTimeout(&s->out, bp, n, time);
}

static size_t
tcp_readt(void *instance, uint8_t *bp, size_t n, systime_t time)
{
  tcp_stream_t* s = instance;
  return chIQReadTimeout(&s->in, bp, n, time);
}

static void
handle_tcp_connect_result(uint8_t* data, uint16_t data_len)
{
  tcp_connect_response_t response;

  datastream_t* ds = ds_new(data, data_len);

  uint32_t txn_id = ds_read_u32(ds);
  response.result = ds_read_s32(ds);
  response.handle = ds_read_u16(ds);

  ds_free(ds);

  txn_complete(tcp_txns, txn_id, &response);
}

static void
handle_tcp_send_result(uint8_t* data, uint16_t data_len)
{
  datastream_t* ds = ds_new(data, data_len);
  int32_t result = ds_read_s32(ds);
  (void)result;
  ds_free(ds);
}

static void
handle_tcp_recv(uint8_t* data, uint16_t data_len)
{
  uint8_t* recv_data;
  uint16_t recv_len;

  datastream_t* ds = ds_new(data, data_len);

  uint16_t handle = ds_read_u16(ds);
  ds_read_buf(ds, &recv_data, &recv_len);

  tcp_stream_t* s = find_stream(handle);
  if (s != NULL) {
    int i;
    for (i = 0; i < recv_len; ++i) {
      chIQPutI(&s->in, recv_data[i]);
    }
  }

  ds_free(ds);
}

static tcp_stream_t*
find_stream(uint16_t handle)
{
  linked_list_node_t* node;
  for (node = conn_list->head; node != NULL; node = node->next) {
    tcp_stream_t* s = node->data;
    if (s->handle == handle)
      return s;
  }

  return NULL;
}
