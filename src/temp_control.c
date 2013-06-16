
#include "ch.h"
#include "temp_control.h"
#include "temp_input.h"
#include "common.h"
#include "message.h"

#include <stdlib.h>

typedef struct {
  temp_port_t* port;
  float setpoint;
} temp_input_t;

typedef struct {
  temp_input_t input1;
  temp_input_t input2;
  Thread* thread;
} temp_control_t;


static msg_t temp_control_thread(void* arg);
static void dispatch_temp_input_msg(msg_id_t id, void* msg_data, void* user_data);


void
temp_control_init()
{
  temp_control_t* tc = calloc(1, sizeof(temp_control_t));

  tc->thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO, temp_control_thread, tc);

//  tc->input1.port = temp_input_init(&SD1);
  tc->input2.port = temp_input_init(&SD2);

  msg_subscribe(MSG_NEW_TEMP, tc->thread, dispatch_temp_input_msg, tc);
  msg_subscribe(MSG_PROBE_TIMEOUT, tc->thread, dispatch_temp_input_msg, tc);
}

static msg_t
temp_control_thread(void* arg)
{
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
//    dispatch_new_temp(tc, msg->temp);
    chprintf(stdout, "got temp %d\r\n", (int)*((float*)msg_data));
    break;

  case MSG_PROBE_TIMEOUT:
    break;

  default:
    break;
  }
}
