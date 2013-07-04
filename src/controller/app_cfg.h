
#ifndef APP_CFG_H
#define APP_CFG_H

#include "types.h"
#include "temp_control.h"


typedef struct {
  temperature_unit_t unit;
} global_settings_t;


void
app_cfg_init(void);

const global_settings_t*
app_cfg_get_global_settings(void);

void
app_cfg_set_global_settings(global_settings_t* settings);

const probe_settings_t*
app_cfg_get_probe_settings(probe_id_t probe);

void
app_cfg_set_probe_settings(probe_id_t probe, probe_settings_t* settings);

const output_settings_t*
app_cfg_get_output_settings(output_id_t output);

void
app_cfg_set_output_settings(output_id_t output, output_settings_t* settings);

#endif
