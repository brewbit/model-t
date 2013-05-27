
#include "ch.h"
#include "hal.h"

#include "wspr.h"
#include "wspr_http.h"
#include "datastream.h"
#include "wspr_parser.h"
#include "terminal.h"
#include "txn.h"

#include <stdlib.h>
#include <string.h>


static txn_cache_t* http_txns;


static void
handle_http_result(uint8_t* data, uint16_t data_len);


void
wspr_http_init()
{
  http_txns = txn_cache_new();
  wspr_set_handler(WSPR_OUT_HTTP_REQ, handle_http_result);
}

void
wspr_http_request(
    http_request_t* request, http_response_handler_t callback, void* callback_data)
{
  int i;

  txn_t* txn = txn_new(http_txns, (txn_callback_t)callback, callback_data);

  datastream_t* ds = ds_new(NULL, 1024);
  ds_write_u32(ds, txn->txn_id);
  ds_write_str(ds, request->host);
  ds_write_u16(ds, request->port);
  ds_write_u8(ds, request->method);

  ds_write_u8(ds, request->num_headers);
  for (i = 0; i < request->num_headers; ++i) {
    ds_write_str(ds, request->headers[i].name);
    ds_write_str(ds, request->headers[i].value);
  }

  ds_write_str(ds, request->url);
  ds_write_str(ds, request->request);

  wspr_send(WSPR_IN_HTTP_REQ, ds->buf, ds_index(ds));

  ds_free(ds);
}

static void
handle_http_result(uint8_t* data, uint16_t data_len)
{
  http_response_t response;
  datastream_t* ds = ds_new(data, data_len);

  uint32_t txn_id = ds_read_u32(ds);
  response.result = ds_read_s32(ds);
  response.response_code = ds_read_u32(ds);
  response.response_body = ds_read_str(ds);
  ds_free(ds);

  txn_complete(http_txns, txn_id, &response);

  free(response.response_body);
}
