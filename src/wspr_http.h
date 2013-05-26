#ifndef __WSPR_HTTP_H__
#define __WSPR_HTTP_H__

#include <stdint.h>

typedef struct {
  char* name;
  char* value;
} http_header_t;

typedef struct {
  char* host;
  uint16_t port;
  char* url;
  uint8_t num_headers;
  http_header_t* headers;
  char* request;
} http_get_request_t;

typedef struct {
  int32_t result;
  uint32_t response_code;
  char* response_body;
} http_get_response_t;

typedef void (*wspr_http_get_handler_t)(void* arg, http_get_response_t* response);

void
wspr_http_init(void);

void
wspr_http_get(http_get_request_t* request, wspr_http_get_handler_t handler, void* handler_arg);

#endif
