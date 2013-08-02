
#include "ch.h"
#include "temp_control.h"
#include "temp_input.h"
#include "common.h"
#include "message.h"
#include "app_cfg.h"
#include "pid.h"

#include <stdlib.h>

typedef struct {
  temp_port_t* port;
  sensor_sample_t last_sample;
} temp_input_t;

typedef struct {
  uint32_t gpio;
} relay_output_t;


static msg_t temp_control_thread(void* arg);
static void dispatch_temp_input_msg(msg_id_t id, void* msg_data, void* user_data);
static void dispatch_output_settings(output_settings_msg_t* msg);
static void dispatch_probe_settings(probe_settings_msg_t* msg);
static void dispatch_new_temp(sensor_msg_t* msg);
static void dispatch_probe_timeout(probe_timeout_msg_t* msg);
static void trigger_output(probe_id_t probe, output_function_t function);
static void evaluate_setpoint(probe_id_t probe, sensor_sample_t setpoint, sensor_sample_t sample);


static temp_input_t inputs[NUM_PROBES];
static relay_output_t outputs[NUM_OUTPUTS];
static Thread* thread;


void
temp_control_init()
{
  thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO, temp_control_thread, NULL);

  inputs[0].port = temp_input_init(PROBE_1, &SD2);
  inputs[1].port = temp_input_init(PROBE_2, &SD1);

  outputs[OUTPUT_1].gpio = GPIOB_RELAY1;
  outputs[OUTPUT_2].gpio = GPIOB_RELAY2;

  set_gains(PROBE_1);
  set_gains(PROBE_2);

  msg_subscribe(MSG_NEW_TEMP, thread, dispatch_temp_input_msg, NULL);
  msg_subscribe(MSG_PROBE_TIMEOUT, thread, dispatch_temp_input_msg, NULL);
  msg_subscribe(MSG_PROBE_SETTINGS, thread, dispatch_temp_input_msg, NULL);
  msg_subscribe(MSG_OUTPUT_SETTINGS, thread, dispatch_temp_input_msg, NULL);
}

static msg_t
temp_control_thread(void* arg)
{
  (void)arg;

  while (1) {
    Thread* tp = chMsgWait();
    thread_msg_t* msg = (thread_msg_t*)chMsgGet(tp);
    dispatch_temp_input_msg(msg->id, msg->msg_data, msg->user_data);
    chMsgRelease(tp, 0);
  }

  return 0;
}

static void
dispatch_temp_input_msg(msg_id_t id, void* msg_data, void* user_data)
{
  (void)user_data;

  switch (id) {
  case MSG_NEW_TEMP:
    dispatch_new_temp(msg_data);
    break;

  case MSG_PROBE_TIMEOUT:
    dispatch_probe_timeout(msg_data);
    break;

  case MSG_PROBE_SETTINGS:
    dispatch_probe_settings(msg_data);
    break;

  case MSG_OUTPUT_SETTINGS:
    dispatch_output_settings(msg_data);
    break;

  default:
    break;
  }
}

static void
dispatch_new_temp(sensor_msg_t* msg)
{
  const probe_settings_t* probe_settings = app_cfg_get_probe_settings(msg->probe);

  evaluate_setpoint(
      msg->probe,
      probe_settings->setpoint,
      msg->sample);
}

static void
dispatch_probe_timeout(probe_timeout_msg_t* msg)
{
  const probe_settings_t* probe_settings = app_cfg_get_probe_settings(msg->probe);

  /* Set the last temp to the setpoint to disable any active outputs */
  evaluate_setpoint(
      msg->probe,
      probe_settings->setpoint,
      probe_settings->setpoint);
}

static void
dispatch_output_settings(output_settings_msg_t* msg)
{
  if (msg->output >= NUM_OUTPUTS)
    return;

  /* Re-evaluate the last temp from both probes with the new output settings */
  const probe_settings_t* probe_settings = app_cfg_get_probe_settings(PROBE_1);

  evaluate_setpoint(
      PROBE_1,
      probe_settings->setpoint,
      inputs[PROBE_1].last_sample);

  probe_settings = app_cfg_get_probe_settings(PROBE_2);
  evaluate_setpoint(
      PROBE_2,
      probe_settings->setpoint,
      inputs[PROBE_2].last_sample);
}

static void
dispatch_probe_settings(probe_settings_msg_t* msg)
{
  if (msg->probe >= NUM_PROBES)
    return;

  /* Re-evaluate the last temp reading with the new probe settings */
  evaluate_setpoint(
      msg->probe,
      msg->settings.setpoint,
      inputs[msg->probe].last_sample);
}

static void
evaluate_setpoint(probe_id_t probe, sensor_sample_t setpoint, sensor_sample_t sample)
{
  /* Run PID */
  int16_t pid;

  pid = pid_exec(probe, setpoint, sample);

  inputs[probe].last_sample = sample;

  if ((sample.value.temp - pid) > setpoint.value.temp) {
    trigger_output(probe, OUTPUT_FUNC_COOLING);
  }
  else if ((sample.value.temp + pid) < setpoint.value.temp) {
    trigger_output(probe, OUTPUT_FUNC_HEATING);
  }
}

static void
trigger_output(probe_id_t probe, output_function_t function)
{
  int i;

  for (i = 0; i < NUM_OUTPUTS; ++i) {
    const output_settings_t* output_settings = app_cfg_get_output_settings(i);

    if (output_settings->trigger == probe) {
      if (output_settings->function == function) {
        palSetPad(GPIOB, outputs[i].gpio);
      }
      else {
        palClearPad(GPIOB, outputs[i].gpio);
      }
    }
  }
}
