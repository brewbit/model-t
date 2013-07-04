#ifndef __WSPR_TCP_H__
#define __WSPR_TCP_H__

#include "wspr.h"

void
wspr_tcp_init(void);

void
tcp_connect(uint32_t ip, uint16_t port);

void
tcp_send(uint16_t handle, uint8_t* data, uint16_t data_len);

#endif
