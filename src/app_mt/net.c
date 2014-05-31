
#include "ch.h"
#include "hal.h"
#include "net.h"
#include "wifi/wlan.h"
#include "wifi/nvmem.h"
#include "wifi/socket.h"
#include "wifi/patch.h"
#include "xflash.h"
#include "app_hdr.h"
#include "common.h"
#include "crc/crc32.h"
#include "sxfs.h"
#include "message.h"
#include "netapp.h"
#include "app_cfg.h"

#include <string.h>
#include <stdio.h>


#define SCAN_INTERVAL 1000
#define SERVICE_NAME "brewbit-model-t"


#define DHCP_TIMEOUT    S2ST(90)
#define CONNECT_TIMEOUT S2ST(90)
#define API_TIMEOUT     S2ST(90)


typedef struct {
  bool valid;
  uint32_t networks_found;
  uint32_t scan_status;
  uint32_t frame_time;
  network_t network;
} net_scan_result_t;


static void
dispatch_net_msg(msg_id_t id, void* msg_data, void* listener_data, void* sub_data);

static void
initialize_and_connect(void);

static void
dispatch_idle(void);

static void
dispatch_dhcp(netapp_dhcp_params_t* dhcp);

static void
dispatch_network_settings(void);

static void
save_or_update_network(network_t* network);

static void
prune_networks(void);

static network_t*
save_network(network_t* net);

static int
enable_scan(bool enable);

static void
scan_exec(void);

static long
get_scan_result(net_scan_result_t* result);


static net_status_t net_status;
static network_t networks[16];
static bool force_reconnect;
static bool wifi_config_applied;
static systime_t state_begin_time;
static systime_t scan_interval_start;


void
net_init()
{
  wlan_init();

  msg_listener_t* l = msg_listener_create("net", 2048, dispatch_net_msg, NULL);
  msg_listener_set_idle_timeout(l, 500);
  msg_subscribe(l, MSG_NET_NETWORK_SETTINGS, NULL);
  msg_subscribe(l, MSG_WLAN_CONNECT, NULL);
  msg_subscribe(l, MSG_WLAN_DISCONNECT, NULL);
  msg_subscribe(l, MSG_WLAN_DHCP, NULL);
}

const net_status_t*
net_get_status()
{
  return &net_status;
}

void
net_scan_start()
{
  memset(networks, 0, sizeof(networks));
  net_status.scan_state = SCAN_START_INTERVAL;
}

void
net_scan_stop()
{
  net_status.scan_state = SCAN_DISABLE;
}

static void
set_state(net_state_t state)
{
  if (net_status.net_state != state) {
    state_begin_time = chTimeNow();
    net_status.net_state = state;
    msg_send(MSG_NET_STATUS, &net_status);
  }
}

static void
dispatch_net_msg(msg_id_t id, void* msg_data, void* listener_data, void* sub_data)
{
  (void)sub_data;
  (void)listener_data;

  switch (id) {
    case MSG_INIT:
      initialize_and_connect();
      break;

    case MSG_IDLE:
      dispatch_idle();
      break;

    case MSG_WLAN_CONNECT:
      set_state(NS_WAIT_DHCP);
      break;

    case MSG_WLAN_DISCONNECT:
      net_status.dhcp_resolved = false;
      set_state(NS_DISCONNECTED);
      break;

    case MSG_WLAN_DHCP:
      dispatch_dhcp(msg_data);
      if (net_status.dhcp_resolved)
        set_state(NS_CONNECTED);
      break;

    case MSG_NET_NETWORK_SETTINGS:
      dispatch_network_settings();
      break;

    default:
      break;
  }
}

static void
dispatch_network_settings()
{
  wifi_config_applied = false;
  force_reconnect = true;
}

static void
dispatch_dhcp(netapp_dhcp_params_t* dhcp)
{
  net_status.dhcp_resolved = (dhcp->status == 0);
  sprintf(net_status.ip_addr, "%d.%d.%d.%d",
      dhcp->ip_addr[3],
      dhcp->ip_addr[2],
      dhcp->ip_addr[1],
      dhcp->ip_addr[0]);
  sprintf(net_status.subnet_mask, "%d.%d.%d.%d",
      dhcp->subnet_mask[3],
      dhcp->subnet_mask[2],
      dhcp->subnet_mask[1],
      dhcp->subnet_mask[0]);
  sprintf(net_status.default_gateway, "%d.%d.%d.%d",
      dhcp->default_gateway[3],
      dhcp->default_gateway[2],
      dhcp->default_gateway[1],
      dhcp->default_gateway[0]);
  sprintf(net_status.dhcp_server, "%d.%d.%d.%d",
      dhcp->dhcp_server[3],
      dhcp->dhcp_server[2],
      dhcp->dhcp_server[1],
      dhcp->dhcp_server[0]);
  sprintf(net_status.dns_server, "%d.%d.%d.%d",
      dhcp->dns_server[3],
      dhcp->dns_server[2],
      dhcp->dns_server[1],
      dhcp->dns_server[0]);
}

static int
enable_scan(bool enable)
{
  static const uint32_t channel_interval_list[16] = {
      2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000,
      2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000
  };

  uint32_t interval = enable ? SCAN_INTERVAL : 0;
  return wlan_ioctl_set_scan_params(interval, 100, 100, 5, 0x1FFF, -80, 0, 205, channel_interval_list);
}

static void
scan_exec()
{
  switch (net_status.scan_state) {
  case SCAN_IDLE:
    break;

  case SCAN_DISABLE:
    enable_scan(false);
    net_status.scan_state = SCAN_IDLE;
    break;

  case SCAN_START_INTERVAL:
    enable_scan(true);
    scan_interval_start = chTimeNow();
    net_status.scan_state = SCAN_WAIT_INTERVAL;
    break;

  case SCAN_WAIT_INTERVAL:
    if ((chTimeNow() - scan_interval_start) > MS2ST(SCAN_INTERVAL)) {
      enable_scan(false);
      net_status.scan_state = SCAN_PROCESS_RESULTS;
    }
    break;

  case SCAN_PROCESS_RESULTS:
  {
    net_scan_result_t result;
    do {
      if (get_scan_result(&result) != 0)
        break;

      if (result.scan_status == 1 && result.valid)
        save_or_update_network(&result.network);
    } while (result.networks_found > 1);

    prune_networks();

    net_status.scan_state = SCAN_START_INTERVAL;
    break;
  }

  default:
    break;
  }
}

static long
get_scan_result(net_scan_result_t* result)
{
  wlan_scan_results_t results;

  wlan_ioctl_get_scan_results(0, &results);

  result->networks_found = results.result_count;
  result->scan_status = results.scan_status;
  result->valid = results.valid;
  result->network.rssi = results.rssi;
  result->network.security_mode = results.security_mode;
  int ssid_len = results.ssid_len;
  memcpy(result->network.ssid, results.ssid, ssid_len);
  result->network.ssid[ssid_len] = 0;

  memcpy(result->network.bssid, results.bssid, 6);

  result->network.last_seen = chTimeNow();

  return 0;
}

static network_t*
find_network(char* ssid)
{
  int i;
  for (i = 0; i < 16; ++i) {
    if (strcmp(networks[i].ssid, ssid) == 0)
      return &networks[i];
  }

  return NULL;
}

static void
save_or_update_network(network_t* network)
{
  network_t* saved_network = find_network(network->ssid);

  if (saved_network == NULL) {
    saved_network = save_network(network);
    if (saved_network != NULL)
      msg_send(MSG_NET_NEW_NETWORK, saved_network);
  }
  else {
    *saved_network = *network;
    msg_send(MSG_NET_NETWORK_UPDATED, saved_network);
  }
}

static network_t*
save_network(network_t* net)
{
  int i;
  for (i = 0; i < 16; ++i) {
    if (strcmp(networks[i].ssid, "") == 0) {
      networks[i] = *net;
      return &networks[i];
    }
  }

  return NULL;
}

#define NETWORK_TIMEOUT S2ST(60)

static void
prune_networks()
{
  int i;
  for (i = 0; i < 16; ++i) {
    if ((strcmp(networks[i].ssid, "") != 0) &&
        (chTimeNow() - networks[i].last_seen) > NETWORK_TIMEOUT) {
      msg_send(MSG_NET_NETWORK_TIMEOUT, &networks[i]);
      memset(&networks[i], 0, sizeof(networks[i]));
    }
  }
}

static void
initialize_and_connect()
{
  const net_settings_t* ns = app_cfg_get_net_settings();

  net_status.dhcp_resolved = false;
  set_state(NS_DISCONNECTED);

  systime_t now = chTimeNow();

  wlan_stop();

  wlan_start(PATCH_LOAD_DEFAULT);

  {
    nvmem_sp_version_t sp_version;
    nvmem_read_sp_version(&sp_version);
    sprintf(net_status.sp_ver, "%d.%d", sp_version.package_id, sp_version.package_build);
    printf("CC3000 Service Pack Version: %s\r\n", net_status.sp_ver);

    if (sp_version.package_id != 1 || sp_version.package_build != 28) {
      printf("  Not up to date. Applying patch.\r\n");
      wlan_apply_patch();
      printf("  Update complete\r\n");

      nvmem_read_sp_version(&sp_version);
      sprintf(net_status.sp_ver, "%d.%d", sp_version.package_id, sp_version.package_build);
      printf("Updated CC3000 Service Pack Version: %s\r\n", net_status.sp_ver);
    }
  }

  if (!wifi_config_applied) {
    wlan_ioctl_set_connection_policy(0, 0, 0);

    uint32_t dhcp_timeout = 14400;
    uint32_t arp_timeout = 3600;
    uint32_t keepalive = 10;
    uint32_t inactivity_timeout = 0;
    netapp_timeout_values(&dhcp_timeout, &arp_timeout, &keepalive, &inactivity_timeout);

    netapp_dhcp(&ns->ip, &ns->subnet_mask, &ns->gateway, &ns->dns_server);

    wlan_stop();

    wlan_start(PATCH_LOAD_DEFAULT);

    wifi_config_applied = true;
  }

  {
    uint8_t mac[6];
    nvmem_get_mac_address(mac);
    sprintf(net_status.mac_addr, "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }

  if (strlen(ns->ssid) > 0) {
    set_state(NS_CONNECTING);

    wlan_disconnect();

    chThdSleepMilliseconds(100);

    wlan_connect(ns->security_mode,
        ns->ssid,
        strlen(ns->ssid),
        NULL,
        (const uint8_t*)ns->passphrase,
        strlen(ns->passphrase));
  }
}

static void
dispatch_idle()
{
  scan_exec();

  if (force_reconnect) {
    net_status.net_state = NS_DISCONNECTED;
    force_reconnect = false;
  }

  switch (net_status.net_state) {
    case NS_DISCONNECTED:
      if (strlen(app_cfg_get_net_settings()->ssid) > 0)
        initialize_and_connect();
      break;

    case NS_WAIT_DHCP:
      if ((chTimeNow() - state_begin_time) > DHCP_TIMEOUT) {
        printf("DHCP Timeout\r\n");
        initialize_and_connect();
      }
      break;

    case NS_CONNECTING:
      if ((chTimeNow() - state_begin_time) > CONNECT_TIMEOUT) {
        printf("Connect Timeout\r\n");
        initialize_and_connect();
      }
      break;

    case NS_CONNECTED:
      if (web_api_get_status()->state > AS_CONNECTING)
        state_begin_time = chTimeNow();
      else if ((chTimeNow() - state_begin_time) > API_TIMEOUT) {
        printf("API Timeout\r\n");
        initialize_and_connect();
      }
      break;

    default:
      break;
  }
}
