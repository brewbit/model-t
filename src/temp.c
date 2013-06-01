#include "temp.h"
#include "onewire.h"
#include "terminal.h"


static msg_t temp_thread(void* arg);
static bool temp_get_reading(temp_port_t* tp, float* temp);


void
temp_init(temp_port_t* tp)
{
  onewire_init(&tp->ob);
  tp->thread = chThdCreateStatic(tp->wa_thread, sizeof(tp->wa_thread), NORMALPRIO, temp_thread, tp);
}

static msg_t
temp_thread(void* arg)
{
  temp_port_t* tp = arg;

  while (1) {
    float currentTempC;
    float currentTempF;

    if (temp_get_reading(tp, &currentTempC)) {
      currentTempF = (currentTempC * 1.8) + 32;
      terminal_write("t: ");
      terminal_write_int((uint32_t) currentTempF);
      terminal_write(".");
      terminal_write_int((uint32_t) (10 * (currentTempF - (uint32_t) currentTempF)));
      terminal_write("\n");

  //    if (currentTempF > setpointTemp) {
  //      relay_on(RELAY_OUTPUT1);
  //      relay_off(RELAY_OUTPUT2);
  //    }
  //    else if (currentTempF < setpointTemp) {
  //      relay_off(RELAY_OUTPUT1);
  //      relay_on(RELAY_OUTPUT2);
  //    }
    }

    chThdSleepSeconds(1);
    terminal_clear();
  }

  return 0;
}

static bool
temp_get_reading(temp_port_t* tp, float* temp)
{
  //int i;
  uint8_t addr[8];
  if (!onewire_reset(&tp->ob)) {
    return false;
  }
  if (!onewire_read_rom(&tp->ob, addr)) {
    //terminal_write("crc does not match\n");
    return false;
  }

//  terminal_write("crc matches\n");
//
//  terminal_write("addr is ");
//  for (i = 0; i < 7; ++i) {
//    terminal_write_hex(addr[i]);
//    terminal_write(" ");
//  }
//  terminal_write("\n");

  if (addr[0] != 0x28) {
//    terminal_write("unrecognized device: ");
//    terminal_write_hex(addr[0]);
//    terminal_write("\n");
    return false;
  }

  // issue a T convert command
  onewire_reset(&tp->ob);
  onewire_send_byte(&tp->ob, SKIP_ROM);
  onewire_send_byte(&tp->ob, 0x44);
  // wait for device to signal conversion complete
  while (onewire_recv_bit(&tp->ob) == 0)
    ;

  // read the scratchpad register
  onewire_reset(&tp->ob);
  onewire_send_byte(&tp->ob, SKIP_ROM);
  onewire_send_byte(&tp->ob, 0xBE);
  int16_t t = onewire_recv_byte(&tp->ob);
  t |= onewire_recv_byte(&tp->ob) << 8;

  *temp = (t * 0.0625);
  return true;
}

