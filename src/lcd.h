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

/* The parameters for this macro are in the range 0-255 */
#define COLOR(r, g, b) (uint16_t)(((((r) * 31) / 255)<<11) + ((((g) * 63) / 255)<<5) + (((b) * 31) / 255))

/* The parameters for this macro are in the range 0-31, 0-63, and 0-31 respectively */
#define COLOR565(r, g, b) (uint16_t)(((r & 0x1F)<<11) + ((g & 0x3F)<<5) + (b & 0x1F))


#define BLACK     COLOR(0, 0, 0)
#define DARK_GRAY COLOR(16, 16, 16)
#define WHITE     COLOR(255, 255, 255)
#define LIME     COLOR(164, 196, 0)
#define GREEN    COLOR(96, 169, 23)
#define EMERALD  COLOR(0, 138, 0)
#define TEAL     COLOR(0, 171, 169)
#define CYAN     COLOR(27, 161, 226)
#define COBALT   COLOR(0, 80, 239)
#define INDIGO   COLOR(106, 0, 255)
#define VIOLET   COLOR(170, 0, 255)
#define PINK     COLOR(244, 114, 208)
#define MAGENTA  COLOR(216, 0, 115)
#define CRIMSON  COLOR(162, 0, 37)
#define RED      COLOR(229, 20, 0)
#define ORANGE   COLOR(250, 104, 0)
#define AMBER    COLOR(240, 163, 10)
#define YELLOW   COLOR(227, 200, 0)
#define BROWN    COLOR(130, 90, 44)
#define OLIVE    COLOR(109, 135, 100)
#define STEEL    COLOR(100, 118, 135)
#define MAUVE    COLOR(118, 96, 138)
#define TAUPE    COLOR(135, 121, 78)
#define PURPLE   COLOR(0xA7, 0x00, 0xAE)


extern rect_t display_rect;

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
drawRect(rect_t rect);
void
drawRoundRect(int x1, int y1, int x2, int y2);
void
fillRect(rect_t rect);
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
set_bg_img(const Image_t* img, point_t anchor);
void
print(const char *st, int x, int y);
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
tile_bitmap(const Image_t* img, rect_t rect);

#endif
