
#ifndef MESSAGE_H
#define MESSAGE_H

#include "ch.h"

typedef enum {
  MSG_IDLE,

  MSG_TOUCH_INPUT,

  MSG_SENSOR_SAMPLE,
  MSG_SENSOR_TIMEOUT,

  MSG_SENSOR_SETTINGS,
  MSG_OUTPUT_SETTINGS,
  MSG_OUTPUT_STATUS,

  MSG_GUI_PUSH_SCREEN,
  MSG_GUI_POP_SCREEN,
  MSG_GUI_HIDE_SCREEN,

  MSG_TEMP_UNIT,

  MSG_NET_STATUS,
  MSG_NET_NEW_NETWORK,     // a new wireless network is now available
  MSG_NET_NETWORK_UPDATED, // new information for a previously seen network is available
  MSG_NET_NETWORK_TIMEOUT, // a wireless network is no longer available

  MSG_OTAU_CHECK,
  MSG_OTAU_START,
  MSG_OTAU_STATUS,

  MSG_API_STATUS,
  MSG_API_FW_UPDATE_CHECK,
  MSG_API_FW_UPDATE_CHECK_RESPONSE,
  MSG_API_FW_DNLD_START,
  MSG_API_FW_CHUNK,

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

struct msg_listener_s;
typedef struct msg_listener_s msg_listener_t;


typedef void (*thread_msg_dispatch_t)(msg_id_t id, void* msg_data, void* listener_data, void* sub_data);

msg_listener_t*
msg_listener_create(const char* name, int stack_size, thread_msg_dispatch_t dispatch, void* user_data);

void
msg_listener_set_idle_timeout(msg_listener_t* l, uint32_t idle_timeout);

void
msg_subscribe(msg_listener_t* l, msg_id_t id, void* user_data);

void
msg_unsubscribe(msg_listener_t* l, msg_id_t id, void* user_data);

// send a message and wait for it to be processed
void
msg_send(msg_id_t id, void* msg_data);

// send a message but don't wait for it to be processed
void
msg_post(msg_id_t id, void* msg_data);

#endif
