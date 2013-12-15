
#ifndef NET_H
#define NET_H

#include <stdbool.h>

typedef enum {
  NET_DISCONNECTED,
  NET_SCANNING,
  NET_CONNECTING,
  NET_CONNECTED
} net_state_t;

typedef struct {
  // AP status
  net_state_t net_state;
  char ssid[33];

  // DHCP status
  bool dhcp_resolved;
  char ip_addr[16];
  char subnet_mask[16];
  char default_gateway[16];
  char dhcp_server[16];
  char dns_server[16];
} net_status_t;


void
net_init(void);

const net_status_t*
net_get_status(void);

#endif
