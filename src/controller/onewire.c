
#include "onewire.h"
#include "common.h"
#include "crc.h"

static const SerialConfig cfg_115k = {
    .sc_speed = 115200,
    .sc_cr1 = 0,
    .sc_cr2 = USART_CR2_STOP1_BITS,
    .sc_cr3 = USART_CR3_HDSEL,
};

static const SerialConfig cfg_9600 = {
    .sc_speed = 9600,
    .sc_cr1 = 0,
    .sc_cr2 = USART_CR2_STOP1_BITS,
    .sc_cr3 = USART_CR3_HDSEL,
};

void
onewire_init(onewire_bus_t* ob)
{
  sdStart(ob, &cfg_115k);
}

bool
onewire_reset(onewire_bus_t* ob)
{
  sdStart(ob, &cfg_9600);

  sdPut(ob, 0xF0);
  msg_t recv = sdGetTimeout(ob, MS2ST(100));

  sdStart(ob, &cfg_115k);

  if (recv < 0) {
    return false;
  }
  if (recv == 0xF0) {
    return false;
  }
  else {
    return true;
  }
}

// Reads the ROM from the device on the bus.
bool
onewire_read_rom(onewire_bus_t* ob, uint8_t* addr)
{
  int i;

  if (!onewire_send_byte(ob, READ_ROM))
    return false;

  for (i = 0; i < 8; ++i) {
    if (!onewire_recv_byte(ob, &addr[i]))
      return false;
  }

  uint8_t crc = crc8(addr, 7);
  return (crc == addr[7]);
}

bool
onewire_send_byte(onewire_bus_t* ob, uint8_t b)
{
  int i;
  for (i = 0; i < 8; ++i) {
    if (!onewire_send_bit(ob, TESTBIT(&b, i)))
      return false;
  }

  return true;
}

bool
onewire_recv_byte(onewire_bus_t* ob, uint8_t* b)
{
  int i;
  uint8_t v = 0;
  for (i = 0; i < 8; ++i) {
    uint8_t bit;
    if (!onewire_recv_bit(ob, &bit))
      return false;
    ASSIGNBIT(&v, i, bit);
  }

  *b = v;
  return true;
}

bool
onewire_recv_bit(onewire_bus_t* ob, uint8_t* bit)
{
  sdPut(ob, 0xFF);
  msg_t ret = sdGetTimeout(ob, MS2ST(100));
  if (ret < 0) {
    return false;
  }

  if (ret == 0xFF)
    *bit = 1;
  else
    *bit = 0;

  return true;
}

bool
onewire_send_bit(onewire_bus_t* ob, uint8_t b)
{
  if (b)
    sdPut(ob, 0xFF);
  else
    sdPut(ob, 0x00);

  msg_t ret = sdGetTimeout(ob, MS2ST(100));
  if (ret < 0) {
    return false;
  }

  return true;
}
