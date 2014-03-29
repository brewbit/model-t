
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


typedef struct {
    bool valid;
    unsigned long networks_found;
    unsigned long scan_status;
    unsigned long frame_time;
    network_t network;
} net_scan_result_t;


static msg_t
mdns_thread(void* arg);

static msg_t
wlan_thread(void* arg);

static void
scan_thread(void);

static void
wlan_event(long event_type, void* data, unsigned char length);

static long
wlan_read_interupt_pin(void);

static void
write_wlan_pin(unsigned char val);

static void
save_or_update_network(network_t* network);

static void
prune_networks(void);

static network_t*
save_network(network_t* net);

static int
enable_scan(bool enable);

static long
perform_scan(void);

static long
get_scan_result(net_scan_result_t* result);

static void
perform_connect(void);


static net_status_t net_status;
static net_state_t last_net_state;
static network_t networks[16];


void
net_init()
{
  wlan_init(wlan_event, NULL, NULL, NULL, wlan_read_interupt_pin, write_wlan_pin);

  /* Thread* thd_wlan = */ chThdCreateFromHeap(NULL, 1024, NORMALPRIO, wlan_thread, NULL);

//  msg_subscribe(MSG_SHUTDOWN, thd_wlan, dispatch, NULL);
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
  net_status.scan_active = true;
}

void
net_scan_stop()
{
  net_status.scan_active = false;
}

void
net_connect(network_t* net, const char* passphrase)
{
  net_settings_t* ns = malloc(sizeof(net_settings_t));
  strncpy(ns->ssid, net->ssid, sizeof(ns->ssid));
  strncpy(ns->passphrase, passphrase, sizeof(ns->passphrase));
  ns->security_mode = net->security_mode;

  app_cfg_set_net_settings(ns);

  free(ns);

  net_status.net_state = NS_CONNECT;
}

static void
wlan_event(long event_type, void* data, unsigned char length)
{
  (void)length;

  switch (event_type) {
    // Notification that the first-time configuration process is complete
  case HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE:
    break;

    // Periodic keep-alive event between the CC3000 and the host microcontroller unit (MCU)
  case HCI_EVNT_WLAN_KEEPALIVE:
    break;

    // WLAN-connected event
  case HCI_EVNT_WLAN_UNSOL_CONNECT:
    net_status.net_state = NS_CONNECTED;
    msg_send(MSG_NET_STATUS, &net_status);
    break;

    // Notification that CC3000 device is disconnected from the access point (AP)
  case HCI_EVNT_WLAN_UNSOL_DISCONNECT:
    if (net_status.net_state == NS_CONNECTING)
      net_status.net_state = NS_CONNECT_FAILED;
    else
      net_status.net_state = NS_DISCONNECTED;
    msg_send(MSG_NET_STATUS, &net_status);
    break;

    // Notification of a Dynamic Host Configuration Protocol (DHCP) state change
  case HCI_EVNT_WLAN_UNSOL_DHCP:
    {
      netapp_dhcp_params_t* dhcp = data;
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
    break;

    // Notification that the CC3000 device finished the initialization process
  case HCI_EVNT_WLAN_UNSOL_INIT:
    break;

  // Notification of ping results
  case HCI_EVNT_WLAN_ASYNC_PING_REPORT:
    break;

  case HCI_EVENT_CC3000_CAN_SHUT_DOWN:
    break;

  default:
    printf("wlan_event(0x%x)\r\n", (unsigned int)event_type);
    break;
  }
}

static long
wlan_read_interupt_pin()
{
  return palReadPad(PORT_WIFI_IRQ, PAD_WIFI_IRQ);
}

static void
write_wlan_pin(unsigned char val)
{
  palWritePad(PORT_WIFI_EN, PAD_WIFI_EN, val);
}

static msg_t
mdns_thread(void* arg)
{
  (void)arg;

  while (1) {
    if (net_status.dhcp_resolved) {
      mdnsAdvertiser(1, SERVICE_NAME, strlen(SERVICE_NAME));
    }
    chThdSleepSeconds(5);
  }

  return 0;
}

static int
enable_scan(bool enable)
{
  static const unsigned long channel_interval_list[16] = {
      2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000,
      2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000
  };

  unsigned long interval = enable ? SCAN_INTERVAL : 0;
  return wlan_ioctl_set_scan_params(interval, 100, 100, 5, 0x1FFF, -80, 0, 205, channel_interval_list);
}

static long
perform_scan()
{
  // enable scanning
  long ret = enable_scan(true);
  if (ret != 0)
    return ret;

  // wait for scan to complete
  chThdSleepMilliseconds(SCAN_INTERVAL);

  // disable scanning
  return enable_scan(false);
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

static void
scan_thread()
{
  while (net_status.scan_active) {
    if (perform_scan() == 0) {
      net_scan_result_t result;
      do {
        if (get_scan_result(&result) != 0)
          break;

        if (result.scan_status == 1 && result.valid)
          save_or_update_network(&result.network);
      } while (result.networks_found > 1);

      prune_networks();
    }

    chThdSleepMilliseconds(500);
  }
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
perform_connect()
{
  const net_settings_t* ns = app_cfg_get_net_settings();
  if (strlen(ns->ssid) > 0) {
    net_status.net_state = NS_CONNECTING;
    msg_send(MSG_NET_STATUS, &net_status);

    wlan_ioctl_set_connection_policy(0, 0, 0);

    wlan_disconnect();

    chThdSleepMilliseconds(100);

    wlan_connect(ns->security_mode,
        ns->ssid, strlen(ns->ssid),
        NULL,
        (const unsigned char*)ns->passphrase, strlen(ns->passphrase));
  }
}

static msg_t
wlan_thread(void* arg)
{
  (void)arg;

  chRegSetThreadName("wlan");

  wlan_start(0);

  {
    nvmem_sp_version_t sp_version;
    nvmem_read_sp_version(&sp_version);
    sprintf(net_status.sp_ver, "%d.%d", sp_version.package_id, sp_version.package_build);
    printf("CC3000 Service Pack Version: %s\r\n", net_status.sp_ver);

    if (sp_version.package_id != 1 || sp_version.package_build != 24) {
      printf("  Not up to date. Applying patch.\r\n");
      wlan_apply_patch();
      printf("  Update complete\r\n");
    }
  }

  {
    unsigned char mac[6];
    nvmem_get_mac_address(mac);
    sprintf(net_status.mac_addr, "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }

  perform_connect();

  chThdCreateFromHeap(NULL, 1024, NORMALPRIO, mdns_thread, NULL);

  while (1) {
    if (net_status.scan_active)
      scan_thread();
    else {
      if (net_status.net_state != last_net_state) {
        switch (net_status.net_state) {
        case NS_CONNECT:
          perform_connect();
          break;

        case NS_CONNECTED:
        case NS_CONNECTING:
        case NS_CONNECT_FAILED:
        case NS_DISCONNECTED:
        default:
          break;
        }

        msg_send(MSG_NET_STATUS, &net_status);
        last_net_state = net_status.net_state;
      }
    }

    chThdSleepMilliseconds(500);
  }

  return 0;
}
