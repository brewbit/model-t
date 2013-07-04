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

#ifndef _BOARD_H_
#define _BOARD_H_

/*
 * Setup for BrewBit Temperature Controller board.
 */

/*
 * Board identifier.
 */
#define BOARD_BREWBIT_MODEL_T
#define BOARD_NAME              "BrewBit Model-T"

/*
 * Board frequencies.
 * NOTE: The HSE crystal is not fitted by default on the board.
 */
#define STM32_LSECLK            0
#define STM32_HSECLK            8000000

/*
 * MCU type as defined in the ST header file stm32f2xx.h.
 */
#define STM32F2XX

/*
 * IO pins assignments.
 */

#define GPIOA_TFT_BKLT  3
#define GPIOA_TFT_RST   8
#define GPIOA_TFT_RD    9
#define GPIOA_TFT_WR    10
#define GPIOA_TFT_RS    11
#define GPIOA_TFT_CS    12

#define GPIOB_LED1      0
#define GPIOB_LED2      1
#define GPIOB_SFLASH_CS 5
#define GPIOB_RELAY1    8
#define GPIOB_RELAY2    9

/*
 * I/O ports initial setup, this configuration is established soon after reset
 * in the initialization code.
 * Please refer to the STM32 Reference Manual for details.
 */
#define PIN_MODE_INPUT(n)           (0 << ((n) * 2))
#define PIN_MODE_OUTPUT(n)          (1 << ((n) * 2))
#define PIN_MODE_ALTERNATE(n)       (2 << ((n) * 2))
#define PIN_MODE_ANALOG(n)          (3 << ((n) * 2))
#define PIN_OTYPE_PUSHPULL(n)       (0 << (n))
#define PIN_OTYPE_OPENDRAIN(n)      (1 << (n))
#define PIN_OSPEED_2M(n)            (0 << ((n) * 2))
#define PIN_OSPEED_25M(n)           (1 << ((n) * 2))
#define PIN_OSPEED_50M(n)           (2 << ((n) * 2))
#define PIN_OSPEED_100M(n)          (3 << ((n) * 2))
#define PIN_PUDR_FLOATING(n)        (0 << ((n) * 2))
#define PIN_PUDR_PULLUP(n)          (1 << ((n) * 2))
#define PIN_PUDR_PULLDOWN(n)        (2 << ((n) * 2))
#define PIN_AFIO_AF(n, v)           ((v##U) << ((n % 8) * 4))

/*
 * Port A setup.
 * All input with pull-up except:
 * PA0  - WiFi Serial - UART4 TX      (alternate 8)
 * PA1  - WiFi Serial - UART4 RX      (alternate 8)
 * PA2  - Onewire-2   - USART2 TX     (alternate 7)
 * PA3  - Backlight Control           (PP Output)
 * PA4  - X+                          (PP Output)
 * PA5  - Y+                          (PP Output)
 * PA6  - X-                          (PP Output)
 * PA7  - Y-                          (PP Output)
 * PA8  - TFT Reset                   (PP Output)
 * PA9  - TFT RD                      (PP Output)
 * PA10 - TFT WR                      (PP Output)
 * PA11 - TFT RS                      (PP Output)
 * PA12 - TFT CS                      (PP Output)
 * PA13 - JTMS                        (alternate 0)
 * PA14 - JTCK                        (alternate 0)
 * PA15 - JTDI                        (alternate 0)
 */
#define VAL_GPIOA_MODER             (PIN_MODE_ALTERNATE(0)   | \
                                     PIN_MODE_ALTERNATE(1)   | \
                                     PIN_MODE_ALTERNATE(2)   | \
                                     PIN_MODE_OUTPUT(3)      | \
                                     PIN_MODE_OUTPUT(4)      | \
                                     PIN_MODE_OUTPUT(5)      | \
                                     PIN_MODE_OUTPUT(6)      | \
                                     PIN_MODE_OUTPUT(7)      | \
                                     PIN_MODE_OUTPUT(8)      | \
                                     PIN_MODE_OUTPUT(9)      | \
                                     PIN_MODE_OUTPUT(10)     | \
                                     PIN_MODE_OUTPUT(11)     | \
                                     PIN_MODE_OUTPUT(12)     | \
                                     PIN_MODE_ALTERNATE(13)  | \
                                     PIN_MODE_ALTERNATE(14)  | \
                                     PIN_MODE_ALTERNATE(15))
#define VAL_GPIOA_OTYPER            (PIN_OTYPE_PUSHPULL(0)  | \
                                     PIN_OTYPE_PUSHPULL(1)  | \
                                     PIN_OTYPE_OPENDRAIN(2) | \
                                     PIN_OTYPE_PUSHPULL(3)  | \
                                     PIN_OTYPE_PUSHPULL(4)  | \
                                     PIN_OTYPE_PUSHPULL(5)  | \
                                     PIN_OTYPE_PUSHPULL(6)  | \
                                     PIN_OTYPE_PUSHPULL(7)  | \
                                     PIN_OTYPE_PUSHPULL(8)  | \
                                     PIN_OTYPE_PUSHPULL(9)  | \
                                     PIN_OTYPE_PUSHPULL(10) | \
                                     PIN_OTYPE_PUSHPULL(11) | \
                                     PIN_OTYPE_PUSHPULL(12) | \
                                     PIN_OTYPE_PUSHPULL(13) | \
                                     PIN_OTYPE_PUSHPULL(14) | \
                                     PIN_OTYPE_PUSHPULL(15))
#define VAL_GPIOA_OSPEEDR           0xFFFFFFFF // all 100 MHz
#define VAL_GPIOA_PUPDR             0x00000000 // all floating
#define VAL_GPIOA_ODR               0xFFFFFFFF // all high by default
#define VAL_GPIOA_AFRL              (PIN_AFIO_AF(0, 8)       | \
                                     PIN_AFIO_AF(1, 8)       | \
                                     PIN_AFIO_AF(2, 7))
#define VAL_GPIOA_AFRH              0x00000000

/*
 * Port B setup.
 * All input with pull-up except:
 * PB0  - Green LED                     (PP Output)
 * PB1  - Red LED                       (PP Output)
 * PB2  - BOOT1                         (PU input)
 * PB3  - JTDO                          (alternate 0)
 * PB4  - nJTRST                        (alternate 0)
 * PB5  - SPI Flash Chip Select         (PP Output)
 * PB6  - Onewire-1 - USART1 TX         (alternate 7)
 * PB7  - Spare
 * PB8  - Relay 1                       (PP Output)
 * PB9  - Relay 2                       (PP Output)
 * PB10 - Sensor Board TX - USART3 TX   (alternate 7)
 * PB11 - Sensor Board RX - USART3 RX   (alternate 7)
 * PB12 - Spare
 * PB13 - SPI SCK  - SPI2               (alternate 5)
 * PB14 - SPI MISO - SPI2               (alternate 5)
 * PB15 - SPI MOSI - SPI2               (alternate 5)
 */
#define VAL_GPIOB_MODER             (PIN_MODE_OUTPUT(0)       | \
                                     PIN_MODE_OUTPUT(1)       | \
                                     PIN_MODE_ALTERNATE(3)    | \
                                     PIN_MODE_ALTERNATE(4)    | \
                                     PIN_MODE_OUTPUT(5)       | \
                                     PIN_MODE_ALTERNATE(6)    | \
                                     PIN_MODE_OUTPUT(8)       | \
                                     PIN_MODE_OUTPUT(9)       | \
                                     PIN_MODE_ALTERNATE(10)   | \
                                     PIN_MODE_ALTERNATE(11)   | \
                                     PIN_MODE_ALTERNATE(13)   | \
                                     PIN_MODE_ALTERNATE(14)   | \
                                     PIN_MODE_ALTERNATE(15))
#define VAL_GPIOB_OTYPER            (PIN_OTYPE_PUSHPULL(0)  | \
                                     PIN_OTYPE_PUSHPULL(1)  | \
                                     PIN_OTYPE_PUSHPULL(2)  | \
                                     PIN_OTYPE_PUSHPULL(3)  | \
                                     PIN_OTYPE_PUSHPULL(4)  | \
                                     PIN_OTYPE_PUSHPULL(5)  | \
                                     PIN_OTYPE_OPENDRAIN(6) | \
                                     PIN_OTYPE_PUSHPULL(7)  | \
                                     PIN_OTYPE_PUSHPULL(8)  | \
                                     PIN_OTYPE_PUSHPULL(9)  | \
                                     PIN_OTYPE_PUSHPULL(10) | \
                                     PIN_OTYPE_PUSHPULL(11) | \
                                     PIN_OTYPE_PUSHPULL(12) | \
                                     PIN_OTYPE_PUSHPULL(13) | \
                                     PIN_OTYPE_PUSHPULL(14) | \
                                     PIN_OTYPE_PUSHPULL(15))
#define VAL_GPIOB_OSPEEDR           0xFFFFFFFF // all 100 MHz
#define VAL_GPIOB_PUPDR             0xFFFFFFFF // all pulled-up
#define VAL_GPIOB_ODR               0x00000000 // all low by default
#define VAL_GPIOB_AFRL              (PIN_AFIO_AF(6, 7))
#define VAL_GPIOB_AFRH              (PIN_AFIO_AF(10, 7)       | \
                                     PIN_AFIO_AF(11, 7)       | \
                                     PIN_AFIO_AF(13, 5)       | \
                                     PIN_AFIO_AF(14, 5)       | \
                                     PIN_AFIO_AF(15, 5))

/*
 * Port C setup.
 * All Push/Pull outputs
 */
#define VAL_GPIOC_MODER             0x55555555 // all output
#define VAL_GPIOC_OTYPER            0x00000000 // all push/pull
#define VAL_GPIOC_OSPEEDR           0xFFFFFFFF // all 100 MHz
#define VAL_GPIOC_PUPDR             0x00000000 // all floating
#define VAL_GPIOC_ODR               0xFFFFFFFF // all high by default
#define VAL_GPIOC_AFRL              0x00000000 // all alt func 0
#define VAL_GPIOC_AFRH              0x00000000 // all alt func 0

/*
 * Port D setup.
 * All input with pull-up.
 */
#define VAL_GPIOD_MODER             0x00000000
#define VAL_GPIOD_OTYPER            0x00000000
#define VAL_GPIOD_OSPEEDR           0xFFFFFFFF
#define VAL_GPIOD_PUPDR             0xFFFFFFFF
#define VAL_GPIOD_ODR               0xFFFFFFFF
#define VAL_GPIOD_AFRL              0x00000000
#define VAL_GPIOD_AFRH              0x00000000

/*
 * Port E setup.
 * All input with pull-up.
 */
#define VAL_GPIOE_MODER             0x00000000
#define VAL_GPIOE_OTYPER            0x00000000
#define VAL_GPIOE_OSPEEDR           0xFFFFFFFF
#define VAL_GPIOE_PUPDR             0xFFFFFFFF
#define VAL_GPIOE_ODR               0xFFFFFFFF
#define VAL_GPIOE_AFRL              0x00000000
#define VAL_GPIOE_AFRH              0x00000000

/*
 * Port F setup.
 * All input with pull-up.
 */
#define VAL_GPIOF_MODER             0x00000000
#define VAL_GPIOF_OTYPER            0x00000000
#define VAL_GPIOF_OSPEEDR           0xFFFFFFFF
#define VAL_GPIOF_PUPDR             0xFFFFFFFF
#define VAL_GPIOF_ODR               0xFFFFFFFF
#define VAL_GPIOF_AFRL              0x00000000
#define VAL_GPIOF_AFRH              0x00000000

/*
 * Port G setup.
 * All input with pull-up.
 */
#define VAL_GPIOG_MODER             0x00000000
#define VAL_GPIOG_OTYPER            0x00000000
#define VAL_GPIOG_OSPEEDR           0xFFFFFFFF
#define VAL_GPIOG_PUPDR             0xFFFFFFFF
#define VAL_GPIOG_ODR               0xFFFFFFFF
#define VAL_GPIOG_AFRL              0x00000000
#define VAL_GPIOG_AFRH              0x00000000

/*
 * Port H setup.
 * All input with pull-up.
 */
#define VAL_GPIOH_MODER             0x00000000
#define VAL_GPIOH_OTYPER            0x00000000
#define VAL_GPIOH_OSPEEDR           0xFFFFFFFF
#define VAL_GPIOH_PUPDR             0xFFFFFFFF
#define VAL_GPIOH_ODR               0xFFFFFFFF
#define VAL_GPIOH_AFRL              0x00000000
#define VAL_GPIOH_AFRH              0x00000000

/*
 * Port I setup.
 * All input with pull-up.
 */
#define VAL_GPIOI_MODER             0x00000000
#define VAL_GPIOI_OTYPER            0x00000000
#define VAL_GPIOI_OSPEEDR           0xFFFFFFFF
#define VAL_GPIOI_PUPDR             0xFFFFFFFF
#define VAL_GPIOI_ODR               0xFFFFFFFF
#define VAL_GPIOI_AFRL              0x00000000
#define VAL_GPIOI_AFRH              0x00000000

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* _BOARD_H_ */
