
#include "temp_widget.h"
#include "gui/label.h"
#include "gfx.h"
#include "message.h"
#include "temp_input.h"
#include "gui.h"
#include "app_cfg.h"

#include <stdio.h>


#define SPACE 8


typedef struct {
  temperature_t temp;
  temperature_unit_t unit;
  widget_t* widget;
} temp_widget_t;


static void temp_widget_destroy(widget_t* w);
static void temp_widget_paint(paint_event_t* event);
static void temp_widget_msg(msg_event_t* event);


static const widget_class_t temp_widget_class = {
    .on_destroy = temp_widget_destroy,
    .on_paint   = temp_widget_paint,
    .on_msg     = temp_widget_msg,
};

widget_t*
temp_widget_create(widget_t* parent, rect_t rect)
{
  temp_widget_t* s = calloc(1, sizeof(temp_widget_t));

  rect.height = font_opensans_62->base;
  s->widget = widget_create(parent, &temp_widget_class, s, rect);

  s->temp = INVALID_TEMP;
  s->unit = app_cfg_get_temp_unit();

  gui_msg_subscribe(MSG_TEMP_UNIT, s->widget);

  return s->widget;
}

static void
temp_widget_destroy(widget_t* w)
{
  temp_widget_t* s = widget_get_instance_data(w);

  gui_msg_unsubscribe(MSG_TEMP_UNIT, s->widget);

  free(s);
}

static void
temp_widget_paint(paint_event_t* event)
{
  temp_widget_t* s = widget_get_instance_data(event->widget);
  rect_t rect = widget_get_rect(event->widget);

  float temp = s->temp / 100.0f;
  char* unit_str;
  if (s->unit == TEMP_F) {
    temp = ((temp * 9) / 5) + 32;
    unit_str = "F";
  }
  else {
    unit_str = "C";
  }

  char temp_str[16];
  if (s->temp == INVALID_TEMP)
    sprintf(temp_str, "--.-");
  else
    sprintf(temp_str, "%0.1f", temp);

  Extents_t temp_ext = font_text_extents(font_opensans_62, temp_str);
  Extents_t unit_ext = font_text_extents(font_opensans_22, unit_str);

  point_t center = rect_center(rect);

  int temp_x = center.x - ((temp_ext.width + SPACE + unit_ext.width) / 2);

  gfx_set_fg_color(WHITE);
  gfx_set_font(font_opensans_62);
  gfx_draw_str(temp_str, -1, temp_x, rect.y);

  gfx_set_fg_color(DARK_GRAY);
  gfx_set_font(font_opensans_22);
  gfx_draw_str(unit_str, -1, temp_x + temp_ext.width + SPACE, rect.y);
}


static void
temp_widget_msg(msg_event_t* event)
{
  temp_widget_t* s = widget_get_instance_data(event->widget);

  if (event->msg_id == MSG_TEMP_UNIT) {
    temperature_unit_t* temp_unit = event->msg_data;
    if (s->unit != *temp_unit) {
      s->unit = *temp_unit;
      widget_invalidate(event->widget);
    }
  }
}

void
temp_widget_set_value(widget_t* w, temperature_t temp)
{
  temp_widget_t* s = widget_get_instance_data(w);

  if (s->temp != temp) {
    s->temp = temp;
    widget_invalidate(s->widget);
  }
}
