
#include "gui_output.h"
#include "lcd.h"
#include "gui/button.h"

typedef struct {
  widget_t* widget;
  widget_t* back_button;
  widget_t* function_button;
  widget_t* trigger_button;
} output_screen_t;


static void output_settings_screen_paint(paint_event_t* event);
static void output_settings_screen_destroy(widget_t* w);

static void back_clicked(click_event_t* event);

widget_class_t output_settings_widget_class = {
    .on_paint   = output_settings_screen_paint,
    .on_destroy = output_settings_screen_destroy
};

widget_t*
output_settings_screen_create()
{
  output_screen_t* s = calloc(1, sizeof(output_screen_t));
  s->widget = widget_create(NULL, &output_settings_widget_class, s, display_rect);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  s->back_button = button_create(s->widget, rect, NULL, img_left, BLACK, back_clicked);

  rect.x = 15;
  rect.y = 99;
  s->function_button = button_create(s->widget, rect, NULL, img_flame, ORANGE, NULL);

  rect.y = 169;
  s->trigger_button = button_create(s->widget, rect, NULL, img_temp_38, PURPLE, NULL);

  return s->widget;
}

static void
output_settings_screen_destroy(widget_t* w)
{
  output_screen_t* s = widget_get_instance_data(w);
  free(s);
}


static void
output_settings_screen_paint(paint_event_t* event)
{
  (void)event;

  set_bg_color(BLACK);
  clrScr();

  setColor(WHITE);
  setFont(font_opensans_22);
  print("Output 1 Setup", 85, 20);

  setFont(font_opensans_16);
  print("Function: Heating", 85, 95);
  print("Trigger: Probe 2", 85, 165);

  setFont(font_opensans_8);
  print("The output will turn on when the temp", 85, 120);
  print("is below the setpoint.", 85, 135);
  print("The temperature will be read from", 85, 190);
  print("Probe 2.", 85, 205);
}


static void
back_clicked(click_event_t* event)
{
  gui_pop_screen();
}
