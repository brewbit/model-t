
#include "ch.h"
#include "app_cfg.h"
#include "message.h"
#include "iflash.h"
#include "common.h"

#include <string.h>


typedef struct {
  global_settings_t global_settings;
  probe_settings_t probe_settings[NUM_PROBES];
  output_settings_t output_settings[NUM_OUTPUTS];
} app_cfg_t;


static msg_t app_cfg_thread(void* arg);


/* app_cfg stored in flash */
__attribute__ ((section("app_cfg")))
app_cfg_t app_cfg_stored;

/* Local RAM copy of app_cfg */
static app_cfg_t app_cfg_local;
static Mutex app_cfg_mtx;


void
app_cfg_init()
{
  app_cfg_local = app_cfg_stored;
  chMtxInit(&app_cfg_mtx);
  chThdCreateFromHeap(NULL, 512, NORMALPRIO, app_cfg_thread, NULL);
}

static msg_t
app_cfg_thread(void* arg)
{
  (void)arg;

  while (1) {
    chMtxLock(&app_cfg_mtx);
    if (memcmp(&app_cfg_local, &app_cfg_stored, sizeof(app_cfg_local)) != 0) {
      flashSectorErase(1);
      flashWrite((flashaddr_t)&app_cfg_stored, (char*)&app_cfg_local, sizeof(app_cfg_local));
    }
    chMtxUnlock();

    chThdSleepSeconds(2);
  }

  return 0;
}

const global_settings_t*
app_cfg_get_global_settings(void)
{
  return &app_cfg_local.global_settings;
}

void
app_cfg_set_global_settings(global_settings_t* settings)
{
  chMtxLock(&app_cfg_mtx);
  app_cfg_local.global_settings = *settings;
  chMtxUnlock();

  msg_broadcast(MSG_SETTINGS, &app_cfg_local.global_settings);
}

const probe_settings_t*
app_cfg_get_probe_settings(probe_id_t probe)
{
  if (probe >= NUM_PROBES)
    return NULL;

  return &app_cfg_local.probe_settings[probe];
}

void
app_cfg_set_probe_settings(probe_id_t probe, probe_settings_t* settings)
{
  if (probe >= NUM_PROBES)
    return;

  chMtxLock(&app_cfg_mtx);
  app_cfg_local.probe_settings[probe] = *settings;
  chMtxUnlock();

  probe_settings_msg_t msg = {
      .probe = probe,
      .settings = *settings
  };
  msg_broadcast(MSG_PROBE_SETTINGS, &msg);
}

const output_settings_t*
app_cfg_get_output_settings(output_id_t output)
{
  if (output >= NUM_OUTPUTS)
    return NULL;

  return &app_cfg_local.output_settings[output];
}

void
app_cfg_set_output_settings(output_id_t output, output_settings_t* settings)
{
  if (output >= NUM_OUTPUTS)
    return;

  chMtxLock(&app_cfg_mtx);
  app_cfg_local.output_settings[output] = *settings;
  chMtxUnlock();

  output_settings_msg_t msg = {
      .output = output,
      .settings = *settings
  };
  msg_broadcast(MSG_OUTPUT_SETTINGS, &msg);
}
