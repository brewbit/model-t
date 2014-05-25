
#ifndef XFLASH_H
#define XFLASH_H

#define XFLASH_SECTOR_SIZE      0x10000 // 64K
#define XFLASH_PAGE_SIZE        0x100   // 256

void
xflash_init(void);

int
xflash_erase(uint32_t addr, uint32_t size);

bool
xflash_is_erased(uint32_t addr, uint32_t len);

int
xflash_write(uint32_t addr, const uint8_t* buf, uint32_t buf_len);

void
xflash_read(uint32_t addr, uint8_t* buf, uint32_t buf_len);

uint32_t
xflash_crc(uint32_t addr, uint32_t size);

#endif
