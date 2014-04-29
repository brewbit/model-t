
#include "ch.h"
#include "hal.h"
#include "gui/wifi.h"
#include "button.h"
#include "label.h"
#include "icon.h"
#include "gfx.h"
#include "gui.h"
#include "net.h"
#include "web_api.h"
#include "gui/activation.h"
#include "gui/wifi_scan.h"
#include "app_cfg.h"

#include <string.h>


typedef struct {
  widget_t* widget;
  widget_t* nwk_status1;
  widget_t* nwk_status2;
  widget_t* acct_status1;
  widget_t* acct_status2;
} wifi_screen_t;


static void wifi_screen_destroy(widget_t* w);
static void wifi_screen_msg(msg_event_t* event);
static void back_button_clicked(button_event_t* event);
static void network_button_clicked(button_event_t* event);
static void acct_button_clicked(button_event_t* event);

static void dispatch_net_status(wifi_screen_t* s, const net_status_t* status);
static void dispatch_api_status(wifi_screen_t* s, const api_status_t* api_state);


static const widget_class_t wifi_screen_widget_class = {
    .on_destroy = wifi_screen_destroy,
    .on_msg     = wifi_screen_msg
};


widget_t*
wifi_screen_create()
{
  wifi_screen_t* s = calloc(1, sizeof(wifi_screen_t));

  s->widget = widget_create(NULL, &wifi_screen_widget_class, s, display_rect);

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
  label_create(s->widget, rect, "Connection Settings", font_opensans_regular_22, WHITE, 1);

  // create network connection button
  {
    rect.x = 10;
    rect.y = 85;
    rect.width = 300;
    rect.height = 66;
    widget_t* nwk_button = button_create(s->widget, rect, NULL, WHITE, BLACK, network_button_clicked);

    rect.x = 5;
    rect.y = 5;
    rect.width = 56;
    rect.height = 56;
    icon_create(nwk_button, rect, img_signal, WHITE, CYAN);

    rect.x = 70;
    rect.y = 5;
    rect.width = 200;
    label_create(nwk_button, rect, "Network Connection", font_opensans_regular_18, WHITE, 1);

    rect.y = 30;
    s->nwk_status1 = label_create(nwk_button, rect, "", font_opensans_regular_12, WHITE, 1);
    rect.y += font_opensans_regular_12->line_height;
    s->nwk_status2 = label_create(nwk_button, rect, "", font_opensans_regular_12, WHITE, 1);
  }

  // create account connection button
  {
    rect.x = 10;
    rect.y = 155;
    rect.width = 300;
    rect.height = 66;
    widget_t* acct_button = button_create(s->widget, rect, NULL, WHITE, BLACK, acct_button_clicked);

    rect.x = 5;
    rect.y = 5;
    rect.width = 56;
    rect.height = 56;
    icon_create(acct_button, rect, img_signal, WHITE, CYAN);

    rect.x = 70;
    rect.y = 5;
    rect.width = 200;
    label_create(acct_button, rect, "Account Connection", font_opensans_regular_18, WHITE, 1);

    rect.y = 30;
    s->acct_status1 = label_create(acct_button, rect, "", font_opensans_regular_12, WHITE, 1);
    rect.y += font_opensans_regular_12->line_height;
    s->acct_status2 = label_create(acct_button, rect, "", font_opensans_regular_12, WHITE, 1);
  }

  const net_status_t* net_status = net_get_status();
  dispatch_net_status(s, net_status);

  const api_status_t* api_status = web_api_get_status();
  dispatch_api_status(s, api_status);

  gui_msg_subscribe(MSG_NET_STATUS, s->widget);
  gui_msg_subscribe(MSG_API_STATUS, s->widget);

  return s->widget;
}

static void
wifi_screen_destroy(widget_t* w)
{
  wifi_screen_t* s = widget_get_instance_data(w);

  gui_msg_unsubscribe(MSG_NET_STATUS, w);
  gui_msg_unsubscribe(MSG_API_STATUS, w);

  free(s);
}

static void
wifi_screen_msg(msg_event_t* event)
{
  wifi_screen_t* s = widget_get_instance_data(event->widget);

  switch (event->msg_id) {
  case MSG_NET_STATUS:
    dispatch_net_status(s, event->msg_data);
    break;

  case MSG_API_STATUS:
    dispatch_api_status(s, event->msg_data);
    break;

  default:
    break;
  }
}

static void
dispatch_net_status(wifi_screen_t* s, const net_status_t* status)
{
  switch (status->net_state) {
  case NS_CONNECT:
  case NS_CONNECTING:
    label_set_text(s->nwk_status1, "Connecting to:");
    label_set_text(s->nwk_status2, app_cfg_get_net_settings()->ssid);
    break;

  case NS_CONNECT_FAILED:
    label_set_text(s->nwk_status1, "Failed to connect to:");
    label_set_text(s->nwk_status2, app_cfg_get_net_settings()->ssid);
    break;

  case NS_CONNECTED:
    label_set_text(s->nwk_status1, "Connected to:");
    label_set_text(s->nwk_status2, app_cfg_get_net_settings()->ssid);
    break;

  case NS_DISCONNECTED:
    label_set_text(s->nwk_status1, "Not connected");
    label_set_text(s->nwk_status2, "Touch to scan for networks");
    break;

  default:
    label_set_text(s->nwk_status1, "Unknown network state");
    label_set_text(s->nwk_status2, "");
    break;
  }
}

static void
dispatch_api_status(wifi_screen_t* s, const api_status_t* api_status)
{
  switch (api_status->state) {
  case AS_AWAITING_NET_CONNECTION:
    label_set_text(s->acct_status1, "Waiting for network connection");
    label_set_text(s->acct_status2, "");
    break;

  case AS_CONNECTING:
    label_set_text(s->acct_status1, "Connecting to server");
    label_set_text(s->acct_status2, "");
    break;

  case AS_REQUESTING_AUTH:
    label_set_text(s->acct_status1, "Authenticating");
    label_set_text(s->acct_status2, "");
    break;

  case AS_REQUESTING_ACTIVATION_TOKEN:
    label_set_text(s->acct_status1, "Requesting activation token");
    label_set_text(s->acct_status2, "");
    break;

  case AS_AWAITING_ACTIVATION:
    label_set_text(s->acct_status1, "Awaiting activation");
    label_set_text(s->acct_status2, "Touch here to view activation token");
    break;

  case AS_CONNECTED:
    label_set_text(s->acct_status1, "Connected");
    label_set_text(s->acct_status2, "");
    break;


  default:
    label_set_text(s->acct_status1, "Unknown state");
    label_set_text(s->acct_status2, "");
    break;
  }
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK)
    gui_pop_screen();
}

static void
network_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* wifi_scan_screen = wifi_scan_screen_create();
    gui_push_screen(wifi_scan_screen);
  }
}

static void
acct_button_clicked(button_event_t* event)
{
  const api_status_t* api_status = web_api_get_status();
  if (event->id == EVT_BUTTON_CLICK &&
      api_status->state == AS_AWAITING_ACTIVATION) {
    widget_t* activation_screen = activation_screen_create(api_status->activation_token);
    gui_push_screen(activation_screen);
  }
}
