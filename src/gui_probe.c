
#include "gui_probe.h"
#include "lcd.h"
#include "gui/button.h"

typedef struct {
  widget_t* widget;
  widget_t* back_button;
  widget_t* up_button;
  widget_t* down_button;
} probe_screen_t;


static void probe_settings_screen_paint(paint_event_t* event);
static void probe_settings_screen_destroy(widget_t* w);

static void back_clicked(click_event_t* event);

widget_class_t probe_settings_widget_class = {
    .on_paint   = probe_settings_screen_paint,
    .on_destroy = probe_settings_screen_destroy
};

widget_t*
probe_settings_screen_create()
{
  probe_screen_t* s = calloc(1, sizeof(probe_screen_t));
  s->widget = widget_create(NULL, &probe_settings_widget_class, s, display_rect);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  s->back_button = button_create(s->widget, rect, NULL, img_left, BLACK, back_clicked);

  rect.x = 15;
  rect.y = 99;
  s->up_button = button_create(s->widget, rect, NULL, img_up, RED, NULL);

  rect.y = 169;
  s->down_button = button_create(s->widget, rect, NULL, img_down, CYAN, NULL);

  return s->widget;
}

static void
probe_settings_screen_destroy(widget_t* w)
{
  probe_screen_t* s = widget_get_instance_data(w);
  free(s);
}


static void
probe_settings_screen_paint(paint_event_t* event)
{
  (void)event;

  set_bg_color(BLACK);
  clrScr();

  setColor(WHITE);
  setFont(font_opensans_22);
  print("Probe 1 Setup", 85, 20);

  setFont(font_opensans_62);
  print("73.2", 100, 100);

  setColor(LIGHT_GRAY);
  setFont(font_opensans_22);
  print("F", 275, 120);
}


static void
back_clicked(click_event_t* event)
{
  gui_pop_screen();
}
