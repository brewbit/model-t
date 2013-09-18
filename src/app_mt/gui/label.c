
#include "label.h"
#include "gui.h"
#include "gfx.h"

#include <stdlib.h>
#include <string.h>


typedef struct {
  char* text;
  const Font_t* font;
  color_t color;
  uint8_t rows;
} label_t;


static void label_paint(paint_event_t* event);
static void label_destroy(widget_t* w);


static const widget_class_t label_widget_class = {
    .on_paint   = label_paint,
    .on_destroy = label_destroy,
};

widget_t*
label_create(widget_t* parent, rect_t rect, const char* text, const Font_t* font, color_t color, uint8_t rows)
{
  label_t* l = chHeapAlloc(NULL, sizeof(label_t));
  memset(l, 0, sizeof(label_t));

  if (text != NULL)
    l->text = strdup(text);
  l->font = font;
  l->color = color;
  l->rows = rows;

  rect.height = (font->line_height * rows);
  return widget_create(parent, &label_widget_class, l, rect);
}

void
label_set_text(widget_t* w, char* text)
{
  label_t* l = widget_get_instance_data(w);
  if (strcmp(l->text, text) != 0) {
    if (l->text != NULL)
      chHeapFree(l->text);
    l->text = strdup(text);
    widget_invalidate(w);
  }
}

static void
label_destroy(widget_t* w)
{
  label_t* l = widget_get_instance_data(w);
  chHeapFree(l);
}

static void
label_paint(paint_event_t* event)
{
  label_t* l = widget_get_instance_data(event->widget);
  rect_t rect = widget_get_rect(event->widget);

  gfx_set_font(l->font);
  gfx_set_fg_color(l->color);

  char c;
  int str_idx = 0;
  bool in_whitespace = true;
  int last_word_start = 0;
  const char* str = l->text;
  int text_width = 0;
  int row = 0;

  while ((c = str[str_idx]) != 0) {
    if (c == ' ') {
      in_whitespace = true;
    }
    else if (in_whitespace) {
      // just found the beginning of a word
      last_word_start = str_idx;
      in_whitespace = false;
    }

    const Glyph_t* g = font_find_glyph(l->font, c);
    text_width += g->advance;

    if (text_width > rect.width) {
      if (!in_whitespace) {
        if (last_word_start != 0)
          str_idx = last_word_start;
      }

      gfx_draw_str(str, str_idx, rect.x, rect.y + (row * l->font->line_height));

      str = str + str_idx;
      str_idx = 0;
      last_word_start = 0;
      text_width = 0;
      row++;
    }
    else {
      str_idx++;
    }
  }

  gfx_draw_str(str, -1, rect.x, rect.y + (row * l->font->line_height));
}
