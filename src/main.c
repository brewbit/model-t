
#include "ch.h"
#include "hal.h"
#include "test.h"

#include "lcd.h"
#include "terminal.h"
#include "wspr.h"
#include "image.h"


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
wlan_test(void)
{
//  chThdSleepSeconds(1);
//  terminal_write("requesting version\n");
//  wspr_send(WSPR_IN_VERSION, NULL, 0);
//
//  chThdSleepSeconds(10);
//  terminal_write("making http get request\n");
//  http_header_t headers[] = {
//      {.name = "Accept",       .value = "application/json" },
//      {.name = "Content-type", .value = "application/json" },
//  };
//  wspr_http_get("brewbit.herokuapp.com", 80,
//      "/api/v1/account/token",
//      headers, 2,
//      "{ \"username\":\"test@test.com\", \"password\":\"test123test\" }");
//  tcp_connect(mkip(192, 168, 1, 146), 4392);
}

int main(void)
{
  halInit();
  chSysInit();

  set_bg_img(img_background);
  lcd_init();

  wspr_init();

  chThdCreateStatic(wa_thread_blinker, sizeof(wa_thread_blinker), NORMALPRIO, thread_blinker, NULL);

  wlan_test();

  while (TRUE) {
    chThdSleepMilliseconds(500);
  }
}
