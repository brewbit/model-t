#ifndef UTFT_h
#define UTFT_h

#include <stdint.h>
#include "font.h"
#include "image.h"

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

#define COLOR(r, g, b) (uint16_t)(((r & 0x1F)<<11) + ((g & 0x3F)<<5) + (b & 0x1F))

#define BLACK COLOR(0, 0, 0)
#define WHITE COLOR(255, 255, 255)
#define RED   COLOR(255, 0, 0)
#define BLUE  COLOR(0, 0, 255)
#define GREEN COLOR(0, 255, 0)

void
lcd_init(void);
void
clrScr(void);
void
drawPixel(int x, int y);
void
drawLine(int x1, int y1, int x2, int y2);
void
fillScr(uint16_t color);
void
drawRect(int x1, int y1, int x2, int y2);
void
drawRoundRect(int x1, int y1, int x2, int y2);
void
fillRect(int x1, int y1, int x2, int y2);
void
fillRoundRect(int x1, int y1, int x2, int y2);
void
drawCircle(int x, int y, int radius);
void
fillCircle(int x, int y, int radius);
void
setColor(uint16_t color);
void
set_bg_color(uint16_t color);
void
set_bg_img(const Image_t* img);
void
print(char *st, int x, int y);
void
printChar(const Glyph_t* g, int x, int y);
void
printNumI(long num, int x, int y, int length, char filler);
void
printNumF(double num, uint8_t dec, int x, int y, char divider, int length,
    char filler);
void
setFont(const Font_t* font);
void
drawBitmap(int x, int y, const Image_t* img);
void
tile_bitmap(const Image_t* img, int x, int y, int w, int h);

#endif
