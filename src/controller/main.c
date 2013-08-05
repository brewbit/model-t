
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

#define MAKE_IPV4_ADDRESS(a, b, c, d)                   ((((uint32_t) a) << 24) | (((uint32_t) b) << 16) | (((uint32_t) c) << 8) | ((uint32_t) d))

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

  widget_t* home_screen = home_screen_create();
  gui_push_screen(home_screen);

  chThdSleepSeconds(15);

  chprintf((BaseChannel*)&SD3, "connecting to echo server\r\n");
  BaseChannel* conn = wspr_tcp_connect(MAKE_IPV4_ADDRESS(192, 168, 1, 146), 35287);
  if (conn != NULL) {
    uint8_t recv[32] = {0};
    chprintf((BaseChannel*)&SD3, "connection established\r\n");
    chIOWriteTimeout(conn, "Hello world!", 12, MS2ST(5000));
    chThdSleepMilliseconds(2000);
    chprintf((BaseChannel*)&SD3, "waiting\r\n");
    chIOReadTimeout(conn, recv, sizeof(recv)-1, MS2ST(5000));
    chprintf((BaseChannel*)&SD3, "recvd: %s\r\n", recv);
  }
  else
    chprintf((BaseChannel*)&SD3, "connection failed\r\n");

  while (TRUE) {
    palSetPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
    palClearPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
  }
}
