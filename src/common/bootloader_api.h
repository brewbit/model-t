
#ifndef BOOTLOADER_H
#define BOOTLOADER_H

typedef struct {
  const char* (*get_version)(void);
} bootloader_api_t;

extern const bootloader_api_t _bootloader_api;

#endif
