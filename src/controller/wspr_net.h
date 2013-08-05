
#ifndef WSPR_NET
#define WSPR_NET

#include "types.h"

typedef enum {
  WSPR_UNFINISHED = -41,
  WSPR_NOTFOUND = -30,
  WSPR_ERROR = -1,

  WSPR_SUCCESS = 0,
  WSPR_NOT_AUTHENTICATED = 6,
  WSPR_NOT_KEYED = 7,
} wifi_state_t;

typedef struct {
  uint8_t configured;
  char* ssid;
  int32_t security;
  wifi_state_t state;
  int32_t rssi;
} wspr_wifi_status_t;

void
wspr_net_init(void);

void
wspr_net_scan(void);

#endif
