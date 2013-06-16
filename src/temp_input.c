#include "temp_input.h"
#include "onewire.h"
#include "common.h"
#include "message.h"


static msg_t temp_input_thread(void* arg);
static bool temp_get_reading(temp_port_t* tp, float* temp);
static void send_temp_msg(thread_msg_id_t id, float temp);


temp_port_t*
temp_input_init(SerialDriver* port)
{
  temp_port_t* tp = calloc(1, sizeof(temp_port_t));

  tp->bus.port = port;
  onewire_init(&tp->bus);

  tp->thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO+1, temp_input_thread, tp);

  return tp;
}

static msg_t
temp_input_thread(void* arg)
{
  temp_port_t* tp = arg;

  while (1) {
    float temp;

    if (temp_get_reading(tp, &temp)) {
      tp->connected = true;
      tp->last_temp_time = chTimeNow();
      send_temp_msg(MSG_NEW_TEMP, temp);
    }
    else if ((chTimeNow() - tp->last_temp_time) > S2ST(5)) {
      if (tp->connected) {
        tp->connected = false;
        send_temp_msg(MSG_PROBE_TIMEOUT, 0);
      }
    }
  }

  return 0;
}

static void
send_temp_msg(thread_msg_id_t id, float temp)
{
  msg_broadcast(id, &temp);
}

static bool
temp_get_reading(temp_port_t* tp, float* temp)
{
  uint8_t addr[8];
  if (!onewire_reset(&tp->bus)) {
    return false;
  }
  if (!onewire_read_rom(&tp->bus, addr)) {
    return false;
  }

  if (addr[0] != 0x28) {
    return false;
  }

  // issue a T convert command
  if (!onewire_reset(&tp->bus))
    return false;
  if (!onewire_send_byte(&tp->bus, SKIP_ROM))
    return false;
  if (!onewire_send_byte(&tp->bus, 0x44))
    return false;

  // wait for device to signal conversion complete
  chThdSleepMilliseconds(100);
  while (1) {
    uint8_t bit;
    if (!onewire_recv_bit(&tp->bus, &bit))
      return false;

    if (bit)
      break;
    else
      chThdSleepMilliseconds(10);
  }

  // read the scratchpad register
  if (!onewire_reset(&tp->bus)) {
    return false;
  }
  if (!onewire_send_byte(&tp->bus, SKIP_ROM))
    return false;
  if (!onewire_send_byte(&tp->bus, 0xBE))
    return false;
  uint8_t t1, t2;
  if (!onewire_recv_byte(&tp->bus, &t1))
    return false;
  if (!onewire_recv_byte(&tp->bus, &t2))
    return false;

  uint16_t t = (t2 << 8) + t1;
  *temp = (t * 0.0625);

  return true;
}

