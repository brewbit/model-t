
#include "ch.h"
#include "hal.h"

#include "lcd.h"
#include "touch.h"
#include "gfx.h"#include "net.h"
#include <chprintf.h>
#include <stdio.h>
#include <string.h>


char device_id[32];

int
main(void)
{
  halInit();
  chSysInit();

  uint32_t* devid = board_get_device_id();
  sprintf(device_id, "%08X%08X%08X",
      devid[0],
      devid[1],
      devid[2]);

  /* start stdout port */
  sdStart(SD_STDIO, NULL);

  rngStart(&RNGD);

  gfx_init();
  touch_init();
  net_init();

  while (TRUE) {
    palSetPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(500);
    palClearPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(500);
  }
}
