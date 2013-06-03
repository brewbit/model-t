
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

//temp_port_t tp1 = {
//    .ob = {
//        .port = &SD1
//    }
//};
//
//temp_port_t tp2 = {
//    .ob = {
//        .port = &SD2
//    }
//};

int main(void)
{
  halInit();
  chSysInit();

  lcd_init();
  touch_init();
//  temp_init(&tp1);
//  temp_init(&tp2);
  wspr_init();
//  bapi_init();

//  widget_t* home_screen = home_screen_create();
  gui_init();

  widget_t* calib_screen = calib_screen_create();
  gui_set_screen(calib_screen);

  while (TRUE) {
    palSetPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
    palClearPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
  }
}
