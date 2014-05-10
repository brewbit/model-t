
#ifndef GUI_WIFI_SCAN_H
#define GUI_WIFI_SCAN_H

#include "widget.h"
#include "net.h"

typedef void (*network_select_handler_t)(const char* ssid, wlan_security_t security_mode, void* user_data);

widget_t*
wifi_scan_screen_create(network_select_handler_t handler, void* user_data);

#endif
