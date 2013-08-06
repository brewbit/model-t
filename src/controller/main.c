
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
#include "gfx.h"
#include "app_cfg.h"
#include "debug_client.h"

void NMIVector(void)
{
  chprintf((BaseChannel*)&SD3, "NMI Vector\r\n");
}

void HardFaultVector(void)
{
  chprintf((BaseChannel*)&SD3, "Hard Fault Vector\r\n");
}

void MemManageVector(void)
{
  chprintf((BaseChannel*)&SD3, "Mem Manage Vector\r\n");
}

void BusFaultVector(void)
{
  chprintf((BaseChannel*)&SD3, "Bus Fault Vector\r\n");
}

void UsageFaultVector(void)
{
  chprintf((BaseChannel*)&SD3, "Usage Fault Vector\r\n");
}



int
main(void)
{
  halInit();
  chSysInit();

  /* start stdout port */
  sdStart(&SD3, NULL);

  app_cfg_init();
  gfx_init();
  touch_init();
  temp_control_init();
  wspr_init();
//  bapi_init();
  gui_init();
  debug_client_init();

  widget_t* home_screen = home_screen_create();
  gui_push_screen(home_screen);

  while (TRUE) {
    palSetPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
    palClearPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
  }
}
