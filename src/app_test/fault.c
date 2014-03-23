
#include "ch.h"
#include "hal.h"

#include <stdint.h>


void
dump_stack(uint32_t *pulFaultStackAddress);


volatile uint32_t r0;
volatile uint32_t r1;
volatile uint32_t r2;
volatile uint32_t r3;
volatile uint32_t r12;
volatile uint32_t lr; /* Link register. */
volatile uint32_t pc; /* Program counter. */
volatile uint32_t psr;/* Program status register. */
volatile unsigned long _CFSR ;
volatile unsigned long _HFSR ;
volatile unsigned long _DFSR ;
volatile unsigned long _AFSR ;
volatile unsigned long _BFAR ;
volatile unsigned long _MMAR ;
volatile unsigned long _SCB_SHCSR;
volatile size_t core;
volatile uint32_t fake;


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
  chDbgPanic("Mem Manage Vector\r\n");
}

void BusFaultVector(void)
{
  chDbgPanic("Bus Fault Vector\r\n");
}

void UsageFaultVector(void)
{
  chDbgPanic("Usage Fault Vector\r\n");
}

void
dump_stack(uint32_t *pulFaultStackAddress)
{
  r0 = pulFaultStackAddress[0];
  r1 = pulFaultStackAddress[1];
  r2 = pulFaultStackAddress[2];
  r3 = pulFaultStackAddress[3];

  r12 = pulFaultStackAddress[4];
  lr = pulFaultStackAddress[5];
  pc = pulFaultStackAddress[6];
  psr = pulFaultStackAddress[7];

  // Configurable Fault Status Register
  // Consists of MMSR, BFSR and UFSR
  _CFSR = (*((volatile unsigned long *)(0xE000ED28))) ;

  // Hard Fault Status Register
  _HFSR = (*((volatile unsigned long *)(0xE000ED2C))) ;

  // Debug Fault Status Register
  _DFSR = (*((volatile unsigned long *)(0xE000ED30))) ;

  // Auxiliary Fault Status Register
  _AFSR = (*((volatile unsigned long *)(0xE000ED3C))) ;

  // Read the Fault Address Registers. These may not contain valid values.
  // Check BFARVALID/MMARVALID to see if they are valid values
  // MemManage Fault Address Register
  _MMAR = (*((volatile unsigned long *)(0xE000ED34))) ;
  // Bus Fault Address Register
  _BFAR = (*((volatile unsigned long *)(0xE000ED38))) ;
  /* When the following line is hit, the variables contain the register values. */
  _SCB_SHCSR = SCB->SHCSR;

  core = chCoreStatus();

  __asm("BKPT #0\n") ; // Break into the debugger

  while(1){
    fake += r0;
    fake += r1;
    fake += r2;
    fake += r3;
    fake += r12;
    fake += lr;
    fake += pc;
    fake += psr;
    fake += _CFSR ;
    fake += _HFSR ;
    fake += _DFSR ;
    fake += _AFSR ;
    fake += _BFAR ;
    fake += _MMAR ;
    fake += _SCB_SHCSR;
    fake += core;
  }
}
