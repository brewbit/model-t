
#include "message.h"
#include "common.h"

#include <stdlib.h>
#include <stdio.h>


typedef struct msg_listener_s {
  Thread* thread;
  thread_msg_dispatch_t dispatch;
  void* user_data;
  struct msg_listener_s* next;
} msg_listener_t;


static void
msg_broadcast(msg_id_t id, void* msg_data, bool wait);


static msg_listener_t* listeners[NUM_THREAD_MSGS];


void
msg_subscribe(msg_id_t id, Thread* thread, thread_msg_dispatch_t dispatch, void* user_data)
{
  if (id >= NUM_THREAD_MSGS)
    return;

  msg_listener_t* listener = chHeapAlloc(NULL, sizeof(msg_listener_t));
  listener->thread = thread;
  listener->dispatch = dispatch;
  listener->user_data = user_data;

  chSysLock();
  listener->next = listeners[id];
  listeners[id] = listener;
  chSysUnlock();
}

void
msg_unsubscribe(msg_id_t id, Thread* thread, thread_msg_dispatch_t dispatch, void* user_data)
{
  if (id >= NUM_THREAD_MSGS)
    return;

  msg_listener_t* listener;
  msg_listener_t* prev_listener = NULL;
  for (listener = listeners[id]; listener != NULL; listener = listener->next) {
    if ((listener->thread == thread) &&
        (listener->dispatch == dispatch) &&
        (listener->user_data == user_data)) {
      if (prev_listener != NULL) {
        prev_listener->next = listener->next;
      }
      else {
        listeners[id] = listener->next;
      }
      chHeapFree(listener);
      break;
    }
    prev_listener = listener;
  }
}

void
msg_send(msg_id_t id, void* msg_data)
{
  msg_broadcast(id, msg_data, true);
}

void
msg_post(msg_id_t id, void* msg_data)
{
  msg_broadcast(id, msg_data, false);
}

static void
msg_broadcast(msg_id_t id, void* msg_data, bool wait)
{
  msg_listener_t* listener;

  if (id >= NUM_THREAD_MSGS)
    return;

  uint32_t* thread_count = NULL;
  if (!wait) {
    thread_count = malloc(sizeof(int));
    *thread_count = 0;

    // count listeners
    for (listener = listeners[id]; listener != NULL; listener = listener->next) {
      if (listener->thread != chThdSelf())
        *thread_count += 1;
    }
  }

  for (listener = listeners[id]; listener != NULL; listener = listener->next) {
    if (listener->thread == chThdSelf()) {
      if (listener->dispatch != NULL)
        listener->dispatch(id, msg_data, listener->user_data);
      else
        chDbgPanic("message broadcast to self, but no dispatch method provided");
    }
    else {
      thread_msg_t* msg = malloc(sizeof(thread_msg_t));
      msg->id = id;
      msg->msg_data = msg_data;
      msg->user_data = listener->user_data;
      msg->waiting_thd = wait ? chThdSelf() : NULL;
      msg->thread_count = thread_count;

      chMBPost(&listener->thread->mb, (msg_t)msg, TIME_INFINITE);
      if (wait)
        chSemWait(&msg->waiting_thd->mb_sem);
    }
  }
}

thread_msg_t*
msg_get()
{
  Thread* curThread = chThdSelf();
  thread_msg_t* msg;
  chMBFetch(&curThread->mb, (msg_t*)&msg, TIME_INFINITE);
  return msg;
}

void
msg_release(thread_msg_t* msg)
{
  // If a thread is waiting for this message to be processed,
  // signal it and let it clean up the message data
  if (msg->waiting_thd != NULL)
    chSemSignal(&msg->waiting_thd->mb_sem);
  else {
    // atomic decrement
    uint32_t result;
    uint32_t thd_cnt;
    do {
      thd_cnt = __LDREXW(msg->thread_count);
      result = __STREXW(thd_cnt - 1, msg->thread_count);
    } while (result != 0);

    // This is the last thread to process the message, so we have to clean
    // it up
    if (thd_cnt == 0) {
      if (msg->msg_data != NULL)
        free(msg->msg_data);

      free(msg->thread_count);
    }
  }

  free(msg);
}
