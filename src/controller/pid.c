
#include "pid.h"

/* proportional gain */
float kp[NUM_PROBES];
/* integral gain */
float ki[NUM_PROBES];
/* derivative gain */
float kd[NUM_PROBES];

/* 1 second */
uint16_t sampleTime = MS2ST(1000);
int32_t  lastSample[NUM_PROBES];
int16_t  output[NUM_PROBES];
int16_t  iTerm[NUM_PROBES];
uint16_t  outMin = -200;
uint16_t  outMax = 200;
bool      inAuto = FALSE;

#define MANUAL    0
#define AUTOMATIC 1

#define DIRECT  0
#define REVERSE 1
int8_t controllerDirection = DIRECT;


int16_t pid_exec(probe_id_t probe, sensor_sample_t setpoint, sensor_sample_t sample)
{
  static uint32_t lastTime[NUM_PROBES];
  static int32_t  lastSample[NUM_PROBES];
  int32_t error;
  int32_t dSample;

  if (!inAuto)
    return output[probe];

  /*How long since we last calculated*/
  uint32_t currentTime = chTimeNow();
  uint32_t timeChange = (uint32_t)(currentTime - lastTime[probe]);

  if(timeChange >= sampleTime) {
    /* P roportional */
    error = setpoint.value.temp - sample.value.temp;

    /* I ntegral */
    iTerm[probe] += (int16_t)(ki[probe] * error);
    if (iTerm[probe] > outMax)
      iTerm[probe] = outMax;
    else if (iTerm[probe] < outMin)
      iTerm[probe] = outMin;

    /* D erivative */
    dSample = (uint32_t)(sample.value.temp - lastSample[probe]);

    /*Compute PID Output*/
    output[probe] = (int16_t)(kp[probe] * error) + iTerm[probe] - (kd[probe] * dSample);

    /*Remember some variables for next time*/
    lastSample[probe] = sample.value.temp;
    lastTime[probe] = currentTime;

    /* return output */
    return output[probe];
  }
  return output[probe];
}


// void set_gains(probe_id_t probe, int16_t Kp, int16_t Ki, int16_t Kd)
void set_gains(probe_id_t probe)
{
  //if (Kp[probe]<0 || Ki[probe]<0|| Kd[probe]<0) return;

  float sampleTimeInSec = ((float)sampleTime)/1000;

  inAuto = AUTOMATIC;

  kp[probe] = .5;
  ki[probe] = 0 * sampleTimeInSec;
  kd[probe] = 0 / sampleTimeInSec;

  if(controllerDirection == REVERSE) {
    kp[probe] = (0 - kp[probe]);
    ki[probe] = (0 - ki[probe]);
    kd[probe] = (0 - kd[probe]);
  }
}


void set_sample_time(probe_id_t probe, uint16_t newSampleTime)
{
   if (newSampleTime > 0) {
      float ratio  = (double)newSampleTime
                      / (double)sampleTime;
      ki[probe] *= ratio;
      kd[probe] /= ratio;
      sampleTime = (uint16_t)newSampleTime;
   }
}

void SetOutputLimits(probe_id_t probe, double Min, double Max)
{
   if(Min > Max) return;
   outMin = Min;
   outMax = Max;

   if(output[probe] > outMax)
     output[probe] = outMax;
   else if(output[probe] < outMin)
     output[probe] = outMin;

   if(iTerm[probe] > outMax)
     iTerm[probe] = outMax;
   else if(iTerm[probe] < outMin)
     iTerm[probe] = outMin;
}

void set_mode(uint8_t Mode, probe_id_t probe, sensor_sample_t sample)
{
  bool newAuto = (Mode == AUTOMATIC);
  if(newAuto && !inAuto) {
    /*we just went from manual to auto*/
    pid_reinit(probe, sample);
  }
  inAuto = newAuto;
}

void pid_reinit(probe_id_t probe, sensor_sample_t sample)
{
  lastSample[probe] = sample.value.temp;
  iTerm[probe] = output[probe];

  if(iTerm[probe] > outMax)
    iTerm[probe] = outMax;
  else if(iTerm[probe] < outMin)
    iTerm[probe]= outMin;
}

void SetControllerDirection(uint8_t Direction)
{
   controllerDirection = Direction;
}
