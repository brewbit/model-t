
#include "web_api_parser.h"


web_api_parser_t*
web_api_parser_new(web_api_msg_handler_t msg_handler)
{
  web_api_parser_t* p = calloc(1, sizeof(web_api_parser_t));

  p->state = WAPI_PARSE_SYNC1;
  p->msg_handler = msg_handler;

  return p;
}

void
web_api_parse(web_api_parser_t* p, uint8_t c)
{
  switch (p->state) {
  case WAPI_PARSE_SYNC1:
    if (c == WAPI_SYNC1)
      p->state = WAPI_PARSE_SYNC2;
    break;

  case WAPI_PARSE_SYNC2:
    if (c == WAPI_SYNC2)
      p->state = WAPI_PARSE_SYNC3;
    else if (c != WAPI_SYNC1)
      p->state = WAPI_PARSE_SYNC1;
    break;

  case WAPI_PARSE_SYNC3:
    if (c == WAPI_SYNC3)
      p->state = WAPI_PARSE_ID;
    else if (c == WAPI_SYNC1)
      p->state = WAPI_PARSE_SYNC2;
    else
      p->state = WAPI_PARSE_SYNC1;
    break;

  case WAPI_PARSE_ID:
    p->id = c;
    p->crc_calc = crc16(0, c);

    p->state = WAPI_PARSE_LEN1;
    break;

  case WAPI_PARSE_LEN1:
    p->data_len = c;
    p->crc_calc = crc16(p->crc_calc, c);

    p->state = WAPI_PARSE_LEN2;
    break;

  case WAPI_PARSE_LEN2:
    p->data_len |= (c << 8);
    p->crc_calc = crc16(p->crc_calc, c);

    p->state = WAPI_PARSE_LEN3;
    break;

  case WAPI_PARSE_LEN3:
    p->data_len |= (c << 16);
    p->crc_calc = crc16(p->crc_calc, c);

    p->state = WAPI_PARSE_LEN4;
    break;

  case WAPI_PARSE_LEN4:
    p->data_len |= (c << 24);
    p->crc_calc = crc16(p->crc_calc, c);

    if (p->data_len == 0)
      p->state = WAPI_PARSE_CRC1;
    else if (p->data_len > sizeof(p->data)) {
      p->data_len_errors++;
      p->state = WAPI_PARSE_SYNC1;
    }
    else
      p->state = WAPI_PARSE_DATA;
    break;

  case WAPI_PARSE_DATA:
    p->data[p->data_idx++] = c;
    p->crc_calc = crc16(p->crc_calc, c);

    if (p->data_idx >= p->data_len)
      p->state = WAPI_PARSE_CRC1;
    break;

  case WAPI_PARSE_CRC1:
    p->crc_recv = c;
    p->state = WAPI_PARSE_CRC2;
    break;

  case WAPI_PARSE_CRC2:
    p->crc_recv |= (c << 8);

    if (p->crc_recv == p->crc_calc) {
      if (p->msg_handler != NULL)
        p->msg_handler(p->id, p->data, p->data_len);
    }
    else {
      chprintf(SD_STDIO, "web api crc error. recv %04x, expected %04x\r\n", p->crc_recv, p->crc_calc);
      p->crc_errors++;
    }

    p->state = WAPI_PARSE_SYNC1;
    break;

  default:
    p->state = WAPI_PARSE_SYNC1;
    break;
  }
}
