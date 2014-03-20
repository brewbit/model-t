
#include "pid.h"

/*
 * This code is based on Brett Beauregard's Improved Beginner PID series of
 * blog posts [1] with additions for self-tuning behavior based on the paper
 * "Self-Tuning of PID Controllers by Adaptive Interaction." by Feng Lin,
 * Robert D Brandt, and George Saikalis [2]
 *
 * [1] http://brettbeauregard.com/blog/2011/04/improving-the-beginners-pid-introduction/
 * [2] http://www.ece.eng.wayne.edu/~flin/Conference/AI-PID.pdf
 */

#define LIMIT(v, min, max) \
  do { \
    if ((v) > (max)) (v) = (max); \
    else if ((v) < (min)) (v) = (min); \
  } while (0)

void
pid_init(pid_t* pid)
{
  pid->sample_time = MS2ST(5000);
  pid->last_time   = (chTimeNow() - pid->sample_time);
  pid->enabled     = true;

  pid_set_gains(pid, 288, 720, 144);
}

#define GAMMA 0.03
void
pid_exec(pid_t* pid, quantity_t setpoint, quantity_t sample)
{
  if (!pid->enabled)
    return;

  systime_t now = chTimeNow();
  systime_t time_diff = (now - pid->last_time);

  if (time_diff >= pid->sample_time) {
    float err_p = (setpoint.value - sample.value);
    pid->err_i += err_p;
    float err_d = (err_p - pid->last_err);

    /* Limit integrator windup */
    LIMIT(pid->err_i, pid->out_min, pid->out_max);

    /* Recalculate gains using Lin et al algorithm */
    pid->kp += -GAMMA * err_p * err_p;
    pid->ki += -GAMMA * err_p * pid->err_i;
    pid->kd += -GAMMA * err_p * err_d;

    pid->out = (pid->kp * err_p) + (pid->ki * pid->err_i) + (pid->kd * err_d);
    LIMIT(pid->out, pid->out_min, pid->out_max);

    pid->last_err = err_p;
    pid->last_time   = now;
  }
}

void
pid_set_gains(pid_t* pid, float kp, float ki, float kd)
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
pid_enable(pid_t* pid, quantity_t sample, bool enabled)
{
  if (enabled && !pid->enabled)
    pid_reinit(pid, sample);

  pid->enabled = enabled;
}

void
pid_reinit(pid_t* pid, quantity_t sample)
{
//  pid->last_err = sample.value;
  pid->err_i = pid->out;
  LIMIT(pid->err_i, pid->out_min, pid->out_max);
}

void
pid_set_output_sign(pid_t* pid, uint8_t sign)
{
   pid->output_sign = sign;

   if (pid->output_sign == NEGATIVE) {
     pid->kp = -pid->kp;
     pid->ki = -pid->ki;
     pid->kd = -pid->kd;
   }
}

void
pid_set_output_limits(pid_t* pid, float min, float max)
{
  if (min >= max)
   return;

  pid->out_min = min;
  pid->out_max = max;

  if (pid->enabled) {
    LIMIT(pid->out, pid->out_min, pid->out_max);
    LIMIT(pid->err_i, pid->out_min, pid->out_max);
  }
}
