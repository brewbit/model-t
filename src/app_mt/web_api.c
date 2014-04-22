
#include <ch.h>
#include <hal.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <pb_encode.h>
#include <pb_decode.h>

#include "web_api.h"
#include "bbmt.pb.h"
#include "message.h"
#include "net.h"
#include "sensor.h"
#include "app_cfg.h"
#include "temp_control.h"
#include "app_cfg.h"
#include "ota_update.h"

#ifndef WEB_API_HOST
#define WEB_API_HOST_STR "dg.brewbit.com"
#else
#define xstr(s) str(s)
#define str(s) #s
#define WEB_API_HOST_STR xstr(WEB_API_HOST)
#endif

#ifndef WEB_API_PORT
#define WEB_API_PORT 31337
#endif

#define SENSOR_REPORT_INTERVAL S2ST(5)
#define SETTINGS_UPDATE_DELAY  S2ST(1 * 60)
#define MIN_SEND_INTERVAL      S2ST(10)
#define RECV_TIMEOUT           S2ST(20)
#define CC3000_FUCKED_TIMEOUT  S2ST(2 * 60)
#define MAX_SEND_ERRS          25


typedef enum {
  RECV_LEN,
  RECV_DATA
} parser_state_t;

typedef struct {
  bool new_sample;
  bool new_settings;
  quantity_t last_sample;
} api_controller_status_t;

typedef struct {
  parser_state_t state;

  uint8_t* recv_buf;
  uint32_t bytes_remaining;

  uint32_t data_len;
  uint8_t data_buf[ApiMessage_size];
} msg_parser_t;

typedef struct {
  int socket;
  api_status_t status;
  bool new_device_settings;
  api_controller_status_t controller_status[NUM_SENSORS];
  systime_t last_sensor_report_time;
  systime_t last_send_time;
  systime_t last_recv_time;
  uint32_t send_errors;

  msg_parser_t parser;
} web_api_t;


static void
set_state(web_api_t* api, api_state_t state);

static void
web_api_dispatch(msg_id_t id, void* msg_data, void* listener_data, void* sub_data);

static void
web_api_idle(web_api_t* api);

static void
socket_message_rx(web_api_t* api, const uint8_t* data, uint32_t data_len);

static void
send_api_msg(web_api_t* api, ApiMessage* msg);

static void
dispatch_api_msg(web_api_t* api, ApiMessage* msg);

static void
dispatch_net_status(web_api_t* api, net_status_t* ns);

static void
dispatch_sensor_sample(web_api_t* api, sensor_msg_t* sample);

static void
dispatch_device_settings_from_device(
    web_api_t* api,
    output_ctrl_t* control_mode);

static void
dispatch_controller_settings_from_device(web_api_t* api,
    controller_settings_msg_t* controller_settings_msg);

static void
send_device_settings(
    web_api_t* api);

static void
send_controller_settings(
    web_api_t* api);

static void
request_activation_token(web_api_t* api);

static void
request_auth(web_api_t* api);

static void
check_for_update(web_api_t* api);

static void
dispatch_firmware_rqst(web_api_t* api, firmware_update_t* firmware_data);

static void
send_sensor_report(web_api_t* api);

static void
dispatch_device_settings_from_server(DeviceSettings* settings);

static void
dispatch_controller_settings_from_server(ControllerSettings* settings);

static bool
socket_connect(web_api_t* api, const char* hostname, uint16_t port);

static void
socket_poll(web_api_t* api);

static bool
send_all(web_api_t* api, void* buf, uint32_t buf_len);


static web_api_t* api;


void
web_api_init()
{
  api = calloc(1, sizeof(web_api_t));
  api->status.state = AS_AWAITING_NET_CONNECTION;

  msg_listener_t* l = msg_listener_create("web_api", 2048, web_api_dispatch, api);
  msg_listener_set_idle_timeout(l, 500);
  msg_listener_enable_watchdog(l, 3 * 60 * 1000);

  msg_subscribe(l, MSG_NET_STATUS, NULL);
  msg_subscribe(l, MSG_API_FW_UPDATE_CHECK, NULL);
  msg_subscribe(l, MSG_API_FW_DNLD_RQST, NULL);
  msg_subscribe(l, MSG_SENSOR_SAMPLE, NULL);
  msg_subscribe(l, MSG_CONTROLLER_SETTINGS, NULL);
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

      case MSG_API_FW_DNLD_RQST:
        dispatch_firmware_rqst(api, msg_data);
        break;

      case MSG_CONTROL_MODE:
        dispatch_device_settings_from_device(api, msg_data);
        break;

      case MSG_CONTROLLER_SETTINGS:
        dispatch_controller_settings_from_device(api, msg_data);
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

static void
web_api_idle(web_api_t* api)
{
  switch (api->status.state) {
    case AS_AWAITING_NET_CONNECTION:
      /* do nothing, wait for net to come up */
      break;

    case AS_CONNECTING:
      printf("Connecting to: %s:%d\r\n", WEB_API_HOST_STR, WEB_API_PORT);
      if (socket_connect(api, WEB_API_HOST_STR, WEB_API_PORT)) {
        api->last_recv_time = chTimeNow();
        api->send_errors = 0;

        api->parser.state = RECV_LEN;
        api->parser.bytes_remaining = 4;
        api->parser.recv_buf = (uint8_t*)&api->parser.data_len;

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

      if (api->new_device_settings) {
        send_device_settings(api);
        api->new_device_settings = false;
      }

      send_controller_settings(api);
      break;
  }

  if (api->status.state > AS_CONNECTING) {
    /* If we haven't heard from the server in a while, disconnect and try again */
    if ((chTimeNow() - api->last_recv_time) > RECV_TIMEOUT) {
      printf("Server timed out\r\n");
      closesocket(api->socket);
      api->socket = -1;
      set_state(api, AS_CONNECTING);
      return;
    }

    /* If we haven't sent anything to the server in a while, send a keepalive */
    if ((chTimeNow() - api->last_send_time) > MIN_SEND_INTERVAL) {
      uint32_t keepalive = 0;
      send_all(api, &keepalive, 4);
    }

    socket_poll(api);
  }

  if (api->status.state > AS_AWAITING_NET_CONNECTION) {
    /* If we haven't heard from the server in a REALLY long time, reset the WiFi module and try again */
    if ((chTimeNow() - api->last_recv_time) > CC3000_FUCKED_TIMEOUT) {
      printf("CC3000 is fucked, restarting\r\n");
      closesocket(api->socket);
      api->socket = -1;
      set_state(api, AS_AWAITING_NET_CONNECTION);

      msg_post(MSG_NET_RESET, NULL);
    }
  }
}

static bool
socket_connect(web_api_t* api, const char* hostname, uint16_t port)
{
  uint32_t hostaddr;
  int ret = gethostbyname(hostname, strlen(hostname), &hostaddr);
  if (ret < 0) {
    printf("gethostbyname failed %d\r\n", ret);
    return false;
  }

  api->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (api->socket < 0) {
    printf("Connect failed %d\r\n", api->socket);
    return false;
  }

  int optval = SOCK_ON;
  ret = setsockopt(api->socket, SOL_SOCKET, SOCKOPT_RECV_NONBLOCK, (char *)&optval, sizeof(optval));
  if (ret < 0) {
    closesocket(api->socket);
    api->socket = -1;
    printf("setsockopt failed %d\r\n", ret);
    return false;
  }

  sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons(port),
    .sin_addr.s_addr = htonl(hostaddr)
  };
  ret = connect(api->socket, (sockaddr*)&addr, sizeof(addr));
  if (ret < 0) {
    closesocket(api->socket);
    api->socket = -1;
    printf("connect failed %d\r\n", ret);
    return false;
  }

  return true;
}

static void
socket_poll(web_api_t* api)
{
  int ret = recv(api->socket, api->parser.recv_buf, api->parser.bytes_remaining, 0);
  if (ret < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      printf("recv failed %d %d\r\n", ret, errno);
      printf("socket disconnected\r\n");
      closesocket(api->socket);
      api->socket = -1;
      set_state(api, AS_CONNECTING);
    }
  }
  else {
    api->parser.bytes_remaining -= ret;
    api->parser.recv_buf += ret;
    if (ret > 0)
      api->last_recv_time = chTimeNow();

    if (api->parser.bytes_remaining == 0) {
      switch (api->parser.state) {
        case RECV_LEN:
          api->parser.data_len = ntohl(api->parser.data_len);
          if (api->parser.data_len > 0) {
            api->parser.state = RECV_DATA;
            api->parser.bytes_remaining = api->parser.data_len;
            api->parser.recv_buf = api->parser.data_buf;
          }
          else {
            api->parser.state = RECV_LEN;
            api->parser.bytes_remaining = 4;
            api->parser.recv_buf = (uint8_t*)&api->parser.data_len;
          }
          break;

        case RECV_DATA:
          api->parser.bytes_remaining = 4;
          api->parser.recv_buf = (uint8_t*)&api->parser.data_len;
          api->parser.state = RECV_LEN;

          socket_message_rx(api, api->parser.data_buf, api->parser.data_len);
          break;
      }
    }
  }
}

static void
send_sensor_report(web_api_t* api)
{
  int i;
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_DEVICE_REPORT;
  msg->has_deviceReport = true;
  msg->deviceReport.controller_reports_count = 0;

  for (i = 0; i < NUM_SENSORS; ++i) {
    if (api->controller_status[i].new_sample) {
      api->controller_status[i].new_sample = false;
      ControllerReport* pr = &msg->deviceReport.controller_reports[msg->deviceReport.controller_reports_count];
      msg->deviceReport.controller_reports_count++;

      pr->controller_index = i;
      pr->sensor_reading = api->controller_status[i].last_sample.value;
      pr->setpoint = temp_control_get_current_setpoint(i);
    }
  }

  if (msg->deviceReport.controller_reports_count > 0) {
    printf("sending sensor report %d\r\n", msg->deviceReport.controller_reports_count);
    send_api_msg(api, msg);
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

  send_api_msg(api, msg);

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

  send_api_msg(api, msg);

  free(msg);
}

static void
dispatch_net_status(web_api_t* api, net_status_t* ns)
{
  if (ns->dhcp_resolved) {
    if (api->status.state == AS_AWAITING_NET_CONNECTION)
      set_state(api, AS_CONNECTING);
  }
  else {
    set_state(api, AS_AWAITING_NET_CONNECTION);
  }
}

static void
dispatch_sensor_sample(web_api_t* api, sensor_msg_t* sample)
{
  if (sample->sensor >= NUM_SENSORS)
    return;

  api_controller_status_t* s = &api->controller_status[sample->sensor];
  s->new_sample = true;
  s->last_sample = sample->sample;
}

static void
dispatch_device_settings_from_device(
    web_api_t* api,
    output_ctrl_t* control_mode)
{
  printf("device settings updated\r\n");

  if (control_mode != NULL)
    api->new_device_settings = true;
}

static void
dispatch_controller_settings_from_device(
    web_api_t* api,
    controller_settings_msg_t* ssm)
{
  printf("controller settings updated\r\n");

  if (ssm != NULL)
    api->controller_status[ssm->controller].new_settings = true;
}

static void
send_device_settings(
    web_api_t* api)
{
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_DEVICE_SETTINGS;
  msg->has_deviceSettings = true;

  msg->deviceSettings.name[0] = 0;

  output_ctrl_t control_mode = app_cfg_get_control_mode();
  switch (control_mode) {
    case PID:
      msg->deviceSettings.control_mode = DeviceSettings_ControlMode_PID;
      break;

    case ON_OFF:
      msg->deviceSettings.control_mode = DeviceSettings_ControlMode_ON_OFF;
      break;

    default:
      printf("Invalid output control mode: %d\r\n", control_mode);
      break;
  }

  printf("Sending device settings\r\n");
  send_api_msg(api, msg);

  free(msg);
}

static void
send_controller_settings(
    web_api_t* api)
{
  int i;
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_CONTROLLER_SETTINGS;
  msg->has_controllerSettings = true;

  for (i = 0; i < NUM_CONTROLLERS; ++i) {
    if (api->controller_status[i].new_settings) {
      api->controller_status[i].new_settings = false;
      const controller_settings_t* ssl = app_cfg_get_controller_settings(i);
      ControllerSettings* ss = &msg->controllerSettings;

      ss->sensor_index = i;
      switch (ssl->setpoint_type) {
        case SP_STATIC:
          ss->setpoint_type = ControllerSettings_SetpointType_STATIC;
          ss->has_static_setpoint = true;
          ss->static_setpoint = ssl->static_setpoint.value;
          break;

        case SP_TEMP_PROFILE:
          ss->setpoint_type = ControllerSettings_SetpointType_TEMP_PROFILE;
          ss->has_temp_profile_id = true;
          ss->temp_profile_id = ssl->temp_profile_id;
          break;

        default:
          printf("Invalid setpoint type: %d\r\n", ssl->setpoint_type);
          break;
      }

      int j;
      for (j = 0; j < NUM_OUTPUTS; ++j) {
        const output_settings_t* osl = &ssl->output_settings[i];
        if (ssl->output_settings[i].enabled) {
          OutputSettings* os = &msg->controllerSettings.output_settings[msg->controllerSettings.output_settings_count];
          msg->controllerSettings.output_settings_count++;

          os->index = i;
          os->function = osl->function;
          os->cycle_delay = osl->cycle_delay.value;
        }
      }

      printf("Sending controller settings\r\n");
      send_api_msg(api, msg);
    }
  }

  free(msg);
}

static void
check_for_update(web_api_t* api)
{
  printf("sending update check\r\n");
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_FIRMWARE_UPDATE_CHECK_REQUEST;
  msg->has_firmwareUpdateCheckRequest = true;
  sprintf(msg->firmwareUpdateCheckRequest.current_version, "%d.%d.%d", MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);

  send_api_msg(api, msg);

  free(msg);
}

static void
dispatch_firmware_rqst(web_api_t* api, firmware_update_t* firmware_data)
{
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_FIRMWARE_DOWNLOAD_REQUEST;
  msg->has_firmwareDownloadRequest = true;
  msg->firmwareDownloadRequest.offset = firmware_data->offset;
  msg->firmwareDownloadRequest.size = firmware_data->size;
  strncpy(msg->firmwareDownloadRequest.requested_version,
      firmware_data->version,
      sizeof(msg->firmwareDownloadRequest.requested_version));

  send_api_msg(api, msg);

  free(msg);
}

static void
send_api_msg(web_api_t* api, ApiMessage* msg)
{
  uint8_t* buffer = malloc(ApiMessage_size);

  pb_ostream_t stream = pb_ostream_from_buffer(buffer, ApiMessage_size);
  bool encoded_ok = pb_encode(&stream, ApiMessage_fields, msg);

  if (encoded_ok) {
    uint32_t buf_len = htonl(stream.bytes_written);
    if (send_all(api, &buf_len, sizeof(buf_len))) {
      if (!send_all(api, buffer, stream.bytes_written)) {
        printf("buffer send failed!\r\n");
      }
    }
    else {
      printf("message length send failed!\r\n");
    }
  }

  free(buffer);
}

static bool
send_all(web_api_t* api, void* buf, uint32_t buf_len)
{
  int bytes_left = buf_len;
  while (bytes_left > 0) {
    int ret = send(api->socket, buf, bytes_left, 0);
    if (ret < 0) {
      printf("send failed %d %d\r\n", ret, errno);
      if ((errno != EAGAIN && errno != EWOULDBLOCK) ||
          (++api->send_errors > MAX_SEND_ERRS)) {
        printf("socket disconnected %d\r\n", (int)api->send_errors);
        closesocket(api->socket);
        api->socket = -1;
        set_state(api, AS_CONNECTING);
      }
      return false;
    }
    bytes_left -= ret;
    buf += ret;
    api->last_send_time = chTimeNow();
  }

  return true;
}

static void
socket_message_rx(web_api_t* api, const uint8_t* data, uint32_t data_len)
{
  ApiMessage* msg = malloc(sizeof(ApiMessage));

  pb_istream_t stream = pb_istream_from_buffer((const uint8_t*)data, data_len);
  bool status = pb_decode(&stream, ApiMessage_fields, msg);

  if (status)
    dispatch_api_msg(api, msg);
  else
    printf("Fucked up message received!\r\n");

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

  case ApiMessage_Type_DEVICE_SETTINGS:
    dispatch_device_settings_from_server(&msg->deviceSettings);
    break;

  case ApiMessage_Type_CONTROLLER_SETTINGS:
    dispatch_controller_settings_from_server(&msg->controllerSettings);
    break;

  default:
    printf("Unsupported API message: %d\r\n", msg->type);
    break;
  }
}

static void
dispatch_device_settings_from_server(DeviceSettings* settings)
{
  printf("got device settings from server\r\n");
  printf("  control mode %d\r\n", settings->control_mode);
  printf("  hysteresis %f\r\n", settings->hysteresis);

  app_cfg_set_control_mode(settings->control_mode);

  quantity_t hysteresis;
  hysteresis.value = settings->hysteresis;
  hysteresis.unit = UNIT_TEMP_DEG_F;
  app_cfg_set_hysteresis(hysteresis);
}

static void
dispatch_controller_settings_from_server(ControllerSettings* settings)
{
  int i;

  printf("got controller settings from server\r\n");

  temp_control_halt(settings->sensor_index);

  controller_settings_t* csl = calloc(1, sizeof(controller_settings_t));
  memcpy(csl, app_cfg_get_controller_settings(settings->sensor_index), sizeof(controller_settings_t));

  csl->controller = settings->sensor_index;

  printf("  got %d temp profiles\r\n", settings->temp_profiles_count);
  for (i = 0; i < (int)settings->temp_profiles_count; ++i) {
    temp_profile_t tp;
    TempProfile* tpm = &settings->temp_profiles[i];

    tp.id = tpm->id;
    strncpy(tp.name, tpm->name, sizeof(tp.name));
    tp.num_steps = tpm->steps_count;
    tp.start_value.value = tpm->start_value;
    tp.start_value.unit = UNIT_TEMP_DEG_F;

    printf("    profile '%s' (%d)\r\n", tp.name, (int)tp.id);
    printf("      steps %d\r\n", (int)tp.num_steps);
    printf("      start %f\r\n", tp.start_value.value);

    int j;
    for (j = 0; j < (int)tpm->steps_count; ++j) {
      temp_profile_step_t* step = &tp.steps[j];
      TempProfileStep* stepm = &tpm->steps[j];

      step->duration = stepm->duration;
      step->value.value = stepm->value;
      step->value.unit = UNIT_TEMP_DEG_F;
      switch(stepm->type) {
        case TempProfileStep_TempProfileStepType_HOLD:
          step->type = STEP_HOLD;
          break;

        case TempProfileStep_TempProfileStepType_RAMP:
          step->type = STEP_RAMP;
          break;

        default:
          printf("Invalid step type: %d\r\n", stepm->type);
          break;
      }
    }

    app_cfg_set_temp_profile(&tp, i);
  }

  printf("  got %d output settings\r\n", settings->output_settings_count);
  csl->output_settings[OUTPUT_1].enabled = false;
  csl->output_settings[OUTPUT_2].enabled = false;
  for (i = 0; i < (int)settings->output_settings_count; ++i) {
    OutputSettings* osm = &settings->output_settings[i];
    output_settings_t* os = &csl->output_settings[osm->index];

    os->cycle_delay.value = osm->cycle_delay;
    os->cycle_delay.unit = UNIT_TIME_MIN;
    os->function = osm->function;
    os->enabled = true;

    printf("    output %d\r\n", i);
    printf("      delay %f\r\n", os->cycle_delay.value);
    printf("      function %d\r\n", os->function);
  }

  printf("  got sensor settings\r\n");

  switch (settings->setpoint_type) {
    case ControllerSettings_SetpointType_STATIC:
      if (!settings->has_static_setpoint)
        printf("Sensor settings specified static setpoint, but none provided!\r\n");
      else {
        csl->setpoint_type = SP_STATIC;
        csl->static_setpoint.value = settings->static_setpoint;
        csl->static_setpoint.unit = UNIT_TEMP_DEG_F;
      }
      break;

    case ControllerSettings_SetpointType_TEMP_PROFILE:
      if (!settings->has_temp_profile_id)
        printf("Sensor settings specified temp profile, but no provided!\r\n");
      else {
        csl->setpoint_type = SP_TEMP_PROFILE;
        csl->temp_profile_id = settings->temp_profile_id;
      }
      break;

    default:
      printf("Invalid setpoint type: %d\r\n", settings->setpoint_type);
      break;
  }

  printf("    sensor %d\r\n", csl->controller);
  printf("      setpoint_type %d\r\n", csl->setpoint_type);
  printf("      static %f\r\n", csl->static_setpoint.value);
  printf("      temp profile %d\r\n", (int)csl->temp_profile_id);

  temp_control_start(csl);
  free(csl);
}

