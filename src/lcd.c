
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

#define BLENDED_COLOR(fg, bg, alpha) COLOR( \
        BLENDED_COMPONENT(RED_COMPONENT(fg), RED_COMPONENT(bg), alpha), \
        BLENDED_COMPONENT(GREEN_COMPONENT(fg), GREEN_COMPONENT(bg), alpha), \
        BLENDED_COMPONENT(BLUE_COMPONENT(fg), BLUE_COMPONENT(bg), alpha))

#define WR_BANK GPIOA
#define WR_BIT 10
#define wr_low() gp_low(WR_BANK, WR_BIT)
#define wr_high() gp_high(WR_BANK, WR_BIT)

#define RD_BANK GPIOA
#define RD_BIT 9
#define rd_low() gp_low(RD_BANK, RD_BIT)
#define rd_high() gp_high(RD_BANK, RD_BIT)

#define RS_BANK GPIOA
#define RS_BIT 11
#define rs_low() gp_low(RS_BANK, RS_BIT)
#define rs_high() gp_high(RS_BANK, RS_BIT)

#define CS_BANK GPIOA
#define CS_BIT 12
#define cs_low() gp_low(CS_BANK, CS_BIT)
#define cs_high() gp_high(CS_BANK, CS_BIT)

#define RST_BANK GPIOA
#define RST_BIT 8
#define rst_low() gp_low(RST_BANK, RST_BIT)
#define rst_high() gp_high(RST_BANK, RST_BIT)

#define swap(type, a, b) { type SWAP_tmp = a; a = b; b = SWAP_tmp; }

typedef enum {
  BG_IMAGE,
  BG_COLOR
} BackgroundType;

void
LCD_Write_Bus(uint16_t V);
void
LCD_Write_COM(char VL);
void
LCD_Write_DATA(uint16_t VL);
void
LCD_Write_COM_DATA(char com1, uint16_t dat1);
void
setPixel(uint16_t color);
void
drawHLine(int x, int y, int l);
void
drawVLine(int x, int y, int l);
void
setXY(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void
clrXY(void);
static uint16_t
get_tile_color(const Image_t* img, int x, int y);
static uint16_t
get_bg_color(int x, int y);

uint16_t fcolor = GREEN;
uint16_t bcolor = BLACK;

BackgroundType bg_type = BG_COLOR;
const Image_t* bg_img = NULL;
point_t bg_anchor;
const Font_t* cfont;

rect_t display_rect = {
    .x = 0,
    .y = 0,
    .width = DISP_WIDTH,
    .height = DISP_HEIGHT,
};

void
LCD_Write_Bus(uint16_t val)
{
  palWritePort(GPIOC, val);

  // command the LCD to read the latched value
  wr_low();
  __asm__("nop");
  __asm__("nop");
  __asm__("nop");
  wr_high();
}

void
LCD_Write_COM(char VL)
{
  rs_low();
  LCD_Write_Bus(VL);
}

void
LCD_Write_DATA(uint16_t VL)
{
  rs_high();
  LCD_Write_Bus(VL);
}

void
LCD_Write_COM_DATA(char com1, uint16_t dat1)
{
  LCD_Write_COM(com1);
  LCD_Write_DATA(dat1);
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

  LCD_Write_COM_DATA(0xe5, 0x8000);
  LCD_Write_COM_DATA(0x00, 0x0001);
  LCD_Write_COM_DATA(0x01, 0x0100);
  LCD_Write_COM_DATA(0x02, 0x0700);
#if (DISP_ORIENT == LANDSCAPE)
  LCD_Write_COM_DATA(0x03, 0x1018);
#else
  LCD_Write_COM_DATA(0x03, 0x1030);
#endif
  LCD_Write_COM_DATA(0x04, 0x0000);
  LCD_Write_COM_DATA(0x08, 0x0202);
  LCD_Write_COM_DATA(0x09, 0x0000);
  LCD_Write_COM_DATA(0x0A, 0x0000);
  LCD_Write_COM_DATA(0x0C, 0x0000);
  LCD_Write_COM_DATA(0x0D, 0x0000);
  LCD_Write_COM_DATA(0x0F, 0x0000);
  //-----Power On sequence-----------------------
  LCD_Write_COM_DATA(0x10, 0x0000);
  LCD_Write_COM_DATA(0x11, 0x0007);
  LCD_Write_COM_DATA(0x12, 0x0000);
  LCD_Write_COM_DATA(0x13, 0x0000);
  chThdSleepMilliseconds(50);
  LCD_Write_COM_DATA(0x10, 0x17B0);
  LCD_Write_COM_DATA(0x11, 0x0007);
  chThdSleepMilliseconds(10);
  LCD_Write_COM_DATA(0x12, 0x013A);
  chThdSleepMilliseconds(10);
  LCD_Write_COM_DATA(0x13, 0x1A00);
  LCD_Write_COM_DATA(0x29, 0x000c);
  chThdSleepMilliseconds(10);
  //-----Gamma control-----------------------
  LCD_Write_COM_DATA(0x30, 0x0000);
  LCD_Write_COM_DATA(0x31, 0x0505);
  LCD_Write_COM_DATA(0x32, 0x0004);
  LCD_Write_COM_DATA(0x35, 0x0006);
  LCD_Write_COM_DATA(0x36, 0x0707);
  LCD_Write_COM_DATA(0x37, 0x0105);
  LCD_Write_COM_DATA(0x38, 0x0002);
  LCD_Write_COM_DATA(0x39, 0x0707);
  LCD_Write_COM_DATA(0x3C, 0x0704);
  LCD_Write_COM_DATA(0x3D, 0x0807);
  //-----Set RAM area-----------------------
  LCD_Write_COM_DATA(0x50, 0x0000);
  LCD_Write_COM_DATA(0x51, 0x00EF);
  LCD_Write_COM_DATA(0x52, 0x0000);
  LCD_Write_COM_DATA(0x53, 0x013F);
  LCD_Write_COM_DATA(0x60, 0x2700);
  LCD_Write_COM_DATA(0x61, 0x0001);
  LCD_Write_COM_DATA(0x6A, 0x0000);
  LCD_Write_COM_DATA(0x21, 0x0000);
  LCD_Write_COM_DATA(0x20, 0x0000);
  //-----Partial Display Control------------
  LCD_Write_COM_DATA(0x80, 0x0000);
  LCD_Write_COM_DATA(0x81, 0x0000);
  LCD_Write_COM_DATA(0x82, 0x0000);
  LCD_Write_COM_DATA(0x83, 0x0000);
  LCD_Write_COM_DATA(0x84, 0x0000);
  LCD_Write_COM_DATA(0x85, 0x0000);
  //-----Panel Control----------------------
  LCD_Write_COM_DATA(0x90, 0x0010);
  LCD_Write_COM_DATA(0x92, 0x0000);
  LCD_Write_COM_DATA(0x93, 0x0003);
  LCD_Write_COM_DATA(0x95, 0x0110);
  LCD_Write_COM_DATA(0x97, 0x0000);
  LCD_Write_COM_DATA(0x98, 0x0000);
  //-----Display on-----------------------
  LCD_Write_COM_DATA(0x07, 0x0173);
  chThdSleepMilliseconds(50);

  cs_high();

  clrScr();
}

void
setXY(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
#if (DISP_ORIENT == LANDSCAPE)
  swap(uint16_t, x1, y1);
  swap(uint16_t, x2, y2)
  y1 = DISP_WIDTH - y1 - 1;
  y2 = DISP_WIDTH - y2 - 1;
  swap(uint16_t, y1, y2)
#endif

  // set cursor
  LCD_Write_COM_DATA(0x20, x1);
#if (DISP_ORIENT == LANDSCAPE)
  LCD_Write_COM_DATA(0x21, y2);
#else
  LCD_Write_COM_DATA(0x21, y1);
#endif

  // set window
  LCD_Write_COM_DATA(0x50, x1);
  LCD_Write_COM_DATA(0x51, x2);
  LCD_Write_COM_DATA(0x52, y1);
  LCD_Write_COM_DATA(0x53, y2);
  LCD_Write_COM(0x22);
}

void
clrXY()
{
  setXY(0, 0, DISP_WIDTH - 1, DISP_HEIGHT - 1);
}

void
drawRect(rect_t rect)
{
  drawHLine(rect.x, rect.y, rect.width-1);
  drawHLine(rect.x, rect.y + rect.height-1, rect.width-1);
  drawVLine(rect.x, rect.y, rect.height);
  drawVLine(rect.x + rect.width-1, rect.y, rect.height);
}

void
drawRoundRect(int x1, int y1, int x2, int y2)
{
  if (x1 > x2) {
    swap(int, x1, x2);
  }
  if (y1 > y2) {
    swap(int, y1, y2);
  }
  if ((x2 - x1) > 4 && (y2 - y1) > 4) {
    drawPixel(x1 + 1, y1 + 1);
    drawPixel(x2 - 1, y1 + 1);
    drawPixel(x1 + 1, y2 - 1);
    drawPixel(x2 - 1, y2 - 1);
    drawHLine(x1 + 2, y1, x2 - x1 - 4);
    drawHLine(x1 + 2, y2, x2 - x1 - 4);
    drawVLine(x1, y1 + 2, y2 - y1 - 4);
    drawVLine(x2, y1 + 2, y2 - y1 - 4);
  }
}

void
fillRect(rect_t rect)
{
  int i;

  cs_low();
  setXY(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
  for (i = 0; i < rect.width * rect.height; ++i) {
    setPixel(fcolor);
  }
  cs_high();
}

void
fillRoundRect(int x1, int y1, int x2, int y2)
{
  if (x1 > x2) {
    swap(int, x1, x2);
  }
  if (y1 > y2) {
    swap(int, y1, y2);
  }

  if ((x2 - x1) > 4 && (y2 - y1) > 4) {
    int i;
    for (i = 0; i < ((y2 - y1) / 2) + 1; i++) {
      switch (i)
      {
      case 0:
        drawHLine(x1 + 2, y1 + i, x2 - x1 - 4);
        drawHLine(x1 + 2, y2 - i, x2 - x1 - 4);
        break;
      case 1:
        drawHLine(x1 + 1, y1 + i, x2 - x1 - 2);
        drawHLine(x1 + 1, y2 - i, x2 - x1 - 2);
        break;
      default:
        drawHLine(x1, y1 + i, x2 - x1);
        drawHLine(x1, y2 - i, x2 - x1);
        break;
      }
    }
  }
}

void
drawCircle(int x, int y, int radius)
{
  int f = 1 - radius;
  int ddF_x = 1;
  int ddF_y = -2 * radius;
  int x1 = 0;
  int y1 = radius;

  cs_low();
  setXY(x, y + radius, x, y + radius);
  LCD_Write_DATA(fcolor);
  setXY(x, y - radius, x, y - radius);
  LCD_Write_DATA(fcolor);
  setXY(x + radius, y, x + radius, y);
  LCD_Write_DATA(fcolor);
  setXY(x - radius, y, x - radius, y);
  LCD_Write_DATA(fcolor);

  while (x1 < y1) {
    if (f >= 0) {
      y1--;
      ddF_y += 2;
      f += ddF_y;
    }
    x1++;
    ddF_x += 2;
    f += ddF_x;
    setXY(x + x1, y + y1, x + x1, y + y1);
    LCD_Write_DATA(fcolor);
    setXY(x - x1, y + y1, x - x1, y + y1);
    LCD_Write_DATA(fcolor);
    setXY(x + x1, y - y1, x + x1, y - y1);
    LCD_Write_DATA(fcolor);
    setXY(x - x1, y - y1, x - x1, y - y1);
    LCD_Write_DATA(fcolor);
    setXY(x + y1, y + x1, x + y1, y + x1);
    LCD_Write_DATA(fcolor);
    setXY(x - y1, y + x1, x - y1, y + x1);
    LCD_Write_DATA(fcolor);
    setXY(x + y1, y - x1, x + y1, y - x1);
    LCD_Write_DATA(fcolor);
    setXY(x - y1, y - x1, x - y1, y - x1);
    LCD_Write_DATA(fcolor);
  }
  cs_high();
  clrXY();
}

void
fillCircle(int x, int y, int radius)
{
  int x1;
  int y1;
  cs_low();
  for (y1 = -radius; y1 <= radius; y1++)
    for (x1 = -radius; x1 <= radius; x1++)
      if (x1 * x1 + y1 * y1 <= radius * radius) {
        setXY(x + x1, y + y1, x + x1, y + y1);
        LCD_Write_DATA(fcolor);
      }
  cs_high();
  clrXY();
}

void
clrScr()
{
  if (bg_type == BG_IMAGE) {
    tile_bitmap(bg_img, display_rect);
  }
  else {
    long i;

    cs_low();
    clrXY();
    rs_high();
    for (i = 0; i < (DISP_WIDTH * DISP_HEIGHT); i++) {
      LCD_Write_Bus(0);
    }
    cs_high();
  }
}

void
fillScr(uint16_t color)
{
  long i;

  cs_low();
  clrXY();
  rs_high();
  for (i = 0; i < (DISP_WIDTH * DISP_HEIGHT); i++) {
    LCD_Write_Bus(color);
  }
  cs_high();
}

void
setColor(uint16_t color)
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

void
setPixel(uint16_t color)
{
  LCD_Write_DATA(color);
}

void
drawPixel(int x, int y)
{
  cs_low();
  setXY(x, y, x, y);
  setPixel(fcolor);
  cs_high();
  clrXY();
}

void
drawLine(int x1, int y1, int x2, int y2)
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
    drawHLine(x1, y1, x2 - x1);
  }
  else if (x1 == x2) {
    if (y1 > y2) {
      swap(int, y1, y2);
    }
    drawVLine(x1, y1, y2 - y1);
  }
  else if (abs(x2 - x1) > abs(y2 - y1)) {
    cs_low();
    delta = ((double) (y2 - y1)) / ((double) (x2 - x1));
    ty = (double) (y1);
    if (x1 > x2) {
      int i;
      for (i = x1; i >= x2; i--) {
        setXY(i, (int) (ty + 0.5), i, (int) (ty + 0.5));
        LCD_Write_DATA(fcolor);
        ty = ty - delta;
      }
    }
    else {
      int i;
      for (i = x1; i <= x2; i++) {
        setXY(i, (int) (ty + 0.5), i, (int) (ty + 0.5));
        LCD_Write_DATA(fcolor);
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
        setXY((int) (tx + 0.5), i, (int) (tx + 0.5), i);
        LCD_Write_DATA(fcolor);
        tx = tx + delta;
      }
    }
    else {
      int i;
      for (i = y1; i < y2 + 1; i++) {
        setXY((int) (tx + 0.5), i, (int) (tx + 0.5), i);
        LCD_Write_DATA(fcolor);
        tx = tx + delta;
      }
    }
    cs_high();
  }

  clrXY();
}

void
drawHLine(int x, int y, int l)
{
  int i;

  cs_low();
  setXY(x, y, x + l, y);
  for (i = 0; i < l + 1; i++) {
    LCD_Write_DATA(fcolor);
  }
  cs_high();
  clrXY();
}

void
drawVLine(int x, int y, int l)
{
  int i;

  cs_low();
  setXY(x, y, x, y + l);
  for (i = 0; i < l; i++) {
    LCD_Write_DATA(fcolor);
  }
  cs_high();
  clrXY();
}

void
printChar(const Glyph_t* g, int x, int y)
{
  uint16_t j;

  cs_low();

  setXY(x, y, x + g->width - 1, y + g->height - 1);

  for (j = 0; j < (g->width * g->height); j++) {
    uint8_t alpha = g->data[j];
    if (alpha == 255) {
      setPixel(fcolor);
    }
    else {
      uint16_t by = y + (j / g->width);
      uint16_t bx = x + (j % g->width);
      uint16_t bcolor = get_bg_color(bx, by);

      if (alpha == 0) {
        setPixel(bcolor);
      }
      else {
        setPixel(BLENDED_COLOR(fcolor, bcolor, alpha));
      }
    }
  }

  cs_high();
  clrXY();
}

void
print(char *st, int x, int y)
{
  int xoff = 0;

  while (*st) {
    const Glyph_t* g = font_find_glyph(cfont, *st++);
    xoff += g->xoffset;
    printChar(g, x + xoff, y + g->yoffset);
    xoff += g->advance;
  }
}

void
printNumI(long num, int x, int y, int length, char filler)
{
  int i;
  char buf[25];
  char st[27];
  int neg = 0;
  int c = 0, f = 0;

  if (num == 0) {
    if (length != 0) {
      for (c = 0; c < (length - 1); c++)
        st[c] = filler;
      st[c] = 48;
      st[c + 1] = 0;
    }
    else {
      st[0] = 48;
      st[1] = 0;
    }
  }
  else {
    if (num < 0) {
      neg = 1;
      num = -num;
    }

    while (num > 0) {
      buf[c] = 48 + (num % 10);
      c++;
      num = (num - (num % 10)) / 10;
    }
    buf[c] = 0;

    if (neg) {
      st[0] = 45;
    }

    if (length > (c + neg)) {
      for (i = 0; i < (length - c - neg); i++) {
        st[i + neg] = filler;
        f++;
      }
    }

    for (i = 0; i < c; i++) {
      st[i + neg + f] = buf[c - i - 1];
    }
    st[c + neg + f] = 0;

  }

  print(st, x, y);
}

void
printNumF(double num, uint8_t dec, int x, int y, char divider, int length,
    char filler)
{
  char buf[25];
  char st[27];
  int neg = 0;
  int c = 0, f = 0;
  int c2, mult;
  unsigned long inum;
  int i;

  if (dec < 1)
    dec = 1;
  if (dec > 5)
    dec = 5;

  if (num == 0) {
    if (length != 0) {
      for (c = 0; c < (length - 2 - dec); c++)
        st[c] = filler;
      st[c] = 48;
      st[c + 1] = divider;
      for (i = 0; i < dec; i++)
        st[c + 2 + i] = 48;
      st[c + 2 + dec] = 0;
    }
    else {
      st[0] = 48;
      st[1] = divider;
      for (i = 0; i < dec; i++)
        st[2 + i] = 48;
      st[2 + dec] = 0;
    }
  }
  else {
    int j;
    if (num < 0) {
      neg = 1;
      num = -num;
    }

    mult = 1;
    for (j = 0; j < dec; j++)
      mult = mult * 10;
    inum = (long) (num * mult + 0.5);

    while (inum > 0) {
      buf[c] = 48 + (inum % 10);
      c++;
      inum = (inum - (inum % 10)) / 10;
    }
    if ((num < 1) && (num > 0)) {
      buf[c] = 48;
      c++;
    }
    while (c < (dec + 1)) {
      buf[c] = 48;
      c++;
    }
    buf[c] = 0;

    if (neg) {
      st[0] = 45;
    }

    if (length > (c + neg - 1)) {
      for (i = 0; i < (length - c - neg - 1); i++) {
        st[i + neg] = filler;
        f++;
      }
    }

    c2 = neg;
    for (i = 0; i < c; i++) {
      st[c2 + f] = buf[c - i - 1];
      c2++;
      if ((c - (c2 - neg)) == dec) {
        st[c2 + f] = divider;
        c2++;
      }
    }
    st[c2 + f] = 0;
  }

  print(st, x, y);
}

void
setFont(const Font_t* font)
{
  cfont = font;
}

void
drawBitmap(int x, int y, const Image_t* img)
{
  unsigned int col;

  int tc;
  cs_low();
  setXY(x, y, x + img->width - 1, y + img->height - 1);
  for (tc = 0; tc < (img->width * img->height); tc++) {
    col = img->data[tc];
    LCD_Write_DATA(col);
  }
  cs_high();

  clrXY();
}

static uint16_t
get_tile_color(const Image_t* img, int x, int y)
{
  uint16_t imx = x % img->width;
  uint16_t imy = y % img->height;
  uint16_t col = img->data[imx + (imy * img->width)];
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
  setXY(rect.x, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1);
  for (i = 0; i < rect.height; ++i) {
    for (j = 0; j < rect.width; ++j) {
      setPixel(get_tile_color(img, j, i));
    }
  }
  cs_high();
  clrXY();
}

