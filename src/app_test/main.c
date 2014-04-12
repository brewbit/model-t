
#include "ch.h"
#include "hal.h"

int
main(void)
{
  halInit();
  chSysInit();

  while (TRUE) {
    palSetPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(1000);
    palClearPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(1000);
  }
}
