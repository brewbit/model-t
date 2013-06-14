#include "temp_input.h"
#include "onewire.h"
#include "common.h"


static msg_t temp_input_thread(void* arg);
static bool temp_get_reading(temp_port_t* tp, float* temp);
static void send_temp_msg(temp_port_t* tp, temp_input_msg_id_t id, float temp);


temp_port_t*
temp_input_init(SerialDriver* port, Thread* handler)
{
  temp_port_t* tp = calloc(1, sizeof(temp_port_t));

  tp->handler = handler;

  tp->bus.port = port;
  onewire_init(&tp->bus);
  tp->thread = chThdCreateStatic(tp->wa_thread, sizeof(tp->wa_thread), NORMALPRIO, temp_input_thread, tp);

  return tp;
}

static msg_t
temp_input_thread(void* arg)
{
  temp_port_t* tp = arg;

  while (1) {
    float temp;

    chSysLock();
    bool got_reading = temp_get_reading(tp, &temp);
    chSysUnlock();

    if (got_reading) {
      tp->connected = true;
      tp->last_temp_time = chTimeNow();
      send_temp_msg(tp, MSG_NEW_TEMP, temp);
    }
    else if ((chTimeNow() - tp->last_temp_time) > S2ST(5)) {
      if (tp->connected) {
        tp->connected = false;
        send_temp_msg(tp, MSG_PROBE_TIMEOUT, 0);
      }
    }

    chThdSleepSeconds(1);
  }

  return 0;
}

static void
send_temp_msg(temp_port_t* tp, temp_input_msg_id_t id, float temp)
{
  temp_input_msg_t msg = {
      .id = id,
      .temp = temp
  };
  if (tp->handler != NULL)
    chMsgSend(tp->handler, (msg_t)&msg);
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
  while (1) {
    uint8_t bit;
    if (!onewire_recv_bit(&tp->bus, &bit))
      return false;

    if (bit)
      break;
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

