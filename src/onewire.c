
#include "onewire.h"
#include "common.h"
#include "terminal.h"
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
  sdStart(ob->port, &cfg_115k);
}

bool
onewire_reset(onewire_bus_t* ob)
{
//  terminal_write("resetting onewire bus\n");

  sdStart(ob->port, &cfg_9600);
  sdPut(ob->port, 0xF0);
  uint8_t recv = sdGet(ob->port);
  sdStart(ob->port, &cfg_115k);
  if (recv == 0xF0) {
//    terminal_write("no devices present on bus\n");
    return false;
  }
  else {
//    terminal_write("devices present on bus\n");
//    terminal_write_hex(recv);
    return true;
  }
}

// Reads the ROM from the device on the bus.
bool
onewire_read_rom(onewire_bus_t* ob, uint8_t* addr)
{
  int i;

  onewire_send_byte(ob, READ_ROM);

  for (i = 0; i < 8; ++i) {
    addr[i] = onewire_recv_byte(ob);
  }

  uint8_t crc = crc8(addr, 7);
  return (crc == addr[7]);
}

void
onewire_send_byte(onewire_bus_t* ob, uint8_t b)
{
  int i;
  for (i = 0; i < 8; ++i) {
    onewire_send_bit(ob, TESTBIT(&b, i));
  }
}

uint8_t
onewire_recv_byte(onewire_bus_t* ob)
{
  int i;
  uint8_t b = 0;
  for (i = 0; i < 8; ++i) {
    ASSIGNBIT(&b, i, onewire_recv_bit(ob));
  }
  return b;
}

uint8_t
onewire_recv_bit(onewire_bus_t* ob)
{
  sdPut(ob->port, 0xFF);
  u16 bit = sdGet(ob->port);
  if (bit == 0xFF)
    return 1;
  else
    return 0;
}

void
onewire_send_bit(onewire_bus_t* ob, uint8_t b)
{
  if (b)
    sdPut(ob->port, 0xFF);
  else
    sdPut(ob->port, 0x00);
  sdGet(ob->port);
}
