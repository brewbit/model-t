
#include "ch.h"
#include "hal.h"
#include "gui/wifi_scan.h"
#include "button.h"
#include "label.h"
#include "icon.h"
#include "listbox.h"
#include "gfx.h"
#include "gui.h"
#include "net.h"
#include "gui/textentry.h"

#include <string.h>
#include <stdio.h>


typedef struct {
  widget_t* widget;
  widget_t* net_list;
  network_select_handler_t handler;
  void* user_data;
} wifi_scan_screen_t;


static void wifi_scan_screen_destroy(widget_t* w);
static void wifi_scan_screen_msg(msg_event_t* event);
static void back_button_clicked(button_event_t* event);
static void network_button_event(button_event_t* event);

static void
dispatch_new_network(wifi_scan_screen_t* s, network_t* network);

static void
dispatch_network_update(wifi_scan_screen_t* s, network_t* network);

static void
dispatch_network_timeout(wifi_scan_screen_t* s, network_t* network);


static const widget_class_t wifi_scan_screen_widget_class = {
    .on_destroy = wifi_scan_screen_destroy,
    .on_msg     = wifi_scan_screen_msg
};


widget_t*
wifi_scan_screen_create(network_select_handler_t handler, void* user_data)
{
  wifi_scan_screen_t* s = calloc(1, sizeof(wifi_scan_screen_t));

  s->widget = widget_create(NULL, &wifi_scan_screen_widget_class, s, display_rect);
  s->handler = handler;
  s->user_data = user_data;

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
  rect.width = 160;
  label_create(s->widget, rect, "Network Scan", font_opensans_regular_22, WHITE, 1);

  rect.x = 5;
  rect.y = 70;
  rect.width = 300;
  rect.height = 160;
  s->net_list = listbox_create(s->widget, rect, 40);

  gui_msg_subscribe(MSG_NET_NEW_NETWORK, s->widget);
  gui_msg_subscribe(MSG_NET_NETWORK_UPDATED, s->widget);
  gui_msg_subscribe(MSG_NET_NETWORK_TIMEOUT, s->widget);

  net_scan_start();

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
dispatch_new_network(wifi_scan_screen_t* s, network_t* network)
{
//  printf("scan result\r\n");
//  printf("  ssid: %s\r\n", network->ssid);
//  printf("  security mode: %d\r\n", network->security_mode);
//  printf("  rssi: %d\r\n", network->rssi);

  rect_t rect = {
      .x = 0,
      .y = 0,
      .width = 220,
      .height = 40
  };
  widget_t* item = button_create(NULL, rect, NULL, WHITE, BLACK, network_button_event);
  button_set_text(item, network->ssid);
  button_set_font(item, font_opensans_regular_22);
  widget_set_user_data(item, network);
  listbox_add_item(s->net_list, item);
}

static void
dispatch_network_update(wifi_scan_screen_t* s, network_t* network)
{
  (void)s;
  (void)network;
//  printf("net update\r\n");
//  printf("  ssid: %s\r\n", network->ssid);
//  printf("  security mode: %d\r\n", network->security_mode);
//  printf("  rssi: %d\r\n", network->rssi);
}

static void
dispatch_network_timeout(wifi_scan_screen_t* s, network_t* network)
{
//  printf("net timeout\r\n");
//  printf("  ssid: %s\r\n", network->ssid);
//  printf("  security mode: %d\r\n", network->security_mode);
//  printf("  rssi: %d\r\n", network->rssi);

  int i;
  for (i = 0; i < listbox_num_items(s->net_list); ++i) {
    widget_t* item = listbox_get_item(s->net_list, i);
    if (network == widget_get_user_data(item)) {
      listbox_delete_item(item);
      break;
    }
  }
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    net_scan_stop();
    gui_pop_screen();
  }
}

static void
network_button_event(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* cont = widget_get_parent(event->widget);
    widget_t* lb = widget_get_parent(cont);
    widget_t* screen = widget_get_parent(lb);
    wifi_scan_screen_t* s = widget_get_instance_data(screen);

    network_t* net = widget_get_user_data(event->widget);
    s->handler(net->ssid, net->security_mode, s->user_data);

    net_scan_stop();
    gui_pop_screen();
  }
}
