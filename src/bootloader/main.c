
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

  /* start stdout port */
  sdStart(SD_STDIO, NULL);

  bootloader_exec();

  return -1;
}
