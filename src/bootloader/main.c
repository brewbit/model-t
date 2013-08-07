
#include "ch.h"
#include "hal.h"

#include "chprintf.h"
#include "app_hdr.h"


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

  chprintf((BaseChannel*)&SD4, "  Reset source: ");

  if (csr & RCC_CSR_BORRSTF)
    chprintf((BaseChannel*)&SD4, "BORRSTF\r\n");

  if (csr & RCC_CSR_PADRSTF)
    chprintf((BaseChannel*)&SD4, "PADRSTF\r\n");

  if (csr & RCC_CSR_PORRSTF)
    chprintf((BaseChannel*)&SD4, "PORRSTF\r\n");

  if (csr & RCC_CSR_SFTRSTF)
    chprintf((BaseChannel*)&SD4, "SFTRSTF\r\n");

  if (csr & RCC_CSR_WDGRSTF)
    chprintf((BaseChannel*)&SD4, "WDGRSTF\r\n");

  if (csr & RCC_CSR_WWDGRSTF)
    chprintf((BaseChannel*)&SD4, "WWDGRSTF\r\n");

  if (csr & RCC_CSR_LPWRRSTF)
    chprintf((BaseChannel*)&SD4, "LPWRRSTF\r\n");

  RCC->CSR |= RCC_CSR_RMVF;
}

int
main(void)
{
  halInit();
  chSysInit();

  /* start stdout port */
  sdStart(&SD4, NULL);

  chprintf((BaseChannel*)&SD4, "Started bootloader\r\n");

  check_reset_source();

  chprintf((BaseChannel*)&SD4, "  app_hdr.magic = 0x%x\r\n", _app_hdr.magic);


  if (_app_hdr.magic == 0xDEADBEEF) {
    chprintf((BaseChannel*)&SD4, "Valid app header found.  Jumping to app.\r\n");
    chThdSleepMilliseconds(100);
    jump_to_app(0x08008200);
  }
  else {
    chprintf((BaseChannel*)&SD4, "Invalid app header found...\r\n");
  }

  while (TRUE) {
    palSetPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
    palClearPad(GPIOB, GPIOB_LED1);
    chThdSleepMilliseconds(500);
  }
}
