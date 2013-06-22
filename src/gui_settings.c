
#include "gui_settings.h"
#include "gfx.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui.h"


typedef struct {
  widget_t* widget;
  widget_t* back_button;
} settings_screen_t;


static void settings_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);


widget_class_t settings_widget_class = {
    .on_destroy = settings_screen_destroy
};

widget_t*
settings_screen_create()
{
  settings_screen_t* s = calloc(1, sizeof(settings_screen_t));
  s->widget = widget_create(NULL, &settings_widget_class, s, display_rect);

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
  label_create(s->widget, rect, "Settings", font_opensans_22, WHITE, 1);

  rect.x = 48;
  rect.y = 86;
  rect.width = 56;
  rect.height = 56;
  button_create(s->widget, rect, img_deg_f, ORANGE, NULL, NULL, NULL, NULL);

  rect.x += 84;
  button_create(s->widget, rect, img_deg_c, CYAN, NULL, NULL, NULL, NULL);

  rect.x += 84;
  button_create(s->widget, rect, img_signal, STEEL, NULL, NULL, NULL, NULL);

  rect.x = 48;
  rect.y += 84;
  button_create(s->widget, rect, img_deg_f, OLIVE, NULL, NULL, NULL, NULL);

  rect.x += 84;
  button_create(s->widget, rect, img_deg_c, PURPLE, NULL, NULL, NULL, NULL);

  rect.x += 84;
  button_create(s->widget, rect, img_signal, GREEN, NULL, NULL, NULL, NULL);


  return s->widget;
}

static void
settings_screen_destroy(widget_t* w)
{
  settings_screen_t* s = widget_get_instance_data(w);
  free(s);
}

static void
back_button_clicked(button_event_t* event)
{
  gui_pop_screen();
}
