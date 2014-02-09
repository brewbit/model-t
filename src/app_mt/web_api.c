
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
#include "net.h"
#include "sensor.h"
#include "app_cfg.h"
#include "snacka_backend/iocallbacks_socket.h"
#include "snacka_backend/cryptocallbacks_chibios.h"

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

#define SENSOR_REPORT_INTERVAL S2ST(60)


typedef struct {
  bool new_sample;
  quantity_t last_sample;
} sensor_status_t;

typedef struct {
  snWebsocket* ws;
  api_status_t status;
  sensor_status_t sensor_status[NUM_SENSORS];
  systime_t last_sensor_report_time;
} web_api_t;


static void
set_state(web_api_t* api, api_state_t state);

static void
web_api_dispatch(msg_id_t id, void* msg_data, void* listener_data, void* sub_data);

static void
web_api_idle(web_api_t* api);

static void
websocket_message_rx(void* userData, snOpcode opcode, const char* data, int numBytes);

static void
send_api_msg(snWebsocket* ws, ApiMessage* msg);

static void
dispatch_api_msg(web_api_t* api, ApiMessage* msg);

static void
dispatch_net_status(web_api_t* api, net_status_t* ns);

static void
dispatch_sensor_sample(web_api_t* api, sensor_msg_t* sample);

static void
request_activation_token(web_api_t* api);

static void
request_auth(web_api_t* api);

static void
check_for_update(web_api_t* api);

static void
start_update(web_api_t* api, const char* ver);

static void
send_sensor_report(web_api_t* api);

static void
dispatch_device_settings(DeviceSettingsNotification* settings);


static web_api_t* api;

static const snIOCallbacks iocb = {
    .initCallback = snSocketInitCallback,
    .deinitCallback = snSocketDeinitCallback,
    .connectCallback = snSocketConnectCallback,
    .disconnectCallback = snSocketDisconnectCallback,
    .readCallback = snSocketReadCallback,
    .writeCallback = snSocketWriteCallback,
    .timeCallback = snSocketTimeCallback
};

static const snCryptoCallbacks cryptcb = {
    .randCallback = snChRandCallback,
    .shaCallback = snChShaCallback
};

static const snWebsocketSettings ws_settings = {
    .maxFrameSize = 2048,
    .logCallback = NULL,
    .frameCallback = NULL,
    .ioCallbacks = &iocb,
    .cryptoCallbacks = &cryptcb,
    .cancelCallback = NULL
};


void
web_api_init()
{
  api = calloc(1, sizeof(web_api_t));
  api->status.state = AS_AWAITING_NET_CONNECTION;

  api->ws = snWebsocket_create(
        NULL, // open callback
        websocket_message_rx,
        NULL, // closed callback
        NULL, // error callback
        api,
        &ws_settings);

  msg_listener_t* l = msg_listener_create("web_api", 2048, web_api_dispatch, api);
  msg_listener_set_idle_timeout(l, 250);

  msg_subscribe(l, MSG_NET_STATUS, NULL);
  msg_subscribe(l, MSG_API_FW_UPDATE_CHECK, NULL);
  msg_subscribe(l, MSG_API_FW_DNLD_START, NULL);
  msg_subscribe(l, MSG_SENSOR_SAMPLE, NULL);
}

const api_status_t*
web_api_get_status()
{
  return &api->status;
}

static void
web_api_dispatch(msg_id_t id, void* msg_data, void* listener_data, void* sub_data)
{
  (void)sub_data;
  (void)msg_data;

  web_api_t* api = listener_data;

  switch (id) {
    case MSG_NET_STATUS:
      dispatch_net_status(api, msg_data);
      break;

    case MSG_SENSOR_SAMPLE:
      dispatch_sensor_sample(api, msg_data);
      break;

    case MSG_IDLE:
      web_api_idle(api);
      break;

    default:
      break;
  }

  // Only process the following if the API connection has been established
  if (api->status.state == AS_CONNECTED) {
    switch (id) {
      case MSG_API_FW_UPDATE_CHECK:
        check_for_update(api);
        break;

      case MSG_API_FW_DNLD_START:
        start_update(api, msg_data);
        break;

      default:
        break;
    }
  }
}

static void
set_state(web_api_t* api, api_state_t state)
{
  if (api->status.state != state) {
    api->status.state = state;

    api_status_t status_msg = {
        .state = state
    };
    msg_send(MSG_API_STATUS, &status_msg);
  }
}

snHTTPHeader device_id_header = {
    .name = "Device-ID",
    .value = device_id
};

static void
web_api_idle(web_api_t* api)
{
  switch (api->status.state) {
    case AS_AWAITING_NET_CONNECTION:
      /* do nothing, wait for net to come up */
      break;

    case AS_CONNECT:
      snWebsocket_connect(api->ws, WEB_API_HOST_STR, NULL, NULL, WEB_API_PORT, &device_id_header, 1);
      set_state(api, AS_CONNECTING);
      break;

    case AS_CONNECTING:
      if (snWebsocket_getState(api->ws) == SN_STATE_OPEN) {
        const char* auth_token = app_cfg_get_auth_token();
        if (strlen(auth_token) > 0) {
          request_auth(api);
          set_state(api, AS_REQUESTING_AUTH);
        }
        else {
          request_activation_token(api);
          set_state(api, AS_REQUESTING_ACTIVATION_TOKEN);
        }
      }
      break;

    case AS_REQUESTING_ACTIVATION_TOKEN:
    case AS_REQUESTING_AUTH:
    case AS_AWAITING_ACTIVATION:
      break;

    case AS_CONNECTED:
      if ((chTimeNow() - api->last_sensor_report_time) > SENSOR_REPORT_INTERVAL) {
        send_sensor_report(api);
        api->last_sensor_report_time = chTimeNow();
      }
      break;
  }

  snWebsocket_poll(api->ws);

  if ((api->status.state != AS_AWAITING_NET_CONNECTION) &&
      (snWebsocket_getState(api->ws) == SN_STATE_CLOSED))
    set_state(api, AS_CONNECT);
}

static void
send_sensor_report(web_api_t* api)
{
  int i;
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_DEVICE_REPORT;
  msg->has_deviceReport = true;
  msg->deviceReport.probeReport_count = 0;

  for (i = 0; i < NUM_SENSORS; ++i) {
    if (api->sensor_status[i].new_sample) {
      api->sensor_status[i].new_sample = false;
      ProbeReport* pr = &msg->deviceReport.probeReport[msg->deviceReport.probeReport_count];
      msg->deviceReport.probeReport_count++;

      pr->id = i;
      pr->value = api->sensor_status[i].last_sample.value;
    }
  }

  if (msg->deviceReport.probeReport_count > 0) {
    printf("sending sensor report %d\r\n", msg->deviceReport.probeReport_count);
    send_api_msg(api->ws, msg);
  }

  free(msg);
}

static void
request_activation_token(web_api_t* api)
{
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_ACTIVATION_TOKEN_REQUEST;
  msg->has_activationTokenRequest = true;
  strcpy(msg->activationTokenRequest.device_id, device_id);

  send_api_msg(api->ws, msg);

  free(msg);
}

static void
request_auth(web_api_t* api)
{
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_AUTH_REQUEST;
  msg->has_authRequest = true;
  strcpy(msg->authRequest.device_id, device_id);
  sprintf(msg->authRequest.auth_token, app_cfg_get_auth_token());

  send_api_msg(api->ws, msg);

  free(msg);
}

static void
dispatch_net_status(web_api_t* api, net_status_t* ns)
{
  if (ns->net_state == NS_CONNECTED)
    set_state(api, AS_CONNECT);
  else
    set_state(api, AS_AWAITING_NET_CONNECTION);
}

static void
dispatch_sensor_sample(web_api_t* api, sensor_msg_t* sample)
{
  if (sample->sensor >= NUM_SENSORS)
    return;

  sensor_status_t* s = &api->sensor_status[sample->sensor];
  s->new_sample = true;
  s->last_sample = sample->sample;
}

static void
check_for_update(web_api_t* api)
{
  printf("sending update check\r\n");
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_FIRMWARE_UPDATE_CHECK_REQUEST;
  msg->has_firmwareUpdateCheckRequest = true;
  sprintf(msg->firmwareUpdateCheckRequest.current_version, "%d.%d.%d", MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);

  send_api_msg(api->ws, msg);

  free(msg);
}

static void
start_update(web_api_t* api, const char* ver)
{
  printf("sending update start\r\n");
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_FIRMWARE_DOWNLOAD_REQUEST;
  msg->has_firmwareDownloadRequest = true;
  strncpy(msg->firmwareDownloadRequest.requested_version, ver,
      sizeof(msg->firmwareDownloadRequest.requested_version));

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
dispatch_api_msg(web_api_t* api, ApiMessage* msg)
{
  switch (msg->type) {
  case ApiMessage_Type_ACTIVATION_TOKEN_RESPONSE:
    printf("got activation token: %s\r\n", msg->activationTokenResponse.activation_token);
    strncpy(api->status.activation_token,
        msg->activationTokenResponse.activation_token,
        sizeof(api->status.activation_token));
    set_state(api, AS_AWAITING_ACTIVATION);
    break;

  case ApiMessage_Type_ACTIVATION_NOTIFICATION:
    printf("got auth token: %s\r\n", msg->activationNotification.auth_token);
    app_cfg_set_auth_token(msg->activationNotification.auth_token);
    set_state(api, AS_CONNECTED);
    break;

  case ApiMessage_Type_AUTH_RESPONSE:
    if (msg->authResponse.authenticated) {
      printf("auth succeeded\r\n");
      set_state(api, AS_CONNECTED);
    }
    else {
      printf("auth failed, restarting activation\r\n");
      app_cfg_set_auth_token("");
      request_activation_token(api);
      set_state(api, AS_REQUESTING_ACTIVATION_TOKEN);
    }
    break;

  case ApiMessage_Type_FIRMWARE_UPDATE_CHECK_RESPONSE:
    msg_send(MSG_API_FW_UPDATE_CHECK_RESPONSE, &msg->firmwareUpdateCheckResponse);
    break;

  case ApiMessage_Type_FIRMWARE_DOWNLOAD_RESPONSE:
    msg_send(MSG_API_FW_CHUNK, &msg->firmwareDownloadResponse);
    break;

  case ApiMessage_Type_DEVICE_SETTINGS_NOTIFICATION:
    dispatch_device_settings(&msg->deviceSettingsNotification);
    break;

  default:
    printf("Unsupported API message: %d\r\n", msg->type);
    break;
  }
}

static void
dispatch_device_settings(DeviceSettingsNotification* settings)
{
  int i;
  for (i = 0; i < settings->output_count; ++i) {
    output_settings_t os;
    OutputSettings* osm = &settings->output[i];

    os.compressor_delay.value = osm->compressor_delay;
    os.compressor_delay.unit = UNIT_TIME_MIN;
    os.function = osm->function;
    os.trigger = osm->trigger_probe_id;

    app_cfg_set_output_settings(osm->id, &os);
  }

  for (i = 0; i < settings->sensor_count; ++i) {
    sensor_settings_t ss;
    SensorSettings* ssm = &settings->sensor[i];

    ss.setpoint.value = ssm->setpoint;
    ss.setpoint.unit = UNIT_TEMP_DEG_C;

    app_cfg_set_sensor_settings(ssm->id, &ss);
  }
}

