
#include "ch.h"
#include "hal.h"

#include "chprintf.h"
#include "app_hdr.h"


int
main(void)
{
  halInit();
  chSysInit();


  while (TRUE) {
    palSetPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
    palClearPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
  }
}
