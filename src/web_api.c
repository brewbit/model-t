
#include "web_api.h"
#include "cJSON.h"

#include <stdlib.h>
#include <string.h>


static cJSON* bbtc_request(char* endpoint, cJSON* request);

//  char* token = NULL;
//  while (token == NULL) {
//    token = bbtc_get_token();
//  }
//  bbtc_acct_info_t* acct_info = bbtc_get_acct_info(token);
//
//  free(token);
//  free(acct_info->email);
//  free(acct_info->name);
//  free(acct_info);

char*
bbtc_get_token()
{
  char* token = NULL;

  cJSON* token_request = cJSON_CreateObject();
  cJSON_AddItemToObject(token_request, "username", cJSON_CreateString("test@test.com"));
  cJSON_AddItemToObject(token_request, "password", cJSON_CreateString("test123test"));

  cJSON* token_response = bbtc_request("/api/v1/account/token", token_request);
  cJSON_Delete(token_request);

  if (token_response) {
    int token_len = strlen(token_response->child->valuestring);
    token = calloc(token_len + 1, sizeof(char));
    memcpy(token, token_response->child->valuestring, token_len);

    cJSON_Delete(token_response);
  }

  return token;
}

bbtc_acct_info_t*
bbtc_get_acct_info(char* token)
{
  bbtc_acct_info_t* acct_info = NULL;

  cJSON* acct_request = cJSON_CreateObject();
  cJSON_AddItemToObject(acct_request, "auth_token", cJSON_CreateString(token));

  cJSON* acct_response = bbtc_request("/api/v1/account", acct_request);
  cJSON_Delete(acct_request);

  if (acct_response) {
    cJSON* user = cJSON_GetObjectItem(acct_response, "user");
    if (user) {
      acct_info = calloc(1, sizeof(bbtc_acct_info_t));
      cJSON* email = cJSON_GetObjectItem(user, "email");
      if (email)
        acct_info->email = strdup(email->valuestring);

      cJSON* name = cJSON_GetObjectItem(user, "name");
      if (name)
        acct_info->name = strdup(name->valuestring);
    }

    cJSON_Delete(acct_response);
  }

  return acct_info;
}

static cJSON*
bbtc_request(char* endpoint, cJSON* request)
{
  cJSON* response = NULL;
//  char* request_str = cJSON_PrintUnformatted(request);
//
//  http_header_t headers[] = {
//      {.name = "Accept",       .value = "application/json" },
//      {.name = "Content-type", .value = "application/json" },
//  };
//  http_status_code_t response_code;
//  char* http_response = NULL;
//  http_get("brewbit.herokuapp.com", 80, endpoint, headers, 2, request_str, &response_code, &http_response);
//  free(request_str);
//
//  if (http_response) {
//    response = cJSON_Parse(http_response);
//    free(http_response);
//  }

  return response;
}
