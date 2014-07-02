
#include "ch.h"
#include "app_cfg.h"
#include "message.h"
#include "sxfs.h"
#include "common.h"
#include "crc/crc32.h"
#include "touch.h"
#include "types.h"

#include <string.h>
#include <stdio.h>


typedef struct {
  uint32_t reset_count;
  unit_t temp_unit;
  output_ctrl_t control_mode;
  quantity_t hysteresis;
  quantity_t screen_saver;
  sensor_config_t sensor_configs[MAX_NUM_SENSOR_CONFIGS];
  matrix_t touch_calib;
  controller_settings_t controller_settings[NUM_CONTROLLERS];
  temp_profile_checkpoint_t temp_profile_checkpoints[NUM_CONTROLLERS];
  ota_update_checkpoint_t ota_update_checkpoint;
  char auth_token[64];
  net_settings_t net_settings;
  fault_data_t fault;
} app_cfg_data_t;

typedef struct {
  app_cfg_data_t data;
  uint32_t crc;
} app_cfg_rec_t;


static msg_t app_cfg_thread(void* arg);
static app_cfg_rec_t* app_cfg_load(sxfs_part_id_t* loaded_from);
static app_cfg_rec_t* app_cfg_load_from(sxfs_part_id_t part);


/* Local RAM copy of app_cfg */
static app_cfg_rec_t app_cfg_local;
static Mutex app_cfg_mtx;


void
app_cfg_init()
{
  chMtxInit(&app_cfg_mtx);

  app_cfg_rec_t* app_cfg = app_cfg_load(NULL);
  if (app_cfg != NULL) {
    app_cfg_local = *app_cfg;
    app_cfg_local.data.reset_count++;
    free(app_cfg);
  }
  else {
    app_cfg_reset();
  }

  chThdCreateFromHeap(NULL, 1024, LOWPRIO, app_cfg_thread, NULL);
}

static msg_t
app_cfg_thread(void* arg)
{
  (void)arg;
  chRegSetThreadName("app_cfg");

  while (!chThdShouldTerminate()) {
    app_cfg_flush();
    chThdSleepSeconds(2);
  }

  return 0;
}

void
app_cfg_reset()
{
  memset(&app_cfg_local.data, 0, sizeof(app_cfg_local.data));

  app_cfg_local.data.reset_count = 0;

  app_cfg_local.data.ota_update_checkpoint.download_in_progress = false;
  app_cfg_local.data.ota_update_checkpoint.update_size = 0;
  app_cfg_local.data.ota_update_checkpoint.last_block_offset = 0;
  memset(app_cfg_local.data.ota_update_checkpoint.update_ver, 0, sizeof(app_cfg_local.data.ota_update_checkpoint.update_ver));

  app_cfg_local.data.temp_unit = UNIT_TEMP_DEG_F;
  app_cfg_local.data.control_mode = ON_OFF;
  app_cfg_local.data.hysteresis.value = 1;
  app_cfg_local.data.hysteresis.unit = UNIT_TEMP_DEG_F;

  app_cfg_local.data.net_settings.security_mode = 0;
  app_cfg_local.data.net_settings.ip_config = IP_CFG_DHCP;
  app_cfg_local.data.net_settings.ip = 0;
  app_cfg_local.data.net_settings.subnet_mask = 0;
  app_cfg_local.data.net_settings.gateway = 0;
  app_cfg_local.data.net_settings.dns_server = 0;

  touch_calib_reset();

  app_cfg_local.data.controller_settings[CONTROLLER_1].controller = CONTROLLER_1;
  app_cfg_local.data.controller_settings[CONTROLLER_1].setpoint_type = SP_STATIC;
  app_cfg_local.data.controller_settings[CONTROLLER_1].static_setpoint.value = 68;
  app_cfg_local.data.controller_settings[CONTROLLER_1].static_setpoint.unit = UNIT_TEMP_DEG_F;

  app_cfg_local.data.controller_settings[CONTROLLER_1].output_settings[OUTPUT_1].enabled = false;
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

  app_cfg_local.data.controller_settings[CONTROLLER_2].output_settings[OUTPUT_2].enabled = false;
  app_cfg_local.data.controller_settings[CONTROLLER_2].output_settings[OUTPUT_2].function = OUTPUT_FUNC_HEATING;
  app_cfg_local.data.controller_settings[CONTROLLER_2].output_settings[OUTPUT_2].cycle_delay.unit = UNIT_TIME_MIN;
  app_cfg_local.data.controller_settings[CONTROLLER_2].output_settings[OUTPUT_2].cycle_delay.value = 3;

  app_cfg_local.crc = crc32_block(0, &app_cfg_local.data, sizeof(app_cfg_data_t));
  app_cfg_flush();
}

static app_cfg_rec_t*
app_cfg_load(sxfs_part_id_t* loaded_from)
{
  app_cfg_rec_t* app_cfg = app_cfg_load_from(SP_APP_CFG_1);
  if (app_cfg != NULL) {
    if (loaded_from != NULL)
      *loaded_from = SP_APP_CFG_1;

    return app_cfg;
  }

  app_cfg = app_cfg_load_from(SP_APP_CFG_2);
  if (app_cfg != NULL) {
    if (loaded_from != NULL)
      *loaded_from = SP_APP_CFG_2;

    return app_cfg;
  }

  return NULL;
}

static app_cfg_rec_t*
app_cfg_load_from(sxfs_part_id_t part)
{
  bool ret;
  app_cfg_rec_t* app_cfg = malloc(sizeof(app_cfg_rec_t));

  ret = sxfs_read(part, 0, (uint8_t*)app_cfg, sizeof(app_cfg_rec_t));
  if (!ret) {
    free(app_cfg);
    return NULL;
  }

  uint32_t calc_crc = crc32_block(0, &app_cfg->data, sizeof(app_cfg_data_t));
  if (calc_crc != app_cfg->crc) {
    free(app_cfg);
    return NULL;
  }

  return app_cfg;
}

unit_t
app_cfg_get_temp_unit(void)
{
  return app_cfg_local.data.temp_unit;
}

void
app_cfg_set_temp_unit(unit_t temp_unit)
{
  if (temp_unit != UNIT_TEMP_DEG_C &&
      temp_unit != UNIT_TEMP_DEG_F)
    return;

  if (temp_unit == app_cfg_local.data.temp_unit)
    return;

  chMtxLock(&app_cfg_mtx);
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

quantity_t
app_cfg_get_screen_saver(void)
{
  return app_cfg_local.data.screen_saver;
}

void
app_cfg_set_screen_saver(quantity_t screen_saver)
{
  if (memcmp(&screen_saver, &app_cfg_local.data.screen_saver, sizeof(quantity_t)) == 0)
    return;

  chMtxLock(&app_cfg_mtx);
  app_cfg_local.data.screen_saver = screen_saver;
  chMtxUnlock();
}

quantity_t
app_cfg_get_probe_offset(sensor_serial_t sensor_serial)
{
  uint8_t i;
  quantity_t offset;

  offset.unit = UNIT_TEMP_DEG_F;
  offset.value = 0;

  for (i = 0; i < MAX_NUM_SENSOR_CONFIGS; i++) {
    if (memcmp(sensor_serial, app_cfg_local.data.sensor_configs[i].sensor_serial, sizeof(sensor_serial_t)) == 0)
      return app_cfg_local.data.sensor_configs[i].offset;
  }
  
  return offset;
}

void
app_cfg_set_probe_offset(quantity_t probe_offset, sensor_serial_t sensor_serial)
{
  uint8_t i;
  int idx, next_idx;

  idx = next_idx = -1;

  for(i = 0; i < MAX_NUM_SENSOR_CONFIGS; i++) {
    sensor_serial_t* sensor_sn = &app_cfg_local.data.sensor_configs[i].sensor_serial;
    if(memcmp(sensor_serial, sensor_sn, sizeof(sensor_serial_t)) == 0) {
      idx = i;
      break;
    }
    /* super cryptic non-zero check */
    else if (((*sensor_sn)[0] == 0) &&
             (memcmp(*sensor_sn, *(sensor_sn + 1), 5) == 0)) {
      next_idx = i;
      break;
    }
  }

  if (idx < 0)
    idx = next_idx;

  if (idx < 0)
    return;

  if (probe_offset.unit == UNIT_TEMP_DEG_C) {
    probe_offset.value *= (9.0f / 5.0f);
    probe_offset.unit = UNIT_TEMP_DEG_F;
  }

  chMtxLock(&app_cfg_mtx);
  memcpy(app_cfg_local.data.sensor_configs[idx].sensor_serial, sensor_serial, sizeof(sensor_serial_t));
  app_cfg_local.data.sensor_configs[idx].offset = probe_offset;
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
app_cfg_set_controller_settings(
    temp_controller_id_t controller,
    settings_source_t source,
    controller_settings_t* settings)
{
  if (controller >= NUM_CONTROLLERS)
    return;

  if ((source == SS_SERVER) ||
      memcmp(settings, &app_cfg_local.data.controller_settings[controller], sizeof(controller_settings_t)) != 0) {
    chMtxLock(&app_cfg_mtx);
    app_cfg_local.data.controller_settings[controller] = *settings;
    chMtxUnlock();

    msg_id_t msg_id;
    if (source == SS_DEVICE)
      msg_id = MSG_CONTROLLER_SETTINGS;
    else
      msg_id = MSG_API_CONTROLLER_SETTINGS;

    msg_send(msg_id, settings);
  }
}


const temp_profile_checkpoint_t*
app_cfg_get_temp_profile_checkpoint(temp_controller_id_t controller)
{
  if (controller >= NUM_CONTROLLERS)
      return NULL;

  return &app_cfg_local.data.temp_profile_checkpoints[controller];
}

void
app_cfg_set_temp_profile_checkpoint(temp_controller_id_t controller, temp_profile_checkpoint_t* checkpoint)
{
  if (controller >= NUM_CONTROLLERS)
      return;

  if (memcmp(checkpoint, &app_cfg_local.data.temp_profile_checkpoints[controller], sizeof(temp_profile_checkpoint_t)) != 0) {
    chMtxLock(&app_cfg_mtx);
    app_cfg_local.data.temp_profile_checkpoints[controller] = *checkpoint;
    chMtxUnlock();
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
  if (memcmp(settings, &app_cfg_local.data.net_settings, sizeof(net_settings_t)) != 0) {
    chMtxLock(&app_cfg_mtx);
    app_cfg_local.data.net_settings = *settings;
    chMtxUnlock();

    msg_send(MSG_NET_NETWORK_SETTINGS, NULL);
  }
}

const ota_update_checkpoint_t*
app_cfg_get_ota_update_checkpoint(void)
{
  return &app_cfg_local.data.ota_update_checkpoint;
}

void
app_cfg_set_ota_update_checkpoint(const ota_update_checkpoint_t* checkpoint)
{
  chMtxLock(&app_cfg_mtx);
  app_cfg_local.data.ota_update_checkpoint = *checkpoint;
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

  sxfs_part_id_t used_app_cfg_part = SP_APP_CFG_1;
  app_cfg_rec_t* app_cfg = app_cfg_load(&used_app_cfg_part);

  app_cfg_local.crc = crc32_block(0, &app_cfg_local.data, sizeof(app_cfg_data_t));

  if (app_cfg == NULL || memcmp(&app_cfg_local, app_cfg, sizeof(app_cfg_rec_t)) != 0) {
    bool ret;
    sxfs_part_id_t unused_app_cfg_part =
        (used_app_cfg_part == SP_APP_CFG_1) ? SP_APP_CFG_2 : SP_APP_CFG_1;

    ret = sxfs_erase_all(unused_app_cfg_part);
    if (ret) {
      ret = sxfs_write(unused_app_cfg_part, 0, (uint8_t*)&app_cfg_local, sizeof(app_cfg_local));
      if (ret) {
        ret = sxfs_erase_all(used_app_cfg_part);
        if (!ret)
          printf("used app cfg erase failed! %d\r\n", used_app_cfg_part);
      }
      else {
        printf("unused app cfg write failed! %d\r\n", unused_app_cfg_part);
      }
    }
    else {
      printf("unused app cfg erase failed! %d\r\n", unused_app_cfg_part);
    }
  }
  chMtxUnlock();

  if (app_cfg != NULL)
    free(app_cfg);
}
