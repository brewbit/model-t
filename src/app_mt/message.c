
#include "message.h"
#include "common.h"
#include "thread_watchdog.h"

#include <stdlib.h>
#include <stdio.h>

#define MAX_MAILBOX_MSGS 32

typedef struct msg_listener_s {
  Thread* thread;
  const char* name;
  thread_msg_dispatch_t dispatch;
  systime_t timeout;
  void* user_data;
  bool watchdog_enabled;
  Mailbox mb;
  msg_t mb_buf[MAX_MAILBOX_MSGS];
} msg_listener_t;

typedef struct {
  msg_listener_t* sender;
  msg_id_t id;
  void* user_data;
  void* msg_data;
  bool processed;
} thread_msg_t;

typedef struct msg_subscription_s {
  msg_listener_t* listener;
  void* user_data;
  struct msg_subscription_s* next;
} msg_subscription_t;


static msg_t
msg_thread_func(void* arg);

static void
msg_loop_exec(msg_listener_t* l);

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
  chMBInit(&l->mb, l->mb_buf, MAX_MAILBOX_MSGS);
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
  chThdSelf()->msg_listener = l;

  chRegSetThreadName(l->name);

  l->dispatch(MSG_INIT, NULL, l->user_data, NULL);

  while (1) {
    msg_loop_exec(l);
  }

  return 0;
}

static void
msg_loop_exec(msg_listener_t* l)
{
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
  msg_subscription_t* sub;

  if (id >= NUM_THREAD_MSGS)
    return;

  msg_listener_t* self = chThdSelf()->msg_listener;

  for (sub = subs[id]; sub != NULL; sub = sub->next) {
    if (sub->listener == self) {
      if (sub->listener->dispatch != NULL)
        sub->listener->dispatch(id, msg_data, sub->listener->user_data, sub->user_data);
      else
        chDbgPanic("message broadcast to self, but no dispatch method provided");
    }
    else {
      thread_msg_t msg = {
        .id = id,
        .msg_data = msg_data,
        .user_data = sub->user_data,
        .sender = self,
        .processed = false
      };

      chMBPost(&sub->listener->mb, (msg_t)&msg, TIME_INFINITE);

      while (!msg.processed) {
        if (self != NULL)
          msg_loop_exec(self);
        else
          chThdSleepMilliseconds(10);
      }
    }
  }
}

static thread_msg_t*
msg_get(msg_listener_t* l)
{
  thread_msg_t* msg;
  msg_t rdy = chMBFetch(&l->mb, (msg_t*)&msg, l->timeout);
  if (rdy == RDY_OK)
    return msg;

  return NULL;
}

static void
msg_release(thread_msg_t* msg)
{
  if (msg == NULL)
    return;

  msg->processed = true;

  if (msg->sender != NULL) {
    static thread_msg_t release_msg = {
        .id = MSG_RELEASE,        .msg_data = NULL,        .user_data = NULL,        .sender = NULL,        .processed = true    };
    chMBPost(&msg->sender->mb, (msg_t)&release_msg, TIME_INFINITE);
  }
}
