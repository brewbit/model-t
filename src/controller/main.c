
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



static void
dispatch_response_headers(http_response_header_t* response)
{
  chprintf(&SD3, "http headers %d %d\r\n", response->result, response->response_code);
}

static void
dispatch_response_body(http_response_body_t* response)
{
  chprintf(&SD3, "http body\r\n");
}


static void
http_callback(void* arg, http_response_t* response)
{
  switch (response->type) {
  case HTTP_RESPONSE_HEADERS:
    dispatch_response_headers((http_response_header_t*)response);
    break;

  case HTTP_RESPONSE_BODY:
    dispatch_response_body((http_response_body_t*)response);
    break;

  case HTTP_RESPONSE_END:
    chprintf(&SD3, "http end\r\n");
    break;

  default:
    break;
  }
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

  widget_t* home_screen = home_screen_create();
  gui_push_screen(home_screen);

  chThdSleepSeconds(15);

  chprintf(&SD3, "making http req\r\n");

//  http_request_t http_request = {
//    .host = "192.168.1.146",
//    .port = 8000,
//    .method = HTTP_GET,
//    .url = "/Inventek_Systems_eS-WiFiPatch/Inventek_Systems_Instructions.txt",
//    .num_headers = 0,
//    .headers = NULL,
//    .request = ""
//  };
//  wspr_http_request(&http_request, http_callback, NULL);

  while (TRUE) {
    palSetPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
    palClearPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
  }
}
