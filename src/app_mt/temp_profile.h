
#ifndef TEMP_PROFILE_H
#define TEMP_PROFILE_H

#include "types.h"
#include "wifi/wlan.h"
#include "sensor.h"

typedef enum {
  STEP_HOLD,
  STEP_RAMP
} temp_profile_step_type_t;

typedef enum {
  TPS_DOWNLOADING,
  TPS_SEEKING_START_VALUE,
  TPS_RUNNING,
  TPS_COMPLETE
} temp_profile_run_state_t;

typedef struct {
  uint32_t duration;
  quantity_t value;
  temp_profile_step_type_t type;
} temp_profile_step_t;

typedef struct {
  uint32_t id;
  char name[100];
  uint32_t num_steps;
  quantity_t start_value;
  temp_profile_step_t steps[32];
} temp_profile_t;

typedef struct {
  sensor_id_t sensor;
  uint32_t temp_profile_id;
  temp_profile_run_state_t state;
  time_t start_time;
  uint32_t current_step;
  time_t current_step_complete_time;
} temp_profile_run_t;

void
temp_profile_start(temp_profile_run_t* run, uint32_t temp_profile_id);

void
temp_profile_update(temp_profile_run_t* run, quantity_t sample);

bool
temp_profile_get_current_setpoint(temp_profile_run_t* run, float* sp);

#endif
