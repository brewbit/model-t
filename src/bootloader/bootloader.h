
#ifndef BOOTLOADER_H
#define BOOTLOADER_H

void
bootloader_exec(void);

const char*
bootloader_get_version(void);

void
bootloader_load_recovery_img(void);

void
bootloader_load_update_img(void);

#endif
