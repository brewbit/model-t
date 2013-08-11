
#include "pid.h"

int32_t window_size = 1000;

void pid_init(pid_t* pid, quantity_t sample)
{
  pid->sample_time = MS2ST(1000);
  pid->last_time   = (chTimeNow() - pid->sample_time);
  pid->window_start_time = chTimeNow();
  pid->window_size = MS2ST(5000);

  set_output_limits(pid, 0, pid->window_size);
  set_mode(pid, sample, AUTOMATIC);

  pid->controller_direction = DIRECT;
  set_gains(pid, 2, 4, 1);
}

void pid_exec(pid_t* pid, quantity_t setpoint, quantity_t sample)
{
  /* If not using PID return, the relay will turn on and off at the exact user defined setpoint. */
  if (!pid->in_auto)
    return;

  uint32_t current_system_time = chTimeNow();
  uint32_t time_diff           = (current_system_time - pid->last_time);

  if (time_diff >= pid->sample_time) {
    float error;
    float dSample;

    error = setpoint.value - sample.value;

    pid->integral += pid->ki * error;
    if (pid->integral > pid->out_max)
      pid->integral = pid->out_max;
    else if (pid->integral < pid->out_min)
      pid->integral = pid->out_min;

    dSample = (sample.value - pid->last_sample);

    /*Compute PID Output*/
    pid->pid_output = pid->kp * error + pid->integral - pid->kd * dSample;

    if (pid->pid_output > pid->out_max)
      pid->pid_output = pid->out_max;
    else if (pid->pid_output < pid->out_min)
      pid->pid_output = pid->out_min;

    /*Remember some variables for next time*/
    pid->last_sample = sample.value;
    pid->last_time   = current_system_time;
  }
}

void set_gains(pid_t* pid, float Kp, float Ki, float Kd)
{
  if (Kp<0 || Ki<0 || Kd<0)
    return;

  pid->kp = Kp;
  pid->ki = Ki;
  pid->kd = Kd;

  /* Sample time is 1 second */
  uint8_t sample_time_s = 1;
  pid->kp = Kp;
  pid->ki = Ki * sample_time_s;
  pid->kd = Kd / sample_time_s;

  if(pid->controller_direction ==REVERSE)
  {
    pid->kp = (0 - pid->kp);
    pid->ki = (0 - pid->ki);
    pid->kd = (0 - pid->kd);
  }
}

void set_mode(pid_t* pid, quantity_t sample, uint8_t Mode)
{
  bool new_mode = (Mode == AUTOMATIC);
  if(new_mode && !pid->in_auto) {
    /*we just went from manual to auto*/
    pid_reinit(pid, sample);
  }
  pid->in_auto = new_mode;
}

void pid_reinit(pid_t* pid, quantity_t sample)
{
  pid->last_sample = sample.value;
  pid->integral = pid->pid_output;

  if(pid->integral > pid->out_max)
    pid->integral = pid->out_max;
  else if(pid->integral < pid->out_min)
    pid->integral = pid->out_min;
}

void set_controller_direction(pid_t* pid, uint8_t direction)
{
   pid->controller_direction = direction;

   if(pid->controller_direction == REVERSE) {
     pid->kp = (0 - pid->kp);
     pid->ki = (0 - pid->ki);
     pid->kd = (0 - pid->kd);
   }
}

void set_output_limits(pid_t* pid, float Min, float Max)
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
