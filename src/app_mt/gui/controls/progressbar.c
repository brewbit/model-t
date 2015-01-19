
#include "progressbar.h"
#include "gui.h"
#include "gfx.h"

#include <stdlib.h>
#include <string.h>


typedef struct {
  widget_t* widget;
  color_t bar_color;
  uint8_t progress;
} progressbar_t;


static void progressbar_paint(paint_event_t* event);
static void progressbar_destroy(widget_t* w);


static const widget_class_t progressbar_widget_class = {
    .on_paint   = progressbar_paint,
    .on_destroy = progressbar_destroy,
};

widget_t*
progressbar_create(widget_t* parent, rect_t rect, color_t bg_color, color_t bar_color)
{
  progressbar_t* l = calloc(1, sizeof(progressbar_t));

  l->progress = 0;
  l->bar_color = bar_color;

  l->widget = widget_create(parent, &progressbar_widget_class, l, rect);
  widget_set_background(l->widget, bg_color);

  return l->widget;
}

uint8_t
progressbar_get_progress(widget_t* w)
{
  progressbar_t* l = widget_get_instance_data(w);

  return l->progress;
}

void
progressbar_set_progress(widget_t* w, uint8_t progress)
{
  progressbar_t* l = widget_get_instance_data(w);

  if (progress > 100)
    progress = 100;

  if (l->progress != progress) {
    l->progress = progress;
    widget_invalidate(w);
  }
}

static void
progressbar_destroy(widget_t* w)
{
  progressbar_t* l = widget_get_instance_data(w);
  free(l);
}

static void
progressbar_paint(paint_event_t* event)
{
//  char prog_str[8];
  progressbar_t* l = widget_get_instance_data(event->widget);
  rect_t rect = widget_get_rect(event->widget);
//  point_t center = rect_center(rect);

  rect.x += 5;
  rect.y += 5;
  rect.width -= 10;
  rect.height -= 10;

  rect.width = rect.width * l->progress / 100;

  gfx_set_fg_color(l->bar_color);
  gfx_fill_rect(rect);

//  sprintf(prog_str, "%d %%", l->progress);
//  Extents_t ext = font_text_extents(font_opensans_16, prog_str);
//
//  gfx_set_font(font_opensans_16);
//  gfx_set_fg_color(WHITE);
//  gfx_draw_str(prog_str, -1,
//      center.x - (ext.width / 2),
//      center.y - (ext.height / 2));
}
