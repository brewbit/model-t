
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
        .size   = 0x00100000 // 1024 KB
    },
    [SP_UPDATE_IMG] = {
        .offset = 0x00110000,
        .size   = 0x00100000 // 1024 KB
    },
    [SP_WEB_API_BACKLOG] = {
        .offset = 0x00210000,
        .size   = 0x00100000 // 1024 KB
    },
    [SP_APP_CFG_1] = {
        .offset = 0x00310000,
        .size   = 0x00010000 // 64 KB
    },
    [SP_APP_CFG_2] = {
        .offset = 0x00320000,
        .size   = 0x00010000 // 64 KB
    },
};


bool
sxfs_erase(sxfs_part_id_t part_id, uint32_t offset, uint32_t len)
{
  if (part_id >= NUM_SXFS_PARTS)
    return false;

  // Round up to the nearest 64K which is the smallest erasable size
  len = (((len - 1) / XFLASH_SECTOR_SIZE) + 1) * XFLASH_SECTOR_SIZE;

  part_info_t pinfo = part_info[part_id];
  if (offset + len > pinfo.size)
    return false;

  if (xflash_erase(pinfo.offset + offset, len) != 0)
    return false;

  return true;
}

bool
sxfs_erase_all(sxfs_part_id_t part_id)
{
  if (part_id >= NUM_SXFS_PARTS)
    return false;

  part_info_t pinfo = part_info[part_id];
  return sxfs_erase(part_id, 0, pinfo.size);
}

bool
sxfs_is_erased(sxfs_part_id_t part_id, uint32_t offset, uint32_t data_len)
{
  if (part_id >= NUM_SXFS_PARTS)
    return false;

  part_info_t pinfo = part_info[part_id];
  return xflash_is_erased(pinfo.offset + offset, data_len);
}

bool
sxfs_write(sxfs_part_id_t part_id, uint32_t offset, uint8_t* data, uint32_t data_len)
{
  if (part_id >= NUM_SXFS_PARTS)
    return false;

  part_info_t pinfo = part_info[part_id];
  if ((offset + data_len) > pinfo.size)
    return false;

  if (xflash_write(pinfo.offset + offset, data, data_len) != 0)
    return false;

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
