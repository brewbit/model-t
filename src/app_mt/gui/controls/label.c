
#include "label.h"
#include "gui.h"
#include "gfx.h"

#include <stdlib.h>
#include <string.h>


typedef struct {
  char* text;
  const font_t* font;
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
label_create(widget_t* parent, rect_t rect, const char* text, const font_t* font, color_t color, uint8_t rows)
{
  label_t* l = calloc(1, sizeof(label_t));

  if (text != NULL)
    l->text = strdup(text);
  l->font = font;
  l->color = color;
  l->rows = rows;

  rect.height = (font->line_height * rows);
  return widget_create(parent, &label_widget_class, l, rect);
}

void
label_set_text(widget_t* w, const char* text)
{
  label_t* l = widget_get_instance_data(w);
  if (strcmp(l->text, text) != 0) {
    if (l->text != NULL)
      free(l->text);
    l->text = strdup(text);
    widget_invalidate(w);
  }
}

static void
label_destroy(widget_t* w)
{
  label_t* l = widget_get_instance_data(w);
  free(l);
}

static char*
get_row_text(label_t* l, const char* text, int width, bool ellipsize)
{
  char* str = strdup(text);
  Extents_t e = font_text_extents(l->font, str);
  if (e.width < width)
    return str;

  if (ellipsize)
    width -= font_text_extents(l->font, "...").width;

  do {
    char* last_space = strrchr(str, ' ');
    if (last_space != NULL) {
      // chop off the last word and see if the string will fit.
      *last_space = '\0';
      e = font_text_extents(l->font, str);
    }
    else {
      // There is no whitespace, so just start chopping characters until it fits.
      do {
        str[strlen(str) - 1] = '\0';
        e = font_text_extents(l->font, str);
      } while (e.width > width);
    }
  } while (e.width > width);

  if (ellipsize) {
    char* estr = malloc(strlen(str) + 3 + 1);
    strcpy(estr, str);
    strcat(estr, "...");
    free(str);
    str = estr;
  }

  return str;
}

static void
label_paint(paint_event_t* event)
{
  int i;
  label_t* l = widget_get_instance_data(event->widget);
  rect_t rect = widget_get_rect(event->widget);

  gfx_set_font(l->font);
  gfx_set_fg_color(l->color);

  const char* text = l->text;
  for (i = 0; i < l->rows; ++i) {
    char* row_text = get_row_text(l, text, rect.width, (i == (l->rows - 1)));

//    if ((i == (l->rows - 1)) &&
//        (l->ellipsize == ELLIPSIZE_END) &&
//        (strlen(row_text) < strlen(text))) {
//
//    }

    gfx_draw_str(row_text, -1, rect.x, rect.y + (i * l->font->line_height));

    text += strlen(row_text);
    while (*text == ' ')
      text++;

    free(row_text);
  }
}
