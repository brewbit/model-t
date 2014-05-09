#include "sensor.h"
#include "onewire.h"
#include "common.h"
#include "message.h"
#include "app_cfg.h"
#include "crc/crc8.h"

#include <string.h>


#define SENSOR_TIMEOUT S2ST (2)
#define SENSOR_SAMPLE_SIZE  (10)
#define MAX_SAMPLE_DELTA    (40)


typedef struct sensor_port_s {
  sensor_id_t sensor;
  float sample_filter[SENSOR_SAMPLE_SIZE];
  uint8_t sample_filter_index;
  uint8_t sample_size;
  onewire_bus_t* bus;
  Thread* thread;
  float last_sample;
  systime_t last_sample_time;
  bool connected;
} sensor_port_t;


static msg_t sensor_thread(void* arg);
static bool sensor_get_sample(sensor_port_t* tp, quantity_t* sample);
static void filter_sample(sensor_port_t* tp, quantity_t* sample);
static void send_sensor_msg(sensor_port_t* tp, quantity_t* sample);
static void send_timeout_msg(sensor_port_t* tp);

static bool read_maxim_temp_sensor(sensor_port_t* tp, quantity_t* sample);


sensor_port_t*
sensor_init(sensor_id_t sensor, onewire_bus_t* port)
{
  sensor_port_t* tp = calloc(1, sizeof(sensor_port_t));

  tp->sensor = sensor;
  tp->bus = port;
  onewire_init(tp->bus);

  tp->thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO, sensor_thread, tp);

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
      filter_sample(tp, &sample);
      tp->connected = true;
      tp->last_sample_time = chTimeNow();
      send_sensor_msg(tp, &sample);
    }
    else {
      if ((chTimeNow() - tp->last_sample_time) > SENSOR_TIMEOUT) {
        if (tp->connected) {
          tp->connected = false;
          send_timeout_msg(tp);
        }
      }
      chThdSleepMilliseconds(100);
    }
  }

  return 0;
}

static void
filter_sample(sensor_port_t* tp, quantity_t* sample)
{
  float filtered_sample = 0;
  uint8_t i;

  if ((fabs(sample->value - tp->last_sample) > MAX_SAMPLE_DELTA) &&
      (tp->sample_size == SENSOR_SAMPLE_SIZE)) {
    sample->value = tp->last_sample;
  }
  else {
    tp->sample_filter[tp->sample_filter_index] = sample->value;
    if (++tp->sample_filter_index >= SENSOR_SAMPLE_SIZE)
      tp->sample_filter_index = 0;

    if (tp->sample_size < SENSOR_SAMPLE_SIZE)
      tp->sample_size++;

    for (i = 0; i < SENSOR_SAMPLE_SIZE; i++) {
      filtered_sample += tp->sample_filter[i];
    }
    sample->value = filtered_sample / tp->sample_size;
    tp->last_sample = sample->value;
  }
}

static void
send_sensor_msg(sensor_port_t* tp, quantity_t* sample)
{
  sensor_msg_t msg = {
      .sensor = tp->sensor,
      .sample = *sample
  };
  msg_send(MSG_SENSOR_SAMPLE, &msg);
}

static void
send_timeout_msg(sensor_port_t* tp)
{
  sensor_timeout_msg_t msg = {
      .sensor = tp->sensor
  };
  msg_send(MSG_SENSOR_TIMEOUT, &msg);
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
  case 0x3B: // MAX31850
  case 0x28: // DS18B20
    return read_maxim_temp_sensor(tp, sample);

  default:
    return false;
  }

  return true;
}

static bool
read_maxim_temp_sensor(sensor_port_t* tp, quantity_t* sample)
{
  int i;

  // issue a T convert command
  if (!onewire_reset(tp->bus))
    return false;
  if (!onewire_send_byte(tp->bus, SKIP_ROM))
    return false;
  if (!onewire_send_byte(tp->bus, 0x44))
    return false;

  // wait for device to signal conversion complete
  chThdSleepMilliseconds(700);
  while (1) {
    uint8_t bit;
    if (!onewire_recv_bit(tp->bus, &bit))
      return false;

    if (bit)
      break;
    else
      chThdSleepMilliseconds(100);
  }

  // read the scratchpad register
  if (!onewire_reset(tp->bus))
    return false;
  if (!onewire_send_byte(tp->bus, SKIP_ROM))
    return false;
  if (!onewire_send_byte(tp->bus, 0xBE))
    return false;

  uint8_t scratchpad[9];
  for (i = 0; i < (int)sizeof(scratchpad); ++i) {
    if (!onewire_recv_byte(tp->bus, &scratchpad[i]))
      return false;
  }

  uint8_t crc = crc8_block(0, scratchpad, 8);
  if (crc != scratchpad[8])
    return false;

  // two unsigned data bytes need to be combined and converted to a signed short
  int16_t t = (scratchpad[1] << 8) + scratchpad[0];

  // convert from 16ths of a degree Celsius to degrees Fahrenheit
  sample->unit = UNIT_TEMP_DEG_F;
  sample->value = ((t / 16.0f) * 1.8f) + 32;

  return true;
}
