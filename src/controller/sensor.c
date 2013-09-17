#include "sensor.h"
#include "onewire.h"
#include "common.h"
#include "message.h"
#include "app_cfg.h"

#include <string.h>


#define SENSOR_TIMEOUT S2ST(2)


typedef struct sensor_port_s {
  sensor_id_t sensor;
  onewire_bus_t* bus;
  Thread* thread;
  systime_t last_sample_time;
  bool connected;
} sensor_port_t;


static msg_t sensor_thread(void* arg);
static bool sensor_get_sample(sensor_port_t* tp, quantity_t* sample);
static void send_sensor_msg(sensor_port_t* tp, quantity_t* sample);
static void send_timeout_msg(sensor_port_t* tp);

static bool read_ds18b20(sensor_port_t* tp, quantity_t* sample);
static bool read_ds2762(sensor_port_t* tp, quantity_t* sample);
static bool read_bb(sensor_port_t* tp, quantity_t* sample);

float tc_convert(float vmeas, float tref);
float calc_polynomial(float x, const float* coeffs, int num_coeffs);

sensor_port_t*
sensor_init(sensor_id_t sensor, onewire_bus_t* port)
{
  sensor_port_t* tp = chHeapAlloc(NULL, sizeof(sensor_port_t));
  memset(tp, 0, sizeof(sensor_port_t));

  tp->sensor = sensor;
  tp->bus = port;
  onewire_init(tp->bus);

  tp->thread = chThdCreateFromHeap(NULL, 1024, HIGHPRIO, sensor_thread, tp);

  return tp;
}

static msg_t
sensor_thread(void* arg)
{
  sensor_port_t* tp = arg;

  chRegSetThreadName("sensor");

  while (1) {
    quantity_t sample;

    if (sensor_get_sample(tp, &sample)) {
        tp->connected = true;
        tp->last_sample_time = chTimeNow();
        send_sensor_msg(tp, &sample);
    }
    else if ((chTimeNow() - tp->last_sample_time) > SENSOR_TIMEOUT) {
      if (tp->connected) {
        tp->connected = false;
        send_timeout_msg(tp);
      }
    }
  }

  return 0;
}

static void
send_sensor_msg(sensor_port_t* tp, quantity_t* sample)
{
  sensor_msg_t msg = {
      .sensor = tp->sensor,
      .sample = *sample
  };
  msg_broadcast(MSG_SENSOR_SAMPLE, &msg);
}

static void
send_timeout_msg(sensor_port_t* tp)
{
  sensor_timeout_msg_t msg = {
      .sensor = tp->sensor
  };
  msg_broadcast(MSG_SENSOR_TIMEOUT, &msg);
}

static bool
sensor_get_sample(sensor_port_t* tp, quantity_t* sample)
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

  case 0x30:
    return read_ds2762(tp, sample);

  case 0xBB:
    return read_bb(tp, sample);

  default:
    return false;
  }

  return true;
}

static bool
read_ds18b20(sensor_port_t* tp, quantity_t* sample)
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
read_ds2762(sensor_port_t* tp, quantity_t* sample)
{
  // Read from the current sense register which measures
  // the voltage across the thermocouple
  if (!onewire_reset(tp->bus))
    return false;
  if (!onewire_send_byte(tp->bus, SKIP_ROM))
    return false;
  // 0x69 = read memory
  if (!onewire_send_byte(tp->bus, 0x69))
    return false;
  // address 0x0E = current register MSB
  if (!onewire_send_byte(tp->bus, 0x0E))
    return false;

  uint8_t v1, v2;
  if (!onewire_recv_byte(tp->bus, &v1))
    return false;
  if (!onewire_recv_byte(tp->bus, &v2))
    return false;

  int16_t v = (v1 << 8) + v2;
  float vmeas = (v >> 3) * 0.015625f; // mV

  // Read from the temperature register which reads the temperature at
  // the chip. This will be used to correct the thermocouple reading.
  if (!onewire_reset(tp->bus))
    return false;
  if (!onewire_send_byte(tp->bus, SKIP_ROM))
    return false;
  // 0x69 = read memory
  if (!onewire_send_byte(tp->bus, 0x69))
    return false;
  // address 0x18 = temp register MSB
  if (!onewire_send_byte(tp->bus, 0x18))
    return false;

  uint8_t t1, t2;
  if (!onewire_recv_byte(tp->bus, &t1))
    return false;
  if (!onewire_recv_byte(tp->bus, &t2))
    return false;

  int16_t t = (t1 << 8) + t2;
  float tref = (t >> 5) * 0.125f; // deg C

  sample->unit = app_cfg_get_temp_unit();
  sample->value = tc_convert(vmeas, tref);
  if (sample->unit == UNIT_TEMP_DEG_F) {
    sample->value = (sample->value * 1.8f) + 32;
  }

  return true;
}

// Conversion algorithm from Agilent:
// http://www.home.agilent.com/upload/cmc_upload/All/5306OSKR-MXD-5501-040107_2.htm
// vmeas = mV
// tref = deg C
float
tc_convert(float vmeas, float tref)
{
  const float c_ref[10] = {
      -1.7600413686e-2f,
      3.8921204975e-2f,
      1.8558770032e-5f,
      -9.9457592874e-8f,
      3.1840945719e-10f,
      -5.6072844889e-13f,
      5.6075059059e-16f,
      -3.2020720003e-19f,
      9.7151147152e-23f,
      -1.2104721275e-26f
  };
  const float c_conv_0_500[10] = {
      0.0f,
      2.508355e1f,
      7.860106e-2f,
      -2.503131e-1f,
      8.315270e-2f,
      -1.228034e-2f,
      9.804036e-4f,
      -4.413030e-5f,
      1.057734e-6f,
      -1.052755e-8f
  };

  // Calculate the thermocouple voltage corresponding to the measured
  // cold junction temp
  float vref = calc_polynomial(tref, c_ref, 10);
  // Calculate the compensated thermocouple voltage
  float v_comp = vmeas + vref;
  // Calculate the hot junction temperature from the compensated voltage
  float t = calc_polynomial(v_comp, c_conv_0_500, 10);
  return t;
}

float
calc_polynomial(float x, const float* coeffs, int num_coeffs)
{
  int i;
  float p = coeffs[0];
  float x_raised = x;

  for (i = 1; i < num_coeffs; ++i) {
    p += coeffs[i] * x_raised;
    x_raised = x_raised * x;
  }

  return p;
}

static bool
read_bb(sensor_port_t* tp, quantity_t* sample)
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
