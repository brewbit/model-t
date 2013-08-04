
#include "ch.h"
#include "hal.h"

#include "wspr.h"
#include "datastream.h"
#include "wspr_parser.h"
#include "wspr_http.h"
#include "wspr_tcp.h"
#include "wspr_net.h"
#include "chprintf.h"

#include <stdlib.h>
#include <string.h>

static msg_t
thread_wspr(void* arg);

static void
recv_wspr_pkt(void* arg, wspr_msg_t req, uint8_t* data, uint16_t data_len);

static void
handle_debug(uint8_t* data, uint16_t data_len);

static void
handle_version(uint8_t* data, uint16_t data_len);

static void
wspr_idle(void);


static SerialDriver* sd = &SD4;
static wspr_parser_t wspr_parser;
static WORKING_AREA(wa_thread_wspr, 2500);
static wspr_msg_handler_t handlers[NUM_WSPR_MSGS] = {
    [WSPR_OUT_VERSION] = handle_version,
    [WSPR_OUT_DEBUG] = handle_debug,
};

void
wspr_init()
{
  wspr_tcp_init();
  wspr_http_init();
  wspr_net_init();

  sdStart(sd, NULL);
  wspr_parser_init(&wspr_parser, recv_wspr_pkt, NULL);

  chThdCreateStatic(wa_thread_wspr, sizeof(wa_thread_wspr), NORMALPRIO, thread_wspr, NULL);
}

void
wspr_set_handler(wspr_msg_t id, wspr_msg_handler_t handler)
{
  if (id < NUM_WSPR_MSGS)
    handlers[id] = handler;
}

static msg_t
thread_wspr(void* arg)
{
  (void)arg;

  chRegSetThreadName("wspr");

  while (1) {
    msg_t c = sdGetTimeout(sd, MS2ST(200));

    if (c >= 0)
      wspr_parse(&wspr_parser, c);
    else
      wspr_idle();
  }

  return 0;
}

static void
wspr_idle()
{
  wspr_tcp_idle();
}

void
wspr_send(wspr_msg_t id, uint8_t* data, uint16_t data_len)
{
  uint8_t* msg = NULL;
  uint16_t msg_len;
  uint8_t result = wspr_pack(id, data, data_len, &msg, &msg_len);

  if (result == 0)
    sdWrite(sd, msg, msg_len);

  if (msg)
    free(msg);
}

static void
handle_debug(uint8_t* data, uint16_t data_len)
{
  datastream_t* ds = ds_new(data, data_len);

  char* str = ds_read_str(ds);
//  terminal_write(str);

  free(str);
  ds_free(ds);
}

static void
handle_version(uint8_t* data, uint16_t data_len)
{
  (void)data;
  (void)data_len;
}

static void
recv_wspr_pkt(void* arg, wspr_msg_t id, uint8_t* data, uint16_t data_len)
{
  (void)arg;

  wspr_msg_handler_t handler = handlers[id];
  if (handler)
    handler(data, data_len);
  else
    chprintf((BaseChannel*)&SD3, "no handler for pkt id %d\r\n", id);
}
