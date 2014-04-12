
#include "iflash.h"
#include <string.h>

/**
 * @brief Maximum program/erase parallelism
 *
 * FLASH_CR_PSIZE_MASK is the mask to configure the parallelism value.
 * FLASH_CR_PSIZE_VALUE is the parallelism value suitable for the voltage range.
 *
 * PSIZE(1:0) is defined as:
 * 00 to program 8 bits per step
 * 01 to program 16 bits per step
 * 10 to program 32 bits per step
 * 11 to program 64 bits per step
 */
// Warning, flashdata_t must be unsigned!!!
#if defined(STM32F2XX) || defined(__DOXYGEN__)
#define FLASH_CR_PSIZE_MASK         FLASH_CR_PSIZE_0 | FLASH_CR_PSIZE_1
#if ((STM32_VDD >= 270) && (STM32_VDD <= 360)) || defined(__DOXYGEN__)
#define FLASH_CR_PSIZE_VALUE        FLASH_CR_PSIZE_1
typedef uint32_t flashdata_t;
#elif (STM32_VDD >= 240) && (STM32_VDD < 270)
#define FLASH_CR_PSIZE_VALUE        FLASH_CR_PSIZE_0
typedef uint16_t flashdata_t;
#elif (STM32_VDD >= 210) && (STM32_VDD < 240)
#define FLASH_CR_PSIZE_VALUE        FLASH_CR_PSIZE_0
typedef uint16_t flashdata_t;
#elif (STM32_VDD >= 180) && (STM32_VDD < 210)
#define FLASH_CR_PSIZE_VALUE        ((uint32_t)0x00000000)
typedef uint8_t flashdata_t;
#else
#error "invalid VDD voltage specified"
#endif
#endif /* defined(STM32F4XX) */

static bool_t iflash_unlock(void);

uint32_t
iflash_sector_size(flashsector_t sector)
{
    if (sector <= 3)
        return 16 * 1024;
    else if (sector == 4)
        return 64 * 1024;
    else if (sector >= 5 && sector <= 11)
        return 128 * 1024;
    return 0;
}

uint32_t
iflash_sector_begin(flashsector_t sector)
{
    uint32_t address = FLASH_BASE;
    while (sector > 0)
    {
        --sector;
        address += iflash_sector_size(sector);
    }
    return address;
}

uint32_t
flashSectorEnd(flashsector_t sector)
{
    return iflash_sector_begin(sector + 1);
}

flashsector_t
iflash_sector_at(uint32_t address)
{
    flashsector_t sector = 0;
    while (address >= flashSectorEnd(sector))
        ++sector;
    return sector;
}

/**
 * @brief Wait for the flash operation to finish.
 */
#define flashWaitWhileBusy() { while (FLASH->SR & FLASH_SR_BSY) {} }

/**
 * @brief Unlock the flash memory for write access.
 * @return TRUE  Unlock was successful.
 * @return FALSE    Unlock failed.
 */
static bool_t
iflash_unlock()
{
  /* Check if unlock is really needed */
  if (!(FLASH->CR & FLASH_CR_LOCK))
    return TRUE;

  /* Write magic unlock sequence */
  FLASH->KEYR = 0x45670123;
  FLASH->KEYR = 0xCDEF89AB;

  /* Check if unlock was successful */
  if (FLASH->CR & FLASH_CR_LOCK)
    return FALSE;
  return TRUE;
}

/**
 * @brief Lock the flash memory for write access.
 */
#define iflash_lock() { FLASH->CR |= FLASH_CR_LOCK; }

int
iflash_sector_erase(flashsector_t sector)
{
  /* Unlock flash for write access */
  if(iflash_unlock() == FALSE)
    return FLASH_RETURN_NO_PERMISSION;

  /* Wait for any busy flags. */
  flashWaitWhileBusy();

  /* Setup parallelism before any program/erase */
  FLASH->CR &= ~FLASH_CR_PSIZE_MASK;
  FLASH->CR |= FLASH_CR_PSIZE_VALUE;

  /* Start deletion of sector.
   * SNB(3:1) is defined as:
   * 0000 sector 0
   * 0001 sector 1
   * ...
   * 1011 sector 11
   * others not allowed */
  FLASH->CR &= ~(FLASH_CR_SNB_0 | FLASH_CR_SNB_1 | FLASH_CR_SNB_2 | FLASH_CR_SNB_3);
  if (sector & 0x1) FLASH->CR |= FLASH_CR_SNB_0;
  if (sector & 0x2) FLASH->CR |= FLASH_CR_SNB_1;
  if (sector & 0x4) FLASH->CR |= FLASH_CR_SNB_2;
  if (sector & 0x8) FLASH->CR |= FLASH_CR_SNB_3;
  FLASH->CR |= FLASH_CR_SER;
  FLASH->CR |= FLASH_CR_STRT;

  /* Wait until it's finished. */
  flashWaitWhileBusy();

  /* Sector erase flag does not clear automatically. */
  FLASH->CR &= ~FLASH_CR_SER;

  /* Lock flash again */
  iflash_lock();

  /* Check deleted sector for errors */
  if (iflash_is_erased(iflash_sector_begin(sector), iflash_sector_size(sector)) == FALSE)
    return FLASH_RETURN_BAD_FLASH;  /* Sector is not empty despite the erase cycle! */

  /* Successfully deleted sector */
  return FLASH_RETURN_SUCCESS;
}

int
iflash_erase(uint32_t address, uint32_t s)
{
  int32_t size = s;
  while (size > 0) {
    flashsector_t sector = iflash_sector_at(address);
    int err = iflash_sector_erase(sector);
    if (err != FLASH_RETURN_SUCCESS)
      return err;
    address = flashSectorEnd(sector);
    size -= iflash_sector_size(sector);
  }

  return FLASH_RETURN_SUCCESS;
}

bool_t
iflash_is_erased(uint32_t address, uint32_t size)
{
  /* Check for default set bits in the flash memory
   * For efficiency, compare flashdata_t values as much as possible,
   * then, fallback to byte per byte comparison. */
  while (size >= sizeof(flashdata_t)) {
    if (*(volatile flashdata_t*)address != (flashdata_t)(-1)) // flashdata_t being unsigned, -1 is 0xFF..FF
      return FALSE;
    address += sizeof(flashdata_t);
    size -= sizeof(flashdata_t);
  }
  while (size > 0) {
    if (*(uint8_t*)address != 0xff)
      return FALSE;
    ++address;
    --size;
  }

  return TRUE;
}

bool_t
iflash_compare(uint32_t address, const uint8_t* buffer, uint32_t size)
{
  /* For efficiency, compare flashdata_t values as much as possible,
   * then, fallback to byte per byte comparison. */
  while (size >= sizeof(flashdata_t)) {
    if (*(volatile flashdata_t*)address != *(flashdata_t*)buffer)
      return FALSE;
    address += sizeof(flashdata_t);
    buffer += sizeof(flashdata_t);
    size -= sizeof(flashdata_t);
  }
  while (size > 0) {
    if (*(volatile uint8_t*)address != *buffer)
      return FALSE;
    ++address;
    ++buffer;
    --size;
  }

  return TRUE;
}

int
iflash_read(uint32_t address, uint8_t* buffer, uint32_t size)
{
  memcpy(buffer, (uint8_t*)address, size);
  return FLASH_RETURN_SUCCESS;
}

static void
iflash_write_data(uint32_t address, const flashdata_t data)
{
  /* Enter flash programming mode */
  FLASH->CR |= FLASH_CR_PG;

  /* Write the data */
  *(flashdata_t*)address = data;

  /* Wait for completion */
  flashWaitWhileBusy();

  /* Exit flash programming mode */
  FLASH->CR &= ~FLASH_CR_PG;
}

int
iflash_write(uint32_t address, const uint8_t* buffer, uint32_t size)
{
  /* Unlock flash for write access */
  if(iflash_unlock() == FALSE)
    return FLASH_RETURN_NO_PERMISSION;

  /* Wait for any busy flags */
  flashWaitWhileBusy();

  /* Setup parallelism before any program/erase */
  FLASH->CR &= ~FLASH_CR_PSIZE_MASK;
  FLASH->CR |= FLASH_CR_PSIZE_VALUE;

  /* Check if the flash address is correctly aligned */
  uint32_t alignOffset = address % sizeof(flashdata_t);
  if (alignOffset != 0) {
    /* Not aligned, thus we have to read the data in flash already present
     * and update them with buffer's data */

    /* Align the flash address correctly */
    uint32_t alignedFlashAddress = address - alignOffset;

    /* Read already present data */
    flashdata_t tmp = *(volatile flashdata_t*)alignedFlashAddress;

    /* Compute how much bytes one must update in the data read */
    uint32_t chunkSize = sizeof(flashdata_t) - alignOffset;
    if (chunkSize > size)
      chunkSize = size; // this happens when both address and address + size are not aligned

    /* Update the read data with buffer's data */
    memcpy((uint8_t*)&tmp + alignOffset, buffer, chunkSize);

    /* Write the new data in flash */
    iflash_write_data(alignedFlashAddress, tmp);

    /* Advance */
    address += chunkSize;
    buffer += chunkSize;
    size -= chunkSize;
  }

  /* Now, address is correctly aligned. One can copy data directly from
   * buffer's data to flash memory until the size of the data remaining to be
   * copied requires special treatment. */
  while (size >= sizeof(flashdata_t)) {
    iflash_write_data(address, *(const flashdata_t*)buffer);
    address += sizeof(flashdata_t);
    buffer += sizeof(flashdata_t);
    size -= sizeof(flashdata_t);
  }

  /* Now, address is correctly aligned, but the remaining data are to
   * small to fill a entier flashdata_t. Thus, one must read data already
   * in flash and update them with buffer's data before writing an entire
   * flashdata_t to flash memory. */
  if (size > 0) {
    flashdata_t tmp = *(volatile flashdata_t*)address;
    memcpy(&tmp, buffer, size);
    iflash_write_data(address, tmp);
  }

  /* Lock flash again */
  iflash_lock();

  return FLASH_RETURN_SUCCESS;
}
