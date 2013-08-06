
#ifndef WSPR_PARSER_H
#define WSPR_PARSER_H

#include <stdint.h>

#define WSPR_SYNC1_CHAR 0x16
#define WSPR_SYNC2_CHAR 0x16

#define MAX_DATA 1024

typedef enum {
  /* WiFi module input messages */
  WSPR_IN_START,
  WSPR_IN_VERSION = WSPR_IN_START,
  WSPR_IN_STATUS,
  WSPR_IN_SCAN,
  WSPR_IN_CFG_STA,
  WSPR_IN_HTTP_REQUEST,
  WSPR_IN_TCP_CONNECT,
  WSPR_IN_TCP_LISTEN,
  WSPR_IN_TCP_SEND,
  WSPR_IN_TCP_RECV,
  WSPR_IN_TCP_CLOSE,
  WSPR_IN_END = WSPR_IN_TCP_CLOSE,

  NUM_WSPR_IN_MSGS = (WSPR_IN_END - WSPR_IN_START + 1),

  /* WiFi module output messages */
  WSPR_OUT_START,
  WSPR_OUT_VERSION = WSPR_OUT_START,
  WSPR_OUT_DEBUG,
  WSPR_OUT_STATUS,
  WSPR_OUT_SCAN_RESULT,
  WSPR_OUT_SCAN_COMPLETE,
  WSPR_OUT_HTTP_RESPONSE_HEADER,
  WSPR_OUT_HTTP_RESPONSE_BODY,
  WSPR_OUT_HTTP_RESPONSE_COMPLETE,
  WSPR_OUT_TCP_CONNECT,
  WSPR_OUT_TCP_LISTEN,
  WSPR_OUT_TCP_ACCEPTED,
  WSPR_OUT_TCP_SEND,
  WSPR_OUT_TCP_RECV,
  WSPR_OUT_TCP_CLOSE,
  WSPR_OUT_END = WSPR_OUT_TCP_CLOSE,

  NUM_WSPR_OUT_MSGS = (WSPR_OUT_END - WSPR_OUT_START + 1),

  NUM_WSPR_MSGS = (NUM_WSPR_IN_MSGS + NUM_WSPR_OUT_MSGS)
} wspr_msg_t;

typedef enum {
  PS_SYNC1,
  PS_SYNC2,
  PS_ID,
  PS_LEN1,
  PS_LEN2,
  PS_DATA,
  PS_CKSUM
} parser_state_t;

typedef void (*wspr_pkt_handler)(void* arg, wspr_msg_t req, uint8_t* data, uint16_t data_len);

typedef struct {
  parser_state_t state;

  wspr_msg_t id;
  uint16_t data_len;
  uint16_t data_rcvd;
  uint8_t data[MAX_DATA];
  uint8_t cksum;

  wspr_pkt_handler handler;
  void* handler_arg;
} wspr_parser_t;

void
wspr_parser_init(wspr_parser_t* parser,
    wspr_pkt_handler handler,
    void* handler_arg);

void
wspr_parse(wspr_parser_t* p, uint8_t c);

#endif
