
#include "ch.h"
#include "hal.h"

#include "web_api.h"
#include "web_api_parser.h"
#include "common.h"
#include "message.h"
#include "sensor.h"
#include "crc/crc16.h"
#include "datastream.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <snacka/websocket.h>


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

static void
websocket_message_rx(void* userData, snOpcode opcode, const char* data, int numBytes);

static void
websocket_closed(void* userData, snStatusCode status);

static void
websocket_error(void* userData, snError error);

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

  chThdSleepMilliseconds(5000);

  printf("ws create\r\n");

  snWebsocket* ws = snWebsocket_create(
        NULL, // open callback
        websocket_message_rx,
        websocket_closed,
        websocket_error,
        NULL); // callback data

  printf("ws connect\r\n");
  chThdSleepSeconds(1);

//  snError err = snWebsocket_connect(ws, "brewbit.herokuapp.com", NULL, NULL, 80);
//  snError err = snWebsocket_connect(ws, "76.88.84.25", NULL, NULL, 10500);
  snError err = snWebsocket_connect(ws, "192.168.1.146", "echo", NULL, 12345);

  printf("ws connect done\r\n");
  chThdSleepSeconds(1);

  if (err != SN_NO_ERROR) {
    printf("websocket connect failed %d\r\n", err);
  }
  else {
    printf("connect OK, handshaking...\r\n");
    chThdSleepSeconds(1);
  }

  while (snWebsocket_getState(ws) != SN_STATE_OPEN) {
    snWebsocket_poll(ws); // <----------------------------- SOMETHING IN HERE IS CRASHING...
    chThdSleepSeconds(1);
  }

  printf("websocket is open\r\n");

//    ApiMessage msg = {
//        .type = ApiMessage_Type_ACTIVATION_TOKEN_REQUEST,
//        .has_activationTokenRequest = true,
//        .activationTokenRequest.device_id = "asdfasdf"
//    };
//
//    printf("sending activation token request...\n");
//    send_api_msg(ws, &msg);
//
//    printf("polling...\n");
//    while (snWebsocket_getState(ws) != SN_STATE_CLOSED) {
//      snWebsocket_poll(ws);
//      usleep(1000 * pollDurationMs);
//    }

  while (1) {
//    if (chMsgIsPendingI(chThdSelf())) {
//      Thread* tp = chMsgWait();
//      thread_msg_t* msg = (thread_msg_t*)chMsgGet(tp);
//
//      web_api_dispatch(msg->id, msg->msg_data, msg->user_data);
//
//      chMsgRelease(tp, 0);
//    }
//    else {
//      switch (state) {
//      case NOT_CONNECTED:
//        break;
//
//      case CONNECTING:
//        break;
//
//      case CONNECTED:
//        read_conn();
//        break;
//      }
      chThdSleepMilliseconds(2000);
//    }
  }

  return 0;
}

static void
websocket_message_rx(void* userData, snOpcode opcode, const char* data, int numBytes)
{
  if (opcode != SN_OPCODE_BINARY)
    return;

//  ApiMessage* msg = malloc(sizeof(ApiMessage));
//
//  pb_istream_t stream = pb_istream_from_buffer(data, numBytes);
//  bool status = pb_decode(&stream, ApiMessage_fields, msg);
//
//  if (status)
//    dispatch_msg(msg);
//
//  free(msg);
}

static void
websocket_closed(void* userData, snStatusCode status)
{
  printf("websocket closed %d\r\n", status);
}

static void
websocket_error(void* userData, snError error)
{
  printf("websocket error %d\r\n", error);
}

//void dispatch_msg(ApiMessage* msg)
//{
//  switch (msg->type) {
//  case ApiMessage_Type_ACTIVATION_TOKEN_REQUEST:
//    printf("rx activation token request\n");
//    break;
//
//  case ApiMessage_Type_ACTIVATION_TOKEN_RESPONSE:
//    printf("rx activation token response\n");
//    break;
//
//  case ApiMessage_Type_ACTIVATION_NOTIFICATION:
//    printf("rx activation notification\n");
//    break;
//
//  case ApiMessage_Type_AUTH_REQUEST:
//    printf("rx auth request\n");
//    break;
//
//  case ApiMessage_Type_AUTH_RESPONSE:
//    printf("rx auth response\n");
//    break;
//
//  case ApiMessage_Type_DEVICE_REPORT:
//    printf("rx device report\n");
//    break;
//
//  default:
//    break;
//  }
//}

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

  msg_crc = crc16_update(0, id);

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

  msg_crc = crc16_update(msg_crc, data_len);
  msg_crc = crc16_update(msg_crc, data_len >> 8);
  msg_crc = crc16_update(msg_crc, data_len >> 16);
  msg_crc = crc16_update(msg_crc, data_len >> 24);

  for (i = 0; i < data_len; ++i) {
    msg_crc = crc16_update(msg_crc, msg_data->buf[i]);
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
//    printf("connecting...\r\n");
//    wspr_tcp_connect(IP_ADDR(192, 168, 1, 146), 31337, on_connect, NULL);
//    state = CONNECTING;
//  }
//}

static void
on_connect(BaseChannel* conn, void* user_data)
{
  (void)user_data;

  if (conn != NULL) {
    printf("connected\r\n");
    wapi_conn = conn;
    state = CONNECTED;
  }
  else {
    printf("retry\r\n");
    state = NOT_CONNECTED;
  }
}

static void
dispatch_sensor_sample(sensor_msg_t* p)
{
  if (wapi_conn != NULL) {
    printf("sending temp\r\n");
    datastream_t* ds = web_api_msg_start(WAPI_SENSOR_SAMPLE);
    ds_write_u8(ds, p->sensor);
    ds_write_float(ds, p->sample.value);
    web_api_msg_end();
  }
}

static void
web_api_msg_handler(web_api_msg_id_t id, uint8_t* data, uint32_t data_len)
{
  printf("recvd web api msg %d\r\n", id);
}
