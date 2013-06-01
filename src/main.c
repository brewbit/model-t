
#include "ch.h"
#include "hal.h"

#include "lcd.h"
#include "terminal.h"
#include "wspr.h"
#include "image.h"
#include "bapi.h"
#include "touch.h"
#include "gui.h"
#include "gui_calib.h"
#include "temp.h"

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

static onewire_bus_t ob1 = {
    .port = &SD1
};

static onewire_bus_t ob2 = {
    .port = &SD2
};

temp_port_t tp2 = {
    .ob = {
        .port = &SD2
    }
};

int main(void)
{
  halInit();
  chSysInit();

  gui_init(&calib_gui);

  set_bg_img(img_background);
  lcd_init();
  touch_init();

  temp_init(&tp2);

  wspr_init();
//  bapi_init();

  gui_paint();

  chThdCreateStatic(wa_thread_blinker, sizeof(wa_thread_blinker), NORMALPRIO, thread_blinker, NULL);

  while (TRUE) {
    chThdSleepMilliseconds(500);
  }
}
