
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
  output_ctrl_t control_mode;
  quantity_t hysteresis;
  matrix_t touch_calib;
  controller_settings_t controller_settings[NUM_CONTROLLERS];
  temp_profile_t temp_profiles[NUM_SENSORS];
  char auth_token[64];
  net_settings_t net_settings;
  fault_data_t fault;
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
    app_cfg_local.data.control_mode = ON_OFF;
    app_cfg_local.data.hysteresis.value = 1;
    app_cfg_local.data.hysteresis.unit = UNIT_TEMP_DEG_F;


    touch_calib_reset();

    app_cfg_local.data.controller_settings[CONTROLLER_1].controller = CONTROLLER_1;
    app_cfg_local.data.controller_settings[CONTROLLER_1].setpoint_type = SP_STATIC;
    app_cfg_local.data.controller_settings[CONTROLLER_1].static_setpoint.value = 68;
    app_cfg_local.data.controller_settings[CONTROLLER_1].static_setpoint.unit = UNIT_TEMP_DEG_F;

    app_cfg_local.data.controller_settings[CONTROLLER_1].output_settings[OUTPUT_1].enabled = true;
    app_cfg_local.data.controller_settings[CONTROLLER_1].output_settings[OUTPUT_1].function = OUTPUT_FUNC_COOLING;
    app_cfg_local.data.controller_settings[CONTROLLER_1].output_settings[OUTPUT_1].cycle_delay.unit = UNIT_TIME_MIN;
    app_cfg_local.data.controller_settings[CONTROLLER_1].output_settings[OUTPUT_1].cycle_delay.value = 3;

    app_cfg_local.data.controller_settings[CONTROLLER_1].output_settings[OUTPUT_2].enabled = false;
    app_cfg_local.data.controller_settings[CONTROLLER_1].output_settings[OUTPUT_2].function = OUTPUT_FUNC_HEATING;
    app_cfg_local.data.controller_settings[CONTROLLER_1].output_settings[OUTPUT_2].cycle_delay.unit = UNIT_TIME_MIN;
    app_cfg_local.data.controller_settings[CONTROLLER_1].output_settings[OUTPUT_2].cycle_delay.value = 3;

    app_cfg_local.data.controller_settings[CONTROLLER_2].controller = CONTROLLER_2;
    app_cfg_local.data.controller_settings[CONTROLLER_2].setpoint_type = SP_STATIC;
    app_cfg_local.data.controller_settings[CONTROLLER_2].static_setpoint.value = 68;
    app_cfg_local.data.controller_settings[CONTROLLER_2].static_setpoint.unit = UNIT_TEMP_DEG_F;

    app_cfg_local.data.controller_settings[CONTROLLER_2].output_settings[OUTPUT_1].enabled = false;
    app_cfg_local.data.controller_settings[CONTROLLER_2].output_settings[OUTPUT_1].function = OUTPUT_FUNC_COOLING;
    app_cfg_local.data.controller_settings[CONTROLLER_2].output_settings[OUTPUT_1].cycle_delay.unit = UNIT_TIME_MIN;
    app_cfg_local.data.controller_settings[CONTROLLER_2].output_settings[OUTPUT_1].cycle_delay.value = 3;

    app_cfg_local.data.controller_settings[CONTROLLER_2].output_settings[OUTPUT_2].enabled = true;
    app_cfg_local.data.controller_settings[CONTROLLER_2].output_settings[OUTPUT_2].function = OUTPUT_FUNC_HEATING;
    app_cfg_local.data.controller_settings[CONTROLLER_2].output_settings[OUTPUT_2].cycle_delay.unit = UNIT_TIME_MIN;
    app_cfg_local.data.controller_settings[CONTROLLER_2].output_settings[OUTPUT_2].cycle_delay.value = 3;

    app_cfg_local.crc = crc32_block(0, &app_cfg_local.data, sizeof(app_cfg_data_t));
  }
}

void
app_cfg_idle()
{
  if ((chTimeNow() - last_idle) > S2ST(2)) {
    app_cfg_flush();
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

output_ctrl_t
app_cfg_get_control_mode(void)
{
  return app_cfg_local.data.control_mode;
}

void
app_cfg_set_control_mode(output_ctrl_t control_mode)
{
  if (control_mode != ON_OFF &&
      control_mode != PID)
    return;

  if (control_mode == app_cfg_local.data.control_mode)
    return;

  chMtxLock(&app_cfg_mtx);
  app_cfg_local.data.control_mode = control_mode;
  chMtxUnlock();

  msg_send(MSG_CONTROL_MODE, &app_cfg_local.data.control_mode);
}

quantity_t
app_cfg_get_hysteresis(void)
{
  return app_cfg_local.data.hysteresis;
}

void
app_cfg_set_hysteresis(quantity_t hysteresis)
{
  if (memcmp(&hysteresis, &app_cfg_local.data.hysteresis, sizeof(quantity_t)) == 0)
    return;

  if (hysteresis.unit == UNIT_TEMP_DEG_C) {
    hysteresis.value *= (9.0f / 5.0f);
    hysteresis.unit = UNIT_TEMP_DEG_F;
  }

  chMtxLock(&app_cfg_mtx);
  app_cfg_local.data.hysteresis = hysteresis;
  chMtxUnlock();
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
app_cfg_get_controller_settings(temp_controller_id_t controller)
{
  if (controller >= NUM_CONTROLLERS)
    return NULL;

  return &app_cfg_local.data.controller_settings[controller];
}

void
app_cfg_set_controller_settings(temp_controller_id_t controller, controller_settings_t* settings)
{
  if (controller >= NUM_CONTROLLERS)
    return;

  if (memcmp(settings, &app_cfg_local.data.controller_settings[controller], sizeof(controller_settings_t)) != 0) {
    chMtxLock(&app_cfg_mtx);
    app_cfg_local.data.controller_settings[controller] = *settings;
    chMtxUnlock();

    controller_settings_msg_t msg = {
        .controller = controller,
        .settings = *settings
    };
    msg_send(MSG_CONTROLLER_SETTINGS, &msg);
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

void
app_cfg_clear_fault_data()
{
  memset(&app_cfg_local.data.fault, 0, sizeof(fault_data_t));
}

const fault_data_t*
app_cfg_get_fault_data()
{
  return &app_cfg_local.data.fault;
}

void
app_cfg_set_fault_data(fault_type_t fault_type, void* data, uint32_t data_size)
{
  if (data_size > MAX_FAULT_DATA)
    data_size = MAX_FAULT_DATA;

  app_cfg_local.data.fault.type = fault_type;
  memcpy(app_cfg_local.data.fault.data, data, data_size);
}

void
app_cfg_flush()
{
  chMtxLock(&app_cfg_mtx);
  app_cfg_local.crc = crc32_block(0, &app_cfg_local.data, sizeof(app_cfg_data_t));

  if (memcmp(&app_cfg_local, &app_cfg_stored, sizeof(app_cfg_local)) != 0) {
    iflash_sector_erase(1);
    iflash_write((uint32_t)&app_cfg_stored, (uint8_t*)&app_cfg_local, sizeof(app_cfg_local));
  }
  chMtxUnlock();
}
