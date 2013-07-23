
#ifndef WSPR_NET
#define WSPR_NET

#include "types.h"

typedef struct {
  uint8_t configured;
  char* ssid;
  int32_t security;
  int32_t state;
  int32_t rssi;
} wspr_wifi_status_t;

void
wspr_net_init(void);

void
wspr_net_scan(void);

#endif
