
#ifndef WIFI_H
#define WIFI_H

#include "widget.h"

widget_t*
wifi_screen_create(void);

typedef enum{
  NO_CONN,
  NET_CONN,
  API_CONN,
  BOTH_CONN,
}connection_status_t;

#endif
