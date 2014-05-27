
#ifndef NET_H
#define NET_H

#include <ch.h>

#include <stdbool.h>

#include "wlan.h"

typedef enum {
  NS_DISCONNECTED,
  NS_CONNECTING,
  NS_WAIT_DHCP,
  NS_CONNECTED,
} net_state_t;

typedef enum {
  SCAN_IDLE,
  SCAN_DISABLE,
  SCAN_START_INTERVAL,
  SCAN_WAIT_INTERVAL,
  SCAN_PROCESS_RESULTS,
} scan_state_t;

typedef struct {
  scan_state_t scan_state;

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
  wlan_security_t security_mode;
  char ssid[33];
  uint8_t bssid[6];
  systime_t last_seen;
} network_t;

typedef enum {
  IP_CFG_DHCP,
  IP_CFG_STATIC
} ip_config_t;

typedef struct {
  char ssid[33];
  char passphrase[128];
  uint32_t security_mode;

  ip_config_t ip_config;
  uint32_t ip;
  uint32_t subnet_mask;
  uint32_t gateway;
  uint32_t dns_server;
} net_settings_t;


void
net_init(void);

const net_status_t*
net_get_status(void);

void
net_scan_start(void);

void
net_scan_stop(void);

#endif
