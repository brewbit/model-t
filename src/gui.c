
#include "ch.h"
#include "gui.h"


static msg_t
gui_thread_func(void* arg);

static uint8_t wa_gui_thread[1024];
static Thread* gui_thread;

void
gui_init(screen_t* main_screen)
{
  gui_thread = chThdCreateStatic(wa_gui_thread, sizeof(wa_gui_thread), NORMALPRIO, gui_thread_func, main_screen);
}

void
gui_set_screen(screen_t* screen)
{
  gui_set_screen_msg_t msg = {
      .id = GUI_SET_SCREEN,
      .screen = screen,
  };
  gui_send_msg((gui_msg_t*)&msg);
}

void
gui_touch_down(point_t* pos, point_t* raw_pos)
{
  gui_touch_msg_t msg = {
      .id = GUI_TOUCH_DOWN,
      .x = pos->x,
      .y = pos->y,
      .raw_x = raw_pos->x,
      .raw_y = raw_pos->y,
  };
  gui_send_msg((gui_msg_t*)&msg);
}

void
gui_touch_up(point_t* pos, point_t* raw_pos)
{
  gui_touch_msg_t msg = {
      .id = GUI_TOUCH_UP,
      .x = pos->x,
      .y = pos->y,
      .raw_x = raw_pos->x,
      .raw_y = raw_pos->y,
  };
  gui_send_msg((gui_msg_t*)&msg);
}

void
gui_paint()
{
  gui_any_msg_t msg = {
      .id = GUI_PAINT
  };
  gui_send_msg((gui_msg_t*)&msg);
}

void
gui_send_msg(gui_msg_t* msg)
{
  chMsgSend(gui_thread, (msg_t)msg);
}

static msg_t
gui_thread_func(void* arg)
{
  screen_t* screen = arg;

  while (1) {
    Thread* tp = chMsgWait();
    gui_msg_t* msg = (gui_msg_t*)chMsgGet(tp);

    switch(msg->u.any.id) {
      case GUI_PAINT:
        if (screen->on_paint)
          screen->on_paint();
        break;

      case GUI_TOUCH_DOWN:
        if (screen->on_raw_touch_down)
          screen->on_raw_touch_down(msg->u.touch.raw_x, msg->u.touch.raw_y);

        if (screen->on_touch_down)
          screen->on_touch_down(msg->u.touch.x, msg->u.touch.y);
        break;

      case GUI_TOUCH_UP:
        if (screen->on_raw_touch_up)
          screen->on_raw_touch_up(msg->u.touch.raw_x, msg->u.touch.raw_y);

        if (screen->on_touch_up)
          screen->on_touch_up(msg->u.touch.x, msg->u.touch.y);
        break;

      case GUI_SET_SCREEN:
        screen = msg->u.set_screen.screen;
        break;

      default:
        if (screen->on_msg)
          screen->on_msg(msg);
        break;
    }

    chMsgRelease(tp, 0);
  }
  return 0;
}
