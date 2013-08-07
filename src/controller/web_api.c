
#include "ch.h"
#include "hal.h"

#include "web_api.h"
#include "wspr_tcp.h"
#include "wspr_net.h"
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
} web_api_msg_id_t;

typedef enum {
  NOT_CONNECTED,
  CONNECTING,
  CONNECTED,
} web_api_state_t;

// 0xB3EB17
// u8 msg type
// u32 data len
// var data
// u16 crc


typedef struct {
  BaseChannel* conn;
  web_api_state_t state;
} web_api_t;


static msg_t
web_api_thread(void* arg);

static void
web_api_dispatch(msg_id_t id, void* msg_data, void* user_data);

static void
dispatch_wifi_status(web_api_t* w, wspr_wifi_status_t* p);

static void
dispatch_new_temp(web_api_t* w, sensor_msg_t* p);

static void
on_connect(BaseChannel* tcp_channel, void* user_data);

static void
read_conn(web_api_t* w);


void
web_api_init()
{
  web_api_t* w = calloc(1, sizeof(web_api_t));
  w->state = NOT_CONNECTED;

  Thread* th_web_api = chThdCreateFromHeap(NULL, 1024, NORMALPRIO, web_api_thread, w);

  msg_subscribe(MSG_WIFI_STATUS, th_web_api, web_api_dispatch, w);
  msg_subscribe(MSG_NEW_TEMP, th_web_api, web_api_dispatch, w);
}

static msg_t
web_api_thread(void* arg)
{
  web_api_t* w = arg;

  while (1) {
    if (chMsgIsPendingI(chThdSelf())) {
      Thread* tp = chMsgWait();
      thread_msg_t* msg = (thread_msg_t*)chMsgGet(tp);

      web_api_dispatch(msg->id, msg->msg_data, msg->user_data);

      chMsgRelease(tp, 0);
    }
    else {
      switch (w->state) {
      case NOT_CONNECTED:
        break;

      case CONNECTING:
        break;

      case CONNECTED:
        read_conn(w);
        break;
      }
      chThdSleepMilliseconds(200);
    }
  }

  return 0;
}

static void
read_conn(web_api_t* w)
{
  while (!chIOGetWouldBlock(w->conn)) {
    uint8_t c = chIOGet(w->conn);
    chprintf(&SD4, "%d\r\n", c);
  }
}

static void
web_api_dispatch(msg_id_t id, void* msg_data, void* user_data)
{
  web_api_t* w = user_data;

  switch (id) {
  case MSG_WIFI_STATUS:
    dispatch_wifi_status(w, msg_data);
    break;

  case MSG_NEW_TEMP:
    dispatch_new_temp(w, msg_data);
    break;

  default:
    break;
  }
}

static void
dispatch_wifi_status(web_api_t* w, wspr_wifi_status_t* p)
{
  if (w->state == NOT_CONNECTED &&
      p->state == WSPR_SUCCESS) {
    chprintf(&SD4, "connecting...\r\n");
    wspr_tcp_connect(IP_ADDR(192, 168, 1, 146), 31337, on_connect, w);
    w->state = CONNECTING;
  }
}

static void
on_connect(BaseChannel* conn, void* user_data)
{
  web_api_t* w = user_data;

  if (conn != NULL) {
    chprintf(&SD4, "connected\r\n");
    w->conn = conn;
    w->state = CONNECTED;
  }
  else {
    chprintf(&SD4, "retry\r\n");
    w->state = NOT_CONNECTED;
  }
}

static void
dispatch_new_temp(web_api_t* w, sensor_msg_t* p)
{
  if (w->conn != NULL) {

  }
}
