#ifndef LCD_H
#define LCD_H

#include <stdint.h>
#include "font.h"

#define PORTRAIT 0
#define LANDSCAPE 1

#define DISP_ORIENT LANDSCAPE

#if (DISP_ORIENT == PORTRAIT)
#define DISP_WIDTH 240
#define DISP_HEIGHT 320
#else
#define DISP_WIDTH 320
#define DISP_HEIGHT 240
#endif


extern const rect_t display_rect;


void lcd_init(void);
void lcd_write(uint16_t val);
void lcd_write_cmd(uint8_t val);
void lcd_write_data(uint16_t VL);
void lcd_write_param(uint8_t cmd, uint16_t val);
void lcd_set_cursor(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void lcd_clr_cursor(void);

#endif
