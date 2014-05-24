#include "cmdline.h"

#include <ch.h>
#include <hal.h>
#include <shell.h>
#include <chprintf.h>

#include "app_cfg.h"
#include "net.h"


static void cmd_mem(BaseChannel *chp, int argc, char *argv[]);
static void cmd_threads(BaseChannel *chp, int argc, char *argv[]);
static void cmd_slinfo(BaseChannel *chp, int argc, char *argv[]);
static void cmd_sysinfo(BaseChannel *chp, int argc, char *argv[]);
static void cmd_app_cfg_reset(BaseChannel *chp, int argc, char *argv[]);


static const ShellCommand commands[] = {
  {"mem", cmd_mem},
  {"threads", cmd_threads},
  {"slinfo", cmd_slinfo},
  {"sysinfo", cmd_sysinfo},
  {"app_cfg_reset", cmd_app_cfg_reset},
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
cmd_slinfo(BaseChannel *chp, int argc, char *argv[])
{
  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: slinfo\r\n");
    return;
  }

  const hci_stats_t* hs = hci_get_stats();
  chprintf(chp, "free buffers     : %u\r\n", hs->num_free_buffers);
  chprintf(chp, "buffer len       : %u\r\n", hs->buffer_len);
  chprintf(chp, "sent packets     : %u\r\n", hs->num_sent_packets);
  chprintf(chp, "released packets : %u\r\n", hs->num_released_packets);
  chprintf(chp, "comm timeouts    : %u\r\n", hs->num_timeouts);
}

static void
cmd_sysinfo(BaseChannel *chp, int argc, char *argv[])
{
  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: sysinfo\r\n");
    return;
  }

  chprintf(chp, "resets : %u\r\n", app_cfg_get_reset_count());
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
  chprintf(chp, "================================================================\r\n");
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

static void
cmd_app_cfg_reset(BaseChannel *chp, int argc, char *argv[])
{
  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: threads\r\n");
    return;
  }

  app_cfg_reset();
}
