
#include "ch.h"
#include "hal.h"
#include "xflash.h"


int
main(void)
{
  halInit();
  chSysInit();
  xflash_init();

  /* start stdout port */
  sdStart(SD_STDIO, NULL);

  bootloader_exec();
}
