
#include "icon.h"
#include "gui.h"
#include "gfx.h"

#include <stdlib.h>
#include <string.h>


typedef struct {
  const Image_t* image;
  uint16_t icon_color;
} icon_t;


static void icon_paint(paint_event_t* event);
static void icon_destroy(widget_t* w);


static const widget_class_t icon_widget_class = {
    .on_paint = icon_paint,
    .on_destroy = icon_destroy,
};

widget_t*
icon_create(widget_t* parent, rect_t rect, const Image_t* image, uint16_t icon_color, uint16_t bg_color)
{
  icon_t* i = calloc(1, sizeof(icon_t));

  i->image = image;
  i->icon_color = icon_color;

  widget_t* w = widget_create(parent, &icon_widget_class, i, rect);
  widget_set_background(w, bg_color, FALSE);

  return w;
}

static void
icon_destroy(widget_t* w)
{
  icon_t* i = widget_get_instance_data(w);
  free(i);
}

void
icon_set_image(widget_t* w, const Image_t* image)
{
  icon_t* i = widget_get_instance_data(w);
  i->image = image;
  widget_invalidate(w);
}

static void
icon_paint(paint_event_t* event)
{
  icon_t* i = widget_get_instance_data(event->widget);

  rect_t rect = widget_get_rect(event->widget);
  point_t center = rect_center(rect);

  if (!widget_is_enabled(event->widget)) {
    gfx_set_bg_color(DARK_GRAY);
    gfx_clear_rect(rect);
  }

  /* draw icon */
  if (i->image != NULL) {
    gfx_set_fg_color(i->icon_color);
    gfx_draw_bitmap(
        center.x - (i->image->width / 2),
        center.y - (i->image->height / 2),
        i->image);
  }
}
