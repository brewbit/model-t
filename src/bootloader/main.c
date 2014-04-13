
#include "ch.h"
#include "hal.h"


int
main(void)
{
  halInit();
  chSysInit();

  /* start stdout port */
  sdStart(SD_STDIO, NULL);

  bootloader_exec();
}
