
#include "ch.h"
#include "app_cfg.h"
#include "message.h"
#include "iflash.h"
#include "common.h"

#include <string.h>


typedef struct {
  bool is_valid;

  unit_t temp_unit;
  matrix_t touch_calib;
  sensor_settings_t sensor_settings[NUM_SENSORS];
  output_settings_t output_settings[NUM_OUTPUTS];
} app_cfg_t;


/* app_cfg stored in flash */
__attribute__ ((section("app_cfg")))
app_cfg_t app_cfg_stored;

/* Local RAM copy of app_cfg */
static app_cfg_t app_cfg_local;
static Mutex app_cfg_mtx;
static systime_t last_idle;


void
app_cfg_init()
{
  if (app_cfg_stored.is_valid) {
    app_cfg_local = app_cfg_stored;
  }
  else {
    app_cfg_local.temp_unit = UNIT_TEMP_DEG_F;

    app_cfg_local.touch_calib.An      = 76320;
    app_cfg_local.touch_calib.Bn      = 3080;
    app_cfg_local.touch_calib.Cn      = -9475080;
    app_cfg_local.touch_calib.Dn      = -560;
    app_cfg_local.touch_calib.En      = 60340;
    app_cfg_local.touch_calib.Fn      = -4360660;
    app_cfg_local.touch_calib.Divider = 205664;

    app_cfg_local.sensor_settings[SENSOR_1].setpoint.unit = UNIT_TEMP_DEG_F;
    app_cfg_local.sensor_settings[SENSOR_1].setpoint.value = 72;

    app_cfg_local.sensor_settings[SENSOR_2].setpoint.unit = UNIT_TEMP_DEG_F;
    app_cfg_local.sensor_settings[SENSOR_2].setpoint.value = 72;

    app_cfg_local.output_settings[OUTPUT_1].function = OUTPUT_FUNC_COOLING;
    app_cfg_local.output_settings[OUTPUT_1].trigger = SENSOR_1;
    app_cfg_local.output_settings[OUTPUT_1].compressor_delay = S2ST(3* 60);
    app_cfg_local.output_settings[OUTPUT_1].setpoint.unit = UNIT_TIME_MIN;
    app_cfg_local.output_settings[OUTPUT_1].setpoint.value = 3;

    app_cfg_local.output_settings[OUTPUT_2].function = OUTPUT_FUNC_HEATING;
    app_cfg_local.output_settings[OUTPUT_2].trigger = SENSOR_1;
    app_cfg_local.output_settings[OUTPUT_2].compressor_delay = S2ST(3 * 60);
    app_cfg_local.output_settings[OUTPUT_2].setpoint.unit = UNIT_TIME_MIN;
    app_cfg_local.output_settings[OUTPUT_2].setpoint.value = 3;


    app_cfg_local.is_valid = TRUE;
  }

  chMtxInit(&app_cfg_mtx);
}

void
app_cfg_idle()
{
  if ((chTimeNow() - last_idle) > S2ST(2)) {
    chMtxLock(&app_cfg_mtx);
    if (memcmp(&app_cfg_local, &app_cfg_stored, sizeof(app_cfg_local)) != 0) {
      flashSectorErase(1);
      flashWrite((flashaddr_t)&app_cfg_stored, (char*)&app_cfg_local, sizeof(app_cfg_local));
    }
    chMtxUnlock();
    last_idle = chTimeNow();
  }
}

unit_t
app_cfg_get_temp_unit(void)
{
  return app_cfg_local.temp_unit;
}

quantity_t
quantity_convert(quantity_t q, unit_t unit)
{
  if (q.unit == unit)
    return q;

  switch (unit) {
  case UNIT_TEMP_DEG_C:
    if (q.unit == UNIT_TEMP_DEG_F)
      q.value = (5.0f / 9.0f) * (q.value - 32);
    break;

  case UNIT_TEMP_DEG_F:
    if (q.unit == UNIT_TEMP_DEG_C)
      q.value = ((9.0f / 5.0f) * q.value) + 32;
    break;

    /* Can't convert any other quantities */
  default:
    break;
  }

  q.unit = unit;

  return q;
}

void
app_cfg_set_temp_unit(unit_t temp_unit)
{
  int i;

  if (temp_unit != UNIT_TEMP_DEG_C &&
      temp_unit != UNIT_TEMP_DEG_F)
    return;

  if (temp_unit == app_cfg_local.temp_unit)
    return;

  chMtxLock(&app_cfg_mtx);

  for (i = 0; i < NUM_SENSORS; ++i) {
    sensor_settings_t* s = &app_cfg_local.sensor_settings[i];
    s->setpoint = quantity_convert(s->setpoint, temp_unit);
  }

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

const sensor_settings_t*
app_cfg_get_sensor_settings(sensor_id_t sensor)
{
  if (sensor >= NUM_SENSORS)
    return NULL;

  return &app_cfg_local.sensor_settings[sensor];
}

void
app_cfg_set_sensor_settings(sensor_id_t sensor, sensor_settings_t* settings)
{
  if (sensor >= NUM_SENSORS)
    return;

  chMtxLock(&app_cfg_mtx);
  app_cfg_local.sensor_settings[sensor] = *settings;
  chMtxUnlock();

  sensor_settings_msg_t msg = {
      .sensor = sensor,
      .settings = *settings
  };
  msg_broadcast(MSG_SENSOR_SETTINGS, &msg);
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
