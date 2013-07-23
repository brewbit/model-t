
#include "ch.h"
#include "app_cfg.h"
#include "message.h"
#include "iflash.h"
#include "common.h"

#include <string.h>


typedef struct {
  bool is_valid;

  temperature_unit_t temp_unit;
  matrix_t touch_calib;
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
  if (app_cfg_stored.is_valid) {
    app_cfg_local = app_cfg_stored;
  }
  else {
    app_cfg_local.temp_unit = TEMP_F;

    app_cfg_local.touch_calib.An      = 76320;
    app_cfg_local.touch_calib.Bn      = 3080;
    app_cfg_local.touch_calib.Cn      = -9475080;
    app_cfg_local.touch_calib.Dn      = -560;
    app_cfg_local.touch_calib.En      = 60340;
    app_cfg_local.touch_calib.Fn      = -4360660;
    app_cfg_local.touch_calib.Divider = 205664;

    app_cfg_local.probe_settings[PROBE_1].setpoint = DEGF(72);
    app_cfg_local.probe_settings[PROBE_2].setpoint = DEGF(72);

    app_cfg_local.output_settings[OUTPUT_1].function = OUTPUT_FUNC_COOLING;
    app_cfg_local.output_settings[OUTPUT_1].trigger = PROBE_1;
    app_cfg_local.output_settings[OUTPUT_2].function = OUTPUT_FUNC_HEATING;
    app_cfg_local.output_settings[OUTPUT_2].trigger = PROBE_1;

    app_cfg_local.is_valid = true;
  }

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

temperature_unit_t
app_cfg_get_temp_unit(void)
{
  return app_cfg_local.temp_unit;
}

void
app_cfg_set_temp_unit(temperature_unit_t temp_unit)
{
  chMtxLock(&app_cfg_mtx);
  app_cfg_local.temp_unit = temp_unit;
  chMtxUnlock();

  msg_broadcast(MSG_TEMP_UNIT, &app_cfg_local.temp_unit);
}

const matrix_t*
app_cfg_get_touch_calib(void)
{
  return &app_cfg_local.touch_calib;
}

void
app_cfg_set_touch_calib(matrix_t* touch_calib)
{
  chMtxLock(&app_cfg_mtx);
  app_cfg_local.touch_calib = *touch_calib;
  chMtxUnlock();
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