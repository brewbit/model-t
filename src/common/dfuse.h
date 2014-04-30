
#include <stdint.h>
#include <stdbool.h>

#include "sxfs.h"


typedef struct {
  uint8_t* data;
  uint32_t size;
} image_rec_t;

typedef enum {
  DFU_PARSE_OK,
  DFU_INVALID_ARGS,
  DFU_INVALID_PREFIX_SIGNATURE,
  DFU_INVALID_PREFIX_FORMAT,
  DFU_INVALID_TARGET_SIGNATURE,
  DFU_INVALID_IMG_ELEMENT_SIZE,
  DFU_INVALID_SUFFIX_SIGNATURE,
  DFU_INVALID_SUFFIX_SPEC,
  DFU_INVALID_SUFFIX_LEN,
  DFU_INVALID_CRC,
} dfu_parse_result_t;

typedef struct {
  uint32_t start;
  uint32_t end;
} addr_range_t;


dfu_parse_result_t
dfuse_verify(sxfs_part_id_t part);

dfu_parse_result_t
dfuse_apply_update(sxfs_part_id_t part, addr_range_t* valid_addr_range);

void
dfuse_write_self(sxfs_part_id_t part, image_rec_t* img_recs, uint32_t num_img_recs);
