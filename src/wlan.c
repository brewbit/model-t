
#include "ch.h"
#include "hal.h"

#include "wlan.h"
#include "datastream.h"
#include "wspr_parser.h"
#include "terminal.h"

#include <stdlib.h>

static msg_t
thread_wlan(void* arg);

static void
send_wspr_pkt(wspr_msg_t id, uint8_t* data, uint16_t data_len);

static void
recv_wspr_pkt(void* arg, wspr_msg_t req, uint8_t* data, uint16_t data_len);

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

static msg_t
thread_wlan(void* arg)
{
  (void)arg;

  chRegSetThreadName("wlan");

  chThdSleepMilliseconds(1000);
  send_wspr_pkt(WSPR_IN_VERSION, NULL, 0);

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
recv_wspr_pkt(void* arg, wspr_msg_t req, uint8_t* data, uint16_t data_len)
{
  (void)arg;
  (void)data;
  (void)data_len;

  switch (req) {
  case WSPR_OUT_VERSION:
    terminal_write("WSPR version: ");
    terminal_write_int(data[0]);
    terminal_write(".");
    terminal_write_int(data[1]);
    terminal_write(".");
    terminal_write_int(data[2]);
    break;
  default:
    break;
  }
}
