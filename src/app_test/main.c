
#include "ch.h"
#include "hal.h"
#include "sensor.h"
#include "gfx.h"
#include "touch.h"
#include "net.h"

int
main(void)
{
  halInit();
  chSysInit();

//  sdStart(SD_STDIO, NULL);

//  app_cfg_init();

  gfx_init();
//  touch_init();

//  sensor_init(SENSOR_1, SD_OW1);
//  sensor_init(SENSOR_2, SD_OW2);
//
//  net_init();
//  gui_init();

//  widget_t* home_screen = home_screen_create();
//  gui_push_screen(home_screen);

//  gfx_clear_screen();
//  gfx_draw_str("hello world", -1, 50, 50);

  while (TRUE) {
    palSetPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(1000);
    palClearPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(1000);
  }
}
