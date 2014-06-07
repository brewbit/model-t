#include "ch.h"
#include "recovery_img.h"
#include "message.h"
#include "dfuse.h"
#include "app_hdr.h"

#include <stdio.h>


static msg_t
recovery_img_thread(void* arg);


void
recovery_img_init()
{
  chThdCreateFromHeap(NULL, 1024, NORMALPRIO, recovery_img_thread, NULL);
}

static msg_t
recovery_img_thread(void* arg)
{
  (void)arg;

  recovery_img_load_state_t state = RECOVERY_IMG_CHECKING;
  msg_send(MSG_RECOVERY_IMG_STATUS, &state);

  dfu_parse_result_t result = dfuse_verify(SP_RECOVERY_IMG);
  if (result != DFU_PARSE_OK) {
    printf("No recovery image detected (%d)\r\n", result);
    printf("  Copying this image to external flash... ");

    recovery_img_write();
  }
  else {
    printf("Recovery image is present\r\n");
    state = RECOVERY_IMG_LOADED;
    msg_send(MSG_RECOVERY_IMG_STATUS, &state);
  }

  return 0;
}

void
recovery_img_write()
{
  recovery_img_load_state_t state;
  dfu_parse_result_t result;

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
