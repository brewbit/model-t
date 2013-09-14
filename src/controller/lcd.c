
#include "ch.h"
#include "hal.h"

#include "lcd.h"
#include "common.h"


#define LCD_REG              (*((volatile uint16_t *) 0x60000000)) /* RS = 0 */
#define LCD_RAM              (*((volatile uint16_t *) 0x60020000)) /* RS = 1 */

#define rst_low() palClearPad(PORT_TFT_RST, PAD_TFT_RST)
#define rst_high() palSetPad(PORT_TFT_RST, PAD_TFT_RST)

#define swap(type, a, b) { type SWAP_tmp = a; a = b; b = SWAP_tmp; }


const rect_t display_rect = {
    .x = 0,
    .y = 0,
    .width = DISP_WIDTH,
    .height = DISP_HEIGHT
};

void
lcd_init()
{
  rst_high();
  chThdSleepMilliseconds(5);
  rst_low();
  chThdSleepMilliseconds(15);
  rst_high();
  chThdSleepMilliseconds(5);

  LCD_REG = 0;
  chprintf(SD_STDIO, "LCD R00h = %x\r\n", LCD_RAM);

  lcd_write_param(0xe5, 0x8000);
  lcd_write_param(0x00, 0x0001);
  lcd_write_param(0x01, 0x0100);
  lcd_write_param(0x02, 0x0700);
#if (DISP_ORIENT == LANDSCAPE)
  lcd_write_param(0x03, 0x1018);
#else
  lcd_write_param(0x03, 0x1030);
#endif
  lcd_write_param(0x04, 0x0000);
  lcd_write_param(0x08, 0x0202);
  lcd_write_param(0x09, 0x0000);
  lcd_write_param(0x0A, 0x0000);
  lcd_write_param(0x0C, 0x0000);
  lcd_write_param(0x0D, 0x0000);
  lcd_write_param(0x0F, 0x0000);
  //-----Power On sequence-----------------------
  lcd_write_param(0x10, 0x0000);
  lcd_write_param(0x11, 0x0007);
  lcd_write_param(0x12, 0x0000);
  lcd_write_param(0x13, 0x0000);
  chThdSleepMilliseconds(50);
  lcd_write_param(0x10, 0x17B0);
  lcd_write_param(0x11, 0x0007);
  chThdSleepMilliseconds(10);
  lcd_write_param(0x12, 0x013A);
  chThdSleepMilliseconds(10);
  lcd_write_param(0x13, 0x1A00);
  lcd_write_param(0x29, 0x000c);
  chThdSleepMilliseconds(10);
  //-----Gamma control-----------------------
  lcd_write_param(0x30, 0x0000);
  lcd_write_param(0x31, 0x0505);
  lcd_write_param(0x32, 0x0004);
  lcd_write_param(0x35, 0x0006);
  lcd_write_param(0x36, 0x0707);
  lcd_write_param(0x37, 0x0105);
  lcd_write_param(0x38, 0x0002);
  lcd_write_param(0x39, 0x0707);
  lcd_write_param(0x3C, 0x0704);
  lcd_write_param(0x3D, 0x0807);
  //-----Set RAM area-----------------------
  lcd_write_param(0x50, 0x0000);
  lcd_write_param(0x51, 0x00EF);
  lcd_write_param(0x52, 0x0000);
  lcd_write_param(0x53, 0x013F);
  lcd_write_param(0x60, 0x2700);
  lcd_write_param(0x61, 0x0001);
  lcd_write_param(0x6A, 0x0000);
  lcd_write_param(0x21, 0x0000);
  lcd_write_param(0x20, 0x0000);
  //-----Partial Display Control------------
  lcd_write_param(0x80, 0x0000);
  lcd_write_param(0x81, 0x0000);
  lcd_write_param(0x82, 0x0000);
  lcd_write_param(0x83, 0x0000);
  lcd_write_param(0x84, 0x0000);
  lcd_write_param(0x85, 0x0000);
  //-----Panel Control----------------------
  lcd_write_param(0x90, 0x0010);
  lcd_write_param(0x92, 0x0000);
  lcd_write_param(0x93, 0x0003);
  lcd_write_param(0x95, 0x0110);
  lcd_write_param(0x97, 0x0000);
  lcd_write_param(0x98, 0x0000);
  //-----Display on-----------------------
  lcd_write_param(0x07, 0x0173);
  chThdSleepMilliseconds(50);
}

void
lcd_write_cmd(uint8_t cmd)
{
  LCD_REG = cmd;
}

void
lcd_write_data(uint16_t val)
{
  LCD_RAM = val;
}

void
lcd_write_param(uint8_t cmd, uint16_t val)
{
  lcd_write_cmd(cmd);
  lcd_write_data(val);
}

void
lcd_set_cursor(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
#if (DISP_ORIENT == LANDSCAPE)
  swap(uint16_t, x1, y1);
  swap(uint16_t, x2, y2)
  y1 = DISP_WIDTH - y1 - 1;
  y2 = DISP_WIDTH - y2 - 1;
  swap(uint16_t, y1, y2)
#endif

  // set cursor
  lcd_write_param(0x20, x1);
#if (DISP_ORIENT == LANDSCAPE)
  lcd_write_param(0x21, y2);
#else
  lcd_write_param(0x21, y1);
#endif

  // set window
  lcd_write_param(0x50, x1);
  lcd_write_param(0x51, x2);
  lcd_write_param(0x52, y1);
  lcd_write_param(0x53, y2);
  lcd_write_cmd(0x22);
}

void
lcd_clr_cursor()
{
  lcd_set_cursor(0, 0, DISP_WIDTH - 1, DISP_HEIGHT - 1);
}
