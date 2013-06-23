
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
#include "settings.h"
#include "app_hdr.h"

int main(void);
void ResetHandler(void);


__attribute__ ((section("app_hdr")))
app_hdr_t _app_hdr = {
    .magic = 0xDEADBEEF,
    .entry = ResetHandler
};


int
main()
{
  halInit();
  chSysInit();

  /* start stdout port */
  sdStart(&SD3, NULL);

  settings_init();
  gfx_init();
  touch_init();
  temp_control_init();
//  wspr_init();
//  bapi_init();
  gui_init();

  widget_t* home_screen = home_screen_create();
  gui_push_screen(home_screen);


  int* h = (int*)0x08008000;
  chprintf(&SD3, "the value of app_hdr is 0x%x (0x%x)\r\n", *h, h);

  while (TRUE) {
    palSetPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
    palClearPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
  }
}
