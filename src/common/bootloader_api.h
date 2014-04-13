
#ifndef BOOTLOADER_API_H
#define BOOTLOADER_API_H

typedef enum {
  BOOT_DEFAULT,
  BOOT_LOAD_RECOVERY_IMG,
  BOOT_LOAD_UPDATE_IMG,
} boot_cmd_t;

typedef struct {
  const char* (*get_version)(void);
} bootloader_api_t;

extern const bootloader_api_t _bootloader_api;

void
bootloader_load_recovery_img(void);

void
bootloader_load_update_img(void);

#endif
