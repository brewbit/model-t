
#include "icon.h"
#include "gui.h"
#include "gfx.h"

#include <stdlib.h>
#include <string.h>


typedef struct {
  const Image_t* image;

  color_t enabled_icon_color;
  color_t enabled_bg_color;
  color_t disabled_icon_color;
  color_t disabled_bg_color;
} icon_t;


static void icon_paint(paint_event_t* event);
static void icon_destroy(widget_t* w);
static void icon_enable(enable_event_t* event);


static const widget_class_t icon_widget_class = {
    .on_paint = icon_paint,
    .on_destroy = icon_destroy,
    .on_enable = icon_enable
};

widget_t*
icon_create(widget_t* parent, rect_t rect, const Image_t* image, color_t icon_color, color_t bg_color)
{
  icon_t* i = calloc(1, sizeof(icon_t));

  i->image = image;
  i->enabled_icon_color = icon_color;
  i->enabled_bg_color = bg_color;
  i->disabled_icon_color = icon_color;
  i->disabled_bg_color = TRANSPARENT;

  widget_t* w = widget_create(parent, &icon_widget_class, i, rect);
  widget_set_background(w, bg_color);

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
  if (i->image != image) {
    i->image = image;
    widget_invalidate(w);
  }
}

void
icon_set_color(widget_t* w, color_t icon_color)
{
  icon_t* i = widget_get_instance_data(w);
  if (i->enabled_icon_color != icon_color) {
    i->enabled_icon_color = icon_color;

    if (widget_is_enabled(w))
      widget_invalidate(w);
  }
}

void
icon_set_disabled_color(widget_t* w, color_t icon_color)
{
  icon_t* i = widget_get_instance_data(w);
  if (i->disabled_icon_color != icon_color) {
    i->disabled_icon_color = icon_color;

    if (!widget_is_enabled(w))
      widget_invalidate(w);
  }
}

static void
icon_paint(paint_event_t* event)
{
  icon_t* i = widget_get_instance_data(event->widget);

  rect_t rect = widget_get_rect(event->widget);
  point_t center = rect_center(rect);

  /* draw icon */
  if (i->image != NULL) {
    if (widget_is_enabled(event->widget)) {
      gfx_set_fg_color(i->enabled_icon_color);
    }
    else {
      gfx_set_fg_color(i->disabled_icon_color);
    }

    gfx_draw_bitmap(
        center.x - (i->image->width / 2),
        center.y - (i->image->height / 2),
        i->image);
  }
}

static void
icon_enable(enable_event_t* event)
{
  icon_t* i = widget_get_instance_data(event->widget);

  if (event->enabled) {
    widget_set_background(event->widget, i->enabled_bg_color);
  }
  else {
    widget_set_background(event->widget, i->disabled_bg_color);
  }
}
