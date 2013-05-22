#include "terminal.h"
#include "lcd.h"
#include "image.h"
#include "font.h"

#include <string.h>

static uint16_t cur_row = 0;
static uint16_t cur_col = 0;

#define CHAR_WIDTH font_terminal->max_width
#define CHAR_HEIGHT font_terminal->max_height
#define MAX_COL display_width() / CHAR_WIDTH

void
terminal_clear()
{
  cur_row = 0;
  cur_col = 0;

  tile_bitmap(img_background, 0, 0, display_width(), display_height());
}

void
terminal_write(char* str)
{
  setColor(COLOR(0, 255, 0));

  while (*str) {
    if (*str == '\n') {
      cur_row++;
      cur_col = 0;
    }
    else {
      const Glyph_t* g = font_find_glyph(font_terminal, *str);
      printChar(g,
          (cur_col * CHAR_WIDTH),
          (cur_row * CHAR_HEIGHT) + g->yoffset);
      cur_col++;
      if (cur_col >= MAX_COL) {
        cur_row++;
        cur_col = 0;
      }
    }
    str++;
  }
}

void
terminal_write_hex(uint32_t num)
{
  int i;
  char buf[16] =
    { '0', 'x', '0' };
  int idx = 2;
  uint64_t div = 0x100000000;
  char* digits = "0123456789ABCDEF";
  int leading = 1;

  for (i = 0; i < 9; ++i) {
    int q = num / div;
    if (q > 0 || !leading) {
      buf[idx++] = digits[q];
      leading = 0;
    }

    num -= q * div;
    div /= 16;
  }
  buf[idx] = 0;

  terminal_write(buf);
}

void
terminal_write_int(uint32_t num)
{
  int i;
  char buf[16];
  int idx = 0;
  int div = 1000000000;
  int leading = 1;

  for (i = 0; i < 10; ++i) {
    int q = num / div;
    if (q > 0 || !leading) {
      buf[idx++] = '0' + q;
      leading = 0;
    }

    num -= q * div;
    div /= 10;
  }
  buf[idx] = 0;

  terminal_write(buf);
}
