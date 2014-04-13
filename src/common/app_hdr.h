
#ifndef APP_HDR_H
#define APP_HDR_H

#include <stdint.h>

typedef struct {
  char magic[8];
  uint32_t major_version;
  uint32_t minor_version;
  uint32_t patch_version;
  uint32_t img_size;
  uint32_t crc;
} app_hdr_t;

extern volatile const app_hdr_t _app_hdr;

#endif
