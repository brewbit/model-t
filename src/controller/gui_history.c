
#include "gui_history.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gfx.h"
#include "gui.h"

typedef struct {
  widget_t* widget;
} history_screen_t;


static void history_screen_destroy(widget_t* w);
static void history_screen_paint(paint_event_t* event);
static void back_button_clicked(button_event_t* event);

widget_class_t history_widget_class = {
    .on_destroy = history_screen_destroy,
    .on_paint = history_screen_paint
};

widget_t*
history_screen_create()
{
  history_screen_t* s = calloc(1, sizeof(history_screen_t));
  s->widget = widget_create(NULL, &history_widget_class, s, display_rect);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  button_create(s->widget, rect, img_left, BLACK, NULL, NULL, NULL, back_button_clicked);

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  label_create(s->widget, rect, "Temp History", font_opensans_22, WHITE, 1);

  return s->widget;
}

static void
history_screen_destroy(widget_t* w)
{
  history_screen_t* s = widget_get_instance_data(w);
  free(s);
}


static void
history_screen_paint(paint_event_t* event)
{

  gfx_draw_line(100, 100, 200, 200);
}

static void
back_button_clicked(button_event_t* event)
{
  (void)event;

  gui_pop_screen();
}
