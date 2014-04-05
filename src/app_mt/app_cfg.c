
#include "ch.h"
#include "app_cfg.h"
#include "message.h"
#include "iflash.h"
#include "common.h"
#include "crc/crc32.h"
#include "touch.h"
#include "types.h"

#include <string.h>


typedef struct {
  uint32_t reset_count;
  unit_t temp_unit;
  matrix_t touch_calib;
  controller_settings_t controller_settings[NUM_SENSORS];
  temp_profile_t temp_profiles[NUM_SENSORS];
  output_settings_t output_settings[NUM_OUTPUTS];
  char auth_token[64];
  net_settings_t net_settings;
} app_cfg_data_t;

typedef struct {
  app_cfg_data_t data;
  uint32_t crc;
} app_cfg_rec_t;


/* app_cfg stored in flash */
__attribute__ ((section("app_cfg")))
app_cfg_rec_t app_cfg_stored;

/* Local RAM copy of app_cfg */
static app_cfg_rec_t app_cfg_local;
static Mutex app_cfg_mtx;
static systime_t last_idle;


void
app_cfg_init()
{
  chMtxInit(&app_cfg_mtx);

  uint32_t calc_crc = crc32_block(0, &app_cfg_stored.data, sizeof(app_cfg_data_t));

  if (app_cfg_stored.crc == calc_crc) {
    app_cfg_local = app_cfg_stored;
    app_cfg_local.data.reset_count++;
  }
  else {
    app_cfg_local.data.reset_count = 0;

    app_cfg_local.data.temp_unit = UNIT_TEMP_DEG_F;

    touch_calib_reset();

    app_cfg_local.data.controller_settings[SENSOR_1].setpoint_type = SP_STATIC;
    app_cfg_local.data.controller_settings[SENSOR_1].static_setpoint.value = 78;
    app_cfg_local.data.controller_settings[SENSOR_1].static_setpoint.unit = UNIT_TEMP_DEG_F;

    app_cfg_local.data.controller_settings[SENSOR_2].setpoint_type = SP_STATIC;
    app_cfg_local.data.controller_settings[SENSOR_2].static_setpoint.value = 78;
    app_cfg_local.data.controller_settings[SENSOR_2].static_setpoint.unit = UNIT_TEMP_DEG_F;

    app_cfg_local.data.output_settings[OUTPUT_1].function = OUTPUT_FUNC_COOLING;
    app_cfg_local.data.output_settings[OUTPUT_1].trigger = SENSOR_1;
    app_cfg_local.data.output_settings[OUTPUT_1].cycle_delay.unit = UNIT_TIME_MIN;
    app_cfg_local.data.output_settings[OUTPUT_1].cycle_delay.value = 3;
    app_cfg_local.data.output_settings[OUTPUT_1].output_mode = ON_OFF;

    app_cfg_local.data.output_settings[OUTPUT_2].function = OUTPUT_FUNC_HEATING;
    app_cfg_local.data.output_settings[OUTPUT_2].trigger = SENSOR_1;
    app_cfg_local.data.output_settings[OUTPUT_2].cycle_delay.unit = UNIT_TIME_MIN;
    app_cfg_local.data.output_settings[OUTPUT_2].cycle_delay.value = 3;
    app_cfg_local.data.output_settings[OUTPUT_2].output_mode = ON_OFF;

    app_cfg_local.crc = crc32_block(0, &app_cfg_local.data, sizeof(app_cfg_data_t));
  }
}

void
app_cfg_idle()
{
  if ((chTimeNow() - last_idle) > S2ST(2)) {
    chMtxLock(&app_cfg_mtx);

    app_cfg_local.crc = crc32_block(0, &app_cfg_local.data, sizeof(app_cfg_data_t));

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
  return app_cfg_local.data.temp_unit;
}

void
app_cfg_set_temp_unit(unit_t temp_unit)
{
  int i;

  if (temp_unit != UNIT_TEMP_DEG_C &&
      temp_unit != UNIT_TEMP_DEG_F)
    return;

  if (temp_unit == app_cfg_local.data.temp_unit)
    return;

  chMtxLock(&app_cfg_mtx);

  for (i = 0; i < NUM_SENSORS; ++i) {
    controller_settings_t* s = &app_cfg_local.data.controller_settings[i];
    s->static_setpoint = quantity_convert(s->static_setpoint, temp_unit);
  }

  app_cfg_local.data.temp_unit = temp_unit;
  chMtxUnlock();

  msg_send(MSG_TEMP_UNIT, &app_cfg_local.data.temp_unit);
}

const matrix_t*
app_cfg_get_touch_calib(void)
{
  return &app_cfg_local.data.touch_calib;
}

void
app_cfg_set_touch_calib(matrix_t* touch_calib)
{
  chMtxLock(&app_cfg_mtx);
  app_cfg_local.data.touch_calib = *touch_calib;
  chMtxUnlock();
}

const controller_settings_t*
app_cfg_get_controller_settings(sensor_id_t sensor)
{
  if (sensor >= NUM_SENSORS)
    return NULL;

  return &app_cfg_local.data.controller_settings[sensor];
}

void
app_cfg_set_controller_settings(sensor_id_t sensor, controller_settings_t* settings)
{
  if (sensor >= NUM_SENSORS)
    return;

  if (memcmp(settings, &app_cfg_local.data.controller_settings[sensor], sizeof(controller_settings_t)) != 0) {
    chMtxLock(&app_cfg_mtx);
    app_cfg_local.data.controller_settings[sensor] = *settings;
    chMtxUnlock();

    controller_settings_msg_t msg = {
        .sensor = sensor,
        .settings = *settings
    };
    msg_send(MSG_CONTROLLER_SETTINGS, &msg);
  }
}

const output_settings_t*
app_cfg_get_output_settings(output_id_t output)
{
  if (output >= NUM_OUTPUTS)
    return NULL;

  return &app_cfg_local.data.output_settings[output];
}

void
app_cfg_set_output_settings(output_id_t output, output_settings_t* settings)
{
  if (output >= NUM_OUTPUTS)
    return;

  if (memcmp(settings, &app_cfg_local.data.output_settings[output], sizeof(output_settings_t)) != 0) {
    chMtxLock(&app_cfg_mtx);
    app_cfg_local.data.output_settings[output] = *settings;
    chMtxUnlock();

    output_settings_msg_t msg = {
        .output = output,
        .settings = *settings
    };
    msg_send(MSG_OUTPUT_SETTINGS, &msg);
  }
}

const char*
app_cfg_get_auth_token()
{
  return app_cfg_local.data.auth_token;
}

void
app_cfg_set_auth_token(const char* auth_token)
{
  chMtxLock(&app_cfg_mtx);
  strncpy(app_cfg_local.data.auth_token,
      auth_token,
      sizeof(app_cfg_local.data.auth_token));
  chMtxUnlock();
}

const net_settings_t*
app_cfg_get_net_settings()
{
  return &app_cfg_local.data.net_settings;
}

void
app_cfg_set_net_settings(const net_settings_t* settings)
{
  chMtxLock(&app_cfg_mtx);
  app_cfg_local.data.net_settings = *settings;
  chMtxUnlock();
}

const temp_profile_t*
app_cfg_get_temp_profile(uint32_t temp_profile_id)
{
  int i;
  for (i = 0; i < NUM_SENSORS; ++i) {
    temp_profile_t* profile = &app_cfg_local.data.temp_profiles[i];
    if (profile->id == temp_profile_id)
      return profile;
  }

  return NULL;
}

void
app_cfg_set_temp_profile(const temp_profile_t* profile, uint32_t index)
{
  if (index >= 2)
    return;

  chMtxLock(&app_cfg_mtx);
  app_cfg_local.data.temp_profiles[index] = *profile;
  chMtxUnlock();
}

uint32_t
app_cfg_get_reset_count(void)
{
  return app_cfg_local.data.reset_count;
}
