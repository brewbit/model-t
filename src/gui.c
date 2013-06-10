
#include "ch.h"
#include "gui.h"
#include "touch.h"


typedef enum {
  GUI_TOUCH,
  GUI_PUSH_SCREEN,
  GUI_POP_SCREEN,
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
} gui_push_screen_event_t;

typedef struct widget_stack_elem_s {
  widget_t* widget;
  struct widget_stack_elem_s* next;
} widget_stack_elem_t;


static msg_t gui_thread_func(void* arg);
static void handle_touch(bool touch_down, point_t raw, point_t calib, void* user_data);
static void gui_send_event(gui_event_t* event);

static void dispatch_touch(gui_touch_event_t* event);
static void dispatch_push_screen(gui_push_screen_event_t* event);
static void dispatch_pop_screen(void);
static void gui_dispatch(gui_event_t* event);
static void gui_idle(void);


static uint8_t wa_gui_thread[1024];
static Thread* gui_thread;
static widget_t* touch_capture_widget;
static widget_stack_elem_t* screen_stack = NULL;


void
gui_init(widget_t* root_screen)
{
  gui_thread = chThdCreateStatic(wa_gui_thread, sizeof(wa_gui_thread), NORMALPRIO, gui_thread_func, root_screen);

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
gui_push_screen(widget_t* screen)
{
  gui_push_screen_event_t event = {
      .id = GUI_PUSH_SCREEN,
      .screen = screen,
  };
  gui_send_event((gui_event_t*)&event);
}

void
gui_pop_screen()
{
  gui_event_t event = {
      .id = GUI_POP_SCREEN
  };
  gui_send_event(&event);
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
  if (chThdSelf() == gui_thread)
    gui_dispatch(event);
  else
    chMsgSend(gui_thread, (msg_t)event);
}

static msg_t
gui_thread_func(void* arg)
{
  gui_push_screen(arg);

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
  widget_paint(screen_stack->widget);

  chThdYield();
}

static void
gui_dispatch(gui_event_t* event)
{
  switch(event->id) {
  case GUI_PAINT:
    widget_paint(screen_stack->widget);
    break;

  case GUI_TOUCH:
    dispatch_touch((gui_touch_event_t*)event);
    break;

  case GUI_PUSH_SCREEN:
    dispatch_push_screen((gui_push_screen_event_t*)event);
    break;

  case GUI_POP_SCREEN:
    dispatch_pop_screen();
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
    dest_widget = widget_hit_test(screen_stack->widget, event->pos);
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
dispatch_push_screen(gui_push_screen_event_t* event)
{
  widget_stack_elem_t* stack_elem = malloc(sizeof(widget_stack_elem_t));
  stack_elem->widget = event->screen;
  stack_elem->next = screen_stack;
  screen_stack = stack_elem;

  widget_invalidate(screen_stack->widget);
}

static void
dispatch_pop_screen()
{
  if ((screen_stack != NULL) &&
      (screen_stack->next != NULL)) {
    widget_destroy(screen_stack->widget);

    screen_stack = screen_stack->next;

    widget_invalidate(screen_stack->widget);
  }
}
