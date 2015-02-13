
#include "ch.h"
#include "hal.h"
#include "gui/network_settings.h"
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
#include "button_list.h"
#include "textentry.h"

#include <string.h>
#include <stdio.h>


typedef struct {
  widget_t* widget;
  widget_t* button_list;
  net_settings_t settings;
} network_settings_screen_t;


static void network_settings_screen_destroy(widget_t* w);
static void back_button_clicked(button_event_t* event);
static void network_button_clicked(button_event_t* event);
static void passphrase_button_clicked(button_event_t* event);
static void ip_cfg_button_clicked(button_event_t* event);
static void ip_button_clicked(button_event_t* event);
static void subnet_button_clicked(button_event_t* event);
static void gateway_button_clicked(button_event_t* event);
static void dns_button_clicked(button_event_t* event);
static void handle_network(const char* ssid, wlan_security_t security_mode, void* user_data);
static void handle_passphrase(const char* passphrase, void* user_data);
static void handle_ip(const char* ip, void* user_data);
static void handle_subnet(const char* ip, void* user_data);
static void handle_gateway(const char* ip, void* user_data);
static void handle_dns(const char* ip, void* user_data);
static void rebuild_screen(network_settings_screen_t* s);


static const widget_class_t network_settings_screen_widget_class = {
    .on_destroy = network_settings_screen_destroy,
};


widget_t*
network_settings_screen_create()
{
  network_settings_screen_t* s = calloc(1, sizeof(network_settings_screen_t));

  s->widget = widget_create(NULL, &network_settings_screen_widget_class, s, display_rect);

  s->button_list = button_list_screen_create(s->widget, "Network Settings", back_button_clicked, s);

  memcpy(&s->settings, app_cfg_get_net_settings(), sizeof(net_settings_t));

  rebuild_screen(s);

  return s->widget;
}

static void
network_settings_screen_destroy(widget_t* w)
{
  network_settings_screen_t* s = widget_get_instance_data(w);

  free(s);
}

static void
back_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    widget_t* w = widget_get_parent(event->widget);
    network_settings_screen_t* s = widget_get_instance_data(w);

    if (s->settings.ip_config == IP_CFG_DHCP) {
      s->settings.ip = 0;
      s->settings.subnet_mask = 0;
      s->settings.gateway = 0;
      s->settings.dns_server = 0;
    }
    app_cfg_set_net_settings(&s->settings);

    gui_pop_screen();
  }
}

static void
network_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    network_settings_screen_t* s = widget_get_user_data(event->widget);

    widget_t* wifi_scan_screen = wifi_scan_screen_create(handle_network, s);
    gui_push_screen(wifi_scan_screen);
  }
}

static void
passphrase_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    network_settings_screen_t* s = widget_get_user_data(event->widget);
    textentry_screen_show(TXT_FMT_ANY, handle_passphrase, s);
  }
}

static void
ip_cfg_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    network_settings_screen_t* s = widget_get_user_data(event->widget);

    switch (s->settings.ip_config) {
      default:
      case IP_CFG_DHCP:
        s->settings.ip_config = IP_CFG_STATIC;

        if (s->settings.subnet_mask == 0)
          s->settings.subnet_mask = 0x00FFFFFF;

        if (s->settings.dns_server == 0)
          s->settings.dns_server = 0x08080808;
        break;

      case IP_CFG_STATIC:
        s->settings.ip_config = IP_CFG_DHCP;
        break;
    }

    rebuild_screen(s);
  }
}

static void
ip_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    network_settings_screen_t* s = widget_get_user_data(event->widget);
    textentry_screen_show(TXT_FMT_IP, handle_ip, s);
  }
}

static void
subnet_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    network_settings_screen_t* s = widget_get_user_data(event->widget);
    textentry_screen_show(TXT_FMT_IP, handle_subnet, s);
  }
}

static void
gateway_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    network_settings_screen_t* s = widget_get_user_data(event->widget);
    textentry_screen_show(TXT_FMT_IP, handle_gateway, s);
  }
}

static void
dns_button_clicked(button_event_t* event)
{
  if (event->id == EVT_BUTTON_CLICK) {
    network_settings_screen_t* s = widget_get_user_data(event->widget);
    textentry_screen_show(TXT_FMT_IP, handle_dns, s);
  }
}

static void
handle_network(const char* ssid, wlan_security_t security_mode, void* user_data)
{
  network_settings_screen_t* s = user_data;
  strncpy(s->settings.ssid, ssid, sizeof(s->settings.ssid));
  s->settings.security_mode = security_mode;
  rebuild_screen(s);
}

static void
handle_passphrase(const char* passphrase, void* user_data)
{
  network_settings_screen_t* s = user_data;
  strncpy(s->settings.passphrase, passphrase, sizeof(s->settings.passphrase));
  rebuild_screen(s);
}

static bool
inet_aton(const char* ipa, uint32_t* ipn)
{
  unsigned int u1, u2, u3, u4;
  int ret = sscanf(ipa, "%u.%u.%u.%u", &u4, &u3, &u2, &u1);
  if (ret != 4)
    return false;

  if (u1 > 0xFF || u2 > 0xFF || u4 > 0xFF || u4 > 0xFF)
    return false;

  *ipn = (u1 << 24) | (u2 << 16) | (u3 << 8) | (u4);

  return true;
}

static char*
inet_ntoa(uint32_t ip)
{
  char* ipa = malloc(16);
  uint8_t* ipc = (uint8_t*)&ip;

  snprintf(ipa, 16, "%d.%d.%d.%d", ipc[0], ipc[1], ipc[2], ipc[3]);

  return ipa;
}

static void
handle_ip(const char* ip, void* user_data)
{
  network_settings_screen_t* s = user_data;

  s->settings.ip = 0;
  inet_aton(ip, &s->settings.ip);
  rebuild_screen(s);
}

static void
handle_subnet(const char* subnet, void* user_data)
{
  network_settings_screen_t* s = user_data;

  s->settings.subnet_mask = 0;
  inet_aton(subnet, &s->settings.subnet_mask);
  rebuild_screen(s);
}

static void
handle_gateway(const char* gateway, void* user_data)
{
  network_settings_screen_t* s = user_data;

  s->settings.gateway = 0;
  inet_aton(gateway, &s->settings.gateway);
  rebuild_screen(s);
}

static void
handle_dns(const char* dns, void* user_data)
{
  network_settings_screen_t* s = user_data;

  s->settings.dns_server = 0;
  inet_aton(dns, &s->settings.dns_server);
  rebuild_screen(s);
}

static void
rebuild_screen(network_settings_screen_t* s)
{
  char* text;
  uint32_t num_buttons = 0;
  button_spec_t buttons[8];

  if (strlen(s->settings.ssid) == 0)
    text = "No network configured Touch to scan";
  else
    text = s->settings.ssid;
  add_button_spec(buttons, &num_buttons, network_button_clicked, img_signal, ORANGE, "Network", text, s);

  if (s->settings.security_mode != WLAN_SEC_UNSEC) {
    if (strlen(s->settings.passphrase) == 0)
      text = "Passphrase not set";
    else
      text = "********";
    add_button_spec(buttons, &num_buttons, passphrase_button_clicked, img_signal, ORANGE, "Passphrase", text, s);
  }

  switch (s->settings.ip_config) {
    default:
    case IP_CFG_DHCP:
      text = "DHCP - Automatic network configuration";
      break;

    case IP_CFG_STATIC:
      text = "Static - Manual network configuration";
      break;
  }
  add_button_spec(buttons, &num_buttons, ip_cfg_button_clicked, img_signal, ORANGE, "IP Config", text, s);

  char* ip_text = NULL;
  char* subnet_text = NULL;
  char* gateway_text = NULL;
  char* dns_text = NULL;
  if (s->settings.ip_config == IP_CFG_STATIC) {
    if (s->settings.ip == 0)
      text = "IP Address not set";
    else
      text = ip_text = inet_ntoa(s->settings.ip);
    add_button_spec(buttons, &num_buttons, ip_button_clicked, img_signal, ORANGE, "IP Address", text, s);

    if (s->settings.subnet_mask == 0)
      text = "Subnet Mask not set";
    else
      text = subnet_text = inet_ntoa(s->settings.subnet_mask);
    add_button_spec(buttons, &num_buttons, subnet_button_clicked, img_signal, ORANGE, "Subnet Mask", text, s);

    if (s->settings.gateway == 0)
      text = "Default Gateway not set";
    else
      text = gateway_text = inet_ntoa(s->settings.gateway);
    add_button_spec(buttons, &num_buttons, gateway_button_clicked, img_signal, ORANGE, "Default Gateway", text, s);

    if (s->settings.dns_server == 0)
      text = "DNS Server not set";
    else
      text = dns_text = inet_ntoa(s->settings.dns_server);
    add_button_spec(buttons, &num_buttons, dns_button_clicked, img_signal, ORANGE, "DNS Server", text, s);
  }

  button_list_set_buttons(s->button_list, buttons, num_buttons);

  if (ip_text != NULL)
    free(ip_text);

  if (subnet_text != NULL)
    free(subnet_text);

  if (gateway_text != NULL)
    free(gateway_text);

  if (dns_text != NULL)
    free(dns_text);
}

