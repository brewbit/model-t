
#include "ch.h"
#include "hal.h"
#include "gui_wifi.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui/icon.h"
#include "gfx.h"
#include "gui.h"

#include <string.h>


typedef struct {
  widget_t* widget;
  widget_t* nwk_status1;
  widget_t* nwk_status2;
} wifi_screen_t;


static void wifi_screen_destroy(widget_t* w);
static void wifi_screen_msg(msg_event_t* event);
static void back_button_clicked(button_event_t* event);

//static void dispatch_wifi_status(wifi_screen_t* s, wspr_wifi_status_t* status);


static widget_class_t wifi_screen_widget_class = {
    .on_destroy = wifi_screen_destroy,
    .on_msg     = wifi_screen_msg
};


widget_t*
wifi_screen_create()
{
  wifi_screen_t* s = chHeapAlloc(NULL, sizeof(wifi_screen_t));
  memset(s, 0, sizeof(wifi_screen_t));

  s->widget = widget_create(NULL, &wifi_screen_widget_class, s, display_rect);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  button_create(s->widget, rect, img_left, BLACK, back_button_clicked);

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  label_create(s->widget, rect, "WiFi Config", font_opensans_regular_22, WHITE, 1);

  rect.x = 10;
  rect.y = 95;
  rect.width = 300;
  rect.height = 66;
  widget_t* nwk_button = button_create(s->widget, rect, NULL, BLACK, NULL);

  rect.x = 5;
  rect.y = 5;
  rect.width = 56;
  rect.height = 56;
  icon_create(nwk_button, rect, img_signal, CYAN);

  rect.x = 70;
  rect.y = 5;
  rect.width = 200;
  label_create(nwk_button, rect, "Network", font_opensans_regular_16, WHITE, 1);

  rect.y = 30;
  s->nwk_status1 = label_create(nwk_button, rect, "Checking network status...", font_opensans_regular_8, WHITE, 1);
  rect.y += font_opensans_regular_8->line_height;
  s->nwk_status2 = label_create(nwk_button, rect, "", font_opensans_regular_8, WHITE, 1);

  gui_msg_subscribe(MSG_WIFI_STATUS, s->widget);
  gui_msg_subscribe(MSG_SCAN_RESULT, s->widget);
  gui_msg_subscribe(MSG_SCAN_COMPLETE, s->widget);

  return s->widget;
}

static void
wifi_screen_destroy(widget_t* w)
{
  wifi_screen_t* s = widget_get_instance_data(w);

  gui_msg_unsubscribe(MSG_WIFI_STATUS, w);
  gui_msg_unsubscribe(MSG_SCAN_RESULT, w);
  gui_msg_unsubscribe(MSG_SCAN_COMPLETE, w);

  chHeapFree(s);
}

static void
wifi_screen_msg(msg_event_t* event)
{
  wifi_screen_t* s = widget_get_instance_data(event->widget);

  switch (event->msg_id) {
  case MSG_WIFI_STATUS:
//    dispatch_wifi_status(s, event->msg_data);
    break;

  default:
    break;
  }
}

//static void
//dispatch_wifi_status(wifi_screen_t* s, wspr_wifi_status_t* status)
//{
//  if (!status->configured) {
//    label_set_text(s->nwk_status1, "No network configured.");
//    label_set_text(s->nwk_status2, "Connect to AP to configure.");
//  }
//  else {
//    switch (status->state) {
//    case 0:
//      label_set_text(s->nwk_status1, "You are currently connected to:");
//      label_set_text(s->nwk_status2, status->ssid);
//      break;
//
//    case -30:
//      label_set_text(s->nwk_status1, "No networks found");
//      label_set_text(s->nwk_status2, "");
//      break;
//
//    default:
//      label_set_text(s->nwk_status1, "Unknown network status");
//      label_set_text(s->nwk_status2, "");
//      break;
//    }
//  }
//}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK)
    gui_pop_screen();
}
