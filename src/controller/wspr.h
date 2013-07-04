#ifndef __WSPR_H__
#define __WSPR_H__

#include "wspr_parser.h"
#include "wspr_tcp.h"
#include "wspr_http.h"

#define mkip(a,b,c,d) ((a) << 24) | ((b) << 16) | ((c) << 8) | (d)

typedef void (*wspr_msg_handler_t)(uint8_t* data, uint16_t data_len);

void
wspr_init(void);

void
wspr_send(wspr_msg_t id, uint8_t* data, uint16_t data_len);

void
wspr_set_handler(wspr_msg_t id, wspr_msg_handler_t handler);

#endif
