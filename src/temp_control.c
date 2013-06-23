
#include "ch.h"
#include "temp_control.h"
#include "temp_input.h"
#include "common.h"
#include "message.h"

#include <stdlib.h>

typedef struct {
  temp_port_t* port;
  probe_settings_t settings;
} temp_input_t;

typedef struct {
  uint32_t gpio;
  output_settings_t settings;
} relay_output_t;


static msg_t temp_control_thread(void* arg);
static void dispatch_temp_input_msg(msg_id_t id, void* msg_data, void* user_data);
static void dispatch_output_settings(output_settings_msg_t* msg);
static void dispatch_probe_settings(probe_settings_msg_t* msg);
static void dispatch_new_temp(temp_msg_t* msg);
static void trigger_output(probe_id_t probe, output_function_t function);


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

  msg_subscribe(MSG_NEW_TEMP, thread, dispatch_temp_input_msg, NULL);
  msg_subscribe(MSG_PROBE_TIMEOUT, thread, dispatch_temp_input_msg, NULL);
  msg_subscribe(MSG_PROBE_SETTINGS, thread, dispatch_temp_input_msg, NULL);
  msg_subscribe(MSG_OUTPUT_SETTINGS, thread, dispatch_temp_input_msg, NULL);
}

probe_settings_t
temp_control_get_probe_settings(probe_id_t probe)
{
  return inputs[probe].settings;
}

output_settings_t
temp_control_get_output_settings(output_id_t output)
{
  return outputs[output].settings;
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
dispatch_new_temp(temp_msg_t* msg)
{
  temperature_t setpoint = inputs[msg->probe].settings.setpoint;
  if (msg->temp > setpoint) {
    trigger_output(msg->probe, OUTPUT_FUNC_COOLING);
  }
  else if (msg->temp < setpoint) {
    trigger_output(msg->probe, OUTPUT_FUNC_HEATING);
  }
}

static void
trigger_output(probe_id_t probe, output_function_t function)
{
  int i;
  for (i = 0; i < NUM_OUTPUTS; ++i) {
    if (outputs[i].settings.trigger == probe) {
      if (outputs[i].settings.function == function) {
        palSetPad(GPIOB, outputs[i].gpio);
      }
      else {
        palClearPad(GPIOB, outputs[i].gpio);
      }
    }
  }
}

static void
dispatch_output_settings(output_settings_msg_t* msg)
{
  if (msg->output >= NUM_OUTPUTS)
    return;

  outputs[msg->output].settings = msg->settings;
}

static void
dispatch_probe_settings(probe_settings_msg_t* msg)
{
  if (msg->probe >= NUM_PROBES)
    return;

  inputs[msg->probe].settings = msg->settings;
}
