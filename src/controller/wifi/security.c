/*****************************************************************************
*
*  security.c  - CC3000 Host Driver Implementation.
*  Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*****************************************************************************/

//*****************************************************************************
//
//! \addtogroup security_api
//! @{
//
//*****************************************************************************

#include "security.h"
#include "core/security.h"

#ifndef CC3000_UNENCRYPTED_SMART_CONFIG
/* Handles for making the APIs asychronous and thread-safe */
extern Mutex             g_main_mutex;


//*****************************************************************************
//
//!  aes_encrypt
//!
//!  @param[in]  key   AES128 key of size 16 bytes
//!  @param[in\out] state   16 bytes of plain text and cipher text
//!
//!  @return  none
//!
//!  @brief   AES128 encryption:
//!           Given AES128 key and  16 bytes plain text, cipher text of 16 bytes
//!           is computed. The AES implementation is in mode ECB (Electronic
//!           Code Book).
//!
//!
//*****************************************************************************

void aes_encrypt(unsigned char *state,
                 unsigned char *key)
{
    chMtxLock(&g_main_mutex);
    c_aes_encrypt(state, key);
    chMtxUnlock();
}

//*****************************************************************************
//
//!  aes_decrypt
//!
//!  @param[in]  key   AES128 key of size 16 bytes
//!  @param[in\out] state   16 bytes of cipher text and plain text
//!
//!  @return  none
//!
//!  @brief   AES128 decryption:
//!           Given AES128 key and  16 bytes cipher text, plain text of 16 bytes
//!           is computed The AES implementation is in mode ECB
//!           (Electronic Code Book).
//!
//!
//*****************************************************************************

void aes_decrypt(unsigned char *state,
                 unsigned char *key)
{
    chMtxLock(&g_main_mutex);
    c_aes_decrypt(state, key);
    chMtxUnlock();
}

//*****************************************************************************
//
//!  aes_read_key
//!
//!  @param[out]  key   AES128 key of size 16 bytes
//!
//!  @return  on success 0, error otherwise.
//!
//!  @brief   Reads AES128 key from EEPROM
//!           Reads the AES128 key from fileID #12 in EEPROM
//!           returns an error if the key does not exist.
//!
//!
//*****************************************************************************

signed long aes_read_key(unsigned char *key)
{
    long ret;
    chMtxLock(&g_main_mutex);
    ret = c_aes_read_key(key);
    chMtxUnlock();
    return (ret);
}

//*****************************************************************************
//
//!  aes_write_key
//!
//!  @param[out]  key   AES128 key of size 16 bytes
//!
//!  @return  on success 0, error otherwise.
//!
//!  @brief   writes AES128 key from EEPROM
//!           Writes the AES128 key to fileID #12 in EEPROM
//!
//!
//*****************************************************************************

signed long aes_write_key(unsigned char *key)
{
    long ret;
    chMtxLock(&g_main_mutex);
    ret = c_aes_write_key(key);
    chMtxUnlock();
    return (ret);
}

#endif //CC3000_UNENCRYPTED_SMART_CONFIG

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
