
#ifndef WEB_API_PARSER_H
#define WEB_API_PARSER_H

#include "common.h"


#define WAPI_SYNC1 0xB3
#define WAPI_SYNC2 0xEB
#define WAPI_SYNC3 0x17


typedef enum {
  WAPI_VERSION,
  WAPI_ACTIVATION,
  WAPI_AUTH,
  WAPI_DEVICE_SETTINGS,
  WAPI_TEMP_PROFILE,
  WAPI_UPGRADE,
  WAPI_ACK,
  WAPI_NACK,
  WAPI_SENSOR_SAMPLE
} web_api_msg_id_t;

typedef enum {
  WAPI_PARSE_SYNC1,
  WAPI_PARSE_SYNC2,
  WAPI_PARSE_SYNC3,
  WAPI_PARSE_ID,
  WAPI_PARSE_LEN1,
  WAPI_PARSE_LEN2,
  WAPI_PARSE_LEN3,
  WAPI_PARSE_LEN4,
  WAPI_PARSE_DATA,
  WAPI_PARSE_CRC1,
  WAPI_PARSE_CRC2,
} web_api_parse_state_t;


typedef void (*web_api_msg_handler_t)(web_api_msg_id_t id, uint8_t* data, uint32_t data_len);


typedef struct {
  web_api_parse_state_t state;
  web_api_msg_handler_t msg_handler;

  uint8_t id;
  uint32_t data_len;
  uint32_t data_idx;
  uint8_t data[512];
  uint16_t crc_recv;
  uint16_t crc_calc;

  uint8_t crc_errors;
  uint8_t data_len_errors;
} web_api_parser_t;

web_api_parser_t*
web_api_parser_new(web_api_msg_handler_t msg_handler);

void
web_api_parse(web_api_parser_t* p, uint8_t c);

#endif
