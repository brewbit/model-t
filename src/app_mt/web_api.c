
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


typedef struct {
  snWebsocket* ws;
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

  printf("ws connect\r\n");
  chThdSleepSeconds(1);

  snError err = snWebsocket_connect(api->ws, "brewbit.herokuapp.com", NULL, NULL, 80);
//  snError err = snWebsocket_connect(ws, "76.88.84.25", NULL, NULL, 10500);
//  snError err = snWebsocket_connect(ws, "192.168.1.146", "echo", NULL, 12345);

  printf("ws connect done\r\n");
  chThdSleepSeconds(1);

  if (err != SN_NO_ERROR) {
    printf("websocket connect failed %d\r\n", err);
  }
  else {
    printf("connect OK, handshaking...\r\n");
    chThdSleepSeconds(1);
  }

  while (snWebsocket_getState(api->ws) != SN_STATE_OPEN) {
    snWebsocket_poll(api->ws);
    chThdSleepSeconds(1);
  }

  printf("websocket is open\r\n");

  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_ACTIVATION_TOKEN_REQUEST;
  msg->has_activationTokenRequest = true;
  strcpy(msg->activationTokenRequest.device_id, "asdfasdf");

  printf("sending activation token request...\r\n");
  send_api_msg(api->ws, msg);
  free(msg);

  while (snWebsocket_getState(api->ws) != SN_STATE_CLOSED) {
    printf("polling...\r\n");
    snWebsocket_poll(api->ws);
    chThdSleepSeconds(1);
  }

  printf("closed...\r\n");
  while (1) {
    chThdSleepSeconds(1);
  }

  return 0;
}

static void
send_api_msg(snWebsocket* ws, ApiMessage* msg)
{
  int i;
  uint8_t* buffer = malloc(ApiMessage_size);

  pb_ostream_t stream = pb_ostream_from_buffer(buffer, ApiMessage_size);
  bool encoded_ok = pb_encode(&stream, ApiMessage_fields, msg);

  printf("encoded %d %d\r\n", stream.bytes_written, encoded_ok);
  for (i = 0; i < stream.bytes_written; ++i) {
    printf("%d ", buffer[i]);
  }
  printf("\r\n");

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
  (void)api;

  switch (msg->type) {
  case ApiMessage_Type_ACTIVATION_TOKEN_REQUEST:
    printf("rx activation token request\n");
    break;

  case ApiMessage_Type_ACTIVATION_TOKEN_RESPONSE:
    printf("rx activation token response\n");
    break;

  case ApiMessage_Type_ACTIVATION_NOTIFICATION:
    printf("rx activation notification\n");
    break;

  case ApiMessage_Type_AUTH_REQUEST:
    printf("rx auth request\n");
    break;

  case ApiMessage_Type_AUTH_RESPONSE:
    printf("rx auth response\n");
    break;

  case ApiMessage_Type_DEVICE_REPORT:
    printf("rx device report\n");
    break;

  default:
    break;
  }
}

