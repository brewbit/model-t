
#ifndef APP_CFG_H
#define APP_CFG_H

#include "types.h"
#include "temp_control.h"
#include "touch_calib.h"
#include "web_api.h"
#include "net.h"


void
app_cfg_init(void);

void
app_cfg_idle(void);

unit_t
app_cfg_get_temp_unit(void);

void
app_cfg_set_temp_unit(unit_t temp_unit);

const matrix_t*
app_cfg_get_touch_calib(void);

void
app_cfg_set_touch_calib(matrix_t* touch_calib);

const sensor_settings_t*
app_cfg_get_sensor_settings(sensor_id_t sensor);

void
app_cfg_set_sensor_settings(sensor_id_t sensor, sensor_settings_t* settings);

const output_settings_t*
app_cfg_get_output_settings(output_id_t output);

void
app_cfg_set_output_settings(output_id_t output, output_settings_t* settings);

const char*
app_cfg_get_auth_token(void);

void
app_cfg_set_auth_token(const char* auth_token);

const net_settings_t*
app_cfg_get_net_settings(void);

void
app_cfg_set_net_settings(const net_settings_t* settings);

const temp_profile_t*
app_cfg_get_temp_profile(uint32_t temp_profile_id);

void
app_cfg_set_temp_profile(const temp_profile_t* profile, uint32_t index);

#endif
