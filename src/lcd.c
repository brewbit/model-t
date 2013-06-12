
#include "ch.h"
#include "hal.h"

#include "lcd.h"
#include "time.h"
#include "common.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>


// Alpha blending support
#define RED_COMPONENT(color) (((color) >> 11) & 0x1F)
#define GREEN_COMPONENT(color) (((color) >> 5) & 0x3F)
#define BLUE_COMPONENT(color) ((color) & 0x1F)

#define BLENDED_COMPONENT(fg, bg, alpha) (((fg) * (alpha) / 255) + ((bg) * (255-(alpha)) / 255))

#define BLENDED_COLOR(fg, bg, alpha) COLOR565( \
        BLENDED_COMPONENT(RED_COMPONENT(fg), RED_COMPONENT(bg), alpha), \
        BLENDED_COMPONENT(GREEN_COMPONENT(fg), GREEN_COMPONENT(bg), alpha), \
        BLENDED_COMPONENT(BLUE_COMPONENT(fg), BLUE_COMPONENT(bg), alpha))

#define wr_low() palClearPad(GPIOA, GPIOA_TFT_WR)
#define wr_high() palSetPad(GPIOA, GPIOA_TFT_WR)

#define rd_low() palClearPad(GPIOA, GPIOA_TFT_RD)
#define rd_high() palSetPad(GPIOA, GPIOA_TFT_RD)

#define rs_low() palClearPad(GPIOA, GPIOA_TFT_RS)
#define rs_high() palSetPad(GPIOA, GPIOA_TFT_RS)

#define cs_low() palClearPad(GPIOA, GPIOA_TFT_CS)
#define cs_high() palSetPad(GPIOA, GPIOA_TFT_CS)

#define rst_low() palClearPad(GPIOA, GPIOA_TFT_RST)
#define rst_high() palSetPad(GPIOA, GPIOA_TFT_RST)

#define swap(type, a, b) { type SWAP_tmp = a; a = b; b = SWAP_tmp; }

typedef enum {
  BG_IMAGE,
  BG_COLOR
} BackgroundType;

static void lcd_write(uint16_t val);
static void lcd_write_cmd(uint8_t val);
static void lcd_write_data(uint16_t VL);
static void lcd_write_param(uint8_t cmd, uint16_t val);
static void set_pixel(uint16_t color);
static void draw_horiz_line(int x, int y, int l);
static void draw_vert_line(int x, int y, int l);
static void set_cursor(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
static void clr_cursor(void);
static uint16_t get_tile_color(const Image_t* img, int x, int y);
static uint16_t get_bg_color(int x, int y);
static void fill_screen(uint16_t color);
static void draw_px(int x, int y);

static uint16_t fcolor = GREEN;
static uint16_t bcolor = BLACK;

static BackgroundType bg_type = BG_COLOR;
static const Image_t* bg_img = NULL;
static point_t bg_anchor;
static const Font_t* cfont;

const rect_t display_rect = {
    .x = 0,
    .y = 0,
    .width = DISP_WIDTH,
    .height = DISP_HEIGHT,
};

static void
lcd_write(uint16_t val)
{
  palWritePort(GPIOC, val);

  // command the LCD to read the latched value
  wr_low();
  __asm__("nop");
  __asm__("nop");
  __asm__("nop");
  wr_high();
}

static void
lcd_write_cmd(uint8_t cmd)
{
  rs_low();
  lcd_write(cmd);
}

static void
lcd_write_data(uint16_t val)
{
  rs_high();
  lcd_write(val);
}

void
lcd_write_param(uint8_t cmd, uint16_t val)
{
  lcd_write_cmd(cmd);
  lcd_write_data(val);
}

void
lcd_init()
{
  /* Initialize control pins to known state. */
  rd_high();
  rs_high();
  wr_high();
  cs_high();

  rst_high();
  chThdSleepMilliseconds(5);
  rst_low();
  chThdSleepMilliseconds(15);
  rst_high();
  chThdSleepMilliseconds(5);

  cs_low();

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

  cs_high();

  clear_screen();
}

static void
set_cursor(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
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

static void
clr_cursor()
{
  set_cursor(0, 0, DISP_WIDTH - 1, DISP_HEIGHT - 1);
}

void
draw_rect(rect_t rect)
{
  draw_horiz_line(rect.x, rect.y, rect.width-1);
  draw_horiz_line(rect.x, rect.y + rect.height-1, rect.width-1);
  draw_vert_line(rect.x, rect.y, rect.height);
  draw_vert_line(rect.x + rect.width-1, rect.y, rect.height);
}

void
fill_rect(rect_t rect)
{
  int i;

  cs_low();
  set_cursor(rect.x, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1);
  for (i = 0; i < (rect.width * rect.height); ++i) {
    set_pixel(fcolor);
  }
  cs_high();
}

void
clear_screen()
{
  if (bg_type == BG_IMAGE) {
    tile_bitmap(bg_img, display_rect);
  }
  else {
    fill_screen(bcolor);
  }
}

static void
fill_screen(uint16_t color)
{
  long i;

  cs_low();
  clr_cursor();
  rs_high();
  for (i = 0; i < (DISP_WIDTH * DISP_HEIGHT); i++) {
    set_pixel(color);
  }
  cs_high();
}

void
set_color(uint16_t color)
{
  fcolor = color;
}

void
set_bg_color(uint16_t color)
{
  bcolor = color;
  bg_type = BG_COLOR;
}

void
set_bg_img(const Image_t* img, point_t anchor)
{
  bg_img = img;
  bg_anchor = anchor;
  bg_type = BG_IMAGE;
}

static void
set_pixel(uint16_t color)
{
  lcd_write_data(color);
}

static void
draw_px(int x, int y)
{
  cs_low();
  set_cursor(x, y, x, y);
  set_pixel(fcolor);
  cs_high();
  clr_cursor();
}

void
draw_line(int x1, int y1, int x2, int y2)
{
  double delta, tx, ty;

  if (((x2 - x1) < 0)) {
    swap(int, x1, x2);
    swap(int, y1, y2);
  }
  if (((y2 - y1) < 0)) {
    swap(int, x1, x2);
    swap(int, y1, y2);
  }

  if (y1 == y2) {
    if (x1 > x2) {
      swap(int, x1, x2);
    }
    draw_horiz_line(x1, y1, x2 - x1);
  }
  else if (x1 == x2) {
    if (y1 > y2) {
      swap(int, y1, y2);
    }
    draw_vert_line(x1, y1, y2 - y1);
  }
  else if (abs(x2 - x1) > abs(y2 - y1)) {
    cs_low();
    delta = ((double) (y2 - y1)) / ((double) (x2 - x1));
    ty = (double) (y1);
    if (x1 > x2) {
      int i;
      for (i = x1; i >= x2; i--) {
        set_cursor(i, (int) (ty + 0.5), i, (int) (ty + 0.5));
        lcd_write_data(fcolor);
        ty = ty - delta;
      }
    }
    else {
      int i;
      for (i = x1; i <= x2; i++) {
        set_cursor(i, (int) (ty + 0.5), i, (int) (ty + 0.5));
        lcd_write_data(fcolor);
        ty = ty + delta;
      }
    }
    cs_high();
  }
  else {
    cs_low();
    delta = ((float) (x2 - x1)) / ((float) (y2 - y1));
    tx = (float) (x1);
    if (y1 > y2) {
      int i;
      for (i = y2 + 1; i > y1; i--) {
        set_cursor((int) (tx + 0.5), i, (int) (tx + 0.5), i);
        lcd_write_data(fcolor);
        tx = tx + delta;
      }
    }
    else {
      int i;
      for (i = y1; i < y2 + 1; i++) {
        set_cursor((int) (tx + 0.5), i, (int) (tx + 0.5), i);
        lcd_write_data(fcolor);
        tx = tx + delta;
      }
    }
    cs_high();
  }

  clr_cursor();
}

static void
draw_horiz_line(int x, int y, int l)
{
  int i;

  cs_low();
  set_cursor(x, y, x + l, y);
  for (i = 0; i < l + 1; i++) {
    lcd_write_data(fcolor);
  }
  cs_high();
  clr_cursor();
}

void
draw_vert_line(int x, int y, int l)
{
  int i;

  cs_low();
  set_cursor(x, y, x, y + l);
  for (i = 0; i < l; i++) {
    lcd_write_data(fcolor);
  }
  cs_high();
  clr_cursor();
}

void
print_char(const Glyph_t* g, int x, int y)
{
  uint16_t j;

  cs_low();

  set_cursor(x, y, x + g->width - 1, y + g->height - 1);

  for (j = 0; j < (g->width * g->height); j++) {
    uint8_t alpha = g->data[j];
    if (alpha == 255) {
      set_pixel(fcolor);
    }
    else {
      uint16_t by = y + (j / g->width);
      uint16_t bx = x + (j % g->width);
      uint16_t bcolor = get_bg_color(bx, by);

      if (alpha == 0) {
        set_pixel(bcolor);
      }
      else {
        set_pixel(BLENDED_COLOR(fcolor, bcolor, alpha));
      }
    }
  }

  cs_high();
  clr_cursor();
}

void
print_str(const char *st, int x, int y)
{
  int xoff = 0;

  while (*st) {
    const Glyph_t* g = font_find_glyph(cfont, *st++);
    xoff += g->xoffset;
    print_char(g, x + xoff, y + g->yoffset);
    xoff += g->advance;
  }
}

void
set_font(const Font_t* font)
{
  cfont = font;
}

void
draw_bitmap(int x, int y, const Image_t* img)
{
  unsigned int col;

  int tc;
  cs_low();
  set_cursor(x, y, x + img->width - 1, y + img->height - 1);
  switch (img->format) {
  case RGB565:
    for (tc = 0; tc < (img->width * img->height); tc++) {
      col = img->px[tc];
      lcd_write_data(col);
    }
    break;

  case RGBA5658:
    for (tc = 0; tc < (img->width * img->height); tc++) {
      uint8_t alpha = img->alpha[tc];
      uint16_t fcolor = img->px[tc];

      if (alpha == 255) {
        set_pixel(fcolor);
      }
      else {
        uint16_t by = y + (tc / img->width);
        uint16_t bx = x + (tc % img->width);

        uint16_t bcolor = get_bg_color(bx, by);

        if (alpha == 0) {
          set_pixel(bcolor);
        }
        else {
          col = BLENDED_COLOR(fcolor, bcolor, alpha);
          set_pixel(col);
        }
      }
    }
    break;
  }
  cs_high();

  clr_cursor();
}

static uint16_t
get_tile_color(const Image_t* img, int x, int y)
{
  uint16_t imx = x % img->width;
  uint16_t imy = y % img->height;
  uint16_t col = img->px[imx + (imy * img->width)];
  return col;
}

static uint16_t
get_bg_color(int x, int y)
{
  if (bg_type == BG_IMAGE) {
    return get_tile_color(bg_img, x - bg_anchor.x, y - bg_anchor.y);
  }
  else {
    return bcolor;
  }
}

void
tile_bitmap(const Image_t* img, rect_t rect)
{
  int i, j;

  cs_low();
  set_cursor(rect.x, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1);
  for (i = 0; i < rect.height; ++i) {
    for (j = 0; j < rect.width; ++j) {
      set_pixel(get_tile_color(img, j, i));
    }
  }
  cs_high();
  clr_cursor();
}

