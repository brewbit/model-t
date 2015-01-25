
#ifndef TEMP_PROFILE_H
#define TEMP_PROFILE_H

#include "types.h"
#include "wifi/wlan.h"
#include "sensor.h"
#include "temp_control.h"


typedef enum {
  TPS_SEEKING_START_VALUE,
  TPS_RUNNING,
  TPS_HOLD_LAST
} temp_profile_run_state_t;

typedef struct {
  temp_controller_id_t controller;
  uint32_t temp_profile_id;
  temp_profile_run_state_t state;
  uint32_t current_step;
  systime_t current_step_start_time;
  systime_t next_checkpoint;
} temp_profile_run_t;

typedef struct {
  uint32_t temp_profile_id;
  temp_profile_run_state_t state;
  uint32_t current_step;
  systime_t current_step_time;
} temp_profile_checkpoint_t;


void
temp_profile_start(temp_profile_run_t* run, temp_controller_id_t controller, uint32_t temp_profile_id, int start_point);

void
temp_profile_resume(temp_profile_run_t* run, temp_controller_id_t controller);

void
temp_profile_update(temp_profile_run_t* run, quantity_t sample);

bool
temp_profile_get_current_setpoint(temp_profile_run_t* run, float* sp);

#endif
