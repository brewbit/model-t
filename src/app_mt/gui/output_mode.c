#include "gui/output.h"
#include "gfx.h"
#include "button.h"
#include "label.h"
#include "icon.h"
#include "gui.h"
#include "temp_control.h"
#include "app_cfg.h"
#include "sensor.h"

#include <string.h>

typedef struct {
  widget_t* widget;
  widget_t* back_button;

  widget_t* on_off_mode_button;
  widget_t* on_off_mode_icon;
  widget_t* on_off_mode_header_label;
  widget_t* on_off_mode_desc_label;

  widget_t* pid_mode_button;
  widget_t* pid_mode_icon;
  widget_t* pid_mode_header_label;
  widget_t* pid_mode_desc_label;

  output_id_t output;
} output_screen_t;


static void output_mode_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);
static void on_off_mode_button_clicked(button_event_t* event);
static void pid_mode_button_clicked(button_event_t* event);


widget_class_t output_mode_widget_class = {
    .on_destroy = output_mode_screen_destroy
};


widget_t*
output_mode_screen_create(output_id_t output)
{
  char* header;
  char* desc;
  output_screen_t* s = chHeapAlloc(NULL, sizeof(output_screen_t));
  memset(s, 0, sizeof(output_screen_t));

  s->widget = widget_create(NULL, &output_mode_widget_class, s, display_rect);
  widget_set_background(s->widget, BLACK, FALSE);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  s->back_button = button_create(s->widget, rect, img_left, WHITE, BLACK, back_button_clicked);

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  char* title = (output == OUTPUT_1) ? "Output 1 Mode" : "Output 2 Mode";
  label_create(s->widget, rect, title, font_opensans_regular_22, WHITE, 1);

  rect.x = 10;
  rect.y = 95;
  rect.width = 300;
  rect.height = 66;
  s->on_off_mode_button = button_create(s->widget, rect, NULL, WHITE, BLACK, on_off_mode_button_clicked);

  rect.y = 165;
  s->pid_mode_button = button_create(s->widget, rect, NULL, WHITE, BLACK, pid_mode_button_clicked);

  rect.x = 5;
  rect.y = 5;
  rect.width = 56;
  rect.height = 56;
  s->on_off_mode_icon = icon_create(s->on_off_mode_button, rect, img_circle, WHITE, RED);

  rect.x = 70;
  rect.y = 5;
  rect.width = 200;
  s->on_off_mode_header_label = label_create(s->on_off_mode_button, rect, NULL, font_opensans_regular_18, WHITE, 1);

  rect.y = 30;
  s->on_off_mode_desc_label = label_create(s->on_off_mode_button, rect, NULL, font_opensans_regular_12, WHITE, 2);
  header = "Output Mode: ON/OFF";
  desc = "Enable the output at the exact user defined setpoint";
  label_set_text(s->on_off_mode_header_label, header);
  label_set_text(s->on_off_mode_desc_label, desc);

  rect.x = 5;
  rect.y = 5;
  rect.width = 56;
  rect.height = 56;
  s->pid_mode_icon = icon_create(s->pid_mode_button, rect, img_circle, WHITE, STEEL);

  rect.x = 70;
  rect.y = 5;
  rect.width = 200;
  s->pid_mode_header_label = label_create(s->pid_mode_button, rect, NULL, font_opensans_regular_18, WHITE, 1);

  rect.y = 30;
  s->pid_mode_desc_label = label_create(s->pid_mode_button, rect, NULL, font_opensans_regular_12, WHITE, 2);
  header = "Output Mode: PID";
  desc = "Minimize difference between measured and setpoint temps.";
  label_set_text(s->pid_mode_header_label, header);
  label_set_text(s->pid_mode_desc_label, desc);

  s->output = output;

  return s->widget;
}

static void
output_mode_screen_destroy(widget_t* w)
{
  output_screen_t* s = widget_get_instance_data(w);
  chHeapFree(s);
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK)
    gui_pop_screen();
}

static void
on_off_mode_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* screen = widget_get_parent(event->widget);
    output_screen_t* s = widget_get_instance_data(screen);

    output_settings_t settings = *app_cfg_get_output_settings(s->output);
    settings.output_mode = ON_OFF;
    app_cfg_set_output_settings(s->output, &settings);

    gui_pop_screen();
  }
}

static void
pid_mode_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* screen = widget_get_parent(event->widget);
    output_screen_t* s = widget_get_instance_data(screen);

    output_settings_t settings = *app_cfg_get_output_settings(s->output);
    settings.output_mode = PID;
    app_cfg_set_output_settings(s->output, &settings);

    gui_pop_screen();
  }
}
