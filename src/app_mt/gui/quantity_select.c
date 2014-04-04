
#include "gui/quantity_select.h"
#include "gfx.h"
#include "button.h"
#include "label.h"
#include "gui.h"
#include "quantity_widget.h"
#include "app_cfg.h"

#include <string.h>

#define QUANTITY_STEPS_PER_VELOCITY 15
#define MAX_TEMP_C (110)
#define MIN_TEMP_C (-10)

#define MAX_TEMP_F (212)
#define MIN_TEMP_F (-10)


typedef struct {
  widget_t* widget;
  widget_t* back_button;
  widget_t* up_button;
  widget_t* down_button;
  widget_t* quantity_widget;

  quantity_t quantity;

  quantity_select_cb_t cb;
  void* cb_data;

  float velocity_steps[4];
  uint8_t num_velocity_steps;

  uint8_t cur_velocity;
  uint8_t quantity_steps;
} quantity_select_screen_t;


static void quantity_select_screen_destroy(widget_t* w);

static void back_button_clicked(button_event_t* event);
static void adjust_button_evt(button_event_t* event);
static void adjust_quantity_velocity(quantity_select_screen_t* s);
static void set_quantity(quantity_select_screen_t* s, quantity_t quantity);


static widget_class_t quantity_select_widget_class = {
    .on_destroy = quantity_select_screen_destroy
};

widget_t*
quantity_select_screen_create(
    const char* title,
    quantity_t quantity,
    float* velocity_steps,
    uint8_t num_velocity_steps,
    quantity_select_cb_t cb,
    void* cb_data)
{
  quantity_select_screen_t* s = chHeapAlloc(NULL, sizeof(quantity_select_screen_t));
  memset(s, 0, sizeof(quantity_select_screen_t));

  s->widget = widget_create(NULL, &quantity_select_widget_class, s, display_rect);

  s->cb = cb;
  s->cb_data = cb_data;

  s->num_velocity_steps = MIN(num_velocity_steps, 4);
  memcpy(s->velocity_steps, velocity_steps, s->num_velocity_steps * sizeof(float));

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
  label_create(s->widget, rect, title, font_opensans_regular_22, WHITE, 1);

  rect.x = 10;
  rect.y = 95;
  rect.width = 56;
  rect.height = 56;
  s->up_button = button_create(s->widget, rect, img_up, WHITE, RED, adjust_button_evt);

  rect.y = 165;
  s->down_button = button_create(s->widget, rect, img_down, WHITE, CYAN, adjust_button_evt);

  rect.x = 66;
  rect.y = 130;
  rect.width = 254;
  s->quantity_widget = quantity_widget_create(s->widget, rect, app_cfg_get_temp_unit());

  set_quantity(s, quantity);

  s->cur_velocity = 0;
  s->quantity_steps = QUANTITY_STEPS_PER_VELOCITY;

  return s->widget;
}

static void
quantity_select_screen_destroy(widget_t* w)
{
  quantity_select_screen_t* s = widget_get_instance_data(w);
  chHeapFree(s);
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* w = widget_get_parent(event->widget);
    quantity_select_screen_t* s = widget_get_instance_data(w);

    if (s->cb)
      s->cb(s->quantity, s->cb_data);

    gui_pop_screen();
  }
}

static void
adjust_button_evt(button_event_t* event)
{
  if (event->id == EVT_BUTTON_DOWN ||
      event->id == EVT_BUTTON_REPEAT) {
    widget_t* screen = widget_get_parent(event->widget);
    quantity_select_screen_t* s = widget_get_instance_data(screen);

    float adjustment;
    if (event->widget == s->up_button)
      adjustment = s->velocity_steps[s->cur_velocity];
    else
      adjustment = -s->velocity_steps[s->cur_velocity];

    quantity_t new_sp = {
        .unit = s->quantity.unit,
        .value = s->quantity.value + adjustment
    };
    set_quantity(s, new_sp);
    adjust_quantity_velocity(s);
  }
  else if (event->id == EVT_BUTTON_UP) {
    widget_t* screen = widget_get_parent(event->widget);
    quantity_select_screen_t* s = widget_get_instance_data(screen);
    s->cur_velocity = 0;
    s->quantity_steps = QUANTITY_STEPS_PER_VELOCITY;
  }
}

static void
adjust_quantity_velocity(quantity_select_screen_t* s)
{
  if ((s->cur_velocity < (s->num_velocity_steps-1)) &&
      (--s->quantity_steps <= 0)) {
    s->cur_velocity++;
    s->quantity_steps = QUANTITY_STEPS_PER_VELOCITY;
  }
}

static void
set_quantity(quantity_select_screen_t* s, quantity_t quantity)
{
  switch (quantity.unit) {
  case UNIT_TEMP_DEG_F:
    quantity.value = LIMIT(quantity.value, MIN_TEMP_F, MAX_TEMP_F);
    break;

  case UNIT_TEMP_DEG_C:
    quantity.value = LIMIT(quantity.value, MIN_TEMP_C, MAX_TEMP_C);
    break;

  case UNIT_TIME_MIN:
    quantity.value = LIMIT(quantity.value, 0, 30);
    break;

  default:
    break;
  }

  s->quantity = quantity;
  quantity_widget_set_value(s->quantity_widget, quantity);
}
