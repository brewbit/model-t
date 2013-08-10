
#include "ch.h"
#include "temp_control.h"
#include "sensor.h"
#include "common.h"
#include "message.h"
#include "app_cfg.h"
#include "pid.h"

#include <stdlib.h>

typedef struct {
  bool     compressor_delay_active;
  uint32_t compressor_delay_start_time;
} compressor_delay_t;

typedef struct {
  temp_port_t*    port;
  sensor_sample_t last_sample;
  bool            active;
} temp_input_t;

typedef struct {
  uint32_t           gpio;
  compressor_delay_t compressor_delay_values;
  pid_t              pid;
} relay_output_t;


static msg_t temp_control_thread(void* arg);
static void dispatch_temp_input_msg(msg_id_t id, void* msg_data, void* user_data);
static void dispatch_output_settings(output_settings_msg_t* msg);
static void dispatch_probe_settings(probe_settings_msg_t* msg);
static void dispatch_new_temp(sensor_msg_t* msg);
static void dispatch_probe_timeout(probe_timeout_msg_t* msg);
static void trigger_output(probe_id_t probe, output_function_t function);
static void evaluate_setpoint(probe_id_t probe, sensor_sample_t setpoint, sensor_sample_t sample);
static void for_each_output(probe_id_t probe, void (*function)(output_id_t output_id, void* user_data), void* user_data);
static void pid_reinit_predicate(output_id_t output_id, void* user_data);
static void manage_pid(void);
static void manage_compressor_delay(void);
static void start_compressor_delay(uint32_t delay_startTime);
static bool compressor_delay_has_expired(uint32_t compressor_delay, uint32_t delay_startTime);

static temp_input_t inputs[NUM_PROBES];
static relay_output_t outputs[NUM_OUTPUTS];
static Thread* thread;


void
temp_control_init()
{
  thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO, temp_control_thread, NULL);

  inputs[PROBE_1].port = temp_input_init(PROBE_1, &SD2);
  inputs[PROBE_2].port = temp_input_init(PROBE_2, &SD1);

  outputs[OUTPUT_1].gpio = GPIOB_RELAY1;
  outputs[OUTPUT_2].gpio = GPIOB_RELAY2;

  pid_init(&outputs[OUTPUT_1].pid);
  pid_init(&outputs[OUTPUT_2].pid);

  msg_subscribe(MSG_SENSOR_SAMPLE, thread, dispatch_temp_input_msg, NULL);
  msg_subscribe(MSG_SENSOR_TIMEOUT, thread, dispatch_temp_input_msg, NULL);
  msg_subscribe(MSG_SENSOR_SETTINGS, thread, dispatch_temp_input_msg, NULL);
  msg_subscribe(MSG_OUTPUT_SETTINGS, thread, dispatch_temp_input_msg, NULL);
}

static msg_t
temp_control_thread(void* arg)
{
  (void)arg;

  while (1) {
    if (chMsgIsPendingI(chThdSelf())) {
      Thread* tp = chMsgWait();
      thread_msg_t* msg = (thread_msg_t*)chMsgGet(tp);
      dispatch_temp_input_msg(msg->id, msg->msg_data, msg->user_data);
      chMsgRelease(tp, 0);
    }
    else {
      manage_compressor_delay();
      manage_pid();
    }
  }
  return 0;
}

static void manage_compressor_delay()
{
  int i;
  uint32_t current_time = chTimeNow();

  for (i = 0; i < NUM_OUTPUTS; ++i) {
    const output_settings_t* output_settings = app_cfg_get_output_settings(i);
    uint32_t delay_start_time = outputs[i].compressor_delay_values.compressor_delay_start_time;

    if ((current_time - delay_start_time) > output_settings->compressor_delay)
      outputs[i].compressor_delay_values.compressor_delay_active = false;
    else
      outputs[i].compressor_delay_values.compressor_delay_active = true;
  }
}

static void manage_pid()
{
  int i;

  for (i = 0; i < NUM_OUTPUTS; ++i) {
    const output_settings_t* output_settings = app_cfg_get_output_settings(i);
    const probe_settings_t* probe_settings = app_cfg_get_probe_settings(output_settings->trigger);

    pid_exec(&outputs[i].pid, probe_settings->setpoint, inputs[output_settings->trigger].last_sample);
  }
}

static void
dispatch_temp_input_msg(msg_id_t id, void* msg_data, void* user_data)
{
  (void)user_data;

  switch (id) {
  case MSG_SENSOR_SAMPLE:
    dispatch_sensor_sample(msg_data);
    break;

  case MSG_SENSOR_TIMEOUT:
    dispatch_sensor_timeout(msg_data);
    break;

  case MSG_SENSOR_SETTINGS:
    dispatch_sensor_settings(msg_data);
    break;

    case MSG_OUTPUT_SETTINGS:
      dispatch_output_settings(msg_data);
      break;

    default:
      break;
  }
}

static void
dispatch_sensor_sample(sensor_msg_t* msg)
{
  const sensor_settings_t* sensor_settings = app_cfg_get_sensor_settings(msg->sensor);

  evaluate_setpoint(
      msg->sensor,
      sensor_settings->setpoint,
      msg->sample);
}

static void
dispatch_sensor_timeout(sensor_timeout_msg_t* msg)
{
  const sensor_settings_t* sensor_settings = app_cfg_get_sensor_settings(msg->sensor);

  /* Set the last temp to the setpoint to disable any active outputs */
  evaluate_setpoint(
      msg->sensor,
      sensor_settings->setpoint,
      sensor_settings->setpoint);
}

static void
dispatch_output_settings(output_settings_msg_t* msg)
{
  if (msg->output >= NUM_OUTPUTS)
    return;

  /* Re-evaluate the last temp from both sensors with the new output settings */
  const sensor_settings_t* sensor_settings = app_cfg_get_sensor_settings(SENSOR_1);

  evaluate_setpoint(
      SENSOR_1,
      sensor_settings->setpoint,
      inputs[SENSOR_1].last_sample);

  sensor_settings = app_cfg_get_sensor_settings(SENSOR_2);
  evaluate_setpoint(
      SENSOR_2,
      sensor_settings->setpoint,
      inputs[SENSOR_2].last_sample);
}

static void
dispatch_sensor_settings(sensor_settings_msg_t* msg)
{
  if (msg->sensor >= NUM_SENSORS)
    return;

  int i;

  for (i = 0; i < NUM_OUTPUTS; ++i) {
    const output_settings_t* output_settings = app_cfg_get_output_settings(i);

    if (output_settings->trigger == msg->sensor) {
      pid_reinit(&outputs[i].pid, inputs[msg->sensor].last_sample);
    }
  }

  /* Re-evaluate the last temp reading with the new probe settings */
  evaluate_setpoint(
      msg->sensor,
      msg->settings.setpoint,
      inputs[msg->sensor].last_sample);
}

static void
evaluate_setpoint(sensor_id_t sensor, quantity_t setpoint, quantity_t sample)
{
  int i;

  inputs[sensor].last_sample = sample;

  for (i = 0; i < NUM_OUTPUTS; ++i) {
    const output_settings_t* output_settings = app_cfg_get_output_settings(i);

    if (output_settings->trigger == sensor) {
      int32_t pid = outputs[i].pid.pid_output;

if ((sample.value - pid) > setpoint.value) {
    trigger_output(sensor, OUTPUT_FUNC_COOLING);
  }
  else if ((sample.value + pid) < setpoint.value) {
    trigger_output(sensor, OUTPUT_FUNC_HEATING);
  }
    }
  }
}

static void
trigger_output(sensor_id_t sensor, output_function_t function)
{
  int i;

  for (i = 0; i < NUM_OUTPUTS; ++i) {
    const output_settings_t* output_settings = app_cfg_get_output_settings(i);

     if (output_settings->trigger == sensor && inputs[sensor].active) {
      if (output_settings->function == function) {
    	if(time_expired) {
    	  palSetPad(GPIOB, outputs[i].gpio);
    	}
      }
      else {
        palClearPad(GPIOB, outputs[i].gpio);
        start_compressor_delay(outputs[i].delay_startTime);
      }
    }
  }
}

static bool
compressor_delay_has_expired(uint32_t compressor_delay, uint32_t delay_startTime)
{
  uint32_t current_time = chTimeNow();

  if ((current_time - delay_startTime) > compressor_delay)
    return TRUE;
  else
    return FALSE;
}

static void
start_compressor_delay(uint32_t delay_startTime)
{
  (void)delay_startTime;
//  delay_startTime = chTimeNow();
}
