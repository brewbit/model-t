
#include "gui_history.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui/scatter_plot.h"
#include "gfx.h"
#include "gui.h"

typedef struct {
  widget_t* widget;
} history_screen_t;


static void history_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);

widget_class_t history_widget_class = {
    .on_destroy = history_screen_destroy,
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
  button_create(s->widget, rect, img_left, BLACK, back_button_clicked);

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  label_create(s->widget, rect, "Temp History", font_opensans_22, WHITE, 1);

  rect.x = 5;
  rect.y = 80;
  rect.width = DISP_WIDTH - 10;
  rect.height = DISP_HEIGHT - 88;
  scatter_plot_create(s->widget, rect);

  return s->widget;
}

static void
history_screen_destroy(widget_t* w)
{
  history_screen_t* s = widget_get_instance_data(w);
  free(s);
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK)
    gui_pop_screen();
}
