
#include <ch.h>
#include <hal.h>
#include <stdio.h>
#include "types.h"

#include "thread_watchdog.h"


typedef struct {
  Thread* tp;
  systime_t last_kick;
  systime_t period;
} ThreadWatchdog;


static msg_t
thread_watchdog_thread(void* arg);


/*  */
static const IWDGConfig iwdg_config = {
  .counter = 0x03FF,
  .div = 0x07        // 256 prescaler
};

static ThreadWatchdog* monitored_threads;
static uint32_t num_monitored_threads;

void
thread_watchdog_init()
{
  chThdCreateFromHeap(NULL, 1024, NORMALPRIO, thread_watchdog_thread, NULL);
  iwdgStart(&IWDGD, &iwdg_config);
}

void
thread_watchdog_enable(Thread* tp, systime_t period)
{
  monitored_threads = realloc(monitored_threads, (num_monitored_threads + 1) * sizeof(ThreadWatchdog));

  ThreadWatchdog* twd = &monitored_threads[num_monitored_threads];
  twd->tp = tp;
  twd->last_kick = chTimeNow();
  twd->period = period;

  num_monitored_threads++;
}

void
thread_watchdog_kick()
{
  uint32_t i;
  for (i = 0; i < num_monitored_threads; ++i) {
    if (monitored_threads[i].tp == chThdSelf()) {
      monitored_threads[i].last_kick = chTimeNow();
      break;
    }
  }
}

static msg_t
thread_watchdog_thread(void* arg)
{
  (void)arg;

  while (1) {
    uint32_t i;
    bool all_threads_responsive = true;
    for (i = 0; i < num_monitored_threads; ++i) {
      ThreadWatchdog* twd = &monitored_threads[i];
      systime_t time_since_last_kick = (chTimeNow() - twd->last_kick);

      if (time_since_last_kick >= twd->period) {
        printf("Thread '%s' timed out: %u / %u\r\n",
            chRegGetThreadName(twd->tp),
            (unsigned int)time_since_last_kick,
            (unsigned int)twd->period);

        all_threads_responsive = false;
        break;
      }
    }

    if (all_threads_responsive)
      iwdgReset(&IWDGD);

    chThdSleepSeconds(1);
  }

  return 0;
}
