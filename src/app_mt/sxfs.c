
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
    [SP_FACTORY_DEFAULT_IMG] = {
        .offset = 0x00000000,
        .size   = 0x00100000 // 1024 KB
    },
    [SP_OTA_UPDATE_IMG] = {
        .offset = 0x00100000,
        .size   = 0x00100000 // 1024 KB
    }
};


bool
sxfs_part_open(sxfs_part_t* part, sxfs_part_id_t part_id)
{
  if (part_id >= NUM_SXFS_PARTS)
    return false;

  part->offset = part_info[part_id].offset;
  part->data_left = part_info[part_id].size;

  return true;
}

bool
sxfs_part_clear(sxfs_part_id_t part_id)
{
  if (part_id >= NUM_SXFS_PARTS)
    return false;

  part_info_t pinfo = part_info[part_id];
  xflash_erase_sectors(pinfo.offset / XFLASH_SECTOR_SIZE,
      (pinfo.offset + pinfo.size - 1) / XFLASH_SECTOR_SIZE);

  return true;
}

void
sxfs_part_write(sxfs_part_t* part, uint8_t* data, uint32_t data_len)
{
  uint32_t nwrite = MIN(data_len, part->data_left);

  if (nwrite > 0) {
    xflash_write(part->offset, data, nwrite);

    part->offset += nwrite;
    part->data_left -= nwrite;
  }
}

bool
sxfs_part_verify(sxfs_part_id_t part_id)
{
  uint32_t i;
  sxfs_part_rec_t part_rec;
  sxfs_file_rec_t file_rec;

  if (part_id >= NUM_SXFS_PARTS)
    return false;

  /* Lookup the partition record address */
  uint32_t part_rec_addr = part_info[part_id].offset;

  /* Read the partition record */
  xflash_read(part_rec_addr, (uint8_t*)&part_rec, sizeof(sxfs_part_rec_t));

  /* Verify that the magic number is correct */
  if (memcmp(part_rec.magic, "SXFS", 4) != 0)
    return false;

  /* Verify that the size is valid */
  if ((sizeof(sxfs_part_rec_t) + part_rec.size) > part_info[part_id].size)
    return false;

  /* Verify that the overall partition CRC is valid */
  if (xflash_crc(part_rec_addr + sizeof(sxfs_part_rec_t), part_rec.size) != part_rec.crc)
    return false;

  /* Verify each individual file */
  for (i = 0; i < part_rec.num_files; ++i) {
    /* Calculate the file record address */
    uint32_t file_rec_addr =
        part_rec_addr +
        sizeof(sxfs_part_rec_t) +
        (sizeof(sxfs_file_rec_t) * i);

    /* Read the file record */
    xflash_read(file_rec_addr, (uint8_t*)&file_rec, sizeof(sxfs_part_rec_t));

    /* Calculate the file data address */
    uint32_t file_data_addr = part_rec_addr + file_rec.offset;

    /* Verify that the file data CRC is valid */
    if (xflash_crc(file_data_addr, file_rec.size) != file_rec.crc)
      return false;
  }

  return true;
}

bool
sxfs_file_open(sxfs_file_t* file, sxfs_part_id_t part_id, uint32_t file_id)
{
  uint32_t i;
  sxfs_part_rec_t part_rec;
  sxfs_file_rec_t file_rec;

  if (part_id >= NUM_SXFS_PARTS)
    return false;

  uint32_t part_rec_addr = part_info[part_id].offset;
  xflash_read(part_rec_addr, (uint8_t*)&part_rec, sizeof(sxfs_part_rec_t));

  if (memcmp(part_rec.magic, "SXFS", 4) != 0)
    return false;

  if ((sizeof(sxfs_part_rec_t) + part_rec.size) > part_info[part_id].size)
    return false;

  if (xflash_crc(part_rec_addr + sizeof(sxfs_part_rec_t), part_rec.size) != part_rec.crc)
    return false;

  for (i = 0; i < part_rec.num_files; ++i) {
    uint32_t file_rec_addr = part_rec_addr +
        sizeof(sxfs_part_rec_t) +
        (sizeof(sxfs_file_rec_t) * i);

    xflash_read(file_rec_addr, (uint8_t*)&file_rec, sizeof(sxfs_part_rec_t));

    if (file_rec.id == file_id) {
      uint32_t file_data_addr = part_rec_addr + file_rec.offset;

      if (xflash_crc(file_data_addr, file_rec.size) != file_rec.crc)
        return false;

      file->offset = file_data_addr;
      file->data_left = file_rec.size;

      return true;
    }
  }

  return false;
}

uint32_t
sxfs_file_read(sxfs_file_t* file, uint8_t* data, uint32_t data_len)
{
  uint32_t nrecv = MIN(data_len, file->data_left);

  if (nrecv > 0) {
    xflash_read(file->offset, data, nrecv);

    file->offset += nrecv;
    file->data_left -= nrecv;
  }

  return nrecv;
}
