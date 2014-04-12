
#include "dfuse.h"
#include "xflash.h"
#include "common.h"

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

static bool
dfuse_read_prefix(uint32_t dfu_base_addr, dfu_prefix_t* prefix)
{
  if (prefix == NULL)
    return false;

  xflash_read(dfu_base_addr, (uint8_t*)prefix, sizeof(dfu_prefix_t));

  prefix->dfu_image_size = U32_BE(prefix->dfu_image_size);

  if ((memcmp(prefix->signature, "DfuSe", 5) != 0) ||
      (prefix->dfu_format_version != 1))
    return false;

  return true;
}

static bool
dfuse_read_target_prefix(uint32_t target_base_addr, dfu_target_prefix_t* prefix)
{
  if (prefix == NULL)
    return false;

  xflash_read(target_base_addr, (uint8_t*)prefix, sizeof(dfu_target_prefix_t));

  prefix->target_named = U32_BE(prefix->target_named);
  prefix->target_size = U32_BE(prefix->target_size);
  prefix->num_elements = U32_BE(prefix->num_elements);

  if (memcmp(prefix->signature, "Target", 6) != 0)
    return false;

  return true;
}

static bool
dfuse_read_image_element(uint32_t addr, dfu_image_element_t* img_element)
{
  if (img_element == NULL)
    return false;

  xflash_read(addr, (uint8_t*)img_element, sizeof(dfu_image_element_t));

  img_element->element_addr = U32_BE(img_element->element_addr);
  img_element->element_size = U32_BE(img_element->element_size);

  return true;
}

static bool
dfuse_read_suffix(uint32_t dfu_base_addr, dfu_prefix_t* prefix, dfu_suffix_t* suffix)
{
  if (prefix == NULL || suffix == NULL)
    return false;

  uint32_t suffix_addr = dfu_base_addr + prefix->dfu_image_size - sizeof(dfu_suffix_t);
  xflash_read(suffix_addr, (uint8_t*)suffix, sizeof(dfu_suffix_t));

  suffix->firmware_version = U16_LE(suffix->firmware_version);
  suffix->product_id = U16_LE(suffix->product_id);
  suffix->vendor_id = U16_LE(suffix->vendor_id);
  suffix->dfu_spec_num = U16_LE(suffix->dfu_spec_num);
  suffix->crc = U32_LE(suffix->crc);

  if ((memcmp(suffix->signature, "UFD", 3) != 0) ||
      (suffix->dfu_spec_num != 0x011A) ||
      (suffix->suffix_len != 16) ||
      (suffix->crc != xflash_crc(dfu_base_addr, prefix->dfu_image_size - 4)))
    return false;

  return true;
}

bool
dfuse_apply_update(uint32_t dfu_base_addr)
{
  int i;
  dfu_prefix_t prefix;
  dfu_suffix_t suffix;

  if (!dfuse_read_prefix(dfu_base_addr, &prefix)) {
    printf("Prefix read failed\r\n");
    return false;
  }

  if (dfuse_read_suffix(dfu_base_addr, &prefix, &suffix)) {
    printf("Suffix read failed\r\n");
    return false;
  }

  // TODO ensure that target/element addresses are in the range specified by the prefix
  uint32_t addr = dfu_base_addr + sizeof(dfu_prefix_t);
  for (i = 0; i < prefix.num_targets; ++i) {
    dfu_target_prefix_t target_prefix;
    if (!dfuse_read_target_prefix(addr, &target_prefix))
      return false;

    addr += sizeof(dfu_target_prefix_t);

    int j;
    for (j = 0; j < target_prefix.num_elements; ++j) {
      dfu_image_element_t img_element;
      dfuse_read_image_element(addr, &img_element);
      addr += sizeof(dfu_image_element_t);

      uint8_t data[16];
      uint32_t data_remaining = img_element.element_size;
      while (data_remaining > 0) {
        uint32_t read_len = MIN(data_remaining, sizeof(data));

        xflash_read(addr, data, read_len);

        addr += read_len;
        data_remaining -= read_len;
      }
    }
  }

  return true;
}

void
dfuse_write_self(uint32_t base_addr, image_rec_t* img_recs, uint32_t num_img_recs)
{
  uint32_t addr = base_addr;
  int i;
  // NOTE: assumes only one target
  uint32_t dfu_image_size = sizeof(dfu_prefix_t) + sizeof(dfu_target_prefix_t) + sizeof(dfu_suffix_t);

  uint32_t target_size = 0;
  for (i = 0; i < num_img_recs; ++i) {
    target_size += sizeof(dfu_image_element_t) + img_recs[i].size;
  }
  dfu_image_size += target_size;
  printf("total image size %d\r\n", dfu_image_size);
  printf("target size %d\r\n", target_size);

  // Clear space for the image
  printf("clearing flash  ");
  xflash_erase(addr, dfu_image_size);
  printf("OK\r\n");

  // write prefix
  dfu_prefix_t prefix = {
      .signature = {'D', 'f', 'u', 'S', 'e'},
      .dfu_format_version = 1,
      .dfu_image_size = U32_BE(dfu_image_size),
      .num_targets = 1
  };
  xflash_write(addr, (uint8_t*)&prefix, sizeof(dfu_prefix_t));
  addr += sizeof(dfu_prefix_t);
  printf("wrote prefix\r\n");

  // write target header
  dfu_target_prefix_t target = {
      .signature = {'T', 'a', 'r', 'g', 'e', 't'},
      .alternate_setting = 0,
      .target_named = U32_BE(0),
      .target_name = {0},
      .target_size = U32_BE(target_size),
      .num_elements = U32_BE(num_img_recs)
  };
  xflash_write(addr, (uint8_t*)&target, sizeof(dfu_target_prefix_t));
  addr += sizeof(dfu_target_prefix_t);
  printf("write target prefix\r\n");

  // write image elements
  for (i = 0; i < num_img_recs; ++i) {
    image_rec_t* img_rec = &img_recs[i];
    dfu_image_element_t img_element = {
        .element_addr = U32_BE((uint32_t)img_rec->data),
        .element_size = U32_BE(img_rec->size)
    };
    xflash_write(addr, (uint8_t*)&img_element, sizeof(dfu_image_element_t));
    addr += sizeof(dfu_image_element_t);
    printf("wrote image element header %d\r\n", i);

    printf("writing image element data 0x%08X 0x%08X\r\n", img_rec->data, img_rec->size);
    xflash_write(addr, img_rec->data, img_rec->size);
    addr += img_rec->size;
    printf("wrote image element data\r\n");
  }

  // write suffix
  dfu_suffix_t suffix = {
      .firmware_version = U16_LE(0xFFFF),
      .product_id = U16_LE(0xFFFF),
      .vendor_id = U16_LE(0xFFFF),
      .dfu_spec_num = U16_LE(0x011A),
      .signature = {'U', 'F', 'D'},
      .suffix_len = 16,
      .crc = U32_LE(0)
  };
  // Write suffix without CRC
  xflash_write(addr, (uint8_t*)&suffix, sizeof(dfu_suffix_t));
  printf("wrote suffix\r\n");

  // Calculate CRC
  suffix.crc = U32_LE(xflash_crc(base_addr, dfu_image_size - 4));
  printf("calculated CRC\r\n");

  // Write suffix with CRC
  xflash_write(addr, (uint8_t*)&suffix, sizeof(dfu_suffix_t));
  printf("write suffix\r\n");
}
