
#ifndef MESSAGE_H
#define MESSAGE_H

#include "ch.h"

typedef enum {
  MSG_TOUCH_INPUT,

  MSG_SENSOR_SAMPLE,
  MSG_SENSOR_TIMEOUT,

  MSG_SENSOR_SETTINGS,
  MSG_OUTPUT_SETTINGS,

  MSG_GUI_PUSH_SCREEN,
  MSG_GUI_POP_SCREEN,
  MSG_GUI_PAINT,

  MSG_TEMP_UNIT,

  MSG_NET_STATUS,
  MSG_NET_NEW_NETWORK,     // a new wireless network is now available
  MSG_NET_NETWORK_UPDATED, // new information for a previously seen network is available
  MSG_NET_NETWORK_TIMEOUT, // a wireless network is no longer available

  MSG_SHOW_ACTIVATION_TOKEN,

  MSG_OTAU_START,
  MSG_OTAU_CHUNK,
  MSG_OTAU_STATUS,

  MSG_CHECK_UPDATE,

  MSG_SHUTDOWN,

  NUM_THREAD_MSGS
} msg_id_t;


typedef struct {
  Thread* waiting_thd;
  uint32_t* thread_count;
  msg_id_t id;
  void* user_data;
  void* msg_data;
} thread_msg_t;


typedef void (*thread_msg_dispatch_t)(msg_id_t id, void* msg_data, void* user_data);


void
msg_subscribe(msg_id_t id, Thread* thread, thread_msg_dispatch_t dispatcher, void* user_data);

void
msg_unsubscribe(msg_id_t id, Thread* thread, thread_msg_dispatch_t dispatch, void* user_data);

// send a message and wait for it to be processed
void
msg_send(msg_id_t id, void* msg_data);

// send a message but don't wait for it to be processed
void
msg_post(msg_id_t id, void* msg_data);

thread_msg_t*
msg_get(void);

thread_msg_t*
msg_get_timeout(systime_t timeout);

void
msg_release(thread_msg_t* msg);

#endif
