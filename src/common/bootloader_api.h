
#ifndef BOOTLOADER_API_H
#define BOOTLOADER_API_H

typedef struct {
  const char* (*get_version)(void);
  void (*load_recovery_img)(void);
  void (*load_update_img)(void);
} bootloader_api_t;

extern const bootloader_api_t _bootloader_api;

#endif
