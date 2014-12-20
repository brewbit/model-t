
#include "pid.h"
#include <stdio.h>

/*
 * This code is based on Brett Beauregard's Improved Beginner PID series of
 * blog posts [1] with additions for self-tuning behavior based on the paper
 * "Self-Tuning of PID Controllers by Adaptive Interaction." by Feng Lin,
 * Robert D Brandt, and George Saikalis [2]
 *
 * [1] http://brettbeauregard.com/blog/2011/04/improving-the-beginners-pid-introduction/
 * [2] http://www.ece.eng.wayne.edu/~flin/Conference/AI-PID.pdf
 */

void
pid_init(pid_controller_t* pid)
{
  pid->sample_time = MS2ST(2000);
  pid->last_time   = (chTimeNow() - pid->sample_time);
  pid->enabled     = false;

  pid_set_gains(pid, 10, .05, .05);
}

void
pid_exec(pid_controller_t* pid, float setpoint, float sample)
{
  if (!pid->enabled)
    return;

  systime_t now = chTimeNow();
  systime_t time_diff = (now - pid->last_time);

  if (time_diff >= pid->sample_time) {
    float err_p = (setpoint - sample);
    float err_d = (sample - pid->last_sample);
    float err_d_tune = (err_p - pid->last_err)/time_diff;

    pid->err_i += (pid->ki * err_p);
    pid->err_i = LIMIT(pid->err_i, pid->out_min, pid->out_max);
    pid->err_i_tune += err_p;
    pid->err_i_tune = LIMIT(pid->err_i_tune, pid->out_min, pid->out_max);

    tune_gains(pid, err_p, err_d_tune);

    pid->out = (pid->kp * err_p) + pid->err_i - (pid->kd * err_d);
    pid->out = LIMIT(pid->out, pid->out_min, pid->out_max);

    pid->last_err  = err_p;
    pid->last_sample = sample;
    pid->last_time = now;
  }
}

#define GAMMA 0.005
void
tune_gains(pid_controller_t* pid, float err_p, float err_d)
{
  if (pid->output_sign == NEGATIVE) {
    pid->kp = pid->kp *-1;
    pid->ki = pid->ki *-1;
    pid->kd = pid->kd *-1;
  }

  pid->kp += -GAMMA * err_p;
  pid->kp = LIMIT(pid->kp, 0, 50);

  pid->ki += -GAMMA * err_p * pid->err_i_tune;
  pid->ki = LIMIT(pid->ki, 0, 5);

  pid->kd += -GAMMA * err_p * err_d;
  pid->kd = LIMIT(pid->kd, 0, 5);

  pid_set_output_sign(pid, pid->output_sign);

  printf("kp, %f\r\nki, %f\r\nkd, %f\r\n",pid->kp, pid->ki, pid->kd);
}

void
pid_set_gains(pid_controller_t* pid, float kp, float ki, float kd)
{
  if (kp < 0 || ki < 0 || kd < 0)
    return;

  uint32_t sample_time_s = pid->sample_time / CH_FREQUENCY;
  pid->kp = kp;
  pid->ki = ki * sample_time_s;
  pid->kd = kd / sample_time_s;

  if (pid->output_sign == NEGATIVE) {
    pid->kp = -pid->kp;
    pid->ki = -pid->ki;
    pid->kd = -pid->kd;
  }
}

void
pid_enable(pid_controller_t* pid, float sample, bool enabled)
{
  if (enabled && !pid->enabled)
    pid_reinit(pid, sample);

  pid->enabled = enabled;
}

void
pid_reinit(pid_controller_t* pid, float sample)
{
  pid->last_sample = sample;
  pid->err_i = 0;
}

void
pid_set_output_sign(pid_controller_t* pid, uint8_t sign)
{
   pid->output_sign = sign;

   if (pid->output_sign == NEGATIVE) {
     pid->kp = -pid->kp;
     pid->ki = -pid->ki;
     pid->kd = -pid->kd;
   }
}

void
pid_set_output_limits(pid_controller_t* pid, float min, float max)
{
  if (min >= max)
   return;

  pid->out_min = min;
  pid->out_max = max;

  if (pid->enabled) {
    pid->out = LIMIT(pid->out, pid->out_min, pid->out_max);
    pid->err_i = LIMIT(pid->err_i, pid->out_min, pid->out_max);
  }
}
