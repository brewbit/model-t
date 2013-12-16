
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
#include <stdio.h>


typedef struct {
  widget_t* widget;
  widget_t* scan_button;
  widget_t* net_list;
} wifi_scan_screen_t;


static void wifi_scan_screen_destroy(widget_t* w);
static void wifi_scan_screen_msg(msg_event_t* event);
static void back_button_clicked(button_event_t* event);
static void scan_button_clicked(button_event_t* event);

static void
dispatch_new_network(wifi_scan_screen_t* s, const network_t* network);

static void
dispatch_network_update(wifi_scan_screen_t* s, const network_t* network);

static void
dispatch_network_timeout(wifi_scan_screen_t* s, const network_t* network);


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
  rect.x = DISP_WIDTH - 56 - 15;
  s->scan_button = button_create(s->widget, rect, img_update, WHITE, BLACK, scan_button_clicked);

  rect.x = 85;
  rect.y = 26;
  rect.width = 160;
  label_create(s->widget, rect, "Network Scan", font_opensans_regular_22, WHITE, 1);

  rect.x = 5;
  rect.y = 70;
  rect.width = 300;
  rect.height = 150;
  s->net_list = listbox_create(s->widget, rect, 30);

  gui_msg_subscribe(MSG_NET_NEW_NETWORK, s->widget);
  gui_msg_subscribe(MSG_NET_NETWORK_UPDATED, s->widget);
  gui_msg_subscribe(MSG_NET_NETWORK_TIMEOUT, s->widget);

  return s->widget;
}

static void
wifi_scan_screen_destroy(widget_t* w)
{
  wifi_scan_screen_t* s = widget_get_instance_data(w);

  gui_msg_unsubscribe(MSG_NET_NEW_NETWORK, w);
  gui_msg_unsubscribe(MSG_NET_NETWORK_UPDATED, w);
  gui_msg_unsubscribe(MSG_NET_NETWORK_TIMEOUT, w);

  free(s);
}

static void
wifi_scan_screen_msg(msg_event_t* event)
{
  wifi_scan_screen_t* s = widget_get_instance_data(event->widget);

  switch (event->msg_id) {
  case MSG_NET_NEW_NETWORK:
    dispatch_new_network(s, event->msg_data);
    break;

  case MSG_NET_NETWORK_UPDATED:
    dispatch_network_update(s, event->msg_data);
    break;

  case MSG_NET_NETWORK_TIMEOUT:
    dispatch_network_timeout(s, event->msg_data);
    break;

  default:
    break;
  }
}

static void
dispatch_new_network(wifi_scan_screen_t* s, const network_t* network)
{
  printf("scan result\r\n");
  printf("  ssid: %s\r\n", network->ssid);
  printf("  security mode: %d\r\n", network->security_mode);
  printf("  rssi: %d\r\n", network->rssi);

  rect_t rect = {
      .x = 0,
      .y = 0,
      .width = 220,
      .height = 40
  };
  listbox_add_row(s->net_list, label_create(NULL, rect, network->ssid, font_opensans_regular_22, WHITE, 1));
}

static void
dispatch_network_update(wifi_scan_screen_t* s, const network_t* network)
{
  printf("net update\r\n");
  printf("  ssid: %s\r\n", network->ssid);
  printf("  security mode: %d\r\n", network->security_mode);
  printf("  rssi: %d\r\n", network->rssi);
}

static void
dispatch_network_timeout(wifi_scan_screen_t* s, const network_t* network)
{
  printf("net timeout\r\n");
  printf("  ssid: %s\r\n", network->ssid);
  printf("  security mode: %d\r\n", network->security_mode);
  printf("  rssi: %d\r\n", network->rssi);
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
//    net_scan_stop();
    gui_pop_screen();
  }
}

static void
scan_button_clicked(button_event_t* event)
{
  widget_t* screen = widget_get_parent(event->widget);
  wifi_scan_screen_t* s = widget_get_instance_data(screen);

  if (event->id == EVT_BUTTON_CLICK) {
//    net_scan_start();
    widget_disable(s->scan_button);
  }
}

