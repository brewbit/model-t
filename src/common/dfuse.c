
#include "dfuse.h"
#include "common.h"
#include "iflash.h"
#include "sxfs.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


// stored big endian
typedef struct __attribute__ ((__packed__)) {
  uint8_t  signature[5];
  uint8_t  dfu_format_version;
  uint32_t dfu_image_size;
  uint8_t  num_targets;
} dfu_prefix_t;

// stored big endian
typedef struct __attribute__ ((__packed__)) {
  uint8_t  signature[6];
  uint8_t  alternate_setting;
  uint32_t target_named;
  uint8_t  target_name[255];
  uint32_t target_size;
  uint32_t num_elements;
} dfu_target_prefix_t;

// stored big endian
typedef struct {
  uint32_t element_addr;
  uint32_t element_size;
} dfu_image_element_t;

// stored little endian
typedef struct __attribute__ ((__packed__)) {
  uint16_t firmware_version;
  uint16_t product_id;
  uint16_t vendor_id;
  uint16_t dfu_spec_num;
  uint8_t signature[3];
  uint8_t suffix_len;
  uint32_t crc;
} dfu_suffix_t;

#define PREFIX_SIGNATURE_OFFSET   0
#define PREFIX_FORMAT_OFFSET      5
#define PREFIX_IMAGE_SIZE_OFFSET  6
#define PREFIX_NUM_TARGETS_OFFSET 10

#define BSWAP16(x) \
      ((((x) >> 8) & 0xff) | \
       (((x) & 0xff) << 8))

#define BSWAP32(x) \
      ((((x) & 0xff000000) >> 24) | \
       (((x) & 0x00ff0000) >>  8) | \
       (((x) & 0x0000ff00) <<  8) | \
       (((x) & 0x000000ff) << 24))

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  #define U16_BE(u) BSWAP16(u)
  #define U32_BE(u) BSWAP32(u)
  #define U16_LE(u) (u)
  #define U32_LE(u) (u)
#else
  #define U16_BE(u) (u)
  #define U32_BE(u) (u)
  #define U16_LE(u) BSWAP16(u)
  #define U32_LE(u) BSWAP32(u)
#endif

static dfu_parse_result_t
dfuse_read_prefix(sxfs_part_id_t part, dfu_prefix_t* prefix)
{
  if (prefix == NULL)
    return DFU_INVALID_ARGS;

  sxfs_read(part, 0, (uint8_t*)prefix, sizeof(dfu_prefix_t));

  prefix->dfu_image_size = U32_LE(prefix->dfu_image_size);

  if (memcmp(prefix->signature, "DfuSe", 5) != 0)
    return DFU_INVALID_PREFIX_SIGNATURE;

  if (prefix->dfu_format_version != 1)
    return DFU_INVALID_PREFIX_FORMAT;

  return DFU_PARSE_OK;
}

static dfu_parse_result_t
dfuse_read_target_prefix(sxfs_part_id_t part, uint32_t offset, dfu_target_prefix_t* prefix)
{
  if (prefix == NULL)
    return DFU_INVALID_ARGS;

  sxfs_read(part, offset, (uint8_t*)prefix, sizeof(dfu_target_prefix_t));

  prefix->target_named = U32_LE(prefix->target_named);
  prefix->target_size = U32_LE(prefix->target_size);
  prefix->num_elements = U32_LE(prefix->num_elements);

  if (memcmp(prefix->signature, "Target", 6) != 0)
    return DFU_INVALID_TARGET_SIGNATURE;

  return DFU_PARSE_OK;
}

static dfu_parse_result_t
dfuse_read_image_element(sxfs_part_id_t part, uint32_t offset, dfu_image_element_t* img_element)
{
  if (img_element == NULL)
    return DFU_INVALID_ARGS;

  sxfs_read(part, offset, (uint8_t*)img_element, sizeof(dfu_image_element_t));

  img_element->element_addr = U32_LE(img_element->element_addr);
  img_element->element_size = U32_LE(img_element->element_size);

  return DFU_PARSE_OK;
}

static dfu_parse_result_t
dfuse_read_suffix(sxfs_part_id_t part, dfu_prefix_t* prefix, dfu_suffix_t* suffix)
{
  if (prefix == NULL || suffix == NULL)
    return DFU_INVALID_ARGS;

  uint32_t suffix_addr = prefix->dfu_image_size;
  sxfs_read(part, suffix_addr, (uint8_t*)suffix, sizeof(dfu_suffix_t));

  suffix->firmware_version = U16_LE(suffix->firmware_version);
  suffix->product_id = U16_LE(suffix->product_id);
  suffix->vendor_id = U16_LE(suffix->vendor_id);
  suffix->dfu_spec_num = U16_LE(suffix->dfu_spec_num);
  suffix->crc = U32_LE(suffix->crc);

  if (memcmp(suffix->signature, "UFD", 3) != 0)
    return DFU_INVALID_SUFFIX_SIGNATURE;

  if (suffix->dfu_spec_num != 0x011A)
    return DFU_INVALID_SUFFIX_SPEC;

  if (suffix->suffix_len != 16)
    return DFU_INVALID_SUFFIX_LEN;

  uint32_t crc;
  sxfs_crc(part, 0, prefix->dfu_image_size + (sizeof(dfu_suffix_t) - 4), &crc);
  if (suffix->crc != crc)
    return DFU_INVALID_CRC;

  return DFU_PARSE_OK;
}


typedef struct {
  void (*prefix)(dfu_prefix_t*);
  void (*target_prefix)(dfu_target_prefix_t*);
  void (*img_element)(dfu_image_element_t*);
  void (*img_data)(uint32_t addr, uint8_t* data, uint32_t size);
  void (*suffix)(dfu_suffix_t*);
} dfu_parse_ops_t;

static dfu_parse_result_t
dfuse_parse(sxfs_part_id_t part, dfu_parse_ops_t* ops)
{
  int i;
  dfu_prefix_t prefix;
  dfu_suffix_t suffix;
  dfu_parse_result_t result;

  result = dfuse_read_prefix(part, &prefix);
  if (result != DFU_PARSE_OK)
    return result;
  if (ops && ops->prefix)
    ops->prefix(&prefix);

  result = dfuse_read_suffix(part, &prefix, &suffix);
  if (result != DFU_PARSE_OK)
    return result;

  // TODO ensure that target/element addresses are in the range specified by the prefix
  uint32_t offset = sizeof(dfu_prefix_t);
  for (i = 0; i < prefix.num_targets; ++i) {
    dfu_target_prefix_t target_prefix;
    result = dfuse_read_target_prefix(part, offset, &target_prefix);
    if (result != DFU_PARSE_OK)
      return result;
    if (ops && ops->target_prefix)
      ops->target_prefix(&target_prefix);

    offset += sizeof(dfu_target_prefix_t);

    int j;
    for (j = 0; j < (int)target_prefix.num_elements; ++j) {
      dfu_image_element_t img_element;
      result = dfuse_read_image_element(part, offset, &img_element);
      if (result != DFU_PARSE_OK)
        return result;
      offset += sizeof(dfu_image_element_t);

      if (ops && ops->img_element)
        ops->img_element(&img_element);

      uint32_t target_addr = img_element.element_addr;
      uint8_t data[256];
      uint32_t data_remaining = img_element.element_size;
      while (data_remaining > 0) {
        uint32_t read_len = MIN(data_remaining, sizeof(data));

        sxfs_read(part, offset, data, read_len);

        if (ops && ops->img_data)
          ops->img_data(target_addr, data, read_len);

        offset += read_len;
        target_addr += read_len;
        data_remaining -= read_len;
      }
    }
  }

  if (ops && ops->suffix)
    ops->suffix(&suffix);

  return DFU_PARSE_OK;
}

bool
dfuse_verify(sxfs_part_id_t part)
{
  dfu_parse_result_t ret;
  ret = dfuse_parse(part, NULL);
  return ret == DFU_PARSE_OK;
}

static void
handle_img_data(uint32_t addr, uint8_t* data, uint32_t size)
{
  if (!iflash_is_erased(addr, size))
    iflash_erase(addr, size);

  iflash_write(addr, data, size);
}

bool
dfuse_apply_update(sxfs_part_id_t part)
{
  dfu_parse_result_t ret;
  dfu_parse_ops_t ops = {
      .img_data = handle_img_data
  };
  ret = dfuse_parse(part, &ops);

  return ret == DFU_PARSE_OK;
}

void
dfuse_write_self(sxfs_part_id_t part, image_rec_t* img_recs, uint32_t num_img_recs)
{
  uint32_t offset = 0;
  int i;
  uint32_t target_size = 0;
  for (i = 0; i < (int)num_img_recs; ++i) {
    target_size += sizeof(dfu_image_element_t) + img_recs[i].size;
  }

  // NOTE: assumes only one target
  uint32_t dfu_image_size = sizeof(dfu_prefix_t) + sizeof(dfu_target_prefix_t) + target_size;

  // Clear space for the image
  sxfs_erase(part);

  // write prefix
  dfu_prefix_t prefix = {
      .signature = {'D', 'f', 'u', 'S', 'e'},
      .dfu_format_version = 1,
      .dfu_image_size = U32_LE(dfu_image_size),
      .num_targets = 1
  };
  sxfs_write(part, offset, (uint8_t*)&prefix, sizeof(dfu_prefix_t));
  offset += sizeof(dfu_prefix_t);

  // write target header
  dfu_target_prefix_t target = {
      .signature = {'T', 'a', 'r', 'g', 'e', 't'},
      .alternate_setting = 0,
      .target_named = U32_LE(0),
      .target_name = {0},
      .target_size = U32_LE(target_size),
      .num_elements = U32_LE(num_img_recs)
  };
  sxfs_write(part, offset, (uint8_t*)&target, sizeof(dfu_target_prefix_t));
  offset += sizeof(dfu_target_prefix_t);

  // write image elements
  for (i = 0; i < (int)num_img_recs; ++i) {
    image_rec_t* img_rec = &img_recs[i];
    dfu_image_element_t img_element = {
        .element_addr = U32_LE((uint32_t)img_rec->data),
        .element_size = U32_LE(img_rec->size)
    };
    sxfs_write(part, offset, (uint8_t*)&img_element, sizeof(dfu_image_element_t));
    offset += sizeof(dfu_image_element_t);

    sxfs_write(part, offset, img_rec->data, img_rec->size);
    offset += img_rec->size;
  }

  // write suffix
  dfu_suffix_t suffix = {
      .firmware_version = U16_LE(0xFFFF),
      .product_id = U16_LE(0xFFFF),
      .vendor_id = U16_LE(0xFFFF),
      .dfu_spec_num = U16_LE(0x011A),
      .signature = {'U', 'F', 'D'},
      .suffix_len = 16,
      .crc = U32_LE(0xFFFFFFFF)
  };
  // Write suffix without CRC
  sxfs_write(part, offset, (uint8_t*)&suffix, sizeof(dfu_suffix_t) - sizeof(uint32_t));
  offset += sizeof(dfu_suffix_t) - sizeof(uint32_t);

  // Calculate CRC
  sxfs_crc(part, 0, dfu_image_size + sizeof(dfu_suffix_t) - sizeof(uint32_t), &suffix.crc);
  suffix.crc = U32_LE(suffix.crc);

  // Write CRC
  sxfs_write(part, offset, (uint8_t*)&suffix.crc, sizeof(uint32_t));
}
