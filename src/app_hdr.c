
#ifndef APP_HDR_H
#define APP_HDR_H

#include <stdint.h>

typedef struct {
  uint32_t magic;
} app_hdr_t;


__attribute__ ((section("app_hdr")))
app_hdr_t _app_hdr = {
    .magic = 0xDEADBEEF
};

#endif
