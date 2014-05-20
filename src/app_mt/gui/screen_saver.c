
#include "gui/screen_saver.h"
#include "button.h"
#include "label.h"
#include "listbox.h"
#include "gui.h"
#include "touch.h"
#include "app_cfg.h"
#include "lcd.h"


#include <string.h>
#include <stdio.h>


typedef struct {
  widget_t* screen;
  systime_t last_touch_time;
  bool active;
} screen_saver_t;


static void screen_saver_destroy(widget_t* w);
static void screen_saver_msg(msg_event_t* event);
static void dispatch_touch_input(screen_saver_t* s);
static msg_t screen_saver_thread(void* arg);


static const widget_class_t screen_saver_widget_class = {
    .on_destroy = screen_saver_destroy,
    .on_msg = screen_saver_msg
};


void
screen_saver_create()
{
  screen_saver_t* s = calloc(1, sizeof(screen_saver_t));

  s->screen = widget_create(NULL, &screen_saver_widget_class, s, display_rect);

  chThdCreateFromHeap(NULL, 1024, NORMALPRIO, screen_saver_thread, s);

  gui_msg_subscribe(MSG_TOUCH_INPUT, s->screen);
}

static void
screen_saver_destroy(widget_t* w)
{
  screen_saver_t* s = widget_get_instance_data(w);

  gui_msg_unsubscribe(MSG_TOUCH_INPUT, w);

  free(s);
}

static msg_t
screen_saver_thread(void* arg)
{
  (void)arg;

  chRegSetThreadName("screen_saver");

  while (1) {
    screen_saver_t* s = arg;
    quantity_t screen_saver_timeout = app_cfg_get_screen_saver();

    if (!s->active &&
        screen_saver_timeout.value > .5f &&
        (chTimeNow() - s->last_touch_time) > S2ST(60 * screen_saver_timeout.value)) {
      s->active = true;
      lcd_set_brightness(0);
      gui_push_screen(s->screen);
    }
    chThdSleepSeconds(10);
  }

  return 0;
}

static void
screen_saver_msg(msg_event_t* event)
{
  screen_saver_t* s = widget_get_instance_data(event->widget);

  switch (event->msg_id) {
    case MSG_TOUCH_INPUT:
      dispatch_touch_input(s);
      break;

    default:
      break;
  }
}

static void
dispatch_touch_input(screen_saver_t* s)
{
  s->last_touch_time = chTimeNow();
  if (s->active) {
    gui_hide_screen();
    lcd_set_brightness(100);
    s->active = false;
  }
}
