
#include "ch.h"
#include "hal.h"
#include "fault.h"
#include "app_cfg.h"

#include <stdint.h>


void
dump_stack(uint32_t *pulFaultStackAddress);

typedef struct {
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r12;
  uint32_t lr; /* Link register. */
  uint32_t pc; /* Program counter. */
  uint32_t psr;/* Program status register. */
  uint32_t _CFSR ;
  uint32_t _HFSR ;
  uint32_t _DFSR ;
  uint32_t _AFSR ;
  uint32_t _BFAR ;
  uint32_t _MMAR ;
  uint32_t _SCB_SHCSR;
} hard_fault_data_t;

hard_fault_data_t hfd;

void port_halt(void)
{
  __asm("BKPT #0\n") ; // Break into the debugger

  port_disable();
  while (TRUE) {
  }
}

void NMIVector(void)
{
  chDbgPanic("NMI Vector\r\n");
}

/* The fault handler implementation calls a function called
dump_stack(). */
void HardFaultVector(void)
{
    __asm volatile
    (
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word dump_stack    \n"
    );
}

void MemManageVector(void)
{
  app_cfg_set_fault_data(MEM_MANAGE_FAULT, NULL, 0);
  app_cfg_flush();
  chDbgPanic("Mem Manage Vector\r\n");
}

void BusFaultVector(void)
{
  app_cfg_set_fault_data(BUS_FAULT, NULL, 0);
  app_cfg_flush();
  chDbgPanic("Bus Fault Vector\r\n");
}

void UsageFaultVector(void)
{
  app_cfg_set_fault_data(USAGE_FAULT, NULL, 0);
  app_cfg_flush();
  chDbgPanic("Usage Fault Vector\r\n");
}

void
dump_stack(uint32_t *pulFaultStackAddress)
{
  hfd.r0 = pulFaultStackAddress[0];
  hfd.r1 = pulFaultStackAddress[1];
  hfd.r2 = pulFaultStackAddress[2];
  hfd.r3 = pulFaultStackAddress[3];

  hfd.r12 = pulFaultStackAddress[4];
  hfd.lr = pulFaultStackAddress[5];
  hfd.pc = pulFaultStackAddress[6];
  hfd.psr = pulFaultStackAddress[7];

  // Configurable Fault Status Register
  // Consists of MMSR, BFSR and UFSR
  hfd._CFSR = (*((volatile uint32_t *)(0xE000ED28))) ;

  // Hard Fault Status Register
  hfd._HFSR = (*((volatile uint32_t *)(0xE000ED2C))) ;

  // Debug Fault Status Register
  hfd._DFSR = (*((volatile uint32_t *)(0xE000ED30))) ;

  // Auxiliary Fault Status Register
  hfd._AFSR = (*((volatile uint32_t *)(0xE000ED3C))) ;

  // Read the Fault Address Registers. These may not contain valid values.
  // Check BFARVALID/MMARVALID to see if they are valid values
  // MemManage Fault Address Register
  hfd._MMAR = (*((volatile uint32_t *)(0xE000ED34))) ;
  // Bus Fault Address Register
  hfd._BFAR = (*((volatile uint32_t *)(0xE000ED38))) ;
  /* When the following line is hit, the variables contain the register values. */
  hfd._SCB_SHCSR = SCB->SHCSR;

  app_cfg_set_fault_data(HARD_FAULT, &hfd, sizeof(hfd));
  app_cfg_flush();

  __asm("BKPT #0\n") ; // Break into the debugger
}
