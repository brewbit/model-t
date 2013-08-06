
#include "ch.h"
#include "hal.h"
#include "debug_client.h"
#include "message.h"
#include "wspr_net.h"
#include "wspr_tcp.h"


typedef enum {
  DBG_WAIT_NET_UP,
  DBG_CONNECT,
  DBG_IDLE,
} dbg_state_t;


static void
debug_client_dispatch(msg_id_t id, void* msg_data, void* user_data);

static msg_t
debug_client_thread(void* arg);

static void
dispatch_wifi_status(wspr_wifi_status_t* status);

static void
on_connect(BaseChannel* tcp_channel);


static BaseChannel* dbg_conn;
static dbg_state_t dbg_state;
static wifi_state_t wifi_state;


void
debug_client_init()
{
  Thread* th_debug_client = chThdCreateFromHeap(NULL, 512, NORMALPRIO, debug_client_thread, NULL);
  msg_subscribe(MSG_WIFI_STATUS, th_debug_client, debug_client_dispatch, NULL);
}

static msg_t
debug_client_thread(void* arg)
{
  (void)arg;

  chRegSetThreadName("debug");

  while (1) {
    if (chMsgIsPendingI(chThdSelf())) {
      Thread* tp = chMsgWait();
      thread_msg_t* msg = (thread_msg_t*)chMsgGet(tp);

      debug_client_dispatch(msg->id, msg->msg_data, msg->user_data);

      chMsgRelease(tp, 0);
    }
    else {
      switch (dbg_state) {
      case DBG_WAIT_NET_UP:
        if (wifi_state == WSPR_SUCCESS)
          dbg_state = DBG_CONNECT;
        break;

      case DBG_CONNECT:
        wspr_tcp_connect(IP_ADDR(192, 168, 1, 146), 35287, on_connect);

        dbg_state = DBG_IDLE;
        break;

      case DBG_IDLE:
        break;

      default:
        break;
      }
      chThdSleepMilliseconds(200);
    }
  }

  return 0;
}

static void
on_connect(BaseChannel* conn)
{
  if (conn != NULL) {
    dbg_conn = conn;
    chprintf(dbg_conn, "this is a test\r\n");
  }
  else
    wspr_tcp_connect(IP_ADDR(192, 168, 1, 146), 35287, on_connect);
  dbg_state = DBG_IDLE;
}

static void
debug_client_dispatch(msg_id_t id, void* msg_data, void* user_data)
{
  (void)user_data;

  switch (id) {
  case MSG_WIFI_STATUS:
    dispatch_wifi_status(msg_data);
    break;

  default:
    break;
  }
}

static void
dispatch_wifi_status(wspr_wifi_status_t* status)
{
  wifi_state = status->state;
}
