
#include "ch.h"
#include "hal.h"

#include "lcd.h"
#include "terminal.h"
#include "wspr.h"
#include "image.h"
#include "bapi.h"
#include "touch.h"
#include "gui.h"


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

void
example_on_raw_touch(uint16_t x, uint16_t y)
{
  terminal_clear();
  terminal_write("raw touch\n");
  terminal_write("x: ");
  terminal_write_int(x);
  terminal_write("\ny: ");
  terminal_write_int(y);
}

void
example_on_touch(uint16_t x, uint16_t y)
{
  terminal_write("\n\ncalibrated touch\n");
  terminal_write("x: ");
  terminal_write_int(x);
  terminal_write("\ny: ");
  terminal_write_int(y);
}

screen_t example_gui = {
    .on_raw_touch = example_on_raw_touch,
    .on_touch     = example_on_touch,
};

int main(void)
{
  halInit();
  chSysInit();

  gui_init(&example_gui);

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
