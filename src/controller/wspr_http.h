#ifndef __WSPR_HTTP_H__
#define __WSPR_HTTP_H__

#include <stdint.h>

typedef enum {
  HTTP_GET,
  HTTP_POST,
  HTTP_HEAD,
  HTTP_PUT,
  HTTP_DELETE,
  HTTP_TRACE,
  HTTP_OPTIONS,
  HTTP_CONNECT,
  HTTP_PATCH,
  HTTP_END,
} http_method_t;

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
  http_method_t method;
} http_request_t;

typedef struct {
  int32_t result;
  uint32_t response_code;
  char* response_body;
} http_response_t;

typedef void (*http_response_handler_t)(void* arg, http_response_t* response);

void
wspr_http_init(void);

void
wspr_http_request(http_request_t* request, http_response_handler_t handler, void* handler_arg);

#endif
