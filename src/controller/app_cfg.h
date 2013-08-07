
#ifndef APP_CFG_H
#define APP_CFG_H

#include "types.h"
#include "temp_control.h"
#include "touch_calib.h"


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

const probe_settings_t*
app_cfg_get_probe_settings(probe_id_t probe);

void
app_cfg_set_probe_settings(probe_id_t probe, probe_settings_t* settings);

const output_settings_t*
app_cfg_get_output_settings(output_id_t output);

void
app_cfg_set_output_settings(output_id_t output, output_settings_t* settings);

#endif
