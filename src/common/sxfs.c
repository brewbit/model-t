
#include "sxfs.h"
#include "xflash.h"
#include "common.h"
#include "crc/crc32.h"

#include <string.h>


typedef struct {
  uint32_t offset;
  uint32_t size;
} part_info_t;


static const part_info_t part_info[NUM_SXFS_PARTS] = {
    [SP_BOOT_PARAMS] = {
        .offset = 0x00000000,
        .size   = 0x00010000 // 64 KB
    },
    [SP_RECOVERY_IMG] = {
        .offset = 0x00010000,
        .size   = 0x00110000 // 1024 KB
    },
    [SP_UPDATE_IMG] = {
        .offset = 0x00110000,
        .size   = 0x00110000 // 1024 KB
    },
};


bool
sxfs_erase(sxfs_part_id_t part_id)
{
  if (part_id >= NUM_SXFS_PARTS)
    return false;

  part_info_t pinfo = part_info[part_id];
  xflash_erase(pinfo.offset, pinfo.size);

  return true;
}

bool
sxfs_write(sxfs_part_id_t part_id, uint32_t offset, uint8_t* data, uint32_t data_len)
{
  if (part_id >= NUM_SXFS_PARTS)
    return false;

  part_info_t pinfo = part_info[part_id];
  if ((offset + data_len) > pinfo.size)
    return false;

  xflash_write(pinfo.offset + offset, data, data_len);

  return true;
}

bool
sxfs_read(sxfs_part_id_t part_id, uint32_t offset, uint8_t* data, uint32_t data_len)
{
  if (part_id >= NUM_SXFS_PARTS)
    return false;

  part_info_t pinfo = part_info[part_id];
  if ((offset + data_len) > pinfo.size)
    return false;

  xflash_read(pinfo.offset + offset, data, data_len);

  return true;
}

bool
sxfs_crc(sxfs_part_id_t part_id, uint32_t offset, uint32_t size, uint32_t* crc)
{
  if (crc == NULL)
    return false;

  if (part_id >= NUM_SXFS_PARTS)
    return false;

  part_info_t pinfo = part_info[part_id];
  if ((offset + size) > pinfo.size)
    return false;

  *crc = xflash_crc(pinfo.offset + offset, size);

  return true;
}
