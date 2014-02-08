/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

                                      ---

    A special exception to the GPL can be applied should you wish to distribute
    a combined work that includes ChibiOS/RT, without being obliged to provide
    the source code for any proprietary components. See the file exception.txt
    for full details of how and when the exception can be applied.
*/

#include "ch.h"
#include "hal.h"


// per-core calibration values
#define TS_CAL1_CNT (*((uint16_t*)0x1FFF7A2C))
#define TS_CAL1_TMP     (30.0f)

#define TS_CAL2_CNT (*((uint16_t*)0x1FFF7A2E))
#define TS_CAL2_TMP     (110.0f)

#define TS_AVG_SLOPE    ((TS_CAL2_CNT - TS_CAL1_CNT) / (TS_CAL2_TMP - TS_CAL1_TMP))

// default calibration values
//#define TS_CAL1_CNT     ((0.76 / 3.3) * 4096)
//#define TS_CAL1_TMP     (25.0f)
//#define TS_AVG_SLOPE    (2.5f * (4096 / 3300))


static const ADCConversionGroup core_temp_conv_grp = {
    .circular = FALSE,
    .num_channels = 1,
    .end_cb = NULL,
    .error_cb = NULL,

    /* HW dependent part.*/
    .cr1   = 0, // 12-bit resolution
    .cr2   = ADC_CR2_SWSTART, // software triggered
    .smpr1 = ADC_SMPR1_SMP_SENSOR(ADC_SAMPLE_480),
    .smpr2 = 0,
    .sqr1  = ADC_SQR1_NUM_CH(1),
    .sqr2  = 0,
    .sqr3  = ADC_SQR3_SQ1_N(ADC_CHANNEL_SENSOR),
};


/**
 * @brief   PAL setup.
 * @details Digital I/O ports static configuration as defined in @p board.h.
 *          This variable is used by the HAL when initializing the PAL driver.
 */
#if HAL_USE_PAL || defined(__DOXYGEN__)
const PALConfig pal_default_config =
{
  {VAL_GPIOA_MODER, VAL_GPIOA_OTYPER, VAL_GPIOA_OSPEEDR, VAL_GPIOA_PUPDR, VAL_GPIOA_ODR, VAL_GPIOA_AFRL, VAL_GPIOA_AFRH},
  {VAL_GPIOB_MODER, VAL_GPIOB_OTYPER, VAL_GPIOB_OSPEEDR, VAL_GPIOB_PUPDR, VAL_GPIOB_ODR, VAL_GPIOB_AFRL, VAL_GPIOB_AFRH},
  {VAL_GPIOC_MODER, VAL_GPIOC_OTYPER, VAL_GPIOC_OSPEEDR, VAL_GPIOC_PUPDR, VAL_GPIOC_ODR, VAL_GPIOC_AFRL, VAL_GPIOC_AFRH},
  {VAL_GPIOD_MODER, VAL_GPIOD_OTYPER, VAL_GPIOD_OSPEEDR, VAL_GPIOD_PUPDR, VAL_GPIOD_ODR, VAL_GPIOD_AFRL, VAL_GPIOD_AFRH},
  {VAL_GPIOE_MODER, VAL_GPIOE_OTYPER, VAL_GPIOE_OSPEEDR, VAL_GPIOE_PUPDR, VAL_GPIOE_ODR, VAL_GPIOE_AFRL, VAL_GPIOE_AFRH},
  {VAL_GPIOF_MODER, VAL_GPIOF_OTYPER, VAL_GPIOF_OSPEEDR, VAL_GPIOF_PUPDR, VAL_GPIOF_ODR, VAL_GPIOF_AFRL, VAL_GPIOF_AFRH},
  {VAL_GPIOG_MODER, VAL_GPIOG_OTYPER, VAL_GPIOG_OSPEEDR, VAL_GPIOG_PUPDR, VAL_GPIOG_ODR, VAL_GPIOG_AFRL, VAL_GPIOG_AFRH},
  {VAL_GPIOH_MODER, VAL_GPIOH_OTYPER, VAL_GPIOH_OSPEEDR, VAL_GPIOH_PUPDR, VAL_GPIOH_ODR, VAL_GPIOH_AFRL, VAL_GPIOH_AFRH},
  {VAL_GPIOI_MODER, VAL_GPIOI_OTYPER, VAL_GPIOI_OSPEEDR, VAL_GPIOI_PUPDR, VAL_GPIOI_ODR, VAL_GPIOI_AFRL, VAL_GPIOI_AFRH}
};
#endif

/*
 * Early initialization code.
 * This initialization must be performed just after stack setup and before
 * any other initialization.
 */
void __early_init(void) {

  stm32_clock_init();
}

/*
 * Board-specific initialization code.
 */
void boardInit(void) {
  rccEnableAHB3(RCC_AHB3ENR_FSMCEN, FALSE);
  // Bank 1 Control Register
  FSMC_Bank1->BTCR[0] = FSMC_BCR1_MBKEN | FSMC_BCR1_MWID_0 | FSMC_BCR1_WREN | FSMC_BCR1_EXTMOD;
  // Bank 1 Read Timing Register
  FSMC_Bank1->BTCR[1] =
      FSMC_BTR1_CLKDIV_0 |
      FSMC_BTR1_ADDSET_0 |
      FSMC_BTR1_DATAST_0 | FSMC_BTR1_DATAST_2;
  // Bank 1 Write Timing Register
  FSMC_Bank1E->BWTR[0] =
      FSMC_BWTR1_CLKDIV_0 |
      FSMC_BWTR1_ADDSET_0 |
      FSMC_BWTR1_DATAST_1;

  adcStart(&ADCD1, NULL);
  adcSTM32EnableTSVREFE();
}

uint32_t*
board_get_device_id()
{
  return (uint32_t*)0x1FFF7A10;
}

float
board_get_core_temp()
{
  adcsample_t sample;

  adcAcquireBus(&ADCD1);
  adcConvert(&ADCD1, &core_temp_conv_grp, &sample, 1);
  adcReleaseBus(&ADCD1);

  return (float)((((float)sample - TS_CAL1_CNT) / TS_AVG_SLOPE) + TS_CAL1_TMP);
}
