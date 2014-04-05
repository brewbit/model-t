
#include "message.h"
#include "common.h"
#include "thread_watchdog.h"

#include <stdlib.h>
#include <stdio.h>


typedef struct msg_listener_s {
  Thread* thread;
  const char* name;
  thread_msg_dispatch_t dispatch;
  systime_t timeout;
  void* user_data;
  bool watchdog_enabled;
} msg_listener_t;

typedef struct msg_subscription_s {
  msg_listener_t* listener;
  void* user_data;
  struct msg_subscription_s* next;
} msg_subscription_t;


static msg_t
msg_thread_func(void* arg);

static void
msg_broadcast(msg_id_t id, void* msg_data, bool wait);

static thread_msg_t*
msg_get(msg_listener_t* l);

static void
msg_release(thread_msg_t* msg);


static msg_subscription_t* subs[NUM_THREAD_MSGS];


msg_listener_t*
msg_listener_create(const char* name, int stack_size, thread_msg_dispatch_t dispatch, void* user_data)
{
  msg_listener_t* l = calloc(1, sizeof(msg_listener_t));
  l->name = name;
  l->dispatch = dispatch;
  l->timeout = TIME_INFINITE;
  l->user_data = user_data;
  l->watchdog_enabled = false;
  l->thread = chThdCreateFromHeap(NULL, stack_size, NORMALPRIO, msg_thread_func, l);
  return l;
}

void
msg_listener_enable_watchdog(msg_listener_t* l, uint32_t period)
{
  l->watchdog_enabled = true;
  thread_watchdog_enable(l->thread, MS2ST(period));
}

void
msg_listener_set_idle_timeout(msg_listener_t* l, uint32_t idle_timeout)
{
  l->timeout = MS2ST(idle_timeout);
}

static msg_t
msg_thread_func(void* arg)
{
  msg_listener_t* l = arg;

  chRegSetThreadName(l->name);

  l->dispatch(MSG_INIT, NULL, l->user_data, NULL);

  while (1) {
    thread_msg_t* msg = msg_get(l);

    if (msg != NULL) {
      l->dispatch(msg->id, msg->msg_data, l->user_data, msg->user_data);
      msg_release(msg);
    }
    else {
      l->dispatch(MSG_IDLE, NULL, l->user_data, NULL);
    }
    if (l->watchdog_enabled)
      thread_watchdog_kick();
  }

  return 0;
}

void
msg_subscribe(msg_listener_t* l, msg_id_t id, void* user_data)
{
  if (id >= NUM_THREAD_MSGS)
    return;

  msg_subscription_t* sub = calloc(1, sizeof(msg_subscription_t));
  sub->listener = l;
  sub->user_data = user_data;

  chSysLock();
  sub->next = subs[id];
  subs[id] = sub;
  chSysUnlock();
}

void
msg_unsubscribe(msg_listener_t* l, msg_id_t id, void* user_data)
{
  if (id >= NUM_THREAD_MSGS)
    return;

  msg_subscription_t* sub;
  msg_subscription_t* prev_sub = NULL;
  for (sub = subs[id]; sub != NULL; sub = sub->next) {
    if ((sub->listener == l) &&
        (sub->user_data == user_data)) {
      if (prev_sub != NULL) {
        prev_sub->next = sub->next;
      }
      else {
        subs[id] = sub->next;
      }
      free(sub);
      break;
    }
    prev_sub = sub;
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
  msg_subscription_t* sub;

  if (id >= NUM_THREAD_MSGS)
    return;

  uint32_t* thread_count = NULL;
  if (!wait) {
    thread_count = malloc(sizeof(int));
    *thread_count = 0;

    // count subs
    for (sub = subs[id]; sub != NULL; sub = sub->next) {
      if (sub->listener->thread != chThdSelf())
        *thread_count += 1;
    }
    if (*thread_count == 0)
      free(thread_count);
  }

  for (sub = subs[id]; sub != NULL; sub = sub->next) {
    if (sub->listener->thread == chThdSelf()) {
      if (sub->listener->dispatch != NULL)
        sub->listener->dispatch(id, msg_data, sub->listener->user_data, sub->user_data);
      else
        chDbgPanic("message broadcast to self, but no dispatch method provided");
    }
    else {

      thread_msg_t* msg = malloc(sizeof(thread_msg_t));
      msg->id = id;
      msg->msg_data = msg_data;
      msg->user_data = sub->user_data;
      msg->waiting_thd = wait ? chThdSelf() : NULL;
      msg->thread_count = thread_count;

      // Ownership of msg is transferred to the thread it was sent to here.
      // Do not reference it again because it might not exist anymore!
      chMBPost(&sub->listener->thread->mb, (msg_t)msg, TIME_INFINITE);

      if (wait)
        chSemWait(&chThdSelf()->mb_sem);
    }
  }
}

static thread_msg_t*
msg_get(msg_listener_t* l)
{
  Thread* curThread = chThdSelf();
  thread_msg_t* msg;
  msg_t rdy = chMBFetch(&curThread->mb, (msg_t*)&msg, l->timeout);
  if (rdy == RDY_OK)
    return msg;

  return NULL;
}

static void
msg_release(thread_msg_t* msg)
{
  if (msg == NULL)
    return;

  // If a thread is waiting for this message to be processed,
  // signal it and let it clean up the message data
  if (msg->waiting_thd != NULL) {
    chSemSignal(&msg->waiting_thd->mb_sem);
  }
  else {
    // atomic decrement
    uint32_t result;
    uint32_t thd_cnt;
    do {
      thd_cnt = __LDREXW(msg->thread_count) - 1;
      result = __STREXW(thd_cnt, msg->thread_count);
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
