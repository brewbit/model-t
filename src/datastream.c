
#include "datastream.h"

#include <stdlib.h>
#include <string.h>

#define CHECK_SIZE(size) \
  if ((ds->idx + (size)) > ds->len) { \
    ds->error = 1; \
    return 0; \
  }
#define CHECK_SIZE_VOID(size) \
  if ((ds->idx + (size)) > ds->len) { \
    ds->error = 1; \
    return; \
  }

datastream_t*
ds_new(uint8_t* buf, uint32_t len)
{
  datastream_t* ds = malloc(sizeof(datastream_t));

  memset(ds, 0, sizeof(datastream_t));

  if (buf) {
    ds->buf = buf;
    ds->free_buf = 0;
  }
  else {
    ds->buf = malloc(len);
    ds->free_buf = 1;
  }
  ds->len = len;

  return ds;
}

void
ds_free(datastream_t* ds)
{
  if (ds->free_buf)
    free(ds->buf);
  free(ds);
}

uint8_t
ds_error(datastream_t* ds)
{
  return ds->error;
}

uint32_t
ds_index(datastream_t* ds)
{
  return ds->idx;
}

uint8_t
ds_read_u8(datastream_t* ds)
{
  CHECK_SIZE(1);

  return ds->buf[ds->idx++];
}

int8_t
ds_read_s8(datastream_t* ds)
{
  CHECK_SIZE(1);

  return (int8_t)ds->buf[ds->idx++];
}

uint16_t
ds_read_u16(datastream_t* ds)
{
  uint16_t ret;

  CHECK_SIZE(2);

  ret = ds->buf[ds->idx++];
  ret |= (ds->buf[ds->idx++] << 8);

  return ret;
}

int16_t
ds_read_s16(datastream_t* ds)
{
  int16_t ret;

  CHECK_SIZE(2);

  ret = ds->buf[ds->idx++];
  ret |= (ds->buf[ds->idx++] << 8);

  return ret;
}

uint32_t
ds_read_u32(datastream_t* ds)
{
  uint32_t ret;

  CHECK_SIZE(4);

  ret = ds->buf[ds->idx++];
  ret |= (ds->buf[ds->idx++] << 8);
  ret |= (ds->buf[ds->idx++] << 16);
  ret |= (ds->buf[ds->idx++] << 24);

  return ret;
}

int32_t
ds_read_s32(datastream_t* ds)
{
  int32_t ret;

  CHECK_SIZE(4);

  ret = ds->buf[ds->idx++];
  ret |= (ds->buf[ds->idx++] << 8);
  ret |= (ds->buf[ds->idx++] << 16);
  ret |= (ds->buf[ds->idx++] << 24);

  return ret;
}

char*
ds_read_str(datastream_t* ds)
{
  CHECK_SIZE(2);
  uint16_t len = ds_read_u16(ds);

  CHECK_SIZE(len);

  char* str = malloc(len + 1);
  memcpy(str, &ds->buf[ds->idx], len);
  str[len] = 0;

  ds->idx += len;

  return str;
}

void
ds_read_buf(datastream_t* ds, uint8_t** data, uint16_t* len)
{
  CHECK_SIZE_VOID(2);
  *len = ds_read_u16(ds);

  CHECK_SIZE_VOID(*len);
  *data = ds->buf + ds->idx;
  ds->idx += *len;
}

void
ds_write_u8(datastream_t* ds, uint8_t i)
{
  CHECK_SIZE_VOID(1);

  ds->buf[ds->idx++] = i;
}

void
ds_write_s8(datastream_t* ds, int8_t i)
{
  CHECK_SIZE_VOID(1);

  ds->buf[ds->idx++] = i;
}

void
ds_write_u16(datastream_t* ds, uint16_t i)
{
  CHECK_SIZE_VOID(2);

  ds->buf[ds->idx++] = i;
  ds->buf[ds->idx++] = i >> 8;
}

void
ds_write_s16(datastream_t* ds, int16_t i)
{
  CHECK_SIZE_VOID(2);

  ds->buf[ds->idx++] = i;
  ds->buf[ds->idx++] = i >> 8;
}

void
ds_write_u32(datastream_t* ds, uint32_t i)
{
  CHECK_SIZE_VOID(4);

  ds->buf[ds->idx++] = i;
  ds->buf[ds->idx++] = i >> 8;
  ds->buf[ds->idx++] = i >> 16;
  ds->buf[ds->idx++] = i >> 24;
}

void
ds_write_s32(datastream_t* ds, int32_t i)
{
  CHECK_SIZE_VOID(4);

  ds->buf[ds->idx++] = i;
  ds->buf[ds->idx++] = i >> 8;
  ds->buf[ds->idx++] = i >> 16;
  ds->buf[ds->idx++] = i >> 24;
}

void
ds_write_str(datastream_t* ds, char* str)
{
  ds_write_buf(ds, (uint8_t*)str, strlen(str));
}

void
ds_write_buf(datastream_t* ds, uint8_t* data, uint16_t len)
{
  CHECK_SIZE_VOID(2 + len);

  ds_write_u16(ds, len);
  memcpy(&ds->buf[ds->idx], data, len);
  ds->idx += len;
}

