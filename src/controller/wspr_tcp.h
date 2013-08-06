#ifndef __WSPR_TCP_H__
#define __WSPR_TCP_H__

#include "wspr.h"
#include "common.h"

#include <chioch.h>


#define IP_ADDR(a, b, c, d) ((((uint32_t) a) << 24) | (((uint32_t) b) << 16) | (((uint32_t) c) << 8) | ((uint32_t) d))


typedef struct {
  BaseChannel channel;
  uint16_t handle;
  InputQueue in;
  OutputQueue out;
  uint8_t out_buf[512];
  uint8_t in_buf[512];
  Semaphore connected;
  VirtualTimer send_timer;
} tcp_stream_t;


typedef void (*wspr_tcp_connect_handler_t)(BaseChannel* tcp_channel);


void
wspr_tcp_init(void);

void
wspr_tcp_idle(void);

bool
wspr_tcp_connect(uint32_t ip, uint16_t port, wspr_tcp_connect_handler_t connect_handler);

#endif
