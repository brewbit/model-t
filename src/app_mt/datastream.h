
#ifndef DATASTREAM_H
#define DATASTREAM_H

#include <stdint.h>

typedef struct {
  uint8_t* buf;
  uint8_t free_buf;
  uint32_t len;
  uint32_t idx;
  uint8_t error;
} datastream_t;


datastream_t*
ds_new(uint8_t* buf, uint32_t buf_len);

void
ds_free(datastream_t* ds);

uint8_t
ds_error(datastream_t* ds);

uint32_t
ds_index(datastream_t* ds);

uint8_t
ds_read_u8(datastream_t* ds);

int8_t
ds_read_s8(datastream_t* ds);

uint16_t
ds_read_u16(datastream_t* ds);

int16_t
ds_read_s16(datastream_t* ds);

uint32_t
ds_read_u32(datastream_t* ds);

int32_t
ds_read_s32(datastream_t* ds);

char*
ds_read_str(datastream_t* ds);

void
ds_read_buf(datastream_t* ds, uint8_t** data, uint16_t* len);

void
ds_write_u8(datastream_t* ds, uint8_t i);

void
ds_write_s8(datastream_t* ds, int8_t i);

void
ds_write_u16(datastream_t* ds, uint16_t i);

void
ds_write_s16(datastream_t* ds, int16_t i);

void
ds_write_u32(datastream_t* ds, uint32_t i);

void
ds_write_s32(datastream_t* ds, int32_t i);

void
ds_write_float(datastream_t* ds, float f);

void
ds_write_str(datastream_t* ds, char* str);

void
ds_write_buf(datastream_t* ds, uint8_t* data, uint16_t len);

#endif
