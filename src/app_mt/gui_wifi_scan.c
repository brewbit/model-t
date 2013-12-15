
#include "ch.h"
#include "hal.h"
#include "gui_wifi_scan.h"
#include "gui/button.h"
#include "gui/label.h"
#include "gui/icon.h"
#include "gui/listbox.h"
#include "gfx.h"
#include "gui.h"
#include "net.h"

#include <string.h>


typedef struct {
  widget_t* widget;
  widget_t* nwk_status1;
  widget_t* nwk_status2;
} wifi_scan_screen_t;


static void wifi_scan_screen_destroy(widget_t* w);
static void wifi_scan_screen_msg(msg_event_t* event);
static void back_button_clicked(button_event_t* event);

static void dispatch_net_status(wifi_scan_screen_t* s, const net_status_t* status);


static widget_class_t wifi_scan_screen_widget_class = {
    .on_destroy = wifi_scan_screen_destroy,
    .on_msg     = wifi_scan_screen_msg
};


widget_t*
wifi_scan_screen_create()
{
  wifi_scan_screen_t* s = calloc(1, sizeof(wifi_scan_screen_t));

  s->widget = widget_create(NULL, &wifi_scan_screen_widget_class, s, display_rect);

  rect_t rect = {
      .x = 15,
      .y = 15,
      .width = 56,
      .height = 56,
  };
  button_create(s->widget, rect, img_left, WHITE, BLACK, back_button_clicked);

  rect.x = 85;
  rect.y = 26;
  rect.width = 220;
  label_create(s->widget, rect, "Network Scan", font_opensans_regular_22, WHITE, 1);

  rect.x = 5;
  rect.y = 70;
  rect.width = 300;
  rect.height = 150;
  widget_t* lb = listbox_create(s->widget, rect, 30);
  listbox_add_row(lb, label_create(NULL, rect, "row 1", font_opensans_regular_22, WHITE, 1));
  listbox_add_row(lb, label_create(NULL, rect, "row 2", font_opensans_regular_22, WHITE, 1));
  listbox_add_row(lb, label_create(NULL, rect, "row 3", font_opensans_regular_22, WHITE, 1));
  listbox_add_row(lb, label_create(NULL, rect, "row 4", font_opensans_regular_22, WHITE, 1));
  listbox_add_row(lb, label_create(NULL, rect, "row 5", font_opensans_regular_22, WHITE, 1));
  listbox_add_row(lb, label_create(NULL, rect, "row 6", font_opensans_regular_22, WHITE, 1));

  gui_msg_subscribe(MSG_NET_SCAN_RESULT, s->widget);
  gui_msg_subscribe(MSG_NET_SCAN_COMPLETE, s->widget);

  return s->widget;
}

static void
wifi_scan_screen_destroy(widget_t* w)
{
  wifi_scan_screen_t* s = widget_get_instance_data(w);

  gui_msg_unsubscribe(MSG_NET_SCAN_RESULT, w);
  gui_msg_unsubscribe(MSG_NET_SCAN_COMPLETE, w);

  free(s);
}

static void
wifi_scan_screen_msg(msg_event_t* event)
{
  wifi_scan_screen_t* s = widget_get_instance_data(event->widget);

  switch (event->msg_id) {
  case MSG_NET_STATUS:
    dispatch_net_status(s, event->msg_data);
    break;

  default:
    break;
  }
}

static void
dispatch_net_status(wifi_scan_screen_t* s, const net_status_t* status)
{
  switch (status->net_state) {
  case NET_DISCONNECTED:
    label_set_text(s->nwk_status1, "Not connected");
    label_set_text(s->nwk_status2, "Touch to scan for networks");
    break;

  case NET_SCANNING:
    label_set_text(s->nwk_status1, "Scanning for networks...");
    label_set_text(s->nwk_status2, "");
    break;

  case NET_CONNECTING:
    label_set_text(s->nwk_status1, "Connecting to:");
    label_set_text(s->nwk_status2, status->ssid);
    break;

  case NET_CONNECTED:
    label_set_text(s->nwk_status1, "Connected to:");
    label_set_text(s->nwk_status2, status->ssid);
    break;
  }
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK)
    gui_pop_screen();
}

