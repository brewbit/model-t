
#include <stdint.h>
#include <stdbool.h>


typedef struct {
  uint8_t* data;
  uint32_t size;
} image_rec_t;


bool
dfuse_apply_update(uint32_t dfu_base_addr);

void
dfuse_write_self(uint32_t base_addr, image_rec_t* img_recs, uint32_t num_img_recs);
