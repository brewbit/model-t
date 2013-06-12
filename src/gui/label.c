
#include "label.h"
#include "gui.h"
#include "gfx.h"

#include <stdlib.h>


typedef struct {
  const char* text;
  const Font_t* font;
  color_t color;
} label_t;


static void label_paint(paint_event_t* event);
static void label_destroy(widget_t* w);


static const widget_class_t label_widget_class = {
    .on_paint   = label_paint,
    .on_destroy = label_destroy,
};

widget_t*
label_create(widget_t* parent, rect_t rect, const char* text, const Font_t* font, color_t color)
{
  label_t* l = calloc(1, sizeof(label_t));

  l->text = text;
  l->font = font;
  l->color = color;

  return widget_create(parent, &label_widget_class, l, rect);
}

static void
label_destroy(widget_t* w)
{
  label_t* l = widget_get_instance_data(w);
  free(l);
}

static void
label_paint(paint_event_t* event)
{
  label_t* l = widget_get_instance_data(event->widget);
  rect_t rect = widget_get_rect(event->widget);

  gfx_set_font(l->font);
  gfx_set_fg_color(l->color);
  gfx_draw_str(l->text, rect.x, rect.y);
}
