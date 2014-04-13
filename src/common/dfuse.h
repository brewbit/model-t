
#include <stdint.h>
#include <stdbool.h>


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
  DFU_INVALID_SUFFIX_SIGNATURE,
  DFU_INVALID_SUFFIX_SPEC,
  DFU_INVALID_SUFFIX_LEN,
  DFU_INVALID_CRC,
} dfu_parse_result_t;


bool
dfuse_verify(uint32_t dfu_base_addr);

bool
dfuse_apply_update(uint32_t dfu_base_addr);

void
dfuse_write_self(uint32_t base_addr, image_rec_t* img_recs, uint32_t num_img_recs);
