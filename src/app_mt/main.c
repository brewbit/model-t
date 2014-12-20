
#include "ch.h"
#include "hal.h"

#include "dfuse.h"
#include "lcd.h"
#include "image.h"
#include "web_api.h"
#include "touch.h"
#include "gui.h"
#include "temp_control.h"
#include "gui/home.h"
#include "gui/recovery.h"
#include "gui/self_test.h"
#include "gfx.h"
#include "app_cfg.h"
#include "net.h"
#include "ota_update.h"
#include "thread_watchdog.h"
#include "app_hdr.h"
#include "screen_saver.h"
#include "cmdline.h"
#include "xflash.h"
#include "recovery_img.h"

#include <stdio.h>
#include <string.h>


char device_id[32];


static void
check_for_faults(void)
{
  const fault_data_t* fault = app_cfg_get_fault_data();
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
}

static void
get_device_id(void)
{
  uint32_t* devid = board_get_device_id();
  sprintf(device_id, "%08X%08X%08X",
      (unsigned int)devid[0],
      (unsigned int)devid[1],
      (unsigned int)devid[2]);
}

static void
toggle_LED1(void)
{
  palSetPad(PORT_LED1, PAD_LED1);
  chThdSleepMilliseconds(1000);
  palClearPad(PORT_LED1, PAD_LED1);
  chThdSleepMilliseconds(1000);
}

static void
create_home_screen(void)
{
  widget_t* home_screen = home_screen_create();
  gui_push_screen(home_screen);
}

int
main(void)
{
  halInit();
  chSysInit();

  get_device_id();

  /* start stdout port */
  sdStart(SD_STDIO, NULL);

  xflash_init();
  cmdline_init();

  rngStart(&RNGD);

  app_cfg_init();

  check_for_faults();

  gfx_init();
  touch_init();

  sensor_init(SENSOR_1, SD_OW1);
  sensor_init(SENSOR_2, SD_OW2);

  temp_control_init(CONTROLLER_1);
  temp_control_init(CONTROLLER_2);

  ota_update_init();
  net_init();
  web_api_init();
  gui_init();
  thread_watchdog_init();

  create_home_screen();

  recovery_screen_create();

  screen_saver_create();

  if (palReadPad(PORT_SELF_TEST_EN, PAD_SELF_TEST_EN) == 0) {
    widget_t* self_test_screen = self_test_screen_create();
    gui_push_screen(self_test_screen);
  }

  recovery_img_init();

  while (TRUE) {
    cmdline_restart();

    toggle_LED1();
  }
}
