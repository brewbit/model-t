
#include "ch.h"
#include "hal.h"
#include "test.h"

#include "lcd.h"
#include "terminal.h"
#include "wspr.h"
#include "image.h"
#include "bapi.h"
#include "touch.h"


static WORKING_AREA(wa_thread_blinker, 128);
static msg_t thread_blinker(void *arg) {

  (void)arg;
  chRegSetThreadName("blinker");

  while (TRUE) {
    palSetPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
    palClearPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
  }

  return 0;
}

int main(void)
{
  halInit();
  chSysInit();

  set_bg_img(img_background);
  lcd_init();
  touch_init();

  wspr_init();
//  bapi_init();

  chThdCreateStatic(wa_thread_blinker, sizeof(wa_thread_blinker), NORMALPRIO, thread_blinker, NULL);

  while (TRUE) {
    chThdSleepMilliseconds(500);
  }
}
