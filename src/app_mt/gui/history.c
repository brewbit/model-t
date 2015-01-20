
#include "gui/history.h"
#include "button.h"
#include "label.h"
#include "scatter_plot.h"
#include "gfx.h"
#include "gui.h"

#include <string.h>

typedef struct {
  widget_t* widget;
} history_screen_t;


static void history_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);

static const widget_class_t history_widget_class = {
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
  widget_t* back_btn = button_create(s->widget, rect, img_left, WHITE, BLACK, back_button_clicked);
  button_set_up_bg_color(back_btn, BLACK);
  button_set_up_icon_color(back_btn, WHITE);
  button_set_down_bg_color(back_btn, BLACK);
  button_set_down_icon_color(back_btn, LIGHT_GRAY);
  button_set_disabled_bg_color(back_btn, BLACK);
  button_set_disabled_icon_color(back_btn, DARK_GRAY);

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  label_create(s->widget, rect, "Temp History", font_opensans_regular_22, WHITE, 1);

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
