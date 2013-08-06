
#include "ch.h"
#include "hal.h"

#include "web_api.h"
#include "wspr_tcp.h"
#include "common.h"
#include "message.h"
#include "temp_input.h"

#include <stdlib.h>
#include <string.h>


typedef enum {
  VERSION,
  ACTIVATION,
  AUTH,
  DEVICE_SETTINGS,
  TEMP_PROFILE,
  UPGRADE,
  ACK,
  NACK,
  TEMP_SAMPLE
};

// 0xB3EB17
// u8 msg type
// u32 data len
// var data
// u16 crc


typedef struct {
  BaseChannel* conn;
} web_api_t;


static msg_t
web_api_thread(void* arg);

static void
web_api_dispatch(msg_id_t id, void* msg_data, void* user_data);

static void
dispatch_new_temp(web_api_t* w, sensor_msg_t* p);


void
web_api_init()
{
  web_api_t* w = calloc(1, sizeof(web_api_t));

  Thread* th_web_api = chThdCreateFromHeap(NULL, 1024, NORMALPRIO, web_api_thread, w);

  msg_subscribe(MSG_NEW_TEMP, th_web_api, web_api_dispatch, w);
}

static msg_t
web_api_thread(void* arg)
{
  (void)arg;

  while (1) {
    Thread* tp = chMsgWait();
    thread_msg_t* msg = (thread_msg_t*)chMsgGet(tp);

    web_api_dispatch(msg->id, msg->msg_data, msg->user_data);

    chMsgRelease(tp, 0);
  }

  return 0;
}

static void
web_api_dispatch(msg_id_t id, void* msg_data, void* user_data)
{
  web_api_t* w = user_data;

  switch (id) {
  case MSG_NEW_TEMP:
    dispatch_new_temp(w, msg_data);
    break;

  default:
    break;
  }
}

static void
dispatch_new_temp(web_api_t* w, sensor_msg_t* p)
{
  if (w->conn != NULL) {

  }
}
