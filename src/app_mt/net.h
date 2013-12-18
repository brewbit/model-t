
#ifndef NET_H
#define NET_H

#include <ch.h>

#include <stdbool.h>

typedef struct {
  // AP status
  bool ap_connected;
  char ssid[33];

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
    bool valid;
    unsigned long networks_found;
    unsigned long scan_status;
    unsigned long frame_time;
    network_t network;
} net_scan_result_t;

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
