
#ifndef SENSOR_SETTINGS_H
#define SENSOR_SETTINGS_H

#include "temp_profile.h"

typedef enum {
  SP_STATIC,
  SP_TEMP_PROFILE
} setpoint_type_t;

typedef struct {
  setpoint_type_t setpoint_type;
  quantity_t static_setpoint;
  temp_profile_t temp_profile;
} sensor_settings_t;

float
sensor_settings_get_current_setpoint(const sensor_settings_t* settings);

#endif
