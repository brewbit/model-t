
#include "ch.h"
#include "gui.h"
#include "touch.h"


typedef enum {
  GUI_TOUCH,
  GUI_SET_SCREEN,
  GUI_PAINT,
} gui_event_id_t;

typedef struct {
  gui_event_id_t id;
} gui_event_t;

typedef struct {
  gui_event_id_t id;
  bool touch_down;
  point_t pos;
} gui_touch_event_t;

typedef struct {
  gui_event_id_t id;
  widget_t* screen;
} gui_set_screen_event_t;


static msg_t gui_thread_func(void* arg);
static void handle_touch(bool touch_down, point_t raw, point_t calib, void* user_data);
static void gui_send_event(gui_event_t* event);

static void dispatch_touch(gui_touch_event_t* event);
static void dispatch_set_screen(gui_set_screen_event_t* event);
static void gui_dispatch(gui_event_t* event);
static void gui_idle(void);


static uint8_t wa_gui_thread[1024];
static Thread* gui_thread;
static widget_t* screen;
static widget_t* touch_capture_widget;


void
gui_init()
{
  gui_thread = chThdCreateStatic(wa_gui_thread, sizeof(wa_gui_thread), NORMALPRIO, gui_thread_func, NULL);

  touch_handler_register(handle_touch, NULL);
}

static void
handle_touch(bool touch_down, point_t raw, point_t calib, void* user_data)
{
  (void)raw;
  (void)user_data;

  gui_touch_event_t event = {
      .id = GUI_TOUCH,
      .touch_down = touch_down,
      .pos = calib,
  };
  gui_send_event((gui_event_t*)&event);
}

void
gui_set_screen(widget_t* screen)
{
  gui_set_screen_event_t event = {
      .id = GUI_SET_SCREEN,
      .screen = screen,
  };
  gui_send_event((gui_event_t*)&event);
}

void
gui_paint()
{
  gui_event_t event = {
      .id = GUI_PAINT
  };
  gui_send_event(&event);
}

void
gui_acquire_touch_capture(widget_t* widget)
{
  touch_capture_widget = widget;
}

void
gui_release_touch_capture(void)
{
  touch_capture_widget = NULL;
}

static void
gui_send_event(gui_event_t* event)
{
  chMsgSend(gui_thread, (msg_t)event);
}

static msg_t
gui_thread_func(void* arg)
{
  (void)arg;

  while (1) {
    if (chMsgIsPendingI(gui_thread)) {
      Thread* tp = chMsgWait();
      gui_event_t* event = (gui_event_t*)chMsgGet(tp);

      gui_dispatch(event);

      chMsgRelease(tp, 0);
    }
    else {
      gui_idle();
    }
  }
  return 0;
}

static void
gui_idle()
{
  widget_paint(screen);

  chThdYield();
}

static void
gui_dispatch(gui_event_t* event)
{
  switch(event->id) {
  case GUI_PAINT:
    widget_paint(screen);
    break;

  case GUI_TOUCH:
    dispatch_touch((gui_touch_event_t*)event);
    break;

  case GUI_SET_SCREEN:
    dispatch_set_screen((gui_set_screen_event_t*)event);
    break;

  default:
    break;
  }
}

static void
dispatch_touch(gui_touch_event_t* event)
{
  widget_t* dest_widget;
  if (touch_capture_widget != NULL)
    dest_widget = touch_capture_widget;
  else {
    dest_widget = widget_hit_test(screen, event->pos);
  }

  if (dest_widget != NULL) {
    touch_event_t te = {
        .id = event->touch_down ? EVT_TOUCH_DOWN : EVT_TOUCH_UP,
        .widget = dest_widget,
        .pos = event->pos,
    };
    widget_dispatch_event(dest_widget, (event_t*)&te);
  }
}

static void
dispatch_set_screen(gui_set_screen_event_t* event)
{
  screen = event->screen;
  widget_paint((widget_t*)screen);
}
