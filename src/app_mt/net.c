
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
wlan_event(long event_type, char * data, unsigned char length);

static long
wlan_read_interupt_pin(void);

static void
write_wlan_pin(unsigned char val);

//static void
//ota_update_mgr(void);

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


static net_status_t net_status;
static net_state_t last_net_state;
static network_t networks[16];


void
net_init()
{
  wlan_init(wlan_event, NULL, NULL, NULL, wlan_read_interupt_pin, write_wlan_pin);

  chThdCreateFromHeap(NULL, 1024, NORMALPRIO, wlan_thread, NULL);
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
  net_status.security_mode = net->security_mode;
  strncpy(net_status.ssid, net->ssid, sizeof(net_status.ssid));
  strncpy(net_status.passphrase, passphrase, sizeof(net_status.passphrase));
  net_status.net_state = NS_CONNECT;
}

static void
wlan_event(long event_type, char * data, unsigned char length)
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
    break;

    // Notification that CC3000 device is disconnected from the access point (AP)
  case HCI_EVNT_WLAN_UNSOL_DISCONNECT:
    if (net_status.net_state == NS_CONNECTING)
      net_status.net_state = NS_CONNECT_FAILED;
    else
      net_status.net_state = NS_DISCONNECTED;
    break;

    // Notification of a Dynamic Host Configuration Protocol (DHCP) state change
  case HCI_EVNT_WLAN_UNSOL_DHCP:
    net_status.dhcp_resolved = (data[20] == 0);
    sprintf(net_status.ip_addr, "%d.%d.%d.%d", data[3], data[2], data[1], data[0]);
    sprintf(net_status.subnet_mask, "%d.%d.%d.%d", data[7], data[6], data[5], data[4]);
    sprintf(net_status.default_gateway, "%d.%d.%d.%d", data[11], data[10], data[9], data[8]);
    sprintf(net_status.dhcp_server, "%d.%d.%d.%d", data[15], data[14], data[13], data[12]);
    sprintf(net_status.dns_server, "%d.%d.%d.%d", data[19], data[18], data[17], data[16]);
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
  unsigned char scan_buf[50];

  long ret = wlan_ioctl_get_scan_results(0, scan_buf);
  if (ret != 0)
    return ret;

  result->networks_found = (scan_buf[3] << 24) | (scan_buf[2] << 16) | (scan_buf[1] << 8) | scan_buf[0];
  result->scan_status = (scan_buf[7] << 24) | (scan_buf[6] << 16) | (scan_buf[5] << 8) | scan_buf[4];
  result->valid = (scan_buf[8] & 0x01);

  result->network.rssi = (scan_buf[8] >> 1) - 128;
  result->network.security_mode = (scan_buf[9] & 0x03);

  int ssid_len = (scan_buf[9] >> 2);
  memcpy(result->network.ssid, &scan_buf[12], ssid_len);
  result->network.ssid[ssid_len] = 0;

  memcpy(result->network.bssid, &scan_buf[44], 6);

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
      msg_broadcast(MSG_NET_NEW_NETWORK, saved_network);
  }
  else {
    *saved_network = *network;
    msg_broadcast(MSG_NET_NETWORK_UPDATED, saved_network);
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
      msg_broadcast(MSG_NET_NETWORK_TIMEOUT, &networks[i]);
      memset(&networks[i], 0, sizeof(networks[i]));
    }
  }
}

static msg_t
wlan_thread(void* arg)
{
  (void)arg;

  chRegSetThreadName("wlan");

  wlan_start(0);

  unsigned char patchVer[2];
  nvmem_read_sp_version(patchVer);
  printf("CC3000 Service Pack Version: %d.%d\r\n", patchVer[0], patchVer[1]);

  if (patchVer[0] != 1 || patchVer[1] != 24) {
    printf("  Not up to date. Applying patch.\r\n");
    wlan_apply_patch();
  }

  if (wlan_ioctl_set_connection_policy(0, 1, 1) != 0)
    printf("set conn policy failed\r\n");

  chThdCreateFromHeap(NULL, 1024, NORMALPRIO, mdns_thread, NULL);

  while (1) {
    if (net_status.scan_active)
      scan_thread();
    else {
      if (net_status.net_state != last_net_state) {
        switch (net_status.net_state) {
        case NS_CONNECT:
        {
          // Delete any stored profiles
          long ret = wlan_ioctl_del_profile(255);
          if (ret != 0)
            net_status.net_state = NS_CONNECT_FAILED;

          // Add the new profile
          ret = wlan_add_profile(net_status.security_mode,
              (unsigned char*)net_status.ssid, strlen(net_status.ssid),
              NULL, 0, 0x18, 0x1e, 0x2,
              (unsigned char*)net_status.passphrase, strlen(net_status.passphrase));
          if (ret != 0)
            net_status.net_state = NS_CONNECT_FAILED;

          // Restart the module
          wlan_stop();
          chThdSleepMilliseconds(100);
          wlan_start(0);

          net_status.net_state = NS_CONNECTING;
          break;
        }

        case NS_CONNECTED:
        {
          tNetappIpconfigRetArgs ip_cfg;
          netapp_ipconfig(&ip_cfg);

          memcpy(net_status.ssid, ip_cfg.uaSSID, 32);
          net_status.ssid[32] = 0;
          break;
        }

        case NS_CONNECTING:
        case NS_CONNECT_FAILED:
        case NS_DISCONNECTED:
        default:
          break;
        }

        msg_broadcast(MSG_NET_STATUS, &net_status);
        last_net_state = net_status.net_state;
      }
    }

    chThdSleepMilliseconds(500);
  }

  return 0;
}
