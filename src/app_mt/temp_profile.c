
#include "temp_profile.h"
#include "sntp.h"
#include "message.h"
#include "app_cfg.h"


void
temp_profile_start(temp_profile_run_t* run, uint32_t temp_profile_id)
{
  run->temp_profile_id = temp_profile_id;
  run->current_step = 0;
  run->current_step_complete_time = 0;

  if (sntp_time_available())
    run->state = TPS_SEEKING_START_VALUE;
  else
    run->state = TPS_WAITING_FOR_TIME_SERVER;
}

void
temp_profile_update(temp_profile_run_t* run, quantity_t sample)
{
  switch (run->state) {
    case TPS_WAITING_FOR_TIME_SERVER:
      if (sntp_time_available())
        run->state = TPS_SEEKING_START_VALUE;
      break;

    case TPS_SEEKING_START_VALUE:
    {
      const temp_profile_t* profile = app_cfg_get_temp_profile(run->temp_profile_id);
      float start_err = sample.value - profile->start_value.value;

      if (start_err < 1 && start_err > -1) {
        run->start_time = sntp_get_time();
        run->current_step = 0;
        run->current_step_complete_time =
            run->start_time + profile->steps[run->current_step].duration;
        run->state = TPS_RUNNING;
      }
      break;
    }

    default:
      break;
  }
}

bool
temp_profile_get_current_setpoint(temp_profile_run_t* run, float* sp)
{
  bool ret = true;
  const temp_profile_t* profile = app_cfg_get_temp_profile(run->temp_profile_id);

  if (run->state == TPS_RUNNING) {
    if (sntp_get_time() > run->current_step_complete_time) {
      if (++run->current_step < profile->num_steps) {
        run->current_step_complete_time += profile->steps[run->current_step].duration;
      }
      else {
        run->state = TPS_COMPLETE;
      }
    }
  }

  switch (run->state) {
    case TPS_DOWNLOADING:
      ret = false;
      break;

    case TPS_WAITING_FOR_TIME_SERVER:
    case TPS_SEEKING_START_VALUE:
      *sp = profile->start_value.value;
      break;

    case TPS_RUNNING:
    {
      int i;
      uint32_t duration_into_profile = sntp_get_time() - run->start_time;
      uint32_t step_begin = 0;
      float last_temp = profile->start_value.value;

      for (i = 0; i < (int)profile->num_steps; ++i) {
        const temp_profile_step_t* step = &profile->steps[i];
        uint32_t step_end = step_begin + step[i].duration;

        if (duration_into_profile >= step_begin &&
            duration_into_profile < step_end) {
          if (step->type == STEP_HOLD) {
            *sp = step->value.value;
          }
          else {
            uint32_t duration_into_step = duration_into_profile - step_begin;
            *sp = last_temp + ((step->value.value - last_temp) * duration_into_step / step->duration);
          }
          break;
        }
        step_begin += step->duration;
        last_temp = step->value.value;
      }
      break;
    }

    case TPS_COMPLETE:
      if (profile->num_steps > 0)
        *sp = profile->steps[profile->num_steps-1].value.value;
      else
        *sp = profile->start_value.value;
      break;
  }

  return ret;
}
