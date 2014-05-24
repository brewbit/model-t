
#include <ch.h>
#include <hal.h>

#include "bootloader_api.h"
#include "sxfs.h"

static void
save_boot_cmd(boot_cmd_t boot_cmd)
{
  sxfs_erase_all(SP_BOOT_PARAMS);
  sxfs_write(SP_BOOT_PARAMS, 0, (uint8_t*)&boot_cmd, sizeof(boot_cmd));

  NVIC_SystemReset();
}

void
bootloader_load_recovery_img()
{
  save_boot_cmd(BOOT_LOAD_RECOVERY_IMG);
}

void
bootloader_load_update_img()
{
  save_boot_cmd(BOOT_LOAD_UPDATE_IMG);
}
