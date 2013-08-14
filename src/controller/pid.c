
#include "pid.h"

void
pid_init(pid_t* pid)
{
  pid->sample_time = MS2ST(5000);
  pid->last_time   = (chTimeNow() - pid->sample_time);
  pid->in_auto     = true;

  set_gains(pid, 288, 720, 144);
}

void
pid_exec(pid_t* pid, quantity_t setpoint, quantity_t sample)
{
  /* If not using PID return, the relay will turn on and off at the exact user defined setpoint. */
  if (!pid->in_auto)
    return;

  systime_t current_system_time = chTimeNow();
  systime_t time_diff           = (current_system_time - pid->last_time);

  if (time_diff >= pid->sample_time) {
    float error;
    float sample_diff;

    error = (setpoint.value - sample.value);

    pid->integral += pid->ki * error;
    if (pid->integral > pid->out_max)
      pid->integral = pid->out_max;
    else if (pid->integral < pid->out_min)
      pid->integral = pid->out_min;

    sample_diff = (sample.value - pid->last_sample);

    /*Compute PID Output*/
    pid->pid_output = (pid->kp * error) + (pid->integral - pid->kd) * sample_diff;

    if (pid->pid_output > pid->out_max)
      pid->pid_output = pid->out_max;
    else if (pid->pid_output < pid->out_min)
      pid->pid_output = pid->out_min;

    /*Remember some variables for next time*/
    pid->last_sample = sample.value;
    pid->last_time   = current_system_time;
  }
}

void
set_gains(pid_t* pid, float Kp, float Ki, float Kd)
{
  if (Kp < 0 || Ki < 0 || Kd < 0)
    return;

  pid->kp = Kp;
  pid->ki = Ki;
  pid->kd = Kd;

  uint32_t sample_time_s = pid->sample_time / CH_FREQUENCY;
  pid->kp = Kp;
  pid->ki = Ki * sample_time_s;
  pid->kd = Kd / sample_time_s;

  if (pid->controller_direction == REVERSE) {
    pid->kp = -pid->kp;
    pid->ki = -pid->ki;
    pid->kd = -pid->kd;
  }
}

void
set_mode(pid_t* pid, quantity_t sample, uint8_t Mode)
{
  bool new_mode = (Mode == AUTOMATIC);
  if (new_mode && !pid->in_auto) {
    /*we just went from manual to auto*/
    pid_reinit(pid, sample);
  }
  pid->in_auto = new_mode;
}

void
pid_reinit(pid_t* pid, quantity_t sample)
{
  pid->last_sample = sample.value;
  pid->integral = pid->pid_output;

  if (pid->integral > pid->out_max)
    pid->integral = pid->out_max;
  else if (pid->integral < pid->out_min)
    pid->integral = pid->out_min;
}

void
set_controller_direction(pid_t* pid, uint8_t direction)
{
   pid->controller_direction = direction;

   if(pid->controller_direction == REVERSE) {
     pid->kp = (0 - pid->kp);
     pid->ki = (0 - pid->ki);
     pid->kd = (0 - pid->kd);
   }
}

void
set_output_limits(pid_t* pid, float Min, float Max)
{
  if(Min >= Max)
   return;
  pid->out_min = Min;
  pid->out_max = Max;

  if(pid->in_auto)
  {
    if(pid->pid_output > pid->out_max)
      pid->pid_output = pid->out_max;
    else if(pid->pid_output < pid->out_min)
      pid->pid_output = pid->out_min;

    if(pid->integral > pid->out_max)
      pid->integral= pid->out_max;
    else if(pid->integral < pid->out_min)
      pid->integral = pid->out_min;
  }
}
