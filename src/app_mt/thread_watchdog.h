
#ifndef THREAD_WATCHDOG_H
#define THREAD_WATCHDOG_H

void
thread_watchdog_init(void);

void
thread_watchdog_enable(Thread* tp, systime_t period);

void
thread_watchdog_kick(void);

#endif
