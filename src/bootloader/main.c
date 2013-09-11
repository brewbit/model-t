
#include "ch.h"
#include "hal.h"

#include "chprintf.h"
#include "app_hdr.h"


static void
jump_to_app(uint32_t address);

static void
check_reset_source(void);


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

static void
check_reset_source()
{
  int csr = RCC->CSR;

  chprintf(SD_STDIO, "  Reset source: ");

  if (csr & RCC_CSR_BORRSTF)
    chprintf(SD_STDIO, "BORRSTF\r\n");

  if (csr & RCC_CSR_PADRSTF)
    chprintf(SD_STDIO, "PADRSTF\r\n");

  if (csr & RCC_CSR_PORRSTF)
    chprintf(SD_STDIO, "PORRSTF\r\n");

  if (csr & RCC_CSR_SFTRSTF)
    chprintf(SD_STDIO, "SFTRSTF\r\n");

  if (csr & RCC_CSR_WDGRSTF)
    chprintf(SD_STDIO, "WDGRSTF\r\n");

  if (csr & RCC_CSR_WWDGRSTF)
    chprintf(SD_STDIO, "WWDGRSTF\r\n");

  if (csr & RCC_CSR_LPWRRSTF)
    chprintf(SD_STDIO, "LPWRRSTF\r\n");

  RCC->CSR |= RCC_CSR_RMVF;
}

int
main(void)
{
  halInit();
  chSysInit();

  /* start stdout port */
  sdStart(SD_STDIO, NULL);

  chprintf(SD_STDIO, "Started bootloader\r\n");

  check_reset_source();

  chprintf(SD_STDIO, "  app_hdr.magic = 0x%x\r\n", _app_hdr.magic);


  if (_app_hdr.magic == 0xDEADBEEF) {
    chprintf(SD_STDIO, "Valid app header found.  Jumping to app.\r\n");
    chThdSleepMilliseconds(100);
    jump_to_app(0x08008200);
  }
  else {
    chprintf(SD_STDIO, "Invalid app header found...\r\n");
  }

  while (TRUE) {
    palSetPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(500);
    palClearPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(500);
  }
}
