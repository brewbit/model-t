
#include <ch.h>
#include <hal.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <snacka/websocket.h>

#include <pb_encode.h>
#include <pb_decode.h>

#include "web_api.h"
#include "bbmt.pb.h"
#include "message.h"

#ifndef WEB_API_HOST
#define WEB_API_HOST_STR "brewbit.herokuapp.com"
#else
#define xstr(s) str(s)
#define str(s) #s
#define WEB_API_HOST_STR xstr(WEB_API_HOST)
#endif

#ifndef WEB_API_PORT
#define WEB_API_PORT 80
#endif


typedef enum {
  AS_REQUEST_AUTH,
  AS_WAIT_FOR_AUTH,
  AS_REQUEST_ACTIVATION_TOKEN,
  AS_WAIT_FOR_ACTIVATION_TOKEN,
  AS_WAIT_FOR_ACTIVATION_NOTIFICATION,
  AS_REQUEST_UPDATE,
  AS_WAIT_FOR_UPDATE,
  AS_IDLE
} api_state_t;


typedef struct {
  snWebsocket* ws;
  api_state_t state;
} web_api_t;

static web_api_t* api;


static msg_t
web_api_thread(void* arg);

static void
web_api_dispatch(msg_id_t id, void* msg_data, void* user_data);

static void
websocket_message_rx(void* userData, snOpcode opcode, const char* data, int numBytes);

static void
websocket_closed(void* userData, snStatusCode status);

static void
websocket_error(void* userData, snError error);

static void
send_api_msg(snWebsocket* ws, ApiMessage* msg);

static void
dispatch_api_msg(web_api_t* api, ApiMessage* msg);

static void
api_exec(web_api_t* api);

static void
request_activation_token(web_api_t* api);

static void
request_auth(web_api_t* api);

static void
request_update(web_api_t* api);


void
web_api_init()
{
  Thread* thd_web_api = chThdCreateFromHeap(NULL, 2048, NORMALPRIO, web_api_thread, NULL);

  msg_subscribe(MSG_CHECK_UPDATE, thd_web_api, web_api_dispatch, NULL);
}

static msg_t
web_api_thread(void* arg)
{
  (void)arg;
  chRegSetThreadName("web");

  chThdSleepMilliseconds(5000);

  api = calloc(1, sizeof(web_api_t));
  api->state = AS_REQUEST_AUTH;

  api->ws = snWebsocket_create(
        NULL, // open callback
        websocket_message_rx,
        websocket_closed,
        websocket_error,
        api); // callback data

  while (1) {
    thread_msg_t* msg = msg_get_timeout(MS2ST(250));

    if (msg != NULL) {
      web_api_dispatch(msg->id, msg->msg_data, msg->user_data);
      msg_release(msg);
    }
    else {
      switch(snWebsocket_getState(api->ws)) {
      case SN_STATE_OPEN:
        api_exec(api);
        // intentional fall-through

      case SN_STATE_CONNECTING:
      case SN_STATE_CLOSING:
        snWebsocket_poll(api->ws);
        break;

      case SN_STATE_CLOSED:
      {
        printf("WS connecting\r\n");
        snError err = snWebsocket_connect(api->ws, WEB_API_HOST_STR, NULL, NULL, WEB_API_PORT);

        if (err != SN_NO_ERROR)
          printf("websocket connect failed %d\r\n", err);
        else
          printf("websocket connect OK\r\n");
        break;
      }

      default:
        break;
      }
    }
  }

  return 0;
}

static void
web_api_dispatch(msg_id_t id, void* msg_data, void* user_data)
{
  switch (id) {
  case MSG_CHECK_UPDATE:
    request_update(api);
    api->state = AS_WAIT_FOR_UPDATE;
    break;

  default:
    break;
  }
}

static void
api_exec(web_api_t* api)
{
  switch (api->state) {
  case AS_REQUEST_AUTH:
    printf("requesting auth\r\n");
    request_auth(api);
    api->state = AS_WAIT_FOR_AUTH;
    break;

  case AS_WAIT_FOR_AUTH:
    break;

  case AS_REQUEST_ACTIVATION_TOKEN:
    request_activation_token(api);
    api->state = AS_WAIT_FOR_ACTIVATION_TOKEN;
    break;

  case AS_WAIT_FOR_ACTIVATION_TOKEN:
    break;

  case AS_WAIT_FOR_ACTIVATION_NOTIFICATION:
    break;

  case AS_REQUEST_UPDATE:
    break;

  case AS_WAIT_FOR_UPDATE:
    break;

  case AS_IDLE:
    break;

  default:
    break;
  }
}

static void
request_activation_token(web_api_t* api)
{
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_ACTIVATION_TOKEN_REQUEST;
  msg->has_activationTokenRequest = true;
  strcpy(msg->activationTokenRequest.device_id, "asdfasdf");

  send_api_msg(api->ws, msg);

  free(msg);
}

static void
request_auth(web_api_t* api)
{
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_AUTH_REQUEST;
  msg->has_authRequest = true;
  uint32_t* devid = (uint32_t*)0x1FFF7A10;
  sprintf(msg->authRequest.device_id, "%08x%08x%08x", devid[0], devid[1], devid[2]);
  sprintf(msg->authRequest.activation_token, "asdfasdf");

  send_api_msg(api->ws, msg);

  free(msg);
}

void
web_api_request_update(void)
{
  msg_post(MSG_CHECK_UPDATE, NULL);
}

static void
request_update(web_api_t* api)
{
  printf("sending update check\r\n");
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_UPDATE_REQUEST;
  msg->has_updateRequest = true;
  msg->updateRequest.device_version = 42;

  send_api_msg(api->ws, msg);

  free(msg);
}

static void
send_api_msg(snWebsocket* ws, ApiMessage* msg)
{
  uint8_t* buffer = malloc(ApiMessage_size);

  pb_ostream_t stream = pb_ostream_from_buffer(buffer, ApiMessage_size);
  bool encoded_ok = pb_encode(&stream, ApiMessage_fields, msg);

  if (encoded_ok)
    snWebsocket_sendBinaryData(ws, stream.bytes_written, (char*)buffer);

  free(buffer);
}

static void
websocket_message_rx(void* userData, snOpcode opcode, const char* data, int numBytes)
{
  web_api_t* api = userData;

  if (opcode != SN_OPCODE_BINARY)
    return;

  ApiMessage* msg = malloc(sizeof(ApiMessage));

  pb_istream_t stream = pb_istream_from_buffer((const uint8_t*)data, numBytes);
  bool status = pb_decode(&stream, ApiMessage_fields, msg);

  if (status)
    dispatch_api_msg(api, msg);

  free(msg);
}

static void
websocket_closed(void* userData, snStatusCode status)
{
  (void)userData;
  printf("websocket closed %d\r\n", status);
}

static void
websocket_error(void* userData, snError error)
{
  (void)userData;
  printf("websocket error %d\r\n", error);
}

static void
dispatch_api_msg(web_api_t* api, ApiMessage* msg)
{
  switch (msg->type) {
  case ApiMessage_Type_ACTIVATION_TOKEN_RESPONSE:
    api->state = AS_WAIT_FOR_ACTIVATION_NOTIFICATION;
    break;

  case ApiMessage_Type_ACTIVATION_NOTIFICATION:
    api->state = AS_IDLE;
    break;

  case ApiMessage_Type_AUTH_RESPONSE:
    printf("got auth response\r\n");
    api->state = AS_IDLE;
    break;

  case ApiMessage_Type_UPDATE_CHUNK:
    printf("got update chunk %d %d\r\n", msg->updateChunk.data.size, msg->updateChunk.offset);
    msg_send(MSG_OTAU_CHUNK, &msg->updateChunk);
    break;

  default:
    printf("Unsupported API message: %d\r\n", msg->type);
    break;
  }
}

