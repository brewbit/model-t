
#include "ch.h"
#include "hal.h"

#include "net.h"

#include "wifi/wlan.h"
#include "wifi/nvmem.h"
#include "wifi/socket.h"

#include "chprintf.h"

#include <string.h>


#define SERVICE_NAME "brewbit-model-t"


static msg_t
mdns_thread(void* arg);

static msg_t
wlan_thread(void* arg);

static void
wlan_event(long event_type, char * data, unsigned char length);

static long
wlan_read_interupt_pin(void);

static void
write_wlan_pin(unsigned char val);


static int connected;


void
net_init()
{
  wlan_init(wlan_event, NULL, NULL, NULL, wlan_read_interupt_pin, write_wlan_pin);

  chThdCreateFromHeap(NULL, 2048, NORMALPRIO, wlan_thread, NULL);
  chThdCreateFromHeap(NULL, 512, NORMALPRIO, mdns_thread, NULL);
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
    chprintf(SD_STDIO, "wlan keepalive\r\n");
    break;

    // WLAN-connected event
  case HCI_EVNT_WLAN_UNSOL_CONNECT:
    chprintf(SD_STDIO, "wlan connect\r\n");
    break;

    // Notification that CC3000 device is disconnected from the access point (AP)
  case HCI_EVNT_WLAN_UNSOL_DISCONNECT:
    chprintf(SD_STDIO, "wlan disconnect\r\n");
    break;

    // Notification of a Dynamic Host Configuration Protocol (DHCP) state change
  case HCI_EVNT_WLAN_UNSOL_DHCP:
    chprintf(SD_STDIO, "wlan dhcp state change\r\n");

    chprintf(SD_STDIO, "  Status: %d\r\n", data[20]);
    chprintf(SD_STDIO, "  IP Address: %d.%d.%d.%d\r\n", data[3], data[2], data[1], data[0]);
    chprintf(SD_STDIO, "  Subnet Mask: %d.%d.%d.%d\r\n", data[7], data[6], data[5], data[4]);
    chprintf(SD_STDIO, "  Default Gateway: %d.%d.%d.%d\r\n", data[11], data[10], data[9], data[8]);
    chprintf(SD_STDIO, "  DHCP Server: %d.%d.%d.%d\r\n", data[15], data[14], data[13], data[12]);
    chprintf(SD_STDIO, "  DNS Server: %d.%d.%d.%d\r\n", data[19], data[18], data[17], data[16]);

    connected = 1;
    break;

    // Notification that the CC3000 device finished the initialization process
  case HCI_EVNT_WLAN_UNSOL_INIT:
    chprintf(SD_STDIO, "wlan init complete\r\n");
    break;

  // Notification of ping results
  case HCI_EVNT_WLAN_ASYNC_PING_REPORT:
    chprintf(SD_STDIO, "wlan ping\r\n");
    break;

  default:
    chprintf(SD_STDIO, "wlan_event(%d)\r\n", event_type);
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
    if (connected) {
      mdnsAdvertiser(1, SERVICE_NAME, strlen(SERVICE_NAME));
    }
    chThdSleepSeconds(5);
  }

  return 0;
}

static msg_t
wlan_thread(void* arg)
{
  (void)arg;
  chRegSetThreadName("wlan");

  chprintf(SD_STDIO, "starting...\r\n");
  wlan_start(0);

  chprintf(SD_STDIO, "getting MAC... ");
  unsigned char mac[6];
  nvmem_get_mac_address(mac);
  chprintf(SD_STDIO, "%02x:%02x:%02x:%02x:%02x:%02x\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  chprintf(SD_STDIO, "connecting... ");
  long ret = wlan_connect(WLAN_SEC_WPA2, "internets", 9, NULL, (unsigned char*)"stenretni", 9);
  chprintf(SD_STDIO, "%d\r\n", ret);

  chprintf(SD_STDIO, "waiting for DHCP... ");
  while (!connected)
    chThdSleepMilliseconds(100);
  chprintf(SD_STDIO, "OK\r\n");



  sockaddr_in serv_addr;


  chprintf(SD_STDIO, "creating socket... ");
  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  chprintf(SD_STDIO, "%d\r\n", s);

  memset(&serv_addr, '0', sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = 0; // INADDR_ANY
  serv_addr.sin_port = htons(5000);

  chprintf(SD_STDIO, "binding... ");
  ret = bind(s, (sockaddr*)&serv_addr, sizeof(serv_addr));
  chprintf(SD_STDIO, "%d\r\n", ret);

  chprintf(SD_STDIO, "listening... ");
  ret = listen(s, 10);
  chprintf(SD_STDIO, "%d\r\n", ret);

  while(1) {
    chprintf(SD_STDIO, "accepting... ");
    sockaddr conn_addr;
    socklen_t conn_addr_len;
    int connfd = accept(s, &conn_addr, &conn_addr_len);
    chprintf(SD_STDIO, "%d\r\n", connfd);

    if (connfd > 0) {
      while (1) {
        char data[16];
        chprintf(SD_STDIO, "receiving... ");
        ret = recv(connfd, data, sizeof(data)-1, 0);
        chprintf(SD_STDIO, "%d\r\n", ret);

        if (ret > 0) {
          data[ret] = 0;
          chprintf(SD_STDIO, "recv: %s", data);
        }
      }
    }

    chThdSleepSeconds(5);
  }
//  unsigned long aiIntervalList[16] = {
//      2000,
//      2000,
//      2000,
//      2000,
//      2000,
//      2000,
//      2000,
//      2000,
//      2000,
//      2000,
//      2000,
//      2000,
//      2000,
//      2000,
//      2000,
//      2000
//  };
//  chprintf(SD_STDIO, "scanning... ");
//  ret = wlan_ioctl_set_scan_params(1,
//      100,
//      100,
//      5,
//      0x1fff,
//      -110,
//      0,
//      205,
//      aiIntervalList);
//  chprintf(SD_STDIO, "%d\r\n", ret);
//
//  while (1) {
//    unsigned char res[64];
//    chprintf(SD_STDIO, "result... ");
//    wlan_ioctl_get_scan_results(0, res);
//    unsigned long count = STREAM_TO_UINT32_f(res, 0);
//    unsigned long status = STREAM_TO_UINT32_f(res, 4);
//    uint8_t results_valid = res[8] & 0x01;
//    uint8_t rssi = res[8] >> 1;
//    uint8_t security = res[9] & 0x03;
//    uint8_t ssid_len = res[9] >> 2;
//    uint16_t timestamp = STREAM_TO_UINT16_f(res, 10);
//
//    char ssid[33];
//    memcpy(ssid, &res[12], 32);
//    ssid[32] = 0;
//
//    char bssid[7];
//    memcpy(bssid, &res[44], 6);
//    bssid[6] = 0;
//
//
//    chprintf(SD_STDIO, "scan status: %d\r\n", status);
//    chprintf(SD_STDIO, "  results left: %d\r\n", count);
//    chprintf(SD_STDIO, "  valid: %d\r\n", results_valid);
//    chprintf(SD_STDIO, "  rssi: %d\r\n", rssi);
//    chprintf(SD_STDIO, "  security: %d\r\n", security);
//    chprintf(SD_STDIO, "  ssid_len: %d\r\n", ssid_len);
//    chprintf(SD_STDIO, "  timestamp: %d\r\n", timestamp);
//    chprintf(SD_STDIO, "  ssid: %s\r\n", ssid);
//    chprintf(SD_STDIO, "  bssid: %x:%x:%x:%x:%x:%x\r\n", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
//
//    chThdSleepSeconds(1);
//  }

  return 0;
}
