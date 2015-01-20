
#include "gui/recovery.h"
#include "button.h"
#include "label.h"
#include "listbox.h"
#include "gui.h"
#include "gfx.h"
#include "net.h"
#include "bootloader_api.h"
#include "touch.h"
#include "touch_calib.h"
#include "bootloader_api.h"

#include <string.h>
#include <stdio.h>


typedef enum {
  RO_RESET_TOUCH_CALIB,
  RO_RESTORE_FIRMWARE,
  NUM_RECOVERY_OPS
} recovery_op_t;

typedef struct {
  widget_t* widget;
  widget_t* restore_touch_calib;
  widget_t* restore_firmware;

  bool shown;
  bool down;
  systime_t down_time;
  recovery_op_t op_sel;
} recovery_screen_t;



static void
recovery_screen_destroy(widget_t* w);

static void
recovery_screen_msg(msg_event_t* event);

static void
back_button_clicked(button_event_t* event);

static void
dispatch_touch_input(recovery_screen_t* s, touch_msg_t* touch);

static void
update_labels(recovery_screen_t* s);

static void
trigger_op(recovery_screen_t* s);


static const widget_class_t recovery_screen_widget_class = {
    .on_destroy = recovery_screen_destroy,
    .on_msg = recovery_screen_msg
};


void
recovery_screen_create()
{
  recovery_screen_t* s = calloc(1, sizeof(recovery_screen_t));

  s->widget = widget_create(NULL, &recovery_screen_widget_class, s, display_rect);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  widget_t* back_btn = button_create(s->widget, rect, img_left, WHITE, BLACK, back_button_clicked);
  button_set_up_bg_color(back_btn, BLACK);
  button_set_up_icon_color(back_btn, WHITE);
  button_set_down_bg_color(back_btn, BLACK);
  button_set_down_icon_color(back_btn, LIGHT_GRAY);
  button_set_disabled_bg_color(back_btn, BLACK);
  button_set_disabled_icon_color(back_btn, DARK_GRAY);

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  label_create(s->widget, rect, "Recovery Mode", font_opensans_regular_22, WHITE, 1);

  rect.x = 20;
  rect.y = 80;
  rect.width = 300;
  rect.height = 30;
  label_create(s->widget, rect, "Tap the screen to select an option, then touch and hold for 5 seconds to execute it.", font_opensans_regular_18, WHITE, 3);

  rect.y += 80;
  s->restore_touch_calib = label_create(s->widget, rect, "", font_opensans_regular_18, WHITE, 1);

  rect.y += 30;
  s->restore_firmware = label_create(s->widget, rect, "", font_opensans_regular_18, WHITE, 1);

  update_labels(s);

  gui_msg_subscribe(MSG_TOUCH_INPUT, s->widget);
}

static void
recovery_screen_destroy(widget_t* w)
{
  recovery_screen_t* s = widget_get_instance_data(w);

  gui_msg_unsubscribe(MSG_TOUCH_INPUT, s->widget);

  free(s);
}

static void
update_labels(recovery_screen_t* s)
{
  switch (s->op_sel) {
    case RO_RESET_TOUCH_CALIB:
      label_set_text(s->restore_touch_calib, "-> Reset touch calibration");
      label_set_text(s->restore_firmware,    "   Restore factory firmware");
      break;

    case RO_RESTORE_FIRMWARE:
      label_set_text(s->restore_touch_calib, "   Reset touch calibration");
      label_set_text(s->restore_firmware,    "-> Restore factory firmware");
      break;

    default:
      break;
  }
}

static void
recovery_screen_msg(msg_event_t* event)
{
  recovery_screen_t* s = widget_get_instance_data(event->widget);

  /*TODO: Instead of just returning think about using widget_destroy to remove from memory */
  if (chTimeNow() > S2ST(60))
    return;

  switch (event->msg_id) {
    case MSG_TOUCH_INPUT:
      dispatch_touch_input(s, event->msg_data);
      break;

    default:
      break;
  }
}

static void
dispatch_touch_input(recovery_screen_t* s, touch_msg_t* touch)
{
  systime_t down_duration = chTimeNow() - s->down_time;

  if (touch->touch_down) {
    if (!s->down) {
      s->down_time = chTimeNow();
      s->down = true;
    }
    else if (!s->shown) {
      if (down_duration > S2ST(30)) {
        s->shown = true;
        s->down_time = chTimeNow();
        gui_push_screen(s->widget);
      }
    }
    else {
      if (down_duration > S2ST(5)) {
        s->down_time = chTimeNow();
        trigger_op(s);
      }
    }
  }
  else {
    s->down = false;
    if (s->shown) {
      if (down_duration < S2ST(5)) {
        s->op_sel++;
        if (s->op_sel >= NUM_RECOVERY_OPS)
          s->op_sel = RO_RESET_TOUCH_CALIB;
        update_labels(s);
      }
    }
  }
}

static void
trigger_op(recovery_screen_t* s)
{
  switch (s->op_sel) {
    case RO_RESET_TOUCH_CALIB:
      touch_calib_reset();
      break;

    case RO_RESTORE_FIRMWARE:
      bootloader_load_recovery_img();
      break;

    default:
      break;
  }

  s->shown = false;
  gui_hide_screen();
}

static void
back_button_clicked(button_event_t* event)
{
  widget_t* w = widget_get_parent(event->widget);
  recovery_screen_t* s = widget_get_instance_data(w);

  if (event->id == EVT_BUTTON_CLICK) {
    s->shown = false;
    gui_hide_screen();
  }
}
