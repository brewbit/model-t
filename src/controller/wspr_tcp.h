#ifndef __WSPR_TCP_H__
#define __WSPR_TCP_H__

#include "wspr.h"

#include <chioch.h>


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


void
wspr_tcp_init(void);

void
wspr_tcp_idle(void);

BaseChannel*
wspr_tcp_connect(uint32_t ip, uint16_t port);

#endif
