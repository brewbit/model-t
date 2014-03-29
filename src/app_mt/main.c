
#include "ch.h"
#include "hal.h"

#include "lcd.h"
#include "image.h"
#include "web_api.h"
#include "touch.h"
#include "gui.h"
#include "temp_control.h"
#include "gui_home.h"
#include "gui_recovery.h"
#include "gfx.h"
#include "app_cfg.h"
#include "net.h"
#include "sntp.h"
#include "ota_update.h"
#include "thread_watchdog.h"
#include <chprintf.h>

#include <stdio.h>
#include <string.h>


char device_id[32];


msg_t
idle_thread(void* arg)
{
  (void)arg;
  chRegSetThreadName("idle");

  while (1) {
    app_cfg_idle();
  }

  return 0;
}

extern Semaphore sem_io_ready;
extern Semaphore sem_write_complete;
extern int irq_count, missed_irq_count, irq_timeout_count, io_thread_loc;


int
main(void)
{
  halInit();
  chSysInit();

  uint32_t* devid = board_get_device_id();
  sprintf(device_id, "%08X%08X%08X",
      devid[0],
      devid[1],
      devid[2]);

  /* start stdout port */
  sdStart(SD_STDIO, NULL);

  rngStart(&RNGD);

  app_cfg_init();

  int resets = app_cfg_get_reset_count();
  printf("Reset Count: %d\r\n", resets);

  gfx_init();
  touch_init();
  temp_control_init();
  net_init();
  ota_update_init();
  web_api_init();
  sntp_init();
  gui_init();
  thread_watchdog_init();

  widget_t* home_screen = home_screen_create();
  gui_push_screen(home_screen);

  recovery_screen_create();

  chThdCreateFromHeap(NULL, 1024, LOWPRIO, idle_thread, NULL);

  while (TRUE) {
    palSetPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(1000);
    palClearPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(1000);
      printf("%d %d %d | %d %d %d | %d %d %d %d\r\n",
          resets,
          !palReadPad(PORT_WIFI_IRQ, PAD_WIFI_IRQ),
          io_thread_loc,
          sem_io_ready.s_cnt,
          sem_write_complete.s_cnt,
          tSLInformation.sem_recv.s_cnt,
          tSLInformation.usNumberOfFreeBuffers,
          irq_count,
          missed_irq_count,
          irq_timeout_count);
  }
}
