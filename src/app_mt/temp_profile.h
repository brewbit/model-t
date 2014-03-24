
#ifndef TEMP_PROFILE_H
#define TEMP_PROFILE_H

#include "types.h"

typedef enum {
  STEP_HOLD,
  STEP_RAMP
} temp_profile_step_type_t;

typedef struct {
  uint32_t duration;
  quantity_t value;
  temp_profile_step_type_t type;
} temp_profile_step_t;

typedef struct {
  char name[100];
  uint32_t num_steps;
  uint32_t start_time;
  quantity_t start_value;
  temp_profile_step_t steps[32];
} temp_profile_t;


float
temp_profile_get_current_setpoint(const temp_profile_t* profile);

#endif
