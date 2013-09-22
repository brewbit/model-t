/* Simple External-Flash File System (sxfs) */

#ifndef SXFS_H
#define SXFS_H


#include <stdint.h>
#include <stdbool.h>


typedef enum {
  SP_FACTORY_DEFAULT_IMG,
  SP_OTA_UPDATE_IMG,
  NUM_SXFS_PARTS
} sxfs_part_id_t;


/* Partition Record */
typedef struct {
  uint8_t magic[4];
  uint32_t size;
  uint32_t num_files;
  uint32_t crc;
} sxfs_part_rec_t;

/* File Record - */
typedef struct {
  uint32_t id;
  uint32_t offset;
  uint32_t size;
  uint32_t crc;
} sxfs_file_rec_t;

/* Partition - represents an open sxfs file */
typedef struct {
  uint32_t offset;
  uint32_t data_left;
} sxfs_part_t;

/* File - represents an open sxfs file */
typedef struct {
  uint32_t offset;
  uint32_t data_left;
} sxfs_file_t;


bool
sxfs_part_open(sxfs_part_t* part, sxfs_part_id_t part_id);

bool
sxfs_part_clear(sxfs_part_id_t part_id);

void
sxfs_part_write(sxfs_part_t* part, uint8_t* data, uint32_t data_len);

bool
sxfs_file_open(sxfs_file_t* file, sxfs_part_id_t part_id, uint32_t file_id);

uint32_t
sxfs_file_read(sxfs_file_t* file, uint8_t* data, uint32_t data_len);

void
sxfs_file_close(sxfs_file_t* file);

#endif
