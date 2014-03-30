/*****************************************************************************
*
*  hci.c  - CC3000 Host Driver Implementation.
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
//! \addtogroup hci_app
//! @{
//
//*****************************************************************************

#include "cc3000_common.h"
#include "hci.h"
#include "cc3000_spi.h"
#include "wlan.h"

#include <string.h>
#include <stdio.h>


//*****************************************************************************
//                  COMMON DEFINES
//*****************************************************************************

#define SL_PATCH_PORTION_SIZE                     (1000)

#define FLOW_CONTROL_EVENT_HANDLE_OFFSET           (0)
#define FLOW_CONTROL_EVENT_BLOCK_MODE_OFFSET       (1)
#define FLOW_CONTROL_EVENT_FREE_BUFFS_OFFSET       (2)
#define FLOW_CONTROL_EVENT_SIZE                    (4)

#define BSD_RSP_PARAMS_SOCKET_OFFSET               (0)
#define BSD_RSP_PARAMS_STATUS_OFFSET               (4)

#define GET_HOST_BY_NAME_RETVAL_OFFSET             (0)
#define GET_HOST_BY_NAME_ADDR_OFFSET               (4)

#define ACCEPT_SD_OFFSET                           (0)
#define ACCEPT_RETURN_STATUS_OFFSET                (4)
#define ACCEPT_ADDRESS__OFFSET                     (8)

#define SL_RECEIVE_SD_OFFSET                       (0)
#define SL_RECEIVE_NUM_BYTES_OFFSET                (4)
#define SL_RECEIVE__FLAGS__OFFSET                  (8)

#define BSD_RECV_FROM_FROMLEN_OFFSET               (4)
#define BSD_RECV_FROM_FROM_OFFSET                  (16)

#define SELECT_STATUS_OFFSET                       (0)
#define SELECT_READFD_OFFSET                       (4)
#define SELECT_WRITEFD_OFFSET                      (8)
#define SELECT_EXFD_OFFSET                         (12)

#define NETAPP_IPCONFIG_IP_OFFSET                  (0)
#define NETAPP_IPCONFIG_SUBNET_OFFSET              (4)
#define NETAPP_IPCONFIG_GW_OFFSET                  (8)
#define NETAPP_IPCONFIG_DHCP_OFFSET                (12)
#define NETAPP_IPCONFIG_DNS_OFFSET                 (16)
#define NETAPP_IPCONFIG_MAC_OFFSET                 (20)
#define NETAPP_IPCONFIG_SSID_OFFSET                (26)

#define NETAPP_PING_PACKETS_SENT_OFFSET            (0)
#define NETAPP_PING_PACKETS_RCVD_OFFSET            (4)
#define NETAPP_PING_MIN_RTT_OFFSET                 (8)
#define NETAPP_PING_MAX_RTT_OFFSET                 (12)
#define NETAPP_PING_AVG_RTT_OFFSET                 (16)

#define GET_SCAN_RESULTS_TABLE_COUNT_OFFSET        (0)
#define GET_SCAN_RESULTS_SCANRESULT_STATUS_OFFSET  (4)
#define GET_SCAN_RESULTS_ISVALID_TO_SSIDLEN_OFFSET (8)
#define GET_SCAN_RESULTS_FRAME_TIME_OFFSET         (10)
#define GET_SCAN_RESULTS_SSID_OFFSET               (12)
#define GET_SCAN_RESULTS_BSSID_OFFSET              (44)


//*****************************************************************************
//            Prototypes for the static functions
//*****************************************************************************

static void
hci_dispatch_event(uint8_t* event_hdr, uint16_t event_size);

static void
hci_dispatch_data(uint8_t* buffer, uint16_t buffer_size);

static int32_t
hci_event_unsol_flowcontrol_handler(uint8_t* pEvent);

static void
update_socket_active_status(uint8_t* resp_params);


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
    uint8_t ucArgsLength)
{ 
  uint8_t *stream;

  stream = spi_get_buffer();

  UINT8_TO_STREAM(stream, HCI_TYPE_CMND);
  stream = UINT16_TO_STREAM(stream, usOpcode);
  UINT8_TO_STREAM(stream, ucArgsLength);

  //Update the opcode of the event we will be waiting for
  spi_write(ucArgsLength + HCI_CMND_HEADER_SIZE);

  return(0);
}

//*****************************************************************************
//
//!  hci_data_send
//!
//!  @param  usOpcode        command operation code
//!   @param  ucArgs           pointer to the command's arguments buffer
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
    uint16_t usTailLength)
{
  uint8_t *stream;

  (void)ucTail;

  stream = spi_get_buffer();

  UINT8_TO_STREAM(stream, HCI_TYPE_DATA);
  UINT8_TO_STREAM(stream, ucOpcode);
  UINT8_TO_STREAM(stream, usArgsLength);
  stream = UINT16_TO_STREAM(stream, usArgsLength + usDataLength + usTailLength);

  // Send the packet over the SPI
  spi_write(HCI_DATA_HEADER_SIZE + usArgsLength + usDataLength + usTailLength);

  return(ESUCCESS);
}


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
//!  @brief              Prepeare HCI header and initiate an HCI data write operation
//
//*****************************************************************************
void hci_data_command_send(
    uint16_t usOpcode,
    uint8_t ucArgsLength,
    uint16_t ucDataLength)
{ 
  uint8_t *stream = spi_get_buffer();

  UINT8_TO_STREAM(stream, HCI_TYPE_DATA);
  UINT8_TO_STREAM(stream, usOpcode);
  UINT8_TO_STREAM(stream, ucArgsLength);
  stream = UINT16_TO_STREAM(stream, ucArgsLength + ucDataLength);

  // Send the command over SPI on data channel
  spi_write(ucArgsLength + ucDataLength + HCI_DATA_CMD_HEADER_SIZE);
}

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
//!  @brief               Prepeare HCI header and initiate an HCI patch write operation
//
//*****************************************************************************
void
hci_patch_send(
    uint8_t ucOpcode,
    char *patch,
    uint16_t usDataLength)
{ 
  uint16_t usTransLength;
  uint8_t *stream = spi_get_buffer();
  uint8_t *data_ptr = stream;

  UINT8_TO_STREAM(stream, HCI_TYPE_PATCH);
  UINT8_TO_STREAM(stream, ucOpcode);
  stream = UINT16_TO_STREAM(stream, usDataLength + HCI_PATCH_PORTION_HEADER_SIZE);

  if (usDataLength <= SL_PATCH_PORTION_SIZE) {
    UINT16_TO_STREAM(stream, usDataLength);
    stream = UINT16_TO_STREAM(stream, usDataLength);
    memcpy(hci_get_patch_buffer(), patch, usDataLength);

    // Update the opcode of the event we will be waiting for
    spi_write(usDataLength + HCI_PATCH_HEADER_SIZE);
  }
  else {
    usTransLength = (usDataLength/SL_PATCH_PORTION_SIZE);
    UINT16_TO_STREAM(stream, usDataLength + HCI_PATCH_PORTION_HEADER_SIZE + usTransLength*HCI_PATCH_PORTION_HEADER_SIZE);
    stream = UINT16_TO_STREAM(stream, SL_PATCH_PORTION_SIZE);
    memcpy(hci_get_patch_buffer(), patch, SL_PATCH_PORTION_SIZE);
    usDataLength -= SL_PATCH_PORTION_SIZE;
    patch += SL_PATCH_PORTION_SIZE;

    // Update the opcode of the event we will be waiting for
    spi_write(SL_PATCH_PORTION_SIZE + HCI_PATCH_HEADER_SIZE);

    while (usDataLength) {
      if (usDataLength <= SL_PATCH_PORTION_SIZE) {
        usTransLength = usDataLength;
        usDataLength = 0;

      }
      else {
        usTransLength = SL_PATCH_PORTION_SIZE;
        usDataLength -= usTransLength;
      }

      *(uint16_t *)data_ptr = usTransLength;
      memcpy(data_ptr + HCI_PATCH_PORTION_HEADER_SIZE, patch, usTransLength);
      patch += usTransLength;

      // Update the opcode of the event we will be waiting for
      spi_write(usTransLength + sizeof(usTransLength));
    }
  }
}


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
    uint16_t buffer_size)
{
  uint8_t type = buffer[0];

  switch (type) {
    case HCI_TYPE_EVNT:
      hci_dispatch_event(buffer, buffer_size);
      break;

    case HCI_TYPE_DATA:
      hci_dispatch_data(buffer, buffer_size);
      break;

    default:
      break;
  }
}

static void
hci_dispatch_data(
    uint8_t* buffer,
    uint16_t buffer_size)
{
  uint8_t arg_size = STREAM_TO_UINT8(buffer, HCI_PACKET_ARGSIZE_OFFSET);
  uint16_t pkt_length = STREAM_TO_UINT16(buffer, HCI_PACKET_LENGTH_OFFSET);

  // Don't copy data over if the app is not expecting it
  if (tSLInformation.usRxDataPending == 0) {
    printf("Received unrequested data packet! %d %d %d\r\n", buffer[0], arg_size, pkt_length);
    return;
  }

  // Data received: note that the only case where from and from length
  // are not null is in recv from, so fill the args accordingly
  if (tSLInformation.from) {
    *(uint32_t *)tSLInformation.fromlen = STREAM_TO_UINT32((buffer + HCI_DATA_HEADER_SIZE), BSD_RECV_FROM_FROMLEN_OFFSET);
    memcpy(tSLInformation.from, (buffer + HCI_DATA_HEADER_SIZE + BSD_RECV_FROM_FROM_OFFSET), *tSLInformation.fromlen);
  }

  // Let's vet length
  int32_t data_length = pkt_length - arg_size;
  if ((data_length <= 0) ||
      (pkt_length + HCI_DATA_HEADER_SIZE > buffer_size)) {
    printf("Invalid data length: %d %d %d\r\n", pkt_length, arg_size, buffer_size);
    data_length = -1;
  }
  else {
    memcpy(tSLInformation.pRetParams,
        buffer + HCI_DATA_HEADER_SIZE + arg_size,
        data_length);
  }

  // fixes the Nvram read not returning length
  if (tSLInformation.fromlen)
    *tSLInformation.fromlen = data_length;

  tSLInformation.usRxDataPending = 0;
  chSemSignal(&tSLInformation.sem_recv);
}


//*****************************************************************************
//
//!  hci_send_patch
//!
//!  @param  event_hdr  event header
//!
//!  @return none
//!
//!  @brief   Handle unsolicited event from type patch request
//
//*****************************************************************************
void
hci_send_patch(uint8_t patch_type)
{
  uint32_t ucLength = 0;
  char *patch;

  switch (patch_type) {
  case HCI_EVENT_PATCHES_DRV_REQ:
    if (tSLInformation.sDriverPatches) {
      patch = tSLInformation.sDriverPatches(&ucLength);

      if (patch) {
        hci_patch_send(HCI_EVENT_PATCHES_DRV_REQ, patch, ucLength);
        return;
      }
    }

    // Send 0 length Patches response event
    hci_patch_send(HCI_EVENT_PATCHES_DRV_REQ, 0, 0);
    break;

  case HCI_EVENT_PATCHES_FW_REQ:
    if (tSLInformation.sFWPatches) {
      patch = tSLInformation.sFWPatches(&ucLength);

      // Build and send a patch
      if (patch) {
        hci_patch_send(HCI_EVENT_PATCHES_FW_REQ, patch, ucLength);
        return;
      }
    }

    // Send 0 length Patches response event
    hci_patch_send(HCI_EVENT_PATCHES_FW_REQ, 0, 0);
    break;

  case HCI_EVENT_PATCHES_BOOTLOAD_REQ:
    if (tSLInformation.sBootLoaderPatches) {
      patch = tSLInformation.sBootLoaderPatches(&ucLength);

      if (patch) {
        hci_patch_send(HCI_EVENT_PATCHES_BOOTLOAD_REQ, patch, ucLength);
        return;
      }
    }

    // Send 0 length Patches response event
    hci_patch_send(HCI_EVENT_PATCHES_BOOTLOAD_REQ, 0, 0);
    break;

  default:
    break;
  }
}


//*****************************************************************************
//
//!  hci_dispatch_event
//!
//!  @param  event_hdr   event header
//!
//!  @return             1 if event supported and handled
//!                      0 if event is not supported
//!
//!  @brief              Handle unsolicited events
//
//*****************************************************************************
static void
hci_dispatch_event(
    uint8_t* event_hdr,
    uint16_t event_size)
{
  uint16_t opcode = STREAM_TO_UINT16(event_hdr, HCI_EVENT_OPCODE_OFFSET);
  uint16_t usLength = STREAM_TO_UINT8(event_hdr, HCI_DATA_LENGTH_OFFSET);
  uint8_t* pucReceivedParams = event_hdr + HCI_EVENT_HEADER_SIZE;

  if ((usLength + HCI_EVENT_HEADER_SIZE - 1) > event_size) {
    printf("Invalid event size: %d %d %d\r\n", opcode, usLength, event_size);
    return;
  }

  switch(opcode) {
    case HCI_EVNT_DATA_UNSOL_FREE_BUFF:
      hci_event_unsol_flowcontrol_handler(event_hdr);

      if (tSLInformation.NumberOfReleasedPackets == tSLInformation.NumberOfSentPackets) {
        if (tSLInformation.InformHostOnTxComplete) {
          tSLInformation.sWlanCB(HCI_EVENT_CC3000_CAN_SHUT_DOWN, NULL, 0);
        }
      }
      break;

    case HCI_EVNT_WLAN_KEEPALIVE:
    case HCI_EVNT_WLAN_UNSOL_CONNECT:
    case HCI_EVNT_WLAN_UNSOL_DISCONNECT:
    case HCI_EVNT_WLAN_UNSOL_INIT:
    case HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE:
      if (tSLInformation.sWlanCB)
        tSLInformation.sWlanCB(opcode, 0, 0);
      break;

    case HCI_EVNT_WLAN_UNSOL_DHCP:
      {
        netapp_dhcp_params_t params;

        //Read IP address
        memcpy(params.ip_addr, pucReceivedParams, NETAPP_IPCONFIG_IP_LENGTH);
        pucReceivedParams += 4;
        //Read subnet
        memcpy(params.subnet_mask, pucReceivedParams, NETAPP_IPCONFIG_IP_LENGTH);
        pucReceivedParams += 4;
        //Read default GW
        memcpy(params.default_gateway, pucReceivedParams, NETAPP_IPCONFIG_IP_LENGTH);
        pucReceivedParams += 4;
        //Read DHCP server
        memcpy(params.dhcp_server, pucReceivedParams, NETAPP_IPCONFIG_IP_LENGTH);
        pucReceivedParams += 4;
        //Read DNS server
        memcpy(params.dns_server, pucReceivedParams, NETAPP_IPCONFIG_IP_LENGTH);
        // read the status
        params.status = STREAM_TO_UINT8(event_hdr, HCI_EVENT_STATUS_OFFSET);

        if (tSLInformation.sWlanCB)
          tSLInformation.sWlanCB(opcode, &params, sizeof(params));
      }
      break;

    case HCI_EVNT_WLAN_ASYNC_PING_REPORT:
      {
        netapp_pingreport_args_t params;
        params.packets_sent = STREAM_TO_UINT32(pucReceivedParams, NETAPP_PING_PACKETS_SENT_OFFSET);
        params.packets_received = STREAM_TO_UINT32(pucReceivedParams, NETAPP_PING_PACKETS_RCVD_OFFSET);
        params.min_round_time = STREAM_TO_UINT32(pucReceivedParams, NETAPP_PING_MIN_RTT_OFFSET);
        params.max_round_time = STREAM_TO_UINT32(pucReceivedParams, NETAPP_PING_MAX_RTT_OFFSET);
        params.avg_round_time = STREAM_TO_UINT32(pucReceivedParams, NETAPP_PING_AVG_RTT_OFFSET);

        if (tSLInformation.sWlanCB)
          tSLInformation.sWlanCB(opcode, (char*)&params, sizeof(params));
      }
      break;

    case HCI_EVNT_BSD_TCP_CLOSE_WAIT:
      {
        int32_t sd = STREAM_TO_UINT32(pucReceivedParams, 0);
        if (tSLInformation.sWlanCB)
          tSLInformation.sWlanCB(opcode, (char *)&sd, sizeof(sd));
      }
      break;

    case HCI_EVNT_SEND:
    case HCI_EVNT_SENDTO:
      {
        tBsdReadReturnParams* params = tSLInformation.pRetParams;
        params->iSocketDescriptor = STREAM_TO_UINT32(pucReceivedParams, SL_RECEIVE_SD_OFFSET);
        params->iNumberOfBytes = STREAM_TO_UINT32(pucReceivedParams, SL_RECEIVE_NUM_BYTES_OFFSET);
      }
      /* Intentional fall-through */

    case HCI_EVNT_WRITE:
      {
        int32_t status = STREAM_TO_UINT32(pucReceivedParams, BSD_RSP_PARAMS_STATUS_OFFSET);

        if (ERROR_SOCKET_INACTIVE == status) {
          // The only synchronous event that can come from SL device in form of
          // command complete is "Command Complete" on data sent, in case SL device
          // was unable to transmit
          tSLInformation.slTransmitDataError = STREAM_TO_UINT8(event_hdr, HCI_EVENT_STATUS_OFFSET);
          update_socket_active_status(pucReceivedParams);
        }
      }
      break;

    case HCI_CMND_READ_BUFFER_SIZE:
      {
        tSLInformation.usNumberOfFreeBuffers = STREAM_TO_UINT8(pucReceivedParams, 0);
        tSLInformation.usSlBufferLength = STREAM_TO_UINT16(pucReceivedParams, 1);
      }
      break;

    case HCI_CMND_WLAN_CONFIGURE_PATCH:
    case HCI_NETAPP_DHCP:
    case HCI_NETAPP_PING_SEND:
    case HCI_NETAPP_PING_STOP:
    case HCI_NETAPP_ARP_FLUSH:
    case HCI_NETAPP_SET_DEBUG_LEVEL:
    case HCI_NETAPP_SET_TIMERS:
    case HCI_EVNT_NVMEM_READ:
    case HCI_EVNT_NVMEM_CREATE_ENTRY:
    case HCI_CMND_NVMEM_WRITE_PATCH:
    case HCI_NETAPP_PING_REPORT:
    case HCI_EVNT_MDNS_ADVERTISE:
      {
        uint8_t* status = tSLInformation.pRetParams;
        *status = STREAM_TO_UINT8(event_hdr, HCI_EVENT_STATUS_OFFSET);
      }
      break;

    case HCI_CMND_SETSOCKOPT:
    case HCI_CMND_WLAN_CONNECT:
    case HCI_CMND_WLAN_IOCTL_STATUSGET:
    case HCI_EVNT_WLAN_IOCTL_ADD_PROFILE:
    case HCI_CMND_WLAN_IOCTL_DEL_PROFILE:
    case HCI_CMND_WLAN_IOCTL_SET_CONNECTION_POLICY:
    case HCI_CMND_WLAN_IOCTL_SET_SCANPARAM:
    case HCI_CMND_WLAN_IOCTL_SIMPLE_CONFIG_START:
    case HCI_CMND_WLAN_IOCTL_SIMPLE_CONFIG_STOP:
    case HCI_CMND_WLAN_IOCTL_SIMPLE_CONFIG_SET_PREFIX:
    case HCI_CMND_EVENT_MASK:
    case HCI_EVNT_WLAN_DISCONNECT:
    case HCI_EVNT_SOCKET:
    case HCI_EVNT_BIND:
    case HCI_CMND_LISTEN:
    case HCI_EVNT_CLOSE_SOCKET:
    case HCI_EVNT_CONNECT:
    case HCI_EVNT_NVMEM_WRITE:
      {
        uint32_t* param = tSLInformation.pRetParams;
        *param = STREAM_TO_UINT32(pucReceivedParams, 0);
      }
      break;

    case HCI_EVNT_READ_SP_VERSION:
      {
        uint8_t* params = tSLInformation.pRetParams;

        params[0] = STREAM_TO_UINT8(event_hdr, HCI_EVENT_STATUS_OFFSET);
        memcpy(&params[1], pucReceivedParams, 4);
      }
      break;

    case HCI_EVNT_BSD_GETHOSTBYNAME:
      {
        tBsdGethostbynameParams* params = tSLInformation.pRetParams;
        params->retVal = STREAM_TO_UINT32(pucReceivedParams, GET_HOST_BY_NAME_RETVAL_OFFSET);
        params->outputAddress = STREAM_TO_UINT32(pucReceivedParams, GET_HOST_BY_NAME_ADDR_OFFSET);
      }
      break;

    case HCI_EVNT_ACCEPT:
      {
        tBsdReturnParams* params = tSLInformation.pRetParams;
        params->iSocketDescriptor = STREAM_TO_UINT32(pucReceivedParams, ACCEPT_SD_OFFSET);
        params->iStatus = STREAM_TO_UINT32(pucReceivedParams, ACCEPT_RETURN_STATUS_OFFSET);

        //This argument returns in network order
        memcpy(&params->tSocketAddress,
            pucReceivedParams + ACCEPT_ADDRESS__OFFSET,
            sizeof(sockaddr));
      }
      break;

    case HCI_EVNT_RECV:
    case HCI_EVNT_RECVFROM:
      {
        tBsdReadReturnParams* params = tSLInformation.pRetParams;
        params->iSocketDescriptor = STREAM_TO_UINT32(pucReceivedParams,SL_RECEIVE_SD_OFFSET);
        params->iNumberOfBytes = STREAM_TO_UINT32(pucReceivedParams,SL_RECEIVE_NUM_BYTES_OFFSET);
        params->uiFlags = STREAM_TO_UINT32(pucReceivedParams,SL_RECEIVE__FLAGS__OFFSET);

        if (params->iNumberOfBytes == ERROR_SOCKET_INACTIVE)
          set_socket_active_status(params->iSocketDescriptor, SOCKET_STATUS_INACTIVE);
      }
      break;

    case HCI_EVNT_SELECT:
      {
        tBsdSelectRecvParams* params = tSLInformation.pRetParams;
        params->iStatus = STREAM_TO_UINT32(pucReceivedParams, SELECT_STATUS_OFFSET);
        params->uiRdfd = STREAM_TO_UINT32(pucReceivedParams, SELECT_READFD_OFFSET);
        params->uiWrfd = STREAM_TO_UINT32(pucReceivedParams, SELECT_WRITEFD_OFFSET);
        params->uiExfd = STREAM_TO_UINT32(pucReceivedParams, SELECT_EXFD_OFFSET);
      }
      break;

    case HCI_CMND_GETSOCKOPT:
      {
        tBsdGetSockOptReturnParams* params = tSLInformation.pRetParams;
        params->iStatus = STREAM_TO_UINT8(event_hdr, HCI_EVENT_STATUS_OFFSET);
        //This argument returns in network order
        memcpy(&params->ucOptValue, pucReceivedParams, 4);
      }
      break;

    case HCI_CMND_WLAN_IOCTL_GET_SCAN_RESULTS:
      {
        wlan_scan_results_t* params = tSLInformation.pRetParams;



        params->result_count = STREAM_TO_UINT32(pucReceivedParams, GET_SCAN_RESULTS_TABLE_COUNT_OFFSET);
        params->scan_status = STREAM_TO_UINT32(pucReceivedParams, GET_SCAN_RESULTS_SCANRESULT_STATUS_OFFSET);

        uint16_t packed_fields = STREAM_TO_UINT16(pucReceivedParams, GET_SCAN_RESULTS_ISVALID_TO_SSIDLEN_OFFSET);
        params->valid = (packed_fields & 0x01);
        params->rssi = ((packed_fields & 0xFF) >> 1) - 128;
        params->security_mode = ((packed_fields >> 8) & 0x03);
        params->ssid_len = (packed_fields >> 10);
        params->frame_time = STREAM_TO_UINT16(pucReceivedParams, GET_SCAN_RESULTS_FRAME_TIME_OFFSET);
        memcpy(params->ssid, &pucReceivedParams[12], NETAPP_IPCONFIG_SSID_LENGTH);
        memcpy(params->bssid, &pucReceivedParams[44], NETAPP_IPCONFIG_MAC_LENGTH);
      }
      break;

    case HCI_NETAPP_IPCONFIG:
      {
        netapp_ipconfig_args_t* params = tSLInformation.pRetParams;
        //Read IP address
        memcpy(params->ip_addr, pucReceivedParams, NETAPP_IPCONFIG_IP_LENGTH);
        pucReceivedParams += NETAPP_IPCONFIG_IP_LENGTH;

        //Read subnet
        memcpy(params->subnet_mask, pucReceivedParams, NETAPP_IPCONFIG_IP_LENGTH);
        pucReceivedParams += NETAPP_IPCONFIG_IP_LENGTH;

        //Read default GW
        memcpy(params->default_gateway, pucReceivedParams, NETAPP_IPCONFIG_IP_LENGTH);
        pucReceivedParams += NETAPP_IPCONFIG_IP_LENGTH;

        //Read DHCP server
        memcpy(params->dhcp_server, pucReceivedParams, NETAPP_IPCONFIG_IP_LENGTH);
        pucReceivedParams += NETAPP_IPCONFIG_IP_LENGTH;

        //Read DNS server
        memcpy(params->dns_server, pucReceivedParams, NETAPP_IPCONFIG_IP_LENGTH);
        pucReceivedParams += NETAPP_IPCONFIG_IP_LENGTH;

        //Read Mac address
        memcpy(params->mac_addr, pucReceivedParams, NETAPP_IPCONFIG_MAC_LENGTH);
        pucReceivedParams += NETAPP_IPCONFIG_MAC_LENGTH;

        //Read SSID
        memcpy(params->ssid, pucReceivedParams, NETAPP_IPCONFIG_SSID_LENGTH);
      }
      break;

    case HCI_EVNT_PATCHES_REQ:
      {
        uint8_t* patch_req_type = tSLInformation.pRetParams;
        *patch_req_type = pucReceivedParams[0];
      }
      break;

    default:
      break;
  }

  // If this is the opcode that the app was waiting for, signal them
  if (opcode == tSLInformation.usRxEventOpcode) {
    tSLInformation.usRxEventOpcode = 0;
    chSemSignal(&tSLInformation.sem_recv);
  }
}


//*****************************************************************************
//
//!  hci_event_unsol_flowcontrol_handler
//!
//!  @param  pEvent  pointer to the string contains parameters for IPERF
//!  @return         ESUCCESS if successful, EFAIL if an error occurred
//!
//!  @brief  Called in case unsolicited event from type
//!          HCI_EVNT_DATA_UNSOL_FREE_BUFF has received.
//!           Keep track on the number of packets transmitted and update the
//!           number of free buffer in the SL device.
//
//*****************************************************************************
static int32_t
hci_event_unsol_flowcontrol_handler(
    uint8_t* pEvent)
{
  int32_t temp, value;
  uint16_t i;
  uint16_t  pusNumberOfHandles=0;
  char *pReadPayload;

  pusNumberOfHandles = STREAM_TO_UINT16(pEvent, HCI_EVENT_HEADER_SIZE);
  pReadPayload = ((char *)pEvent +
                  HCI_EVENT_HEADER_SIZE + sizeof(pusNumberOfHandles));
  temp = 0;

  for(i = 0; i < pusNumberOfHandles; i++) {
    value = STREAM_TO_UINT16(pReadPayload, FLOW_CONTROL_EVENT_FREE_BUFFS_OFFSET);
    temp += value;
    pReadPayload += FLOW_CONTROL_EVENT_SIZE;
  }

  tSLInformation.usNumberOfFreeBuffers += temp;
  tSLInformation.NumberOfReleasedPackets += temp;

  return(ESUCCESS);
}

//*****************************************************************************
//
//!  update_socket_active_status
//!
//!  @param  resp_params  Socket IS
//!  @return     Current status of the socket.
//!
//!  @brief  Retrieve socket status
//
//*****************************************************************************
static void
update_socket_active_status(
    uint8_t* resp_params)
{
  int32_t status, sd;

  sd = STREAM_TO_UINT32(resp_params, BSD_RSP_PARAMS_SOCKET_OFFSET);
  status = STREAM_TO_UINT32(resp_params, BSD_RSP_PARAMS_STATUS_OFFSET);

  if(ERROR_SOCKET_INACTIVE == status) {
    set_socket_active_status(sd, SOCKET_STATUS_INACTIVE);
  }
}


//*****************************************************************************
//
//!  hci_wait_for_event
//!
//!  @param  usOpcode      command operation code
//!  @param  pRetParams    command return parameters
//!
//!  @return               none
//!
//
//*****************************************************************************
void
hci_wait_for_event(
    uint16_t usOpcode,
    void *pRetParams)
{
  // In the blocking implementation the control to caller will be returned only
  // after the end of current transaction
  tSLInformation.from = NULL;
  tSLInformation.fromlen = NULL;
  tSLInformation.pRetParams = pRetParams;
  tSLInformation.usRxEventOpcode = usOpcode;
  chSemWait(&tSLInformation.sem_recv);
}

//*****************************************************************************
//
//!  hci_wait_for_data
//!
//!  @param  pBuf       data buffer
//!  @param  from       from information
//!  @param  fromlen  from information length
//!
//!  @return               none
//!
//
//*****************************************************************************
void
hci_wait_for_data(
    uint8_t *pBuf,
    uint8_t *from,
    uint8_t *fromlen)
{
  // In the blocking implementation the control to caller will be returned only
  // after the end of current transaction, i.e. only after data will be received
  tSLInformation.from = from;
  tSLInformation.fromlen = fromlen;
  tSLInformation.pRetParams = pBuf;
  tSLInformation.usRxDataPending = 1;
  chSemWait(&tSLInformation.sem_recv);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
