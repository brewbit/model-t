/*****************************************************************************
*
*  hci.h  - CC3000 Host Driver Implementation.
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
#ifndef __HCI_H__
#define __HCI_H__

#include "cc3000_common.h"
#include "hci_msg.h"
#include "cc3000_spi.h"

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef  __cplusplus
extern "C" {
#endif


#define hci_get_cmd_buffer()      (spi_get_buffer() + HCI_CMND_HEADER_SIZE)
#define hci_get_data_buffer()     (spi_get_buffer() + HCI_DATA_HEADER_SIZE)
#define hci_get_data_cmd_buffer() (spi_get_buffer() + HCI_DATA_CMD_HEADER_SIZE)
#define hci_get_patch_buffer()    (spi_get_buffer() + HCI_PATCH_HEADER_SIZE)


//*****************************************************************************
//
//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************

void
hci_init(void);

//*****************************************************************************
//
//!  hci_command_send
//!
//!  @param  usOpcode     command operation code
//!  @param  pucBuff      pointer to the command's arguments buffer
//!  @param  ucArgsLength length of the arguments
//!
//!  @return              none
//!
//!  @brief               Initiate an HCI command.
//
//*****************************************************************************
uint16_t
hci_command_send(
    uint16_t usOpcode,
    uint8_t ucArgsLength);


//*****************************************************************************
//
//!  hci_data_send
//!
//!  @param  usOpcode        command operation code
//!  @param  ucArgs          pointer to the command's arguments buffer
//!  @param  usArgsLength    length of the arguments
//!  @param  ucTail          pointer to the data buffer
//!  @param  usTailLength    buffer length
//!
//!  @return none
//!
//!  @brief              Initiate an HCI data write operation
//
//*****************************************************************************
long
hci_data_send(
    uint8_t ucOpcode,
    uint16_t usArgsLength,
    uint16_t usDataLength,
    const uint8_t *ucTail,
    uint16_t usTailLength);


//*****************************************************************************
//
//!  hci_data_command_send
//!
//!  @param  usOpcode      command operation code
//!  @param  pucBuff       pointer to the data buffer
//!  @param  ucArgsLength  arguments length
//!  @param  ucDataLength  data length
//!
//!  @return none
//!
//!  @brief              Prepare HCI header and initiate an HCI data write operation
//
//*****************************************************************************
void
hci_data_command_send(
    uint16_t usOpcode,
    uint8_t ucArgsLength,
    uint16_t ucDataLength);

//*****************************************************************************
//
//!  hci_patch_send
//!
//!  @param  usOpcode      command operation code
//!  @param  pucBuff       pointer to the command's arguments buffer
//!  @param  patch         pointer to patch content buffer
//!  @param  usDataLength  data length
//!
//!  @return              none
//!
//!  @brief               Prepare HCI header and initiate an HCI patch write operation
//
//*****************************************************************************
void
hci_patch_send(
    uint8_t ucOpcode,
    char *patch,
    uint16_t usDataLength);


//*****************************************************************************
//
//!  hci_wait_for_event
//!
//!  @param  usOpcode      command operation code
//!  @param  pRetParams    command return parameters
//!
//!  @return               none
//!
//!  @brief                Wait for event, pass it to the hci_event_handler and
//!                        update the event opcode in a global variable.
//
//*****************************************************************************
void
hci_wait_for_event(
    uint16_t usOpcode,
    void *pRetParams);

//*****************************************************************************
//
//!  hci_wait_for_data
//!
//!  @param  pBuf       data buffer
//!  @param  from       from information
//!  @param  fromlen    from information length
//!
//!  @return               none
//!
//!  @brief                Wait for data, pass it to the hci_event_handler
//!                        and update in a global variable that there is
//!                        data to read.
//
//*****************************************************************************
void
hci_wait_for_data(
    uint8_t *pBuf,
    uint8_t *from,
    uint8_t *fromlen);

//*****************************************************************************
//
//!  hci_dispatch_packet
//!
//!  @param         buffer - pointer to the received data buffer
//!                 buffer_size - the size of the passed buffer
//!
//!                 The function triggers Received event/data processing
//!
//!  @param         Pointer to the received data
//!  @return        none
//!
//!  @brief         The function triggers Received event/data processing. It is
//!                       called from the SPI library to receive the data
//
//*****************************************************************************
void
hci_dispatch_packet(
    uint8_t* buffer,
    uint16_t buffer_size);

bool
hci_claim_buffer(void);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // __HCI_H__
