
#include "message.h"

#include <stdlib.h>


typedef struct msg_listener_s {
  Thread* thread;
  thread_msg_dispatch_t dispatch;
  void* user_data;
  struct msg_listener_s* next;
} msg_listener_t;


static msg_listener_t* listeners[NUM_THREAD_MSGS];


void
msg_subscribe(thread_msg_id_t id, Thread* thread, thread_msg_dispatch_t dispatch, void* user_data)
{
  if (id >= NUM_THREAD_MSGS)
    return;

  msg_listener_t* listener = malloc(sizeof(msg_listener_t));
  listener->thread = thread;
  listener->dispatch = dispatch;
  listener->user_data = user_data;

  chSysLock();
  listener->next = listeners[id];
  listeners[id] = listener;
  chSysUnlock();
}

void
msg_unsubscribe(thread_msg_id_t id, Thread* thread)
{
  if (id >= NUM_THREAD_MSGS)
    return;

  msg_listener_t* listener;
  msg_listener_t* prev_listener = NULL;
  for (listener = listeners[id]; listener != NULL; listener = listener->next) {
    if (listener->thread == thread) {
      if (prev_listener != NULL) {
        prev_listener->next = listener->next;
      }
      else {
        listeners[id] = listener->next;
      }
      break;
    }
    prev_listener = listener;
  }
}

void
msg_broadcast(thread_msg_id_t id, void* msg_data)
{
  if (id >= NUM_THREAD_MSGS)
    return;

  thread_msg_t msg = {
      .id = id,
      .msg_data = msg_data
  };

  msg_listener_t* listener;
  for (listener = listeners[id]; listener != NULL; listener = listener->next) {
    if (listener->thread == chThdSelf()) {
      if (listener->dispatch != NULL)
        listener->dispatch(id, msg_data, listener->user_data);
      else
        chDbgPanic("message broadcast to self, but no dispatch method provided");
    }
    else {
      msg.user_data = listener->user_data;
      chMsgSend(listener->thread, (msg_t)&msg);
    }
  }
}
