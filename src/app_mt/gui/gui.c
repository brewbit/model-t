
#include "ch.h"
#include "gui.h"
#include "touch.h"
#include "message.h"
#include "screen_saver.h"


typedef struct widget_stack_elem_s {
  widget_t* widget;
  struct widget_stack_elem_s* next;
} widget_stack_elem_t;


static void dispatch_touch(touch_msg_t* event);
static void dispatch_push_screen(widget_t* screen);
static void dispatch_pop_screen(bool destroy);
static void gui_dispatch(msg_id_t id, void* msg_data, void* listener_data, void* sub_data);
static void dispatch_msg_to_widget(widget_t* w, msg_id_t id, void* msg_data);


static msg_listener_t* gui_msg_listener;
static widget_t* touch_capture_widget;
static widget_stack_elem_t* screen_stack = NULL;
static systime_t last_paint_time;


void
gui_init()
{
  gui_msg_listener = msg_listener_create("gui", 2048, gui_dispatch, NULL);
  msg_listener_set_idle_timeout(gui_msg_listener, 100);
  msg_listener_enable_watchdog(gui_msg_listener, 5000);

  msg_subscribe(gui_msg_listener, MSG_TOUCH_INPUT, NULL);
  msg_subscribe(gui_msg_listener, MSG_GUI_PUSH_SCREEN, NULL);
  msg_subscribe(gui_msg_listener, MSG_GUI_POP_SCREEN, NULL);
  msg_subscribe(gui_msg_listener, MSG_GUI_HIDE_SCREEN, NULL);
}

void
gui_push_screen(widget_t* screen)
{
  msg_send(MSG_GUI_PUSH_SCREEN, screen);
}

void
gui_pop_screen()
{
  msg_send(MSG_GUI_POP_SCREEN, NULL);
}

void
gui_hide_screen()
{
  msg_send(MSG_GUI_HIDE_SCREEN, NULL);
}

void
gui_acquire_touch_capture(widget_t* w)
{
  if (widget_is_enabled(w) &&
      widget_is_visible(w))
    touch_capture_widget = w;
}

void
gui_release_touch_capture(void)
{
  touch_capture_widget = NULL;
}

void
gui_msg_subscribe(msg_id_t id, widget_t* w)
{
  if (w == NULL)
    return;

  msg_subscribe(gui_msg_listener, id, w);
}

void
gui_msg_unsubscribe(msg_id_t id, widget_t* w)
{
  if (w == NULL)
    return;

  msg_unsubscribe(gui_msg_listener, id, w);
}

static void
gui_dispatch(msg_id_t id, void* msg_data, void* listener_data, void* sub_data)
{
  (void)listener_data;

  if (sub_data != NULL) {
    dispatch_msg_to_widget(sub_data, id, msg_data);
  }
  else {
    switch(id) {
    case MSG_GUI_PUSH_SCREEN:
      dispatch_push_screen(msg_data);
      break;

    case MSG_GUI_POP_SCREEN:
      dispatch_pop_screen(true);
      break;

    case MSG_GUI_HIDE_SCREEN:
      dispatch_pop_screen(false);
      break;

    case MSG_TOUCH_INPUT:
      if (!screen_saver_is_active())
    	dispatch_touch(msg_data);
      break;

    default:
      break;
    }
  }

  if ((chTimeNow() - last_paint_time) >= MS2ST(100)) {
    if (screen_stack != NULL) {
      widget_paint(screen_stack->widget);
    }
    last_paint_time = chTimeNow();
  }
}

static void
dispatch_touch(touch_msg_t* touch)
{
  widget_t* dest_widget = NULL;

  if ((touch_capture_widget != NULL) &&
      (!widget_is_enabled(touch_capture_widget) ||
       !widget_is_visible(touch_capture_widget)))
    gui_release_touch_capture();

  if (touch_capture_widget != NULL)
    dest_widget = touch_capture_widget;
  else if (screen_stack != NULL) {
    dest_widget = widget_hit_test(screen_stack->widget, touch->calib);
  }

  if (dest_widget != NULL) {
    point_t pos = widget_rel_pos(dest_widget, touch->calib);
    touch_event_t te = {
        .id = touch->touch_down ? EVT_TOUCH_DOWN : EVT_TOUCH_UP,
        .widget = dest_widget,
        .pos = pos,
    };
    widget_dispatch_event(dest_widget, (event_t*)&te);
  }
}

static void
dispatch_push_screen(widget_t* screen)
{
  widget_stack_elem_t* stack_elem = calloc(1, sizeof(widget_stack_elem_t));
  stack_elem->widget = screen;
  stack_elem->next = screen_stack;
  screen_stack = stack_elem;

  widget_invalidate(screen_stack->widget);
}

static void
dispatch_pop_screen(bool destroy)
{
  if ((screen_stack != NULL) &&
      (screen_stack->next != NULL)) {
    if (destroy)
      widget_destroy(screen_stack->widget);

    screen_stack = screen_stack->next;

    widget_invalidate(screen_stack->widget);
  }
}

static void
dispatch_msg_to_widget(widget_t* w, msg_id_t id, void* msg_data)
{
  msg_event_t event = {
      .id = EVT_MSG,
      .widget = w,
      .msg_id = id,
      .msg_data = msg_data
  };
  widget_dispatch_event(w, (event_t*)&event);
}
