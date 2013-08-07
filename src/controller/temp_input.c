#include "temp_input.h"
#include "onewire.h"
#include "common.h"
#include "message.h"
#include "app_cfg.h"


#define PROBE_TIMEOUT S2ST(2)


typedef struct temp_port_s {
  probe_id_t probe;
  onewire_bus_t* bus;
  Thread* thread;
  systime_t last_temp_time;
  bool connected;
} temp_port_t;


static msg_t temp_input_thread(void* arg);
static bool temp_get_sample(temp_port_t* tp, quantity_t* temp);
static void send_temp_msg(temp_port_t* tp, quantity_t* sample);
static void send_timeout_msg(temp_port_t* tp);

static bool read_ds18b20(temp_port_t* tp, quantity_t* temp);
static bool read_bb(temp_port_t* tp, quantity_t* sample);


temp_port_t*
temp_input_init(probe_id_t probe, onewire_bus_t* port)
{
  temp_port_t* tp = calloc(1, sizeof(temp_port_t));

  tp->probe = probe;
  tp->bus = port;
  onewire_init(tp->bus);

  tp->thread = chThdCreateFromHeap(NULL, 1024, HIGHPRIO, temp_input_thread, tp);

  return tp;
}

static msg_t
temp_input_thread(void* arg)
{
  temp_port_t* tp = arg;

  while (1) {
    quantity_t sample;

    if (temp_get_sample(tp, &sample)) {
        tp->connected = true;
        tp->last_temp_time = chTimeNow();
        send_temp_msg(tp, &sample);
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
send_temp_msg(temp_port_t* tp, quantity_t* sample)
{
  sensor_msg_t msg = {
      .probe = tp->probe,
      .sample = *sample
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
temp_get_sample(temp_port_t* tp, quantity_t* sample)
{
  uint8_t addr[8];
  if (!onewire_reset(tp->bus)) {
    return false;
  }
  if (!onewire_read_rom(tp->bus, addr)) {
    return false;
  }

  switch (addr[0]) {
  case 0x28:
    return read_ds18b20(tp, sample);

  case 0xBB:
    return read_bb(tp, sample);

  default:
    return false;
  }

  return true;
}

static bool
read_ds18b20(temp_port_t* tp, quantity_t* sample)
{
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
  sample->unit = app_cfg_get_temp_unit();
  sample->value = (t / 16.0f);
  if (sample->unit == UNIT_TEMP_DEG_F) {
    sample->value = (sample->value * 1.8f) + 32;
  }

  return true;
}

static bool
read_bb(temp_port_t* tp, quantity_t* sample)
{
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
  sample->unit = UNIT_HUMIDITY_PCT;
  sample->value = t;

  return true;
}
