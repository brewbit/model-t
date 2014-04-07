
#include "ch.h"
#include "hal.h"

#include "lcd.h"
#include "image.h"
#include "web_api.h"
#include "touch.h"
#include "gui.h"
#include "temp_control.h"
#include "gui/home.h"
#include "gui/recovery.h"
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
extern int irq_count, irq_timeout_count, io_thread_loc;


int
main(void)
{
  halInit();
  chSysInit();

  uint32_t* devid = board_get_device_id();
  sprintf(device_id, "%08X%08X%08X",
      (unsigned int)devid[0],
      (unsigned int)devid[1],
      (unsigned int)devid[2]);

  /* start stdout port */
  sdStart(SD_STDIO, NULL);

  rngStart(&RNGD);

  app_cfg_init();

  int resets = app_cfg_get_reset_count();
  printf("Reset Count: %d\r\n", resets);

  fault_data_t* fault = app_cfg_get_fault_data();
  if (fault->type != FAULT_NONE) {
    int i;

    printf("!!! FAULT DETECTED !!!\r\n");
    printf("  type: %d\r\n  ", fault->type);
    for (i = 0; i < MAX_FAULT_DATA / 8; ++i) {
      int j;
      for (j = 0; j < 8; ++j) {
        printf("%d ", fault->data[i * 8 + j]);
      }
      printf("\r\n");
    }

    app_cfg_clear_fault_data();
  }

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

    printf("SYS: %d\r\n",
        resets);

    const hci_stats_t* hs = hci_get_stats();
    printf("HCI: %u %u %u %u %u\r\n",
        hs->num_free_buffers,
        hs->buffer_len,
        (unsigned int)hs->num_sent_packets,
        (unsigned int)hs->num_released_packets,
        (unsigned int)hs->num_timeouts);
  }
}
