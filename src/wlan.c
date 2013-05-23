
#include "ch.h"
#include "hal.h"

#include "wlan.h"
#include "datastream.h"
#include "wspr_parser.h"
#include "terminal.h"

#include <stdlib.h>
#include <string.h>

static msg_t
thread_wlan(void* arg);

static void
send_wspr_pkt(wspr_msg_t id, uint8_t* data, uint16_t data_len);

static void
recv_wspr_pkt(void* arg, wspr_msg_t req, uint8_t* data, uint16_t data_len);

static void
tcp_connect(uint32_t ip, uint16_t port);

static void
tcp_send(uint16_t handle, uint8_t* data, uint16_t data_len);

static void
handle_tcp_connect_result(uint8_t* data, uint16_t data_len);

static void
handle_tcp_send_result(uint8_t* data, uint16_t data_len);

SerialDriver* sd = &SD4;
wspr_parser_t wspr_parser;
uint8_t wa_thread_wlan[2500];

void
wlan_init()
{
  sdStart(sd, NULL);
  wspr_parser_init(&wspr_parser, recv_wspr_pkt, NULL);

  chThdCreateStatic(wa_thread_wlan, sizeof(wa_thread_wlan), NORMALPRIO, thread_wlan, NULL);
}

uint8_t send_buf[1024];

static void
tcp_connect(uint32_t ip, uint16_t port)
{
  datastream_t* ds = ds_new(NULL, 1024);

  ds_write_u32(ds, ip);
  ds_write_u16(ds, port);

  send_wspr_pkt(WSPR_IN_TCP_CONNECT, ds->buf, ds_index(ds));

  ds_free(ds);
}

static void
tcp_send(uint16_t handle, uint8_t* data, uint16_t data_len)
{
  datastream_t* ds = ds_new(NULL, 1024);

  ds_write_u16(ds, handle);
  ds_write_buf(ds, data, data_len);

  send_wspr_pkt(WSPR_IN_TCP_SEND, ds->buf, ds_index(ds));

  ds_free(ds);
}

static msg_t
thread_wlan(void* arg)
{
  (void)arg;

  chRegSetThreadName("wlan");

  chThdSleepSeconds(1);
  send_wspr_pkt(WSPR_IN_VERSION, NULL, 0);

  chThdSleepSeconds(15);
  tcp_connect(mkip(192, 168, 1, 146), 4392);

  while (1) {
    uint8_t c = sdGet(sd);
    wspr_parse(&wspr_parser, c);
  }

  return 0;
}

static void
send_wspr_pkt(wspr_msg_t id, uint8_t* data, uint16_t data_len)
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
handle_tcp_connect_result(uint8_t* data, uint16_t data_len)
{
  datastream_t* ds = ds_new(data, data_len);
  int32_t result = ds_read_s32(ds);
  uint16_t handle = ds_read_u16(ds);
  ds_free(ds);

  terminal_write("tcp connection ");
  if (result == 0) {
    terminal_write("succeeded! handle: ");
    terminal_write_int(handle);

    terminal_write("\nsending message... ");

    char* str = "hello from the temp controller!";
    tcp_send(0, (uint8_t*)str, (uint16_t)strlen(str));
  }
  else {
    terminal_write("failed");
  }
}

static void
handle_tcp_send_result(uint8_t* data, uint16_t data_len)
{
  datastream_t* ds = ds_new(data, data_len);
  int32_t result = ds_read_s32(ds);
  ds_free(ds);

  if (result == 0) {
    terminal_write("succeeded!\n");
  }
  else {
    terminal_write("failed :(\n");
  }
}

static void
wspr_debug(uint8_t* data, uint16_t data_len)
{
  datastream_t* ds = ds_new(data, data_len);

  char* str = ds_read_str(ds);
  terminal_write(str);

  free(str);
  ds_free(ds);
}

static void
recv_wspr_pkt(void* arg, wspr_msg_t req, uint8_t* data, uint16_t data_len)
{
  (void)arg;

  switch (req) {
    case WSPR_OUT_VERSION:
      terminal_write("WSPR version: ");
      terminal_write_int(data[0]);
      terminal_write(".");
      terminal_write_int(data[1]);
      terminal_write(".");
      terminal_write_int(data[2]);
      terminal_write("\n");
      break;

    case WSPR_OUT_TCP_CONNECT:
      handle_tcp_connect_result(data, data_len);
      break;

    case WSPR_OUT_TCP_SEND:
      handle_tcp_send_result(data, data_len);
      break;

    case WSPR_OUT_DEBUG:
      wspr_debug(data, data_len);
      break;

    default:
      break;
  }
}
