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

// NIST ITS-90 Thermocouple Conversion Algorithm
// http://srdata.nist.gov/its90/download/type_k.tab
// vmeas = mV
// tref = deg C
float
tc_convert(float vmeas, float tref)
{
  // coefficients for converting a temperature in the range 0 to 1372 deg C
  // into the corresponding K type thermocouple voltage
  const float c_v_hi[10] = {
      -0.176004136860E-01f,
       0.389212049750E-01f,
       0.185587700320E-04f,
      -0.994575928740E-07f,
       0.318409457190E-09f,
      -0.560728448890E-12f,
       0.560750590590E-15f,
      -0.320207200030E-18f,
       0.971511471520E-22f,
      -0.121047212750E-25f
  };
  const float a0 = 0.118597600000;
  const float a1 = -0.118343200000e-03;
  const float a2 = 0.126968600000e+03;

  // coefficients for converting a temperature in the range -270 to 0 deg C
  // into the corresponding K type thermocouple voltage
  const float c_v_lo[11] = {
      0.000000000000E+00f,
      0.394501280250E-01f,
      0.236223735980E-04f,
     -0.328589067840E-06f,
     -0.499048287770E-08f,
     -0.675090591730E-10f,
     -0.574103274280E-12f,
     -0.310888728940E-14f,
     -0.104516093650E-16f,
     -0.198892668780E-19f,
     -0.163226974860E-22f
  };

  // coefficients for converting a thermocouple voltage in the range
  // -5.891 to 0.000 mV to the corresponding temperature
  const float c_t_lo[9] = {
      0.0000000E+00f,
      2.5173462E+01f,
     -1.1662878E+00f,
     -1.0833638E+00f,
     -8.9773540E-01f,
     -3.7342377E-01f,
     -8.6632643E-02f,
     -1.0450598E-02f,
     -5.1920577E-04f
  };

  // coefficients for converting a thermocouple voltage in the range
  // 0.000 to 20.644 mV to the corresponding temperature
   const float c_t_mid[10] = {
       0.000000E+00f,
       2.508355E+01f,
       7.860106E-02f,
      -2.503131E-01f,
       8.315270E-02f,
      -1.228034E-02f,
       9.804036E-04f,
      -4.413030E-05f,
       1.057734E-06f,
      -1.052755E-08f
   };

   // coefficients for converting a thermocouple voltage in the range
   // 20.644 to 54.886 mV to the corresponding temperature
   const float c_t_hi[7] = {
      -1.318058E+02f,
       4.830222E+01f,
      -1.646031E+00f,
       5.464731E-02f,
      -9.650715E-04f,
       8.802193E-06f,
      -3.110810E-08f
   };

  // Calculate the thermocouple voltage corresponding to the measured
  // cold junction temp
  float vref;
  if (tref > 0) {
    float tref_a2 = (tref - a2);
    vref = calc_polynomial(tref, c_v_hi, 10) + (a0 * exp(a1 * (tref_a2 * tref_a2)));
  }
  else {
    vref = calc_polynomial(tref, c_v_lo, 11);
  }

  // Calculate the compensated thermocouple voltage
  float v_comp = vmeas + vref;

  // Calculate the hot junction temperature from the compensated voltage
  float t;
  if (v_comp < 0.0f)
    t = calc_polynomial(v_comp, c_t_lo, 9);
  else if (v_comp < 20.644f)
    t = calc_polynomial(v_comp, c_t_mid, 10);
  else
    t = calc_polynomial(v_comp, c_t_hi, 7);

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
