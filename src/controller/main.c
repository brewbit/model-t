
#include "ch.h"
#include "hal.h"

#include "lcd.h"
#include "image.h"
#include "web_api.h"
#include "touch.h"
#include "gui.h"
#include "temp_control.h"
#include "gui_calib.h"
#include "gui_home.h"
#include "chprintf.h"
#include "gfx.h"
#include "app_cfg.h"
#include "debug_client.h"
#include "wifi/wlan.h"
#include "wifi/evnt_handler.h"
#include "wifi/nvmem.h"

void NMIVector(void)
{
  chDbgPanic("NMI Vector\r\n");
}

void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
  /* These are volatile to try and prevent the compiler/linker optimising them
  away as the variables never actually get used.  If the debugger won't show the
  values of the variables, make them global my moving their declaration outside
  of this function. */
  volatile uint32_t r0;
  volatile uint32_t r1;
  volatile uint32_t r2;
  volatile uint32_t r3;
  volatile uint32_t r12;
  volatile uint32_t lr; /* Link register. */
  volatile uint32_t pc; /* Program counter. */
  volatile uint32_t psr;/* Program status register. */

  r0 = pulFaultStackAddress[0];
  r1 = pulFaultStackAddress[1];
  r2 = pulFaultStackAddress[2];
  r3 = pulFaultStackAddress[3];

  r12 = pulFaultStackAddress[4];
  lr = pulFaultStackAddress[5];
  pc = pulFaultStackAddress[6];
  psr = pulFaultStackAddress[7];

//  chprintf(SD_STDIO, "Hard Fault:\r\n");
//  chprintf(SD_STDIO, "  r0: %08x\r\n", r0);
//  chprintf(SD_STDIO, "  r1: %08x\r\n", r1);
//  chprintf(SD_STDIO, "  r2: %08x\r\n", r2);
//  chprintf(SD_STDIO, "  r3: %08x\r\n", r3);
//  chprintf(SD_STDIO, "  r12: %08x\r\n", r12);
//  chprintf(SD_STDIO, "  lr: %08x\r\n", lr);
//  chprintf(SD_STDIO, "  pc: %08x\r\n", pc);
//  chprintf(SD_STDIO, "  psr: %08x\r\n", psr);
//  chThdSleepSeconds(5);

  /* When the following line is hit, the variables contain the register values. */
  chSysHalt();
}

void HardFaultVector(void)
{
  __asm volatile
  (
      " tst lr, #4                                                \n"
      " ite eq                                                    \n"
      " mrseq r0, msp                                             \n"
      " mrsne r0, psp                                             \n"
      " ldr r1, [r0, #24]                                         \n"
      " ldr r2, handler2_address_const                            \n"
      " bx r2                                                     \n"
      " handler2_address_const: .word prvGetRegistersFromStack    \n"
  );
}

void MemManageVector(void)
{
  chDbgPanic("Mem Manage Vector\r\n");
}

void BusFaultVector(void)
{
  chDbgPanic("Bus Fault Vector\r\n");
}

void UsageFaultVector(void)
{
  chDbgPanic("Usage Fault Vector\r\n");
}

msg_t
idle_thread(void* arg)
{
  (void)arg;

  while (1) {
    gui_idle();
    app_cfg_idle();

    hci_unsolicited_event_handler();
  }

  return 0;
}

#include "wifi/cc3000_common.h"

void wlan_event(long event_type, char * data, unsigned char length )
{
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

long wlan_read_interupt_pin()
{
  return palReadPad(PORT_WIFI_IRQ, PAD_WIFI_IRQ);
}

void write_wlan_pin(unsigned char val)
{
  palWritePad(PORT_WIFI_EN, PAD_WIFI_EN, val);
}

msg_t
wlan_thread(void* arg)
{
  wlan_init(wlan_event, NULL, NULL, NULL, wlan_read_interupt_pin, write_wlan_pin);
  chprintf(SD_STDIO, "stopping...\r\n");
  wlan_stop();

  chprintf(SD_STDIO, "starting...\r\n");
  wlan_start(0);

  chprintf(SD_STDIO, "getting MAC... ");
  unsigned char mac[6];
  nvmem_get_mac_address(mac);
  chprintf(SD_STDIO, "%02x:%02x:%02x:%02x:%02x:%02x\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  chprintf(SD_STDIO, "connecting... ");
  long ret = wlan_connect(WLAN_SEC_WPA2, "internets", 9, NULL, "stenretni", 9);
  chprintf(SD_STDIO, "%d\r\n", ret);


  unsigned long aiIntervalList[16] = {
      2000,
      2000,
      2000,
      2000,
      2000,
      2000,
      2000,
      2000,
      2000,
      2000,
      2000,
      2000,
      2000,
      2000,
      2000,
      2000
  };
  chprintf(SD_STDIO, "scanning... ");
  ret = wlan_ioctl_set_scan_params(1,
      100,
      100,
      5,
      0x7ff,
      -110,
      0,
      205,
      aiIntervalList);
  chprintf(SD_STDIO, "%d\r\n", ret);

  unsigned char res[64];
  chprintf(SD_STDIO, "result... ");
  wlan_ioctl_get_scan_results(0, res);
  unsigned long count = STREAM_TO_UINT32_f(res, 0);
  unsigned long status = STREAM_TO_UINT32_f(res, 4);
  chprintf(SD_STDIO, "%d %d\r\n", count, status);
}

int
main(void)
{
  halInit();
  chSysInit();

  /* start stdout port */
  sdStart(SD_STDIO, NULL);

  // Disable flash chip
  palSetPad(PORT_SFLASH_CS, PAD_SFLASH_CS);

  app_cfg_init();
  gfx_init();
  touch_init();
  temp_control_init();
  web_api_init();
  gui_init();
//  debug_client_init();

  widget_t* home_screen = home_screen_create();
  gui_push_screen(home_screen);

  chThdCreateFromHeap(NULL, 1024, LOWPRIO, idle_thread, NULL);
  chThdCreateFromHeap(NULL, 2048, NORMALPRIO, wlan_thread, NULL);

  while (TRUE) {
    palSetPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(500);
    palClearPad(PORT_LED1, PAD_LED1);
    chThdSleepMilliseconds(500);
  }
}
