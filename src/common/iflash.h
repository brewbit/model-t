
#ifndef IFLASH_H
#define IFLASH_H

#include <ch.h>
#include <hal.h>
#include <stdint.h>

/**
 * @brief Number of sectors in the flash memory.
 */
#if !defined(FLASH_SECTOR_COUNT) || defined(__DOXYGEN__)
#define FLASH_SECTOR_COUNT 12
#endif

/* Error codes */

/** @brief Flash operation successful */
#define FLASH_RETURN_SUCCESS TRUE

/** @brief Flash operation error because of denied access, corrupted memory.*/
#define FLASH_RETURN_NO_PERMISSION -1

/** @brief Flash operation error because of bad flash, corrupted memory */
#define FLASH_RETURN_BAD_FLASH -11

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Index of a sector */
typedef uint8_t flashsector_t;

/**
 * @brief Get the size of @p sector.
 * @return @p sector size in bytes.
 */
uint32_t iflash_sector_size(flashsector_t sector);

/**
 * @brief Get the beginning address of @p sector.
 * @param sector Sector to retrieve the beginning address of.
 * @return First address (inclusive) of @p sector.
 */
uint32_t iflash_sector_begin(flashsector_t sector);

/**
 * @brief Get the end address of @p sector.
 * @param sector Sector to retrieve the end address of.
 * @return End address (exclusive) of @p sector (i.e. beginning address of the next sector).
 */
uint32_t iflash_sector_end(flashsector_t sector);

/**
 * @brief Get the sector containing @p address.
 * @warning @p address must be in the flash addresses range.
 * @param address Address to be searched for.
 * @return Sector containing @p address.
 */
flashsector_t iflash_sector_at(uint32_t address);

/**
 * @brief Erase the flash @p sector.
 * @details The sector is checked for errors after erase.
 * @note The sector is deleted regardless of its current state.
 *
 * @param sector Sector which is going to be erased.
 * @return FLASH_RETURN_SUCCESS         No error erasing the sector.
 * @return FLASH_RETURN_BAD_FLASH       Flash cell error.
 * @return FLASH_RETURN_NO_PERMISSION   Access denied.
 */
int iflash_sector_erase(flashsector_t sector);

/**
 * @brief Erase the sectors containing the span of @p size bytes starting at @p address.
 *
 * @warning If @p address doesn't match the beginning of a sector, the
 * data contained between the beginning of the sector and @p address will
 * be erased too. The same applies for data contained at @p address + @p size
 * up to the end of the sector.
 *
 * @param address Starting address of the span in flash memory.
 * @param size Size of the span in bytes.
 * @return FLASH_RETURN_SUCCESS         No error erasing the flash memory.
 * @return FLASH_RETURN_BAD_FLASH       Flash cell error.
 * @return FLASH_RETURN_NO_PERMISSION   Access denied.
 */
int iflash_erase(uint32_t address, uint32_t size);

/**
 * @brief Check if the @p size bytes of flash memory starting at @p address are erased.
 * @note If the memory is erased, one can write data into it safely.
 * @param address First address in flash memory to be checked.
 * @param size Size of the memory space to be checked in bytes.
 * @return TRUE Memory is already erased.
 * @return FALSE Memory is not erased.
 */
bool_t iflash_is_erased(uint32_t address, uint32_t size);

/**
 * @brief Check if the data in @p buffer are identical to the one in flash memory.
 * @param address First address in flash memory to be checked.
 * @param buffer Buffer containing the data to compare.
 * @param size Size of @p buffer in bytes.
 * @return TRUE if the flash memory and the buffer contain identical data.
 * @return FALSE if the flash memory and the buffer don't contain identical data.
 */
bool_t iflash_compare(uint32_t address, const uint8_t* buffer, uint32_t size);

/**
 * @brief Copy data from the flash memory to a @p buffer.
 * @warning The @p buffer must be at least @p size bytes long.
 * @param address First address of the flash memory to be copied.
 * @param buffer Buffer to copy to.
 * @param size Size of the data to be copied in bytes.
 * @return FLASH_RETURN_SUCCESS if successfully copied.
 */
int iflash_read(uint32_t address, uint8_t* buffer, uint32_t size);

/**
 * @brief Copy data from a @p buffer to the flash memory.
 * @warning The flash memory area receiving the data must be erased.
 * @warning The @p buffer must be at least @p size bytes long.
 * @param address First address in the flash memory where to copy the data to.
 * @param buffer Buffer containing the data to copy.
 * @param size Size of the data to be copied in bytes.
 * @return FLASH_RETURN_SUCCESS         No error.
 * @return FLASH_RETURN_NO_PERMISSION   Access denied.
 */
int iflash_write(uint32_t address, const uint8_t* buffer, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_H */
