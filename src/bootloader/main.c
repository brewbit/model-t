
#include "ch.h"
#include "hal.h"
#include "xflash.h"
#include "bootloader.h"


int
main(void)
{
  halInit();
  chSysInit();
  xflash_init();
  bootloader_exec();
  return -1;
}
