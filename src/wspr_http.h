#ifndef __WSPR_HTTP_H__
#define __WSPR_HTTP_H__

typedef struct {
  char* name;
  char* value;
} http_header_t;

void
wspr_http_init(void);

void
wspr_http_get(char* host, uint16_t port, http_header_t* headers, uint8_t num_headers, char* url, char* request);

#endif
