
#include "gui/output_function.h"
#include "gfx.h"
#include "button.h"
#include "label.h"
#include "icon.h"
#include "gui.h"
#include "temp_control.h"
#include "app_cfg.h"

#include <string.h>

typedef struct {
  widget_t* widget;

  widget_t* function_heat_button;
  widget_t* function_heat_icon;
  widget_t* function_heat_header_label;
  widget_t* function_heat_desc_label;

  widget_t* function_cool_button;
  widget_t* function_cool_icon;
  widget_t* function_cool_header_label;
  widget_t* function_cool_desc_label;

  output_id_t output;
} output_screen_t;


static void output_function_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);
static void function_cool_button_clicked(button_event_t* event);
static void function_heat_button_clicked(button_event_t* event);


widget_class_t output_function_widget_class = {
    .on_destroy = output_function_screen_destroy
};

widget_t*
output_function_screen_create(output_id_t output)
{
  char* header;
  char* desc;
  output_screen_t* s = calloc(1, sizeof(output_screen_t));

  s->widget = widget_create(NULL, &output_function_widget_class, s, display_rect);
  widget_set_background(s->widget, BLACK, FALSE);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  button_create(s->widget, rect, img_left, WHITE, BLACK, back_button_clicked);

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  char* title = (output == OUTPUT_1) ? "Output 1 Func" : "Output 2 Func";
  label_create(s->widget, rect, title, font_opensans_regular_22, WHITE, 1);

  rect.x = 10;
  rect.y = 95;
  rect.width = 300;
  rect.height = 66;
  s->function_heat_button = button_create(s->widget, rect, NULL, WHITE, BLACK, function_heat_button_clicked);

  rect.y = 165;
  s->function_cool_button = button_create(s->widget, rect, NULL, WHITE, BLACK, function_cool_button_clicked);

  rect.x = 5;
  rect.y = 5;
  rect.width = 56;
  rect.height = 56;
  s->function_heat_icon = icon_create(s->function_heat_button, rect, img_flame, WHITE, ORANGE);

  rect.x = 70;
  rect.y = 5;
  rect.width = 200;
  s->function_heat_header_label = label_create(s->function_heat_button, rect, NULL, font_opensans_regular_18, WHITE, 1);

  rect.y = 30;
  s->function_heat_desc_label = label_create(s->function_heat_button, rect, NULL, font_opensans_regular_12, WHITE, 2);
  header = "Function: Heating";
  desc = "The output will turn on when the temp is below the setpoint.";
  label_set_text(s->function_heat_header_label, header);
  label_set_text(s->function_heat_desc_label, desc);

  rect.x = 5;
  rect.y = 5;
  rect.width = 56;
  rect.height = 56;
  s->function_cool_icon = icon_create(s->function_cool_button, rect, img_snowflake, WHITE, CYAN);

  rect.x = 70;
  rect.y = 5;
  rect.width = 200;
  s->function_cool_header_label = label_create(s->function_cool_button, rect, NULL, font_opensans_regular_18, WHITE, 1);

  rect.y = 30;
  s->function_cool_desc_label = label_create(s->function_cool_button, rect, NULL, font_opensans_regular_12, WHITE, 2);
  header = "Function: Cooling";
  desc = "The output will turn on when the temp is above the setpoint.";
  label_set_text(s->function_cool_header_label, header);
  label_set_text(s->function_cool_desc_label, desc);

  s->output = output;

  return s->widget;
}

static void
output_function_screen_destroy(widget_t* w)
{
  output_screen_t* s = widget_get_instance_data(w);
  free(s);
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK)
    gui_pop_screen();
}

static void
function_heat_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* w = widget_get_parent(event->widget);
    output_screen_t* s = widget_get_instance_data(w);

    output_settings_t settings = *app_cfg_get_output_settings(s->output);
    settings.function = OUTPUT_FUNC_HEATING;
    app_cfg_set_output_settings(s->output, &settings);

    gui_pop_screen();
  }
}

static void
function_cool_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* w = widget_get_parent(event->widget);
    output_screen_t* s = widget_get_instance_data(w);

    output_settings_t settings = *app_cfg_get_output_settings(s->output);
    settings.function = OUTPUT_FUNC_COOLING;
    app_cfg_set_output_settings(s->output, &settings);

    gui_pop_screen();
  }
}

