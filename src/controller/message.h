
#ifndef MESSAGE_H
#define MESSAGE_H

#include "ch.h"

typedef enum {
  MSG_TOUCH_INPUT,

  MSG_NEW_TEMP,
  MSG_PROBE_TIMEOUT,

  MSG_PROBE_SETTINGS,
  MSG_OUTPUT_SETTINGS,

  MSG_GUI_PUSH_SCREEN,
  MSG_GUI_POP_SCREEN,

  MSG_TEMP_UNIT,

  NUM_THREAD_MSGS
} msg_id_t;


typedef struct {
  msg_id_t id;
  void* user_data;
  void* msg_data;
} thread_msg_t;


typedef void (*thread_msg_dispatch_t)(msg_id_t id, void* msg_data, void* user_data);


void
msg_subscribe(msg_id_t id, Thread* thread, thread_msg_dispatch_t dispatcher, void* user_data);

void
msg_unsubscribe(msg_id_t id, Thread* thread, thread_msg_dispatch_t dispatch, void* user_data);

void
msg_broadcast(msg_id_t id, void* msg_data);

#endif
