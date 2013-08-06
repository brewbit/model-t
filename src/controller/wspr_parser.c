
#include "ch.h"
#include "hal.h"
#include "wspr_parser.h"
#include "datastream.h"
#include "chprintf.h"

#include <stdlib.h>
#include <string.h>

// sync1, sync2, id, len1, len2, cksum1-3
#define PKT_OVERHEAD 9

void
wspr_parser_init(wspr_parser_t* p, wspr_pkt_handler handler, void* handler_arg)
{
  memset(p, 0, sizeof(wspr_parser_t));
  p->state = PS_SYNC1;
  p->handler = handler;
  p->handler_arg = handler_arg;
}

void
wspr_parse(wspr_parser_t* p, uint8_t c)
{
  switch (p->state) {
    case PS_SYNC1:
      if (c == WSPR_SYNC1_CHAR)
        p->state = PS_SYNC2;
      break;

    case PS_SYNC2:
      if (c == WSPR_SYNC2_CHAR)
        p->state = PS_ID;
      else if (c != WSPR_SYNC1_CHAR)
        p->state = PS_SYNC1;
      break;

    case PS_ID:
      p->id = c;
      p->cksum = c;
      p->state = PS_LEN1;
      break;

    case PS_LEN1:
      p->data_len = c;
      p->cksum += c;
      p->state = PS_LEN2;
      break;

    case PS_LEN2:
      p->data_len |= (c << 8);
      p->cksum += c;
      p->data_rcvd = 0;

      if (p->data_len > MAX_DATA)
        p->state = PS_SYNC1;
      else if (p->data_len == 0)
        p->state = PS_CKSUM;
      else
        p->state = PS_DATA;
      break;

    case PS_DATA:
      p->data[p->data_rcvd++] = c;
      p->cksum += c;

      if (p->data_rcvd >= p->data_len)
        p->state = PS_CKSUM;
      break;

    case PS_CKSUM:
      if (p->cksum == c && p->handler != NULL)
        p->handler(p->handler_arg, p->id, p->data, p->data_len);
      else
        chprintf((BaseChannel*)&SD3, "wspr chksum failed\r\n");
      p->state = PS_SYNC1;
      break;
  }
}
