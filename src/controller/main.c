
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
#include "wifi/wlan.h"

void NMIVector(void)
{
  chprintf(SD_STDIO, "NMI Vector\r\n");
}

void HardFaultVector(void)
{
  chprintf(SD_STDIO, "Hard Fault Vector\r\n");
}

void MemManageVector(void)
{
  chprintf(SD_STDIO, "Mem Manage Vector\r\n");
}

void BusFaultVector(void)
{
  chprintf(SD_STDIO, "Bus Fault Vector\r\n");
}

void UsageFaultVector(void)
{
  chprintf(SD_STDIO, "Usage Fault Vector\r\n");
}

msg_t
idle_thread(void* arg)
{
  (void)arg;

  while (1) {
//    wspr_idle();
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

  chThdCreateFromHeap(NULL, 1024, LOWPRIO, idle_thread, NULL);

  app_cfg_init();
  gfx_init();
  touch_init();
  temp_control_init();
  wlan_init(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  web_api_init();
  gui_init();
//  debug_client_init();

  widget_t* home_screen = home_screen_create();
  gui_push_screen(home_screen);

  while (TRUE) {
    palSetPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(500);
    palClearPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(500);
  }
}
