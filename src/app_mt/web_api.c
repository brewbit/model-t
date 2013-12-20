
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


typedef enum {
  AS_REQUEST_ACTIVATION_TOKEN,
  AS_WAIT_FOR_ACTIVATION_TOKEN,
  AS_WAIT_FOR_ACTIVATION_NOTIFICATION,
  AS_IDLE
} api_state_t;


typedef struct {
  snWebsocket* ws;
  api_state_t state;
} web_api_t;


static msg_t
web_api_thread(void* arg);

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


void
web_api_init()
{
  chThdCreateFromHeap(NULL, 1024, NORMALPRIO, web_api_thread, NULL);
}

static msg_t
web_api_thread(void* arg)
{
  (void)arg;
  chRegSetThreadName("web");

  chThdSleepMilliseconds(5000);

  web_api_t* api = calloc(1, sizeof(web_api_t));

  api->ws = snWebsocket_create(
        NULL, // open callback
        websocket_message_rx,
        websocket_closed,
        websocket_error,
        api); // callback data

  while (1) {
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
      snError err = snWebsocket_connect(api->ws, "brewbit.herokuapp.com", NULL, NULL, 80);

      if (err != SN_NO_ERROR)
        printf("websocket connect failed %d\r\n", err);
      break;
    }

    default:
      break;
    }

    chThdSleepMilliseconds(250);
  }

  return 0;
}

static void
api_exec(web_api_t* api)
{
  switch (api->state) {
  case AS_REQUEST_ACTIVATION_TOKEN:
    request_activation_token(api);
    api->state = AS_WAIT_FOR_ACTIVATION_TOKEN;
    break;

  case AS_WAIT_FOR_ACTIVATION_TOKEN:
    break;

  case AS_WAIT_FOR_ACTIVATION_NOTIFICATION:
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
    printf("rx auth response\r\n");
    break;

  case ApiMessage_Type_DEVICE_REPORT:
    printf("rx device report\r\n");
    break;

  default:
    printf("Unsupported API message: %d\r\n", msg->type);
    break;
  }
}

