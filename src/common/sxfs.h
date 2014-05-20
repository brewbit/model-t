/* Simple External-Flash File System (sxfs) */

#ifndef SXFS_H
#define SXFS_H


#include <stdint.h>
#include <stdbool.h>


typedef enum {
  SP_BOOT_PARAMS,
  SP_RECOVERY_IMG,
  SP_UPDATE_IMG,
  SP_OFFLINE_LOG,
  NUM_SXFS_PARTS
} sxfs_part_id_t;


bool
sxfs_write(sxfs_part_id_t part_id, uint32_t offset, uint8_t* data, uint32_t data_len);

bool
sxfs_read(sxfs_part_id_t part_id, uint32_t offset, uint8_t* data, uint32_t data_len);

bool
sxfs_erase(sxfs_part_id_t part_id);

bool
sxfs_crc(sxfs_part_id_t part_id, uint32_t offset, uint32_t size, uint32_t* crc);

#endif
