
#ifndef NET_H
#define NET_H

#include <ch.h>

#include <stdbool.h>

typedef enum {
  NS_DISCONNECTED,
  NS_CONNECT,
  NS_CONNECTING,
  NS_CONNECTED,
  NS_CONNECT_FAILED
} net_state_t;

typedef struct {
  bool scan_active;

  char sp_ver[8];
  char mac_addr[32];

  net_state_t net_state;

  // DHCP status
  bool dhcp_resolved;
  char ip_addr[16];
  char subnet_mask[16];
  char default_gateway[16];
  char dhcp_server[16];
  char dns_server[16];
} net_status_t;

typedef struct {
  long rssi;
  unsigned long security_mode;
  char          ssid[33];
  unsigned char bssid[6];
  systime_t last_seen;
} network_t;

typedef struct {
  char ssid[33];
  char passphrase[128];
  unsigned long security_mode;
} net_settings_t;


void
net_init(void);

const net_status_t*
net_get_status(void);

void
net_scan_start(void);

void
net_scan_stop(void);

void
net_connect(network_t* net, const char* passphrase);

#endif
