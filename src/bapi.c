
#include "ch.h"
#include "hal.h"

#include "bapi.h"
#include "cJSON.h"
#include "wspr_http.h"
#include "terminal.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>


#define DEVICE_ID "595f44fec1e92a71d3e9e77456ba80d1"
#define BAPI_HOST "brewbit.herokuapp.com"
#define BAPI_PORT 80


typedef enum {
  BAPI_GET_ACTIVATION_TOKEN,
  BAPI_GET_AUTH_TOKEN,
  BAPI_GET_ACCT_INFO,
  BAPI_POST_TEMP,
  BAPI_IDLE,
} bapi_state_t;

struct bapi_s;
typedef void (*bapi_response_handler_t)(struct bapi_s* bapi, cJSON* response);

typedef struct bapi_s {
  bapi_state_t state;
  Semaphore cmd_sem;
  char* activation_token;
  char* auth_token;
  bapi_acct_info_t acct_info;

  bapi_response_handler_t response_handler;
} bapi_t;

static void bapi_get_activation_token(bapi_t* bapi);
static void bapi_complete_get_activation_token(bapi_t* bapi, cJSON* response);

static void bapi_get_auth_token(bapi_t* bapi);
static void bapi_complete_get_auth_token(bapi_t* bapi, cJSON* response);

static void bapi_get_acct_info(bapi_t* bapi);
static void bapi_complete_get_acct_info(bapi_t* bapi, cJSON* response);

static void bapi_post_temp(bapi_t* bapi);
static void bapi_complete_post_temp(bapi_t* bapi, cJSON* acct_response);

static void bapi_request(bapi_t* bapi, char* endpoint, http_method_t method, cJSON* request, bapi_response_handler_t handler);
static void bapi_request_complete(void* arg, http_response_t* response);

static msg_t bapi_thread(void* arg);


static bapi_t bapi;
static uint8_t wp_bapi_thread[2500];

void
bapi_init()
{
  chSemInit(&bapi.cmd_sem, 0);
  bapi.state = BAPI_GET_ACTIVATION_TOKEN;

  chThdCreateStatic(wp_bapi_thread, sizeof(wp_bapi_thread), NORMALPRIO, bapi_thread, &bapi);
}

static msg_t
bapi_thread(void* arg)
{
  bapi_t* bapi = arg;

  chprintf(stdout, "bapi thread started\r\n");

  while (1) {
    switch (bapi->state) {
      case BAPI_GET_ACTIVATION_TOKEN:
        bapi_get_activation_token(bapi);
        break;

      case BAPI_GET_AUTH_TOKEN:
        bapi_get_auth_token(bapi);
        break;

      case BAPI_GET_ACCT_INFO:
        bapi_get_acct_info(bapi);
        break;

      case BAPI_POST_TEMP:
        bapi_post_temp(bapi);
        break;

      case BAPI_IDLE:
        break;
    }
    chThdSleepSeconds(1);
  }

  return 0;
}

static void
bapi_get_activation_token(bapi_t* bapi)
{
  cJSON* token_request = cJSON_CreateObject();
  cJSON_AddItemToObject(token_request, "device_id", cJSON_CreateString(DEVICE_ID));

  chprintf(stdout, "making activation request\r\n");
  bapi_request(bapi, "/api/v1/activation/new", HTTP_GET, token_request, bapi_complete_get_activation_token);
  cJSON_Delete(token_request);

  chSemWaitTimeoutS(&bapi->cmd_sem, 30000);
}

static void
bapi_complete_get_activation_token(bapi_t* bapi, cJSON* token_response)
{
  chprintf(stdout, "got activation response\r\n");
  if (token_response) {
    cJSON* activation_token = cJSON_GetObjectItem(token_response, "activation_token");
    if (activation_token != NULL) {
      bapi->activation_token = strdup(activation_token->valuestring);

      chprintf(stdout, "activation token is: %s\r\n", bapi->activation_token);
      bapi->state = BAPI_GET_AUTH_TOKEN;
      chSemSignal(&bapi->cmd_sem);
    }
  }
}

static void
bapi_get_auth_token(bapi_t* bapi)
{
  cJSON* token_request = cJSON_CreateObject();
  cJSON_AddItemToObject(token_request, "device_id", cJSON_CreateString(DEVICE_ID));
  cJSON_AddItemToObject(token_request, "activation_token", cJSON_CreateString(bapi->activation_token));

  chprintf(stdout, "making auth request\r\n");
  bapi_request(bapi, "/api/v1/activation", HTTP_POST, token_request, bapi_complete_get_auth_token);
  cJSON_Delete(token_request);

  chSemWaitTimeoutS(&bapi->cmd_sem, 30000);
}

static void
bapi_complete_get_auth_token(bapi_t* bapi, cJSON* token_response)
{
  chprintf(stdout, "got auth response\r\n");
  if (token_response) {
    cJSON* auth_token = cJSON_GetObjectItem(token_response, "auth_token");
    if ((auth_token != NULL) &&
        (auth_token->type != cJSON_NULL)) {
      bapi->auth_token = strdup(auth_token->valuestring);

      chprintf(stdout, "token is: %s\r\n", bapi->auth_token);
      bapi->state = BAPI_GET_ACCT_INFO;
      chSemSignal(&bapi->cmd_sem);
    }
  }
}

static void
bapi_get_acct_info(bapi_t* bapi)
{
  cJSON* acct_request = cJSON_CreateObject();
  cJSON_AddItemToObject(acct_request, "auth_token", cJSON_CreateString(bapi->auth_token));

  chprintf(stdout, "making acct info request\r\n");
  bapi_request(bapi, "/api/v1/account", HTTP_GET, acct_request, bapi_complete_get_acct_info);
  cJSON_Delete(acct_request);

  chSemWaitTimeoutS(&bapi->cmd_sem, 30000);
}

static void
bapi_complete_get_acct_info(bapi_t* bapi, cJSON* acct_response)
{
  chprintf(stdout, "got acct info:\r\n");
  if (acct_response) {
    cJSON* user = cJSON_GetObjectItem(acct_response, "user");
    if (user) {
      cJSON* email = cJSON_GetObjectItem(user, "email");
      if (email)
        bapi->acct_info.email = strdup(email->valuestring);

      cJSON* name = cJSON_GetObjectItem(user, "name");
      if (name)
        bapi->acct_info.name = strdup(name->valuestring);

      chprintf(stdout, "  email: %s\r\n", bapi->acct_info.email);
      chprintf(stdout, "  name: %s\r\n", bapi->acct_info.name);
      bapi->state = BAPI_POST_TEMP;
      chSemSignal(&bapi->cmd_sem);
    }
  }
}

static void
bapi_post_temp(bapi_t* bapi)
{
  cJSON* temp_post = cJSON_CreateObject();
  cJSON_AddItemToObject(temp_post, "auth_token", cJSON_CreateString(bapi->auth_token));
  cJSON_AddItemToObject(temp_post, "last_known_temperature", cJSON_CreateNumber(69.69));

  chprintf(stdout, "making temp post request\r\n");
  bapi_request(bapi, "/api/v1/devices/12", HTTP_PUT, temp_post, bapi_complete_post_temp);
  cJSON_Delete(temp_post);

  chSemWaitTimeoutS(&bapi->cmd_sem, 30000);
}

static void
bapi_complete_post_temp(bapi_t* bapi, cJSON* acct_response)
{
  chprintf(stdout, "got temp post response:\r\n");
  bapi->state = BAPI_POST_TEMP;
}

static void
bapi_request(bapi_t* bapi, char* endpoint, http_method_t method, cJSON* request, bapi_response_handler_t handler)
{
  char* request_str = cJSON_PrintUnformatted(request);

  bapi->response_handler = handler;

  http_header_t headers[] = {
      {.name = "Accept",       .value = "application/json" },
      {.name = "Content-type", .value = "application/json" },
  };
  http_request_t http_request = {
    .host = BAPI_HOST,
    .port = BAPI_PORT,
    .method = method,
    .url = endpoint,
    .num_headers = 2,
    .headers = headers,
    .request = request_str
  };
  wspr_http_request(&http_request, bapi_request_complete, bapi);
  free(request_str);
}

static void
bapi_request_complete(void* arg, http_response_t* response)
{
  bapi_t* bapi = arg;
  cJSON* json_response = NULL;

  if ((response->result == 0) &&
      (response->response_code == 200) &&
      (response->response_body != NULL)) {
    json_response = cJSON_Parse(response->response_body);
  }

  if (bapi->response_handler != NULL)
    bapi->response_handler(bapi, json_response);

  if (json_response != NULL)
    cJSON_Delete(json_response);
}
