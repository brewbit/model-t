#include "temp_input.h"
#include "onewire.h"
#include "common.h"
#include "message.h"


#define PROBE_TIMEOUT S2ST(2)


typedef struct temp_port_s {
  probe_id_t probe;
  onewire_bus_t* bus;
  Thread* thread;
  systime_t last_temp_time;
  bool connected;
} temp_port_t;


static msg_t temp_input_thread(void* arg);
static bool temp_get_reading(temp_port_t* tp, temperature_t* temp);
static void send_temp_msg(temp_port_t* tp, temperature_t temp);
static void send_timeout_msg(temp_port_t* tp);


temp_port_t*
temp_input_init(probe_id_t probe, onewire_bus_t* port)
{
  temp_port_t* tp = calloc(1, sizeof(temp_port_t));

  tp->probe = probe;
  tp->bus = port;
  onewire_init(tp->bus);

  tp->thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO+1, temp_input_thread, tp);

  return tp;
}

static msg_t
temp_input_thread(void* arg)
{
  temp_port_t* tp = arg;

  while (1) {
    temperature_t temp;

    if (temp_get_reading(tp, &temp)) {
    	if (temp < DEGF(300)) {
		  tp->connected = true;
		  tp->last_temp_time = chTimeNow();
		  send_temp_msg(tp, temp);
    	}
    }
    else if ((chTimeNow() - tp->last_temp_time) > PROBE_TIMEOUT) {
      if (tp->connected) {
        tp->connected = false;
        send_timeout_msg(tp);
      }
    }
  }

  return 0;
}

static void
send_temp_msg(temp_port_t* tp, temperature_t temp)
{
  temp_msg_t msg = {
      .probe = tp->probe,
      .temp = temp
  };
  msg_broadcast(MSG_NEW_TEMP, &msg);
}

static void
send_timeout_msg(temp_port_t* tp)
{
  probe_timeout_msg_t msg = {
      .probe = tp->probe
  };
  msg_broadcast(MSG_PROBE_TIMEOUT, &msg);
}

static bool
temp_get_reading(temp_port_t* tp, temperature_t* temp)
{
  uint8_t addr[8];
  if (!onewire_reset(tp->bus)) {
    return false;
  }
  if (!onewire_read_rom(tp->bus, addr)) {
    return false;
  }

  if (addr[0] != 0x28) {
    return false;
  }

  // issue a T convert command
  if (!onewire_reset(tp->bus))
    return false;
  if (!onewire_send_byte(tp->bus, SKIP_ROM))
    return false;
  if (!onewire_send_byte(tp->bus, 0x44))
    return false;

  // wait for device to signal conversion complete
  chThdSleepMilliseconds(100);
  while (1) {
    uint8_t bit;
    if (!onewire_recv_bit(tp->bus, &bit))
      return false;

    if (bit)
      break;
    else
      chThdSleepMilliseconds(10);
  }

  // read the scratchpad register
  if (!onewire_reset(tp->bus)) {
    return false;
  }
  if (!onewire_send_byte(tp->bus, SKIP_ROM))
    return false;
  if (!onewire_send_byte(tp->bus, 0xBE))
    return false;
  uint8_t t1, t2;
  if (!onewire_recv_byte(tp->bus, &t1))
    return false;
  if (!onewire_recv_byte(tp->bus, &t2))
    return false;

  uint16_t t = (t2 << 8) + t1;
  *temp = (temperature_t)((t * 100) / 16);

  return true;
}

