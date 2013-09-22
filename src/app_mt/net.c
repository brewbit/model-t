
#include "ch.h"
#include "hal.h"
#include "net.h"
#include "wifi/wlan.h"
#include "wifi/nvmem.h"
#include "wifi/socket.h"
#include "wifi/patch.h"
#include "xflash.h"
#include "chprintf.h"
#include "app_hdr.h"
#include "common.h"
#include "crc/crc32.h"
#include "sxfs.h"

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

static void
ota_update_mgr(void);


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
    chprintf(SD_STDIO, "wlan_event(0x%x)\r\n", event_type);
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

  wlan_start(0);

  unsigned char patchVer[2];
  nvmem_read_sp_version(patchVer);
  chprintf(SD_STDIO, "CC3000 Service Pack Version: %d.%d\r\n", patchVer[0], patchVer[1]);

  if (patchVer[0] != 1 || patchVer[1] != 24) {
    chprintf(SD_STDIO, "  Not up to date. Applying patch.\r\n");
    wlan_apply_patch();
  }

  long ret = wlan_connect(WLAN_SEC_WPA2, "internets", 9, NULL, (unsigned char*)"stenretni", 9);
  if (ret != 0) {
    chprintf(SD_STDIO, "Could not connect to network %d\r\n", ret);
  }

  while (!connected)
    chThdSleepMilliseconds(100);

  ota_update_mgr();

  return 0;
}

// TODO: move this elsewhere
static void
ota_update_mgr()
{
  int ret;
  sockaddr_in serv_addr;

  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s == -1) {
    chprintf(SD_STDIO, "FATAL: Could not open socket for OTA update process\r\n");
    return;
  }

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = 0; // INADDR_ANY
  serv_addr.sin_port = htons(5000);

  ret = bind(s, (sockaddr*)&serv_addr, sizeof(serv_addr));
  if (ret != 0) {
    chprintf(SD_STDIO, "FATAL: Could not bind socket for OTA update process\r\n");
    return;
  }

  ret = listen(s, 10);
  if (ret != 0) {
    chprintf(SD_STDIO, "FATAL: Could not listen on socket for OTA update process\r\n");
    return;
  }

  while(1) {
    sockaddr conn_addr;
    socklen_t conn_addr_len;
    int connfd = accept(s, &conn_addr, &conn_addr_len);

    if (connfd > 0) {
      sxfs_part_rec_t part_rec;
      uint8_t* p = (uint8_t*)&part_rec;
      int left_to_recv = sizeof(part_rec);
      while (left_to_recv > 0) {
        ret = recv(connfd, p, left_to_recv, 0);

        if (ret < 0) {
          chprintf(SD_STDIO, "error receiving app header: %d\r\n", ret);
        }
        else {
          p += ret;
          left_to_recv -= ret;
        }
      }

      chprintf(SD_STDIO, "Received image part rec:\r\n");
      chprintf(SD_STDIO, "  Magic: %s\r\n",
          (memcmp(part_rec.magic, "SXFS", 4) == 0) ?
              "OK" : "ERROR");
      chprintf(SD_STDIO, "  # Files: %d\r\n", part_rec.num_files);
      chprintf(SD_STDIO, "  Size: %d\r\n", part_rec.size);
      chprintf(SD_STDIO, "  CRC: 0x%x\r\n", part_rec.crc);

      if (!sxfs_part_clear(SP_OTA_UPDATE_IMG))
        chprintf(SD_STDIO, "part clear failed\r\n");

#define IMG_CHUNK_SIZE 1408
      uint8_t* img_data = chHeapAlloc(NULL, IMG_CHUNK_SIZE);
      left_to_recv = part_rec.size;

      sxfs_part_t part;
      if (!sxfs_part_open(&part, SP_OTA_UPDATE_IMG))
        chprintf(SD_STDIO, "part open failed\r\n");

      sxfs_part_write(&part, (uint8_t*)&part_rec, sizeof(part_rec));

      while (left_to_recv > 0) {
        uint32_t nrecv = MIN(left_to_recv, IMG_CHUNK_SIZE);
        ret = recv(connfd, img_data, nrecv, 0);

        if (ret < 0) {
          chprintf(SD_STDIO, "error receiving app image: %d\r\n", ret);
        }
        else {
          sxfs_part_write(&part, img_data, ret);

          left_to_recv -= ret;

          chprintf(SD_STDIO, "%d of %d left\r", left_to_recv, part_rec.size);
        }
      }

      if (!sxfs_part_verify(SP_OTA_UPDATE_IMG))
        chprintf(SD_STDIO, "sxfs verify failed\r\n");
      else
        chprintf(SD_STDIO, "image verified\r\n");

      uint8_t ack = 1;
      send(connfd, &ack, 1, 0);

      closesocket(connfd);

      chHeapFree(img_data);

      chThdSleepSeconds(1);
      NVIC_SystemReset();
    }

    chThdSleepSeconds(5);
  }
}
