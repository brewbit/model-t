
#include <ch.h>
#include <hal.h>

#include "bootloader.h"
#include "bootloader_api.h"
#include "sxfs.h"
#include "dfuse.h"
#include "app_hdr.h"
#include "crc/crc32.h"

#include <chprintf.h>
#include <string.h>


#define APP_FLASH_START       0x08004000
#define BOOTLOADER_FLASH_SIZE 0x4000 /* 16k */


typedef void (*app_entry_t)(void);

extern uint8_t __app_start__[];


static void
jump_to_app(uint32_t address);

static void
boot_app(void);

static void
write_app_img(sxfs_part_id_t part);

static void
process_boot_cmd(void);


__attribute__ ((section("bootloader_api")))
const bootloader_api_t _bootloader_api = {
    .get_version = bootloader_get_version,
};


void
bootloader_exec()
{
  process_boot_cmd();

  boot_app();

  /* Uh oh, we should have jumped to the app... */
  write_app_img(SP_RECOVERY_IMG);

  boot_app();

  chSysHalt();
}

static void
jump_to_app(uint32_t address)
{
  uint32_t jump_addr = *(volatile uint32_t*)(address + 0x04); /* reset ptr in vector table */
  app_entry_t app_entry = (app_entry_t)jump_addr;

  /* Reset all interrupts to default */
  chSysDisable();

  /* Clear pending interrupts just to be on the safe side */
  SCB_ICSR = ICSR_PENDSVCLR;

  /* Disable all interrupts */
  int i;
  for(i = 0; i < 8; ++i)
    NVIC->ICER[i] = NVIC->IABR[i];

  /* Set stack pointer as in application's vector table */
  __set_MSP(*(volatile uint32_t*)address);
  app_entry();
}

static void
process_boot_cmd()
{
  boot_cmd_t boot_cmd;
  sxfs_read(SP_BOOT_PARAMS, 0, (uint8_t*)&boot_cmd, sizeof(boot_cmd));

  switch (boot_cmd) {
    case BOOT_LOAD_RECOVERY_IMG:
      write_app_img(SP_RECOVERY_IMG);
      break;

    case BOOT_LOAD_UPDATE_IMG:
      write_app_img(SP_UPDATE_IMG);
      break;

    case BOOT_DEFAULT:
    default:
      break;
  }

  if (boot_cmd != BOOT_DEFAULT) {
    boot_cmd = BOOT_DEFAULT;
    sxfs_erase_all(SP_BOOT_PARAMS);
    sxfs_write(SP_BOOT_PARAMS, 0, (uint8_t*)&boot_cmd, sizeof(boot_cmd));
  }
}

static void
boot_app()
{
  if (memcmp((const void*)_app_hdr.magic, "BBMT-APP", 8) == 0) {
    uint32_t crc_calc = crc32_block(0xffffffff, __app_start__, _app_hdr.img_size) ^ 0xffffffff;

    if (crc_calc == _app_hdr.crc) {
      chThdSleepMilliseconds(100);
      jump_to_app((uint32_t)__app_start__);
    }
  }
}

static void
write_app_img(sxfs_part_id_t part)
{
  /* Disallow overwriting of the bootloader */
  addr_range_t valid_addr_range = {
      .start = APP_FLASH_START,
      .end = APP_FLASH_START + board_get_flash_size() - BOOTLOADER_FLASH_SIZE - 1
  };

  dfuse_apply_update(part, &valid_addr_range);
}

const char*
bootloader_get_version()
{
  return VERSION_STR;
}
