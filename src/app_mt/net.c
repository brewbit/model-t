
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

//static void
//ota_update_mgr(void);

static void
broadcast_net_status(void);


static net_status_t net_status;


void
net_init()
{
  wlan_init(wlan_event, NULL, NULL, NULL, wlan_read_interupt_pin, write_wlan_pin);

  chThdCreateFromHeap(NULL, 1024, NORMALPRIO, wlan_thread, NULL);
  chThdCreateFromHeap(NULL, 1024, NORMALPRIO, mdns_thread, NULL);
}

const net_status_t*
net_get_status()
{
  return &net_status;
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
    break;

    // Notification that CC3000 device is disconnected from the access point (AP)
  case HCI_EVNT_WLAN_UNSOL_DISCONNECT:
    break;

    // Notification of a Dynamic Host Configuration Protocol (DHCP) state change
  case HCI_EVNT_WLAN_UNSOL_DHCP:
    net_status.dhcp_resolved = (data[20] == 0);
    sprintf(net_status.ip_addr, "%d.%d.%d.%d", data[3], data[2], data[1], data[0]);
    sprintf(net_status.subnet_mask, "%d.%d.%d.%d", data[7], data[6], data[5], data[4]);
    sprintf(net_status.default_gateway, "%d.%d.%d.%d", data[11], data[10], data[9], data[8]);
    sprintf(net_status.dhcp_server, "%d.%d.%d.%d", data[15], data[14], data[13], data[12]);
    sprintf(net_status.dns_server, "%d.%d.%d.%d", data[19], data[18], data[17], data[16]);

    broadcast_net_status();
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

  net_state_t last_state = (net_state_t)-1;
  while (1) {
    net_status.net_state = wlan_ioctl_statusget();

    if (net_status.net_state != last_state) {
      if ((net_status.net_state == NET_CONNECTING) ||
          (net_status.net_state == NET_CONNECTED)) {
        tNetappIpconfigRetArgs ip_cfg;
        netapp_ipconfig(&ip_cfg);

        memcpy(net_status.ssid, ip_cfg.uaSSID, 32);
        net_status.ssid[32] = 0;
      }

      broadcast_net_status();
      last_state = net_status.net_state;
    }

    chThdSleepMilliseconds(250);
  }

//  ota_update_mgr();

  return 0;
}

static void
broadcast_net_status()
{
  msg_broadcast(MSG_NET_STATUS, &net_status);
}

// TODO: move this elsewhere
//static void
//ota_update_mgr()
//{
//  int ret;
//  sockaddr_in serv_addr;
//
//  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//  if (s == -1) {
//    printf("FATAL: Could not open socket for OTA update process\r\n");
//    return;
//  }
//
//  memset(&serv_addr, 0, sizeof(serv_addr));
//  serv_addr.sin_family = AF_INET;
//  serv_addr.sin_addr.s_addr = INADDR_ANY;
//  serv_addr.sin_port = htons(5000);
//
//  ret = bind(s, (sockaddr*)&serv_addr, sizeof(serv_addr));
//  if (ret != 0) {
//    printf("FATAL: Could not bind socket for OTA update process\r\n");
//    return;
//  }
//
//  ret = listen(s, 10);
//  if (ret != 0) {
//    printf("FATAL: Could not listen on socket for OTA update process\r\n");
//    return;
//  }
//
//  while(1) {
//    sockaddr conn_addr;
//    socklen_t conn_addr_len;
//    int connfd = accept(s, &conn_addr, &conn_addr_len);
//
//    if (connfd > 0) {
//      sxfs_part_rec_t part_rec;
//      uint8_t* p = (uint8_t*)&part_rec;
//      int left_to_recv = sizeof(part_rec);
//      while (left_to_recv > 0) {
//        ret = recv(connfd, p, left_to_recv, 0);
//
//        if (ret < 0) {
//          printf("error receiving app header: %d\r\n", ret);
//        }
//        else {
//          p += ret;
//          left_to_recv -= ret;
//        }
//      }
//
//      printf("Received image part rec:\r\n");
//      printf("  Magic: %s\r\n",
//          (memcmp(part_rec.magic, "SXFS", 4) == 0) ?
//              "OK" : "ERROR");
//      printf("  # Files: %d\r\n", part_rec.num_files);
//      printf("  Size: %d\r\n", part_rec.size);
//      printf("  CRC: 0x%x\r\n", part_rec.crc);
//
//      if (!sxfs_part_clear(SP_OTA_UPDATE_IMG))
//        printf("part clear failed\r\n");
//
//#define IMG_CHUNK_SIZE 1408
//      uint8_t* img_data = chHeapAlloc(NULL, IMG_CHUNK_SIZE);
//      left_to_recv = part_rec.size;
//
//      sxfs_part_t part;
//      if (!sxfs_part_open(&part, SP_OTA_UPDATE_IMG))
//        printf("part open failed\r\n");
//
//      sxfs_part_write(&part, (uint8_t*)&part_rec, sizeof(part_rec));
//
//      while (left_to_recv > 0) {
//        uint32_t nrecv = MIN(left_to_recv, IMG_CHUNK_SIZE);
//        ret = recv(connfd, img_data, nrecv, 0);
//
//        if (ret < 0) {
//          printf("error receiving app image: %d\r\n", ret);
//        }
//        else {
//          sxfs_part_write(&part, img_data, ret);
//
//          left_to_recv -= ret;
//
//          printf("%d of %d left\r", left_to_recv, part_rec.size);
//        }
//      }
//
//      if (!sxfs_part_verify(SP_OTA_UPDATE_IMG))
//        printf("sxfs verify failed\r\n");
//      else
//        printf("image verified\r\n");
//
//      uint8_t ack = 1;
//      send(connfd, &ack, 1, 0);
//
//      closesocket(connfd);
//
//      chHeapFree(img_data);
//
//      chThdSleepSeconds(1);
//      NVIC_SystemReset();
//    }
//
//    chThdSleepSeconds(5);
//  }
//}
