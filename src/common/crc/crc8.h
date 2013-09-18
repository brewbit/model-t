/* ======================================================================== */
/*  CRC-8 routines                                     J. Zbiciak, 2001    */
/* ------------------------------------------------------------------------ */
/*  The contents of this file are hereby released into the public domain.   */
/*  This does not affect the rest of the program code in jzIntv, which      */
/*  remains under the GPL except where specific files state differently,    */
/*  such as this one.                                                       */
/*                                                                          */
/*  Programs are free to use the CRC-8 functions contained in this file    */
/*  for whatever purpose they desire, with no strings attached.             */
/* ======================================================================== */


#ifndef CRC_8_H
#define CRC_8_H

#include <stdint.h>

/* ======================================================================== */
/*  CRC8_TBL    -- Lookup table used for the CRC-8 code.                  */
/* ======================================================================== */
extern const uint8_t crc8_tbl[256];

/* ======================================================================== */
/*  CRC8_UPDATE -- Updates a 8-bit CRC using the lookup table above.      */
/*                  Note:  The 8-bit CRC is set up as a left-shifting      */
/*                  CRC with no inversions.                                 */
/*                                                                          */
/*                  All-caps version is a macro for stuff that can use it.  */
/* ======================================================================== */
uint8_t crc8_update(uint8_t crc, uint8_t data);
#define CRC8_UPDATE(crc, d) (crc8_tbl[(crc ^ d) & 0xff])

/* ======================================================================== */
/*  CRC8_BLOCK  -- Updates a 8-bit CRC on a block of 8-bit data.          */
/*                  Note:  The 8-bit CRC is set up as a left-shifting      */
/*                  CRC with no inversions.                                 */
/* ======================================================================== */
uint8_t crc8_block(uint8_t crc, uint8_t *data, uint32_t len);

#endif
/* ======================================================================== */
/*     This specific file is placed in the public domain by its author,     */
/*                              Joseph Zbiciak.                             */
/* ======================================================================== */
