#include "cmdline.h"

#include <ch.h>
#include <hal.h>
#include <shell.h>
#include <chprintf.h>


static void cmd_mem(BaseChannel *chp, int argc, char *argv[]);
static void cmd_threads(BaseChannel *chp, int argc, char *argv[]);


static const ShellCommand commands[] = {
  {"mem", cmd_mem},
  {"threads", cmd_threads},
  {NULL, NULL}
};

static const ShellConfig shell_cfg = {
  (BaseChannel *)SD_STDIO,
  commands
};

static Thread* shelltp = NULL;


void
cmdline_init()
{
  shellInit();
}

void
cmdline_restart()
{
  if (chThdTerminated(shelltp)) {
    chThdRelease(shelltp);    /* Recovers memory of the previous shell. */
    shelltp = NULL;           /* Triggers spawning of a new shell.      */
  }

  if (shelltp == NULL)
    shelltp = shellCreate(&shell_cfg, 1024, NORMALPRIO);
}

static void
cmd_mem(BaseChannel *chp, int argc, char *argv[])
{
  size_t n, size;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: mem\r\n");
    return;
  }
  n = chHeapStatus(NULL, &size);
  chprintf(chp, "core free memory : %u bytes\r\n", chCoreStatus());
  chprintf(chp, "heap fragments   : %u\r\n", n);
  chprintf(chp, "heap free total  : %u bytes\r\n", size);
}

static void
cmd_threads(BaseChannel *chp, int argc, char *argv[])
{
  static const char *states[] = {THD_STATE_NAMES};
  Thread *tp;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: threads\r\n");
    return;
  }
  chprintf(chp, "name            trace     addr    stack prio refs state     time\r\n");
  chprintf(chp, "===========================================================\r\n");
  tp = chRegFirstThread();
  do {
    chprintf(chp, "%-16s %4d %.8lx %.8lx %4lu %4lu %-9s %lu\r\n",
            chRegGetThreadName(tp),
            tp->tracepoint,
            (uint32_t)tp,
            (uint32_t)tp->p_ctx.r13,
            (uint32_t)tp->p_prio,
            (uint32_t)(tp->p_refs - 1),
            states[tp->p_state],
            (uint32_t)tp->p_time);
    tp = chRegNextThread(tp);
  } while (tp != NULL);
}
