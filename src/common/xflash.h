
#ifndef XFLASH_H
#define XFLASH_H

#define XFLASH_SECTOR_SIZE      0x10000 // 64K
#define XFLASH_SUBSECTOR_8_SIZE 0x2000  // 8KB
#define XFLASH_SUBSECTOR_4_SIZE 0x1000  // 4KB
#define XFLASH_PAGE_SIZE        0x100   // 256

int
xflash_erase(uint32_t addr, uint32_t size);

void
xflash_erase_sectors(uint32_t start, uint32_t end);

bool
xflash_is_erased(uint32_t addr, uint32_t len);

void
xflash_write(uint32_t addr, const uint8_t* buf, uint32_t buf_len);

void
xflash_read(uint32_t addr, uint8_t* buf, uint32_t buf_len);

uint32_t
xflash_crc(uint32_t addr, uint32_t size);

#endif
