
#include "ch.h"
#include "hal.h"

#include "chprintf.h"
#include "app_hdr.h"
#include "crc/crc32.h"

#include <string.h>


extern uint8_t __app_start__[];


static void
jump_to_app(uint32_t address);


static void
jump_to_app(uint32_t address)
{
    typedef void (*funcPtr)(void);

    u32 jumpAddr = *(vu32*)(address + 0x04); /* reset ptr in vector table */
    funcPtr usrMain = (funcPtr)jumpAddr;

    /* Reset all interrupts to default */
    chSysDisable();

    /* Clear pending interrupts just to be on the save side */
    SCB_ICSR = ICSR_PENDSVCLR;

    /* Disable all interrupts */
    int i;
    for(i = 0; i < 8; ++i)
        NVIC->ICER[i] = NVIC->IABR[i];

    /* Set stack pointer as in application's vector table */
    __set_MSP(*(vu32*)address);
    usrMain();
}

int
main(void)
{
  halInit();
  chSysInit();

  /* start stdout port */
  sdStart(SD_STDIO, NULL);

  chprintf(SD_STDIO, "BrewBit Model-T Bootloader v%d.%d.%d\r\n",
      MAJOR_VERSION,
      MINOR_VERSION,
      PATCH_VERSION);

  chprintf(SD_STDIO, "Searching for app... ");
  if (memcmp(_app_hdr.magic, "BBMT-APP", 8) == 0) {
    chprintf(SD_STDIO, "OK\r\n");
    chprintf(SD_STDIO, "  Version: %d.%d.%d\r\n",
        _app_hdr.major_version,
        _app_hdr.minor_version,
        _app_hdr.patch_version);
    chprintf(SD_STDIO, "  Image Size: %d bytes\r\n", _app_hdr.img_size);
    chprintf(SD_STDIO, "  CRC: 0x%08x\r\n", _app_hdr.crc);

    chprintf(SD_STDIO, "Verifying CRC... ");
    uint32_t crc_calc = crc32_block(0xffffffff, __app_start__, _app_hdr.img_size) ^ 0xffffffff;

    if (crc_calc == _app_hdr.crc) {
      chprintf(SD_STDIO, "OK\r\n");

      chprintf(SD_STDIO, "Starting app.\r\n");
      chThdSleepMilliseconds(100);
      jump_to_app(0x08008200);
    }
    else {
      chprintf(SD_STDIO, "ERROR\r\n");
      chprintf(SD_STDIO, "  Calculated CRC: 0x%08x\r\n", crc_calc);
    }
  }
  else {
    chprintf(SD_STDIO, "ERROR\r\n");
    chprintf(SD_STDIO, "  No valid app found.\r\n");
  }

  while (TRUE) {
    palSetPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(500);
    palClearPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(500);
  }
}
