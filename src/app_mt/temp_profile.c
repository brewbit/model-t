
#include "temp_profile.h"
#include "sntp.h"
#include "message.h"
#include "app_cfg.h"

//TODO change to 5 hours...
#define CHECKPOINT_PERIOD S2ST(1 * 60)

static void write_checkpoint(temp_profile_run_t* run);


void
temp_profile_init(temp_profile_run_t* run, temp_controller_id_t controller)
{
  const temp_profile_checkpoint_t* checkpoint = app_cfg_get_temp_profile_checkpoint(controller);

  run->controller = controller;
  run->temp_profile_id = checkpoint->temp_profile_id;
  run->state = checkpoint->state;
  run->current_step = checkpoint->current_step;
  run->current_step_start_time = chTimeNow() - checkpoint->current_step_time;
  run->next_checkpoint = chTimeNow() + CHECKPOINT_PERIOD;
}

void
temp_profile_start(temp_profile_run_t* run, uint32_t temp_profile_id)
{
  run->temp_profile_id = temp_profile_id;
  run->current_step = 0;
  run->current_step_start_time = chTimeNow();

  run->state = TPS_SEEKING_START_VALUE;

  write_checkpoint(run);
}

void
temp_profile_update(temp_profile_run_t* run, quantity_t sample)
{
  switch (run->state) {
    case TPS_SEEKING_START_VALUE:
    {
      const temp_profile_t* profile = app_cfg_get_temp_profile(run->temp_profile_id);
      float start_err = sample.value - profile->start_value.value;

      if (start_err < 1 && start_err > -1) {
        run->current_step = 0;
        run->current_step_start_time = chTimeNow();
        run->state = TPS_RUNNING;
      }
      break;
    }

    default:
      break;
  }

  if (chTimeNow() > run->next_checkpoint)
    write_checkpoint(run);
}

static void
write_checkpoint(temp_profile_run_t* run)
{
  temp_profile_checkpoint_t checkpoint = {
      .temp_profile_id = run->temp_profile_id,
      .state = run->state,
      .current_step = run->current_step,
      .current_step_time = chTimeNow() - run->current_step_start_time
  };
  app_cfg_set_temp_profile_checkpoint(run->controller, &checkpoint);
  run->next_checkpoint = chTimeNow() + CHECKPOINT_PERIOD;
}

bool
temp_profile_get_current_setpoint(temp_profile_run_t* run, float* sp)
{
  bool ret = true;
  const temp_profile_t* profile = app_cfg_get_temp_profile(run->temp_profile_id);

  if (run->state == TPS_RUNNING) {
    if (chTimeNow() > run->current_step_start_time + S2ST(profile->steps[run->current_step].duration)) {
      if (++run->current_step < profile->num_steps) {
        run->current_step_start_time = chTimeNow();
      }
      else {
        run->state = TPS_COMPLETE;
      }
    }
  }

  switch (run->state) {
    case TPS_SEEKING_START_VALUE:
      *sp = profile->start_value.value;
      break;

    case TPS_RUNNING:
    {
      const temp_profile_step_t* cur_step = &profile->steps[run->current_step];
      if (cur_step->type == STEP_HOLD) {
        *sp = cur_step->value.value;
      }
      else {
        float last_temp;
        if (run->current_step == 0)
          last_temp = profile->start_value.value;
        else
          last_temp = profile->steps[run->current_step - 1].value.value;

        systime_t duration_into_step = chTimeNow() - run->current_step_start_time;
        *sp = last_temp + ((cur_step->value.value - last_temp) * duration_into_step / S2ST(cur_step->duration));
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
