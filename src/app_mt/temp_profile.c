
#include "temp_profile.h"
#include "sntp.h"

float
temp_profile_get_current_setpoint(const temp_profile_t* profile)
{
  int i;

  uint32_t duration_into_profile = sntp_get_time() - profile->start_time;
  uint32_t step_begin = 0;
  float last_temp = profile->start_value.value;

  for (i = 0; i < profile->num_steps; ++i) {
    const temp_profile_step_t* step = &profile->steps[i];
    uint32_t step_end = step_begin + step[i].duration;

    if (duration_into_profile >= step_begin &&
        duration_into_profile < step_end) {
      if (step->type == STEP_HOLD)
        return step->value.value;
      else {
        uint32_t duration_into_step = duration_into_profile - step_begin;
        return last_temp + ((last_temp - step->value.value) * duration_into_step / step->duration);
      }
    }
    step_begin += step->duration;
  }

  return profile->steps[profile->num_steps-1].value.value;
}
