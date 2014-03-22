
#include <ch.h>
#include <hal.h>
#include <stdio.h>

#include "watchdog.h"
#include "message.h"


static msg_t
watchdog_thread(void* arg);


/*  */
static const IWDGConfig iwdg_config = {
  .counter = 0x03FF,
  .div = 0x07        // 256 prescaler
};

void
watchdog_init()
{
  chThdCreateFromHeap(NULL, 2048, NORMALPRIO, watchdog_thread, NULL);
  iwdgStart(&IWDGD, &iwdg_config);
}

static msg_t
watchdog_thread(void* arg)
{
  (void)arg;

  while (1) {
    iwdgReset(&IWDGD);

    /* Broadcast the health check message to all listening threads. If any
     * thread is blocked for too long, this will cause the watchdog to trip.
     */
    msg_send(MSG_HEALTH_CHECK, NULL);

    chThdSleepSeconds(1);
  }

  return 0;
}
