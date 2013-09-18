
#include "ch.h"
#include "hal.h"

#include "lcd.h"
#include "image.h"
#include "web_api.h"
#include "touch.h"
#include "gui.h"
#include "temp_control.h"
#include "gui_calib.h"
#include "gui_home.h"
#include "chprintf.h"
#include "gfx.h"
#include "app_cfg.h"
#include "debug_client.h"
#include "net.h"

#include <string.h>


msg_t
idle_thread(void* arg)
{
  (void)arg;
  chRegSetThreadName("idle");

  while (1) {
    gui_idle();
    app_cfg_idle();
  }

  return 0;
}

int
main(void)
{
  halInit();
  chSysInit();

  /* start stdout port */
  sdStart(SD_STDIO, NULL);

  app_cfg_init();
  gfx_init();
  touch_init();
  temp_control_init();
  net_init();
  web_api_init();
  gui_init();
//  debug_client_init();

  widget_t* home_screen = home_screen_create();
  gui_push_screen(home_screen);

  chThdCreateFromHeap(NULL, 1024, LOWPRIO, idle_thread, NULL);

  while (TRUE) {
    palSetPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(500);
    palClearPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(500);
  }
}
