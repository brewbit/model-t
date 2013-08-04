
#include "gui_output.h"
#include "gfx.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui/icon.h"
#include "gui.h"
#include "temp_control.h"
#include "app_cfg.h"

typedef struct {
  widget_t* widget;
  widget_t* back_button;

  widget_t* trigger_probe1_button;
  widget_t* trigger_probe1_icon;
  widget_t* trigger_probe1_header_label;
  widget_t* trigger_probe1_desc_label;

  widget_t* trigger_probe2_button;
  widget_t* trigger_probe2_icon;
  widget_t* trigger_probe2_header_label;
  widget_t* trigger_probe2_desc_label;

  output_id_t output;
} output_screen_t;


static void output_trigger_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);
static void trigger_probe1_button_clicked(button_event_t* event);
static void trigger_probe2_button_clicked(button_event_t* event);


widget_class_t output_trigger_widget_class = {
    .on_destroy = output_trigger_screen_destroy
};


widget_t*
output_trigger_screen_create(output_id_t output)
{
  char* header;
  char* desc;
  output_screen_t* s = calloc(1, sizeof(output_screen_t));
  s->widget = widget_create(NULL, &output_trigger_widget_class, s, display_rect);
  widget_set_background(s->widget, BLACK, FALSE);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  s->back_button = button_create(s->widget, rect, img_left, BLACK, NULL, NULL, NULL, back_button_clicked);

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  char* title = (output == OUTPUT_1) ? "Output 1 Trig" : "Output 2 Trig";
  label_create(s->widget, rect, title, font_opensans_22, WHITE, 1);

  rect.x = 10;
  rect.y = 95;
  rect.width = 300;
  rect.height = 66;
  s->trigger_probe1_button = button_create(s->widget, rect, NULL, BLACK, NULL, NULL, NULL, trigger_probe1_button_clicked);

  rect.y = 165;
  s->trigger_probe2_button = button_create(s->widget, rect, NULL, BLACK, NULL, NULL, NULL, trigger_probe2_button_clicked);

  rect.x = 5;
  rect.y = 5;
  rect.width = 56;
  rect.height = 56;
  s->trigger_probe1_icon = icon_create(s->trigger_probe1_button, rect, img_temp_38, AMBER);

  rect.x = 70;
  rect.y = 5;
  rect.width = 200;
  s->trigger_probe1_header_label = label_create(s->trigger_probe1_button, rect, NULL, font_opensans_16, WHITE, 1);

  rect.y = 30;
  s->trigger_probe1_desc_label = label_create(s->trigger_probe1_button, rect, NULL, font_opensans_8, WHITE, 2);
  header = "Trigger: Probe 1";
  desc = "The temperature will be read from Probe 1.";
  label_set_text(s->trigger_probe1_header_label, header);
  label_set_text(s->trigger_probe1_desc_label, desc);

  rect.x = 5;
  rect.y = 5;
  rect.width = 56;
  rect.height = 56;
  s->trigger_probe2_icon = icon_create(s->trigger_probe2_button, rect, img_temp_38, PURPLE);

  rect.x = 70;
  rect.y = 5;
  rect.width = 200;
  s->trigger_probe2_header_label = label_create(s->trigger_probe2_button, rect, NULL, font_opensans_16, WHITE, 1);

  rect.y = 30;
  s->trigger_probe2_desc_label = label_create(s->trigger_probe2_button, rect, NULL, font_opensans_8, WHITE, 2);
  header = "Trigger: Probe 2";
  desc = "The temperature will be read from Probe 2.";
  label_set_text(s->trigger_probe2_header_label, header);
  label_set_text(s->trigger_probe2_desc_label, desc);

  s->output = output;

  return s->widget;
}

static void
output_trigger_screen_destroy(widget_t* w)
{
  output_screen_t* s = widget_get_instance_data(w);
  free(s);
}

static void
back_button_clicked(button_event_t* event)
{
  (void)event;

  gui_pop_screen();
}

static void
trigger_probe1_button_clicked(button_event_t* event)
{
  widget_t* screen = widget_get_parent(event->widget);
  output_screen_t* s = widget_get_instance_data(screen);

  output_settings_t settings = *app_cfg_get_output_settings(s->output);
  settings.trigger = PROBE_1;
  app_cfg_set_output_settings(s->output, &settings);

  gui_pop_screen();
}

static void
trigger_probe2_button_clicked(button_event_t* event)
{
  widget_t* screen = widget_get_parent(event->widget);
  output_screen_t* s = widget_get_instance_data(screen);

  output_settings_t settings = *app_cfg_get_output_settings(s->output);
  settings.trigger = PROBE_2;
  app_cfg_set_output_settings(s->output, &settings);

  gui_pop_screen();
}
