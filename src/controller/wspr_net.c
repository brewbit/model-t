
#include "ch.h"
#include "hal.h"
#include "wspr_net.h"
#include "wspr.h"
#include "chprintf.h"
#include "datastream.h"
#include "message.h"

#include <stdlib.h>


static void handle_status(uint8_t* data, uint16_t data_len);
static void handle_scan_result(uint8_t* data, uint16_t data_len);
static void handle_scan_complete(uint8_t* data, uint16_t data_len);

static void wspr_net_req_status(void* arg);


static VirtualTimer status_timer;


void
wspr_net_init()
{
  wspr_set_handler(WSPR_OUT_STATUS, handle_status);
  wspr_set_handler(WSPR_OUT_SCAN_RESULT, handle_scan_result);
  wspr_set_handler(WSPR_OUT_SCAN_COMPLETE, handle_scan_complete);

  chVTSetI(&status_timer, S2ST(1), wspr_net_req_status, NULL);
}

static void
wspr_net_req_status(void* arg)
{
  (void)arg;

  wspr_send(WSPR_IN_STATUS, NULL, 0);

  chVTSetI(&status_timer, S2ST(1), wspr_net_req_status, NULL);
}

void
wspr_net_scan()
{
  wspr_send(WSPR_IN_SCAN, NULL, 0);
}

static void
handle_status(uint8_t* data, uint16_t data_len)
{
  wspr_wifi_status_t status = {0};

  datastream_t* ds = ds_new(data, data_len);
  status.configured = ds_read_u8(ds);
  status.ssid = ds_read_str(ds);
  status.security = ds_read_s32(ds);
  status.state = ds_read_s32(ds);
  status.rssi = ds_read_s32(ds);

  msg_broadcast(MSG_WIFI_STATUS, &status);

  free(status.ssid);
}

static void
handle_scan_result(uint8_t* data, uint16_t data_len)
{
  chprintf((BaseChannel*)&SD3, "scan result\r\n");
}

static void
handle_scan_complete(uint8_t* data, uint16_t data_len)
{
  chprintf((BaseChannel*)&SD3, "scan complete\r\n");
}
