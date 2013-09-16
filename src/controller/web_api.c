
#include "ch.h"
#include "hal.h"

#include "web_api.h"
#include "web_api_parser.h"
#include "common.h"
#include "message.h"
#include "sensor.h"
#include "crc.h"
#include "datastream.h"

#include <stdlib.h>
#include <string.h>


typedef enum {
  NOT_CONNECTED,
  CONNECTING,
  CONNECTED,
} web_api_state_t;


static msg_t
web_api_thread(void* arg);

static void
web_api_dispatch(msg_id_t id, void* msg_data, void* user_data);

//static void
//dispatch_wifi_status(wspr_wifi_status_t* p);

static void
dispatch_sensor_sample(sensor_msg_t* p);

static void
on_connect(BaseChannel* tcp_channel, void* user_data);

static void
read_conn(void);

static datastream_t*
web_api_msg_start(web_api_msg_id_t id);

static void
web_api_msg_end(void);

static void
web_api_msg_handler(web_api_msg_id_t id, uint8_t* data, uint32_t data_len);


static BaseChannel* wapi_conn;
static Mutex wapi_tx_mutex;
static datastream_t* msg_data;
static uint16_t msg_crc;
static web_api_state_t state;
static web_api_parser_t* parser;


void
web_api_init()
{
  parser = web_api_parser_new(web_api_msg_handler);
  chMtxInit(&wapi_tx_mutex);
  state = NOT_CONNECTED;

  Thread* th_web_api = chThdCreateFromHeap(NULL, 1024, NORMALPRIO, web_api_thread, NULL);

  msg_subscribe(MSG_WIFI_STATUS, th_web_api, web_api_dispatch, NULL);
  msg_subscribe(MSG_SENSOR_SAMPLE, th_web_api, web_api_dispatch, NULL);
}

static msg_t
web_api_thread(void* arg)
{
  (void)arg;
  chRegSetThreadName("web");

  while (1) {
    if (chMsgIsPendingI(chThdSelf())) {
      Thread* tp = chMsgWait();
      thread_msg_t* msg = (thread_msg_t*)chMsgGet(tp);

      web_api_dispatch(msg->id, msg->msg_data, msg->user_data);

      chMsgRelease(tp, 0);
    }
    else {
      switch (state) {
      case NOT_CONNECTED:
        break;

      case CONNECTING:
        break;

      case CONNECTED:
        read_conn();
        break;
      }
      chThdSleepMilliseconds(200);
    }
  }

  return 0;
}

static void
read_conn()
{
  while (!chIOGetWouldBlock(wapi_conn)) {
    uint8_t c = chIOGet(wapi_conn);
    web_api_parse(parser, c);
  }
}

static datastream_t*
web_api_msg_start(web_api_msg_id_t id)
{
  chMtxLock(&wapi_tx_mutex);

  chIOPut(wapi_conn, 0xB3);
  chIOPut(wapi_conn, 0xEB);
  chIOPut(wapi_conn, 0x17);
  chIOPut(wapi_conn, id);

  msg_crc = crc16(0, id);

  msg_data = ds_new(NULL, 1024);

  return msg_data;
}

static void
web_api_msg_end()
{
  uint32_t i;

  uint32_t data_len = ds_index(msg_data);

  chIOPut(wapi_conn, data_len);
  chIOPut(wapi_conn, data_len >> 8);
  chIOPut(wapi_conn, data_len >> 16);
  chIOPut(wapi_conn, data_len >> 24);

  if (data_len > 0)
    chSequentialStreamWrite(wapi_conn, msg_data->buf, data_len);

  msg_crc = crc16(msg_crc, data_len);
  msg_crc = crc16(msg_crc, data_len >> 8);
  msg_crc = crc16(msg_crc, data_len >> 16);
  msg_crc = crc16(msg_crc, data_len >> 24);

  for (i = 0; i < data_len; ++i) {
    msg_crc = crc16(msg_crc, msg_data->buf[i]);
  }

  chIOPut(wapi_conn, msg_crc);
  chIOPut(wapi_conn, msg_crc >> 8);

  ds_free(msg_data);
  msg_data = NULL;

  chMtxUnlock();
}

static void
web_api_dispatch(msg_id_t id, void* msg_data, void* user_data)
{
  (void)user_data;

  switch (id) {
  case MSG_WIFI_STATUS:
//    dispatch_wifi_status(msg_data);
    break;

  case MSG_SENSOR_SAMPLE:
    dispatch_sensor_sample(msg_data);
    break;

  default:
    break;
  }
}

//static void
//dispatch_wifi_status(wspr_wifi_status_t* p)
//{
//  if (state == NOT_CONNECTED &&
//      p->state == WSPR_SUCCESS) {
//    chprintf(SD_STDIO, "connecting...\r\n");
//    wspr_tcp_connect(IP_ADDR(192, 168, 1, 146), 31337, on_connect, NULL);
//    state = CONNECTING;
//  }
//}

static void
on_connect(BaseChannel* conn, void* user_data)
{
  (void)user_data;

  if (conn != NULL) {
    chprintf(SD_STDIO, "connected\r\n");
    wapi_conn = conn;
    state = CONNECTED;
  }
  else {
    chprintf(SD_STDIO, "retry\r\n");
    state = NOT_CONNECTED;
  }
}

static void
dispatch_sensor_sample(sensor_msg_t* p)
{
  if (wapi_conn != NULL) {
    chprintf(SD_STDIO, "sending temp\r\n");
    datastream_t* ds = web_api_msg_start(WAPI_SENSOR_SAMPLE);
    ds_write_u8(ds, p->sensor);
    ds_write_float(ds, p->sample.value);
    web_api_msg_end();
  }
}

static void
web_api_msg_handler(web_api_msg_id_t id, uint8_t* data, uint32_t data_len)
{
  chprintf(SD_STDIO, "recvd web api msg %d\r\n", id);
}
