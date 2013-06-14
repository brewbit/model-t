
#include "ch.h"
#include "temp_control.h"
#include "temp_input.h"
#include "common.h"

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
static void dispatch_temp_input_msg(temp_control_t* tc, temp_input_msg_t* msg);


void
temp_control_init()
{
  temp_control_t* tc = calloc(1, sizeof(temp_control_t));

  tc->thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO, temp_control_thread, tc);

  tc->input1.port = temp_input_init(&SD1, tc->thread);
  tc->input2.port = temp_input_init(&SD2, tc->thread);
}

static msg_t
temp_control_thread(void* arg)
{
  temp_control_t* tc = arg;

  chprintf(stdout, "starting temp control thread\r\n");

  while (1) {
    Thread* tp = chMsgWait();
    msg_t msg = chMsgGet(tp);
    dispatch_temp_input_msg(tc, (temp_input_msg_t*)msg);
    chMsgRelease(tp, 0);
  }

  return 0;
}

static void
dispatch_temp_input_msg(temp_control_t* tc, temp_input_msg_t* msg)
{
  switch (msg->id) {
  case MSG_NEW_TEMP:
//    dispatch_new_temp(tc, msg->temp);
    chprintf(stdout, "got temp %d\r\n", (int)msg->temp);
    break;

  case MSG_PROBE_TIMEOUT:
    break;

  default:
    break;
  }
}
