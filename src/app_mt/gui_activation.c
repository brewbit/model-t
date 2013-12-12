
#include "ch.h"
#include "hal.h"
#include "gui_activation.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui/icon.h"
#include "gfx.h"
#include "gui.h"

#include <string.h>


typedef struct {
  widget_t* widget;
  widget_t* activation_code;
} activation_screen_t;


static void activation_screen_destroy(widget_t* w);
static void activation_screen_msg(msg_event_t* event);
static void back_button_clicked(button_event_t* event);

static widget_class_t activation_screen_widget_class = {
    .on_destroy = activation_screen_destroy,
    .on_msg     = activation_screen_msg
};


widget_t*
activation_screen_create()
{
  activation_screen_t* screen = chHeapAlloc(NULL, sizeof(activation_screen_t));
  memset(screen, 0, sizeof(activation_screen_t));

  screen->widget = widget_create(NULL, &activation_screen_widget_class, screen, display_rect);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  button_create(screen->widget, rect, img_left, WHITE, BLACK, back_button_clicked);

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  label_create(screen->widget, rect, "Activation Code", font_opensans_regular_22, WHITE, 1);

  rect.x = 40;
  rect.y = 100;
  rect.width = 220;
  label_create(screen->widget, rect, "Please enter the following code on BrewBit.com", font_opensans_regular_18, WHITE, 1);

  rect.x = 100;
  rect.y = 150;
  rect.width = 220;
  label_create(screen->widget, rect, "3A19F7", font_opensans_regular_22, ORANGE, 1);

  gui_msg_subscribe(MSG_SHOW_ACTIVATION_TOKEN, screen->widget);

  return screen->widget;
}

static void
activation_screen_destroy(widget_t* w)
{
  activation_screen_t* screen = widget_get_instance_data(w);

  gui_msg_unsubscribe(MSG_SHOW_ACTIVATION_TOKEN, w);

  chHeapFree(screen);
}

static void
activation_screen_msg(msg_event_t* event)
{
  activation_screen_t* screen = widget_get_instance_data(event->widget);

  switch (event->msg_id) {
  case MSG_SHOW_ACTIVATION_TOKEN:
    break;

  default:
    break;
  }
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK)
    gui_pop_screen();
}
