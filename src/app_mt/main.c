
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
#include "sntp.h"
#include "ota_update.h"
#include "thread_watchdog.h"
#include "app_hdr.h"
#include "screen_saver.h"
#include "cmdline.h"

#include <stdio.h>
#include <string.h>


char device_id[32];


static void
ensure_recovery_image_loaded(void)
{
  recovery_img_load_state_t state = RECOVERY_IMG_CHECKING;
  msg_send(MSG_RECOVERY_IMG_STATUS, &state);

  dfu_parse_result_t result = dfuse_verify(SP_RECOVERY_IMG);
  if (result != DFU_PARSE_OK) {
    extern uint8_t __app_base__;
    image_rec_t img_recs[2] = {
        {
            .data = (uint8_t*)&_app_hdr,
            .size = sizeof(_app_hdr)
        },
        {
            .data = &__app_base__,
            .size = _app_hdr.img_size
        },
    };

    printf("No recovery image detected (%d)\r\n", result);
    printf("  Copying this image to external flash... ");

    state = RECOVERY_IMG_LOADING;
    msg_send(MSG_RECOVERY_IMG_STATUS, &state);
    dfuse_write_self(SP_RECOVERY_IMG, img_recs, 2);

    state = RECOVERY_IMG_CHECKING;
    msg_send(MSG_RECOVERY_IMG_STATUS, &state);

    result = dfuse_verify(SP_RECOVERY_IMG);
    if (result == DFU_PARSE_OK) {
      state = RECOVERY_IMG_LOADED;
      msg_send(MSG_RECOVERY_IMG_STATUS, &state);
    }
    else {
      state = RECOVERY_IMG_FAILED;
      msg_send(MSG_RECOVERY_IMG_STATUS, &state);
    }

    printf("OK\r\n");
  }
  else {
    printf("Recovery image is present\r\n");
    state = RECOVERY_IMG_LOADED;
    msg_send(MSG_RECOVERY_IMG_STATUS, &state);
  }
}

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

int
main(void)
{
  halInit();
  chSysInit();

  get_device_id();

  /* start stdout port */
  sdStart(SD_STDIO, NULL);

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

  net_init();
  ota_update_init();
  web_api_init();
  sntp_init();
  gui_init();
  thread_watchdog_init();

  create_home_screen();

  recovery_screen_create();

  screen_saver_create();

  if (palReadPad(PORT_SELF_TEST_EN, PAD_SELF_TEST_EN) == 0) {
    widget_t* self_test_screen = self_test_screen_create();
    gui_push_screen(self_test_screen);
  }

  ensure_recovery_image_loaded();

  chThdCreateFromHeap(NULL, 1024, LOWPRIO, idle_thread, NULL);

  while (TRUE) {
    cmdline_restart();

    toggle_LED1();
  }
}
