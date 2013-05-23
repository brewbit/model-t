
#include "ch.h"
#include "hal.h"

#include "wspr.h"
#include "wspr_http.h"
#include "datastream.h"
#include "wspr_parser.h"
#include "terminal.h"

#include <stdlib.h>
#include <string.h>


static void
handle_http_get_result(uint8_t* data, uint16_t data_len);


void
wspr_http_init()
{
  wspr_set_handler(WSPR_OUT_HTTP_GET, handle_http_get_result);
}

void
wspr_http_get(char* host, uint16_t port, http_header_t* headers, uint8_t num_headers, char* url, char* request)
{
  int i;
  datastream_t* ds = ds_new(NULL, 1024);
  ds_write_str(ds, host);
  ds_write_u16(ds, port);

  ds_write_u8(ds, num_headers);
  for (i = 0; i < num_headers; ++i) {
    ds_write_str(ds, headers[i].name);
    ds_write_str(ds, headers[i].value);
  }

  ds_write_str(ds, url);
  ds_write_str(ds, request);

  wspr_send(WSPR_IN_HTTP_GET, ds->buf, ds_index(ds));

  ds_free(ds);
}

static void
handle_http_get_result(uint8_t* data, uint16_t data_len)
{
  datastream_t* ds = ds_new(data, data_len);

  int32_t result = ds_read_s32(ds);
  uint32_t response_code = ds_read_u32(ds);
  char* http_response = ds_read_str(ds);

  terminal_write("http get ");
  if (result == 0)
    terminal_write("succeeded!");
  else
    terminal_write("failed :(");
  terminal_write("\n  response code: ");
  terminal_write_int(response_code);
  terminal_write("\n  response body: ");
  terminal_write(http_response);
  terminal_write("\n");

  ds_free(ds);
  free(http_response);
}
