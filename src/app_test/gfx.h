#ifndef GFX_H
#define GFX_H

#include <stdint.h>
#include "font.h"
#include "lcd.h"


/* The parameters for this macro are in the range 0-31, 0-63, and 0-31 respectively */
#define COLOR16(r, g, b) (uint16_t)(((r & 0x1F)<<11) + ((g & 0x3F)<<5) + (b & 0x1F))

/* The parameters for this macro are in the range 0-255 */
#define COLOR24(r, g, b) COLOR16((((r) * 31) / 255), \
                                 (((g) * 63) / 255), \
                                 (((b) * 31) / 255))


#define BLACK      COLOR24(16, 16, 16)
#define DARK_GRAY  COLOR24(64, 64, 64)
#define LIGHT_GRAY COLOR24(150, 150, 150)
#define WHITE      COLOR24(255, 255, 255)
#define LIME       COLOR24(164, 196, 0)
#define GREEN      COLOR24(96, 169, 23)
#define EMERALD    COLOR24(0, 138, 0)
#define TEAL       COLOR24(0, 171, 169)
#define CYAN       COLOR24(27, 161, 226)
#define COBALT     COLOR24(0, 80, 239)
#define INDIGO     COLOR24(106, 0, 255)
#define VIOLET     COLOR24(170, 0, 255)
#define PINK       COLOR24(244, 114, 208)
#define MAGENTA    COLOR24(216, 0, 115)
#define CRIMSON    COLOR24(162, 0, 37)
#define RED        COLOR24(229, 20, 0)
#define ORANGE     COLOR24(250, 104, 0)
#define AMBER      COLOR24(240, 163, 10)
#define YELLOW     COLOR24(227, 200, 0)
#define BROWN      COLOR24(130, 90, 44)
#define OLIVE      COLOR24(109, 135, 100)
#define STEEL      COLOR24(100, 118, 135)
#define MAUVE      COLOR24(118, 96, 138)
#define TAUPE      COLOR24(135, 121, 78)
#define PURPLE     COLOR24(167, 0, 174)


void
gfx_init(void);

void
gfx_ctx_push(void);

void
gfx_ctx_pop(void);

void
gfx_push_translation(uint16_t x, uint16_t y);

void
gfx_clear_screen(void);

void
gfx_clear_rect(rect_t rect);

void
gfx_draw_line(int x1, int y1, int x2, int y2);

void
gfx_draw_rect(rect_t rect);

void
gfx_fill_rect(rect_t rect);

void
gfx_set_fg_color(uint16_t color);

void
gfx_set_bg_color(uint16_t color);

void
gfx_draw_str(const char *st, int n, int x, int y);

void
gfx_draw_glyph(const glyph_t* g, int x, int y);

void
gfx_set_font(const font_t* font);

#endif
