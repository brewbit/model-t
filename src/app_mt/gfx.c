
#include "gfx.h"
#include "lcd.h"
#include "common.h"

#include <limits.h>
#include <string.h>


// Alpha blending support
#define RED_COMPONENT(color) (((color) >> 11) & 0x1F)
#define GREEN_COMPONENT(color) (((color) >> 5) & 0x3F)
#define BLUE_COMPONENT(color) ((color) & 0x1F)

#define BLENDED_COMPONENT(fg, bg, alpha) (((fg) * (alpha) / 255) + ((bg) * (255-(alpha)) / 255))

#define BLENDED_COLOR(fg, bg, alpha) COLOR16( \
        BLENDED_COMPONENT(RED_COMPONENT(fg), RED_COMPONENT(bg), alpha), \
        BLENDED_COMPONENT(GREEN_COMPONENT(fg), GREEN_COMPONENT(bg), alpha), \
        BLENDED_COMPONENT(BLUE_COMPONENT(fg), BLUE_COMPONENT(bg), alpha))

#define swap(type, a, b) { type SWAP_tmp = a; a = b; b = SWAP_tmp; }


typedef enum {
  BG_IMAGE,
  BG_COLOR
} BackgroundType;


static void draw_horiz_line(int x, int y, int l);
static void draw_vert_line(int x, int y, int l);
static uint16_t get_tile_color(const Image_t* img, int x, int y);
static uint16_t get_bg_color(int x, int y);
static void fill_rect(rect_t rect, uint16_t color);

typedef struct gfx_ctx_s {
  uint16_t fcolor;
  uint16_t bcolor;
  BackgroundType bg_type;
  const Image_t* bg_img;
  point_t bg_anchor;
  const Font_t* cfont;
  point_t translation;

  struct gfx_ctx_s* next;
} gfx_ctx_t;

gfx_ctx_t* ctx;


void
gfx_init()
{
  lcd_init();

  ctx = chHeapAlloc(NULL, sizeof(gfx_ctx_t));
  memset(ctx, 0, sizeof(gfx_ctx_t));

  ctx->fcolor = GREEN;
  ctx->bcolor = BLACK;
  ctx->bg_type = BG_COLOR;

  gfx_clear_screen();
}

void
gfx_ctx_push()
{
  gfx_ctx_t* new_ctx = chHeapAlloc(NULL, sizeof(gfx_ctx_t));
  memset(new_ctx, 0, sizeof(gfx_ctx_t));

  *new_ctx = *ctx;
  new_ctx->next = ctx;
  ctx = new_ctx;
}

void
gfx_ctx_pop()
{
  gfx_ctx_t* old_ctx = ctx;
  if (old_ctx->next != NULL) {
    ctx = old_ctx->next;
    chHeapFree(old_ctx);
  }
}

void
gfx_push_translation(uint16_t x, uint16_t y)
{
  ctx->translation.x += x;
  ctx->translation.y += y;
}

static void
gfx_set_cursor(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
  lcd_set_cursor(
      ctx->translation.x + x1,
      ctx->translation.y + y1,
      ctx->translation.x + x2,
      ctx->translation.y + y2);
}

void
gfx_draw_rect(rect_t rect)
{
  draw_horiz_line(rect.x, rect.y, rect.width-1);
  draw_horiz_line(rect.x, rect.y + rect.height-1, rect.width-1);
  draw_vert_line(rect.x, rect.y, rect.height);
  draw_vert_line(rect.x + rect.width-1, rect.y, rect.height);
}

void
gfx_fill_rect(rect_t rect)
{
  fill_rect(rect, ctx->fcolor);
}

static void
fill_rect(rect_t rect, uint16_t color)
{
  int i;

  gfx_set_cursor(rect.x, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1);
  for (i = 0; i < (rect.width * rect.height); ++i) {
    lcd_write_data(color);
  }
}

void
gfx_clear_screen()
{
  gfx_clear_rect(display_rect);
}

void
gfx_clear_rect(rect_t rect)
{
  if (ctx->bg_type == BG_IMAGE) {
    gfx_tile_bitmap(ctx->bg_img, rect);
  }
  else {
    fill_rect(rect, ctx->bcolor);
  }
}

void
gfx_set_fg_color(uint16_t color)
{
  ctx->fcolor = color;
}

void
gfx_set_bg_color(uint16_t color)
{
  ctx->bcolor = color;
  ctx->bg_type = BG_COLOR;
}

void
gfx_set_bg_img(const Image_t* img, point_t anchor)
{
  ctx->bg_img = img;
  ctx->bg_anchor = anchor;
  ctx->bg_type = BG_IMAGE;
}

void
gfx_set_font(const Font_t* font)
{
  ctx->cfont = font;
}

void
gfx_draw_line(int x1, int y1, int x2, int y2)
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
    delta = ((double) (y2 - y1)) / ((double) (x2 - x1));
    ty = (double) (y1);
    if (x1 > x2) {
      int i;
      for (i = x1; i >= x2; i--) {
        gfx_set_cursor(i, (int) (ty + 0.5), i, (int) (ty + 0.5));
        lcd_write_data(ctx->fcolor);
        ty = ty - delta;
      }
    }
    else {
      int i;
      for (i = x1; i <= x2; i++) {
        gfx_set_cursor(i, (int) (ty + 0.5), i, (int) (ty + 0.5));
        lcd_write_data(ctx->fcolor);
        ty = ty + delta;
      }
    }
  }
  else {
    delta = ((float) (x2 - x1)) / ((float) (y2 - y1));
    tx = (float) (x1);
    if (y1 > y2) {
      int i;
      for (i = y2 + 1; i > y1; i--) {
        gfx_set_cursor((int) (tx + 0.5), i, (int) (tx + 0.5), i);
        lcd_write_data(ctx->fcolor);
        tx = tx + delta;
      }
    }
    else {
      int i;
      for (i = y1; i < y2 + 1; i++) {
        gfx_set_cursor((int) (tx + 0.5), i, (int) (tx + 0.5), i);
        lcd_write_data(ctx->fcolor);
        tx = tx + delta;
      }
    }
  }

  lcd_clr_cursor();
}

static void
draw_horiz_line(int x, int y, int l)
{
  int i;

  gfx_set_cursor(x, y, x + l, y);
  for (i = 0; i < l + 1; i++) {
    lcd_write_data(ctx->fcolor);
  }
  lcd_clr_cursor();
}

void
draw_vert_line(int x, int y, int l)
{
  int i;

  gfx_set_cursor(x, y, x, y + l);
  for (i = 0; i < l; i++) {
    lcd_write_data(ctx->fcolor);
  }
  lcd_clr_cursor();
}

void
gfx_print_char(const Glyph_t* g, int x, int y)
{
  uint16_t j;

  gfx_set_cursor(x, y, x + g->width - 1, y + g->height - 1);

  for (j = 0; j < (g->width * g->height); j++) {
    uint8_t alpha = g->data[j];
    if (alpha == 255) {
      lcd_write_data(ctx->fcolor);
    }
    else {
      uint16_t by = y + (j / g->width);
      uint16_t bx = x + (j % g->width);
      uint16_t bcolor = get_bg_color(bx, by);

      if (alpha == 0) {
        lcd_write_data(bcolor);
      }
      else {
        lcd_write_data(BLENDED_COLOR(ctx->fcolor, ctx->bcolor, alpha));
      }
    }
  }

  lcd_clr_cursor();
}

void
gfx_draw_str(const char *str, int n, int x, int y)
{
  int xoff = 0;

  if (n == -1)
    n = INT_MAX;

  while ((*str != 0) &&
         (n-- > 0)) {
    const Glyph_t* g = font_find_glyph(ctx->cfont, *str++);
    gfx_print_char(g, x + xoff + g->xoffset, y + g->yoffset);
    xoff += g->advance;
  }
}

void
gfx_draw_bitmap(int x, int y, const Image_t* img)
{
  unsigned int col;

  int tc;
  gfx_set_cursor(x, y, x + img->width - 1, y + img->height - 1);
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
        lcd_write_data(fcolor);
      }
      else {
        uint16_t by = y + (tc / img->width);
        uint16_t bx = x + (tc % img->width);

        uint16_t bcolor = get_bg_color(bx, by);

        if (alpha == 0) {
          lcd_write_data(bcolor);
        }
        else {
          col = BLENDED_COLOR(fcolor, bcolor, alpha);
          lcd_write_data(col);
        }
      }
    }
    break;
  }

  lcd_clr_cursor();
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
  if (ctx->bg_type == BG_IMAGE) {
    return get_tile_color(ctx->bg_img, x - ctx->bg_anchor.x, y - ctx->bg_anchor.y);
  }
  else {
    return ctx->bcolor;
  }
}

void
gfx_tile_bitmap(const Image_t* img, rect_t rect)
{
  int i, j;

  gfx_set_cursor(rect.x, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1);
  for (i = 0; i < rect.height; ++i) {
    for (j = 0; j < rect.width; ++j) {
      lcd_write_data(get_tile_color(img, j, i));
    }
  }
  lcd_clr_cursor();
}

