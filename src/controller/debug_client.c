
#include "ch.h"
#include "hal.h"
#include "debug_client.h"
#include "message.h"
#include "wspr_net.h"
#include "wspr_tcp.h"


static void
debug_client_dispatch(msg_id_t id, void* msg_data, void* user_data);

static msg_t
debug_client_thread(void* arg);

static void
dispatch_wifi_status(wspr_wifi_status_t* status);


BaseChannel* dbg_conn;


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

  while (1) {
    Thread* tp = chMsgWait();
    thread_msg_t* msg = (thread_msg_t*)chMsgGet(tp);

    debug_client_dispatch(msg->id, msg->msg_data, msg->user_data);

    chMsgRelease(tp, 0);
  }

  return 0;
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
  if (status->state == WSPR_SUCCESS) {
    while (dbg_conn == NULL)
      dbg_conn = wspr_tcp_connect(IP_ADDR(192, 168, 1, 146), 35287);
  }
}
