
#include "ch.h"
#include "hal.h"

#include "lcd.h"
#include "wspr.h"
#include "image.h"
#include "bapi.h"
#include "touch.h"
#include "gui.h"
#include "temp_control.h"
#include "gui_calib.h"
#include "gui_home.h"
#include "chprintf.h"


int main(void)
{
  halInit();
  chSysInit();

  /* start stdout port */
  sdStart(&SD3, NULL);

  chThdSleepMilliseconds(500);
  lcd_init();
  touch_init();
  temp_control_init();
//  wspr_init();
//  bapi_init();

  widget_t* home_screen = home_screen_create();
  gui_init(home_screen);

  while (TRUE) {
    palSetPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
    palClearPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
  }
}
