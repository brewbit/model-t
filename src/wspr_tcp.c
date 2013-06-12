
#include "ch.h"
#include "hal.h"

#include "wspr.h"
#include "wspr_tcp.h"
#include "datastream.h"
#include "wspr_parser.h"

#include <stdlib.h>
#include <string.h>

static void
handle_tcp_connect_result(uint8_t* data, uint16_t data_len);

static void
handle_tcp_send_result(uint8_t* data, uint16_t data_len);

void
wspr_tcp_init()
{
//  wspr_set_handler(WSPR_OUT_TCP_LISTEN, handle_tcp_listen_result);
//  wspr_set_handler(WSPR_OUT_TCP_ACCEPTED, handle_tcp_accepted_result);
  wspr_set_handler(WSPR_OUT_TCP_CONNECT, handle_tcp_connect_result);
  wspr_set_handler(WSPR_OUT_TCP_SEND, handle_tcp_send_result);
//  wspr_set_handler(WSPR_OUT_TCP_RECV, handle_tcp_recv_result);
//  wspr_set_handler(WSPR_OUT_TCP_CLOSE, handle_tcp_close_result);
}

void
tcp_connect(uint32_t ip, uint16_t port)
{
  datastream_t* ds = ds_new(NULL, 1024);

  ds_write_u32(ds, ip);
  ds_write_u16(ds, port);

  wspr_send(WSPR_IN_TCP_CONNECT, ds->buf, ds_index(ds));

  ds_free(ds);
}

void
tcp_send(uint16_t handle, uint8_t* data, uint16_t data_len)
{
  datastream_t* ds = ds_new(NULL, 1024);

  ds_write_u16(ds, handle);
  ds_write_buf(ds, data, data_len);

  wspr_send(WSPR_IN_TCP_SEND, ds->buf, ds_index(ds));

  ds_free(ds);
}

static void
handle_tcp_connect_result(uint8_t* data, uint16_t data_len)
{
  datastream_t* ds = ds_new(data, data_len);
  int32_t result = ds_read_s32(ds);
  uint16_t handle = ds_read_u16(ds);
  ds_free(ds);

  if (result == 0) {
    char* str = "hello from the temp controller!";
    tcp_send(0, (uint8_t*)str, (uint16_t)strlen(str));
  }
}

static void
handle_tcp_send_result(uint8_t* data, uint16_t data_len)
{
  datastream_t* ds = ds_new(data, data_len);
  int32_t result = ds_read_s32(ds);
  ds_free(ds);
}
