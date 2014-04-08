
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

#define SENSOR_REPORT_INTERVAL S2ST(5)
#define SETTINGS_UPDATE_DELAY  S2ST(1 * 60)


typedef enum {
  RECV_LEN,
  RECV_DATA
} parser_state_t;

typedef struct {
  bool new_sample;
  bool new_settings;
  quantity_t last_sample;
} api_sensor_status_t;

typedef struct {
  bool new_settings;
} api_output_status_t;

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
  api_sensor_status_t sensor_status[NUM_SENSORS];
  api_output_status_t output_status[NUM_OUTPUTS];
  systime_t last_sensor_report_time;
  systime_t last_settings_update_time;

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
dispatch_device_settings_from_device(web_api_t* api,
    controller_settings_msg_t* controller_settings_msg,
    output_settings_msg_t* output_settings_msg);

static void
send_device_settings(
    web_api_t* api);

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
dispatch_device_settings_from_server(DeviceSettingsNotification* settings);

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
  msg_subscribe(l, MSG_API_FW_DNLD_START, NULL);
  msg_subscribe(l, MSG_SENSOR_SAMPLE, NULL);
  msg_subscribe(l, MSG_CONTROLLER_SETTINGS, NULL);
  msg_subscribe(l, MSG_OUTPUT_SETTINGS, NULL);
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

      case MSG_CONTROLLER_SETTINGS:
        dispatch_device_settings_from_device(api, msg_data, NULL);
        break;

      case MSG_OUTPUT_SETTINGS:
        dispatch_device_settings_from_device(api, NULL, msg_data);
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

      if ((api->last_settings_update_time != 0) &&
          (chTimeNow() - api->last_settings_update_time) > SETTINGS_UPDATE_DELAY) {
        send_device_settings(api);
        api->last_settings_update_time = 0;
      }

      if (api->last_settings_update_time != 0) {
        printf("Settings update pending, but not yet sent\r\n");
      }
      break;
  }

  if (api->status.state > AS_CONNECTING)
    socket_poll(api);
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
  switch (api->parser.state) {
    case RECV_LEN:
      break;

    case RECV_DATA:
      break;
  }

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

    if (api->parser.bytes_remaining == 0) {
      switch (api->parser.state) {
        case RECV_LEN:
          api->parser.state = RECV_DATA;
          api->parser.data_len = ntohl(api->parser.data_len);
          api->parser.bytes_remaining = api->parser.data_len;
          api->parser.recv_buf = api->parser.data_buf;
          break;

        case RECV_DATA:
          api->parser.state = RECV_LEN;
          api->parser.bytes_remaining = 4;
          api->parser.recv_buf = (uint8_t*)&api->parser.data_len;

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
  msg->deviceReport.sensor_report_count = 0;

  for (i = 0; i < NUM_SENSORS; ++i) {
    if (api->sensor_status[i].new_sample) {
      api->sensor_status[i].new_sample = false;
      SensorReport* pr = &msg->deviceReport.sensor_report[msg->deviceReport.sensor_report_count];
      msg->deviceReport.sensor_report_count++;

      pr->id = i;
      pr->value = api->sensor_status[i].last_sample.value;
      pr->setpoint = temp_control_get_current_setpoint(i);
    }
  }

  if (msg->deviceReport.sensor_report_count > 0) {
    printf("sending sensor report %d\r\n", msg->deviceReport.sensor_report_count);
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
  if (ns->net_state == NS_CONNECTED)
    set_state(api, AS_CONNECTING);
  else
    set_state(api, AS_AWAITING_NET_CONNECTION);
}

static void
dispatch_sensor_sample(web_api_t* api, sensor_msg_t* sample)
{
  if (sample->sensor >= NUM_SENSORS)
    return;

  api_sensor_status_t* s = &api->sensor_status[sample->sensor];
  s->new_sample = true;
  s->last_sample = sample->sample;
}

static void
dispatch_device_settings_from_device(
    web_api_t* api,
    controller_settings_msg_t* ssm,
    output_settings_msg_t* osm)
{
  printf("settings updated\r\n");

  if (ssm != NULL)
    api->sensor_status[ssm->sensor].new_settings = true;

  if (osm != NULL)
    api->output_status[osm->output].new_settings = true;

  api->last_settings_update_time = chTimeNow();
}

static void
send_device_settings(
    web_api_t* api)
{
  int i;
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_DEVICE_SETTINGS_NOTIFICATION;
  msg->has_deviceSettingsNotification = true;

  for (i = 0; i < NUM_SENSORS; ++i) {
    if (api->sensor_status[i].new_settings) {
      const controller_settings_t* ssl = app_cfg_get_controller_settings(i);
      SensorSettings* ss = &msg->deviceSettingsNotification.sensor[i];
      msg->deviceSettingsNotification.sensor_count++;

      ss->id = i;
      switch (ssl->setpoint_type) {
        case SP_STATIC:
          ss->setpoint_type = SensorSettings_SetpointType_STATIC;
          ss->has_static_setpoint = true;
          ss->static_setpoint = ssl->static_setpoint.value;
          break;

        case SP_TEMP_PROFILE:
          ss->setpoint_type = SensorSettings_SetpointType_TEMP_PROFILE;
          ss->has_temp_profile_id = true;
          ss->temp_profile_id = ssl->temp_profile_id;
          break;

        default:
          printf("Invalid setpoint type: %d\r\n", ssl->setpoint_type);
          break;
      }
    }
  }

  for (i = 0; i < NUM_OUTPUTS; ++i) {
    if (api->output_status[i].new_settings) {
      const output_settings_t* osl = app_cfg_get_output_settings(i);
      OutputSettings* os = &msg->deviceSettingsNotification.output[i];
      msg->deviceSettingsNotification.output_count++;

      os->id = i;
      os->function = osl->function;
      switch (osl->output_mode) {
        case PID:
          os->output_mode = OutputSettings_OutputControlMode_PID;
          break;

        case ON_OFF:
          os->output_mode = OutputSettings_OutputControlMode_ON_OFF;
          break;

        default:
          printf("Invalid output control mode: %d\r\n", osl->output_mode);
          break;
      }
      os->cycle_delay = osl->cycle_delay.value;
      os->trigger_sensor_id = osl->trigger;
    }
  }

  if (msg->deviceSettingsNotification.sensor_count > 0 ||
      msg->deviceSettingsNotification.output_count > 0) {
    printf("Sending device settings %d %d\r\n",
        msg->deviceSettingsNotification.sensor_count,
        msg->deviceSettingsNotification.output_count);
    send_api_msg(api, msg);
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
start_update(web_api_t* api, const char* ver)
{
  printf("sending update start\r\n");
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_FIRMWARE_DOWNLOAD_REQUEST;
  msg->has_firmwareDownloadRequest = true;
  strncpy(msg->firmwareDownloadRequest.requested_version, ver,
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
      printf("send failed %d\r\n", ret);
      return false;
    }
    bytes_left -= ret;
    buf += ret;
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
    dispatch_device_settings_from_server(&msg->deviceSettingsNotification);
    break;

  default:
    printf("Unsupported API message: %d\r\n", msg->type);
    break;
  }
}

static void
dispatch_device_settings_from_server(DeviceSettingsNotification* settings)
{
  int i;

  printf("got device settings from server\r\n");

  temp_control_halt();

  temp_control_cmd_t* tcc = calloc(1, sizeof(temp_control_cmd_t));
  tcc->output_settings[OUTPUT_1].hysteresis.value = 1;
  tcc->output_settings[OUTPUT_1].hysteresis.unit = UNIT_TEMP_DEG_F;
  tcc->output_settings[OUTPUT_2].hysteresis.value = 1;
  tcc->output_settings[OUTPUT_2].hysteresis.unit = UNIT_TEMP_DEG_F;

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

  printf("  got %d output settings\r\n", settings->output_count);
  for (i = 0; i < (int)settings->output_count; ++i) {
    output_settings_t* os = &tcc->output_settings[i];
    OutputSettings* osm = &settings->output[i];

    os->cycle_delay.value = osm->cycle_delay;
    os->cycle_delay.unit = UNIT_TIME_MIN;
    os->function = osm->function;
    os->trigger = osm->trigger_sensor_id;

    printf("    output %d\r\n", i);
    printf("      delay %f\r\n", os->cycle_delay.value);
    printf("      function %d\r\n", os->function);
    printf("      trigger %d\r\n", os->trigger);
  }

  printf("  got %d sensor settings\r\n", settings->sensor_count);
  for (i = 0; i < (int)settings->sensor_count; ++i) {
    controller_settings_t* ss = &tcc->controller_settings[i];
    SensorSettings* ssm = &settings->sensor[i];

    switch (ssm->setpoint_type) {
      case SensorSettings_SetpointType_STATIC:
        if (!ssm->has_static_setpoint)
          printf("Sensor settings specified static setpoint, but none provided!\r\n");
        else {
          ss->setpoint_type = SP_STATIC;
          ss->static_setpoint.value = ssm->static_setpoint;
          ss->static_setpoint.unit = UNIT_TEMP_DEG_F;
        }
        break;

      case SensorSettings_SetpointType_TEMP_PROFILE:
        if (!ssm->has_temp_profile_id)
          printf("Sensor settings specified temp profile, but no provided!\r\n");
        else {
          ss->setpoint_type = SP_TEMP_PROFILE;
          ss->temp_profile_id = ssm->temp_profile_id;
        }
        break;

      default:
        printf("Invalid setpoint type: %d\r\n", ssm->setpoint_type);
        break;
    }

    printf("    sensor %d\r\n", i);
    printf("      setpoint_type %d\r\n", ss->setpoint_type);
    printf("      static %f\r\n", ss->static_setpoint.value);
    printf("      temp profile %d\r\n", (int)ss->temp_profile_id);
  }

  temp_control_start(tcc);
  free(tcc);
}

