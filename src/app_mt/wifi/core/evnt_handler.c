/*****************************************************************************
*
*  evnt_handler.c  - CC3000 Host Driver Implementation.
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
//! \addtogroup evnt_handler_api
//! @{
//
//******************************************************************************

//******************************************************************************
//                  INCLUDE FILES
//******************************************************************************

#include "cc3000_common.h"
#include "string.h"
#include "hci.h"
#include "evnt_handler.h"
#include "wlan.h"
#include "socket.h"
#include "netapp.h"
#include "cc3000_spi.h"

#include <stdio.h>

 

//*****************************************************************************
//                  COMMON DEFINES
//*****************************************************************************

#define FLOW_CONTROL_EVENT_HANDLE_OFFSET    (0)
#define FLOW_CONTROL_EVENT_BLOCK_MODE_OFFSET  (1)
#define FLOW_CONTROL_EVENT_FREE_BUFFS_OFFSET  (2)
#define FLOW_CONTROL_EVENT_SIZE          (4)

#define BSD_RSP_PARAMS_SOCKET_OFFSET    (0)
#define BSD_RSP_PARAMS_STATUS_OFFSET    (4)

#define GET_HOST_BY_NAME_RETVAL_OFFSET  (0)
#define GET_HOST_BY_NAME_ADDR_OFFSET  (4)

#define ACCEPT_SD_OFFSET      (0)
#define ACCEPT_RETURN_STATUS_OFFSET  (4)
#define ACCEPT_ADDRESS__OFFSET    (8)

#define SL_RECEIVE_SD_OFFSET      (0)
#define SL_RECEIVE_NUM_BYTES_OFFSET    (4)
#define SL_RECEIVE__FLAGS__OFFSET    (8)


#define SELECT_STATUS_OFFSET      (0)
#define SELECT_READFD_OFFSET      (4)
#define SELECT_WRITEFD_OFFSET      (8)
#define SELECT_EXFD_OFFSET        (12)


#define NETAPP_IPCONFIG_IP_OFFSET        (0)
#define NETAPP_IPCONFIG_SUBNET_OFFSET      (4)
#define NETAPP_IPCONFIG_GW_OFFSET        (8)
#define NETAPP_IPCONFIG_DHCP_OFFSET        (12)
#define NETAPP_IPCONFIG_DNS_OFFSET        (16)
#define NETAPP_IPCONFIG_MAC_OFFSET        (20)
#define NETAPP_IPCONFIG_SSID_OFFSET        (26)

#define NETAPP_IPCONFIG_IP_LENGTH        (4)
#define NETAPP_IPCONFIG_MAC_LENGTH        (6)
#define NETAPP_IPCONFIG_SSID_LENGTH        (32)


#define NETAPP_PING_PACKETS_SENT_OFFSET      (0)
#define NETAPP_PING_PACKETS_RCVD_OFFSET      (4)
#define NETAPP_PING_MIN_RTT_OFFSET        (8)
#define NETAPP_PING_MAX_RTT_OFFSET        (12)
#define NETAPP_PING_AVG_RTT_OFFSET        (16)

#define GET_SCAN_RESULTS_TABlE_COUNT_OFFSET        (0)
#define GET_SCAN_RESULTS_SCANRESULT_STATUS_OFFSET    (4)
#define GET_SCAN_RESULTS_ISVALID_TO_SSIDLEN_OFFSET    (8)
#define GET_SCAN_RESULTS_FRAME_TIME_OFFSET        (10)
#define GET_SCAN_RESULTS_SSID_MAC_LENGTH        (38)



//*****************************************************************************
//                  GLOBAL VARAIABLES
//*****************************************************************************

uint32_t socket_active_status = SOCKET_STATUS_INIT_VAL;


//*****************************************************************************
//            Prototypes for the static functions
//*****************************************************************************

//static void
//hci_unsol_handle_patch_request(char *event_hdr);

static void
hci_dispatch_event(uint8_t* event_hdr, uint16_t event_size);

static void
hci_dispatch_data(uint8_t* buffer, uint16_t buffer_size);

static int32_t
hci_event_unsol_flowcontrol_handler(uint8_t* pEvent);

static void
update_socket_active_status(char *resp_params);


//*****************************************************************************
//
//!  hci_dispatch_packet
//!
//!  @param         pvBuffer - pointer to the received data buffer
//!                      The function triggers Received event/data processing
//!
//!  @param         Pointer to the received data
//!  @return        none
//!
//!  @brief         The function triggers Received event/data processing. It is
//!                       called from the SPI library to receive the data
//
//*****************************************************************************
void
hci_dispatch_packet(uint8_t* buffer, uint16_t buffer_size)
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
hci_dispatch_data(uint8_t* buffer, uint16_t buffer_size)
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
//!  hci_unsol_handle_patch_request
//!
//!  @param  event_hdr  event header
//!
//!  @return none
//!
//!  @brief   Handle unsolicited event from type patch request
//
//*****************************************************************************
//static void
//hci_unsol_handle_patch_request(char *event_hdr)
//{
//  char *params = (char *)(event_hdr) + HCI_EVENT_HEADER_SIZE;
//  uint32_t ucLength = 0;
//  char *patch;
//
//  switch (*params)
//  {
//  case HCI_EVENT_PATCHES_DRV_REQ:
//
//    if (tSLInformation.sDriverPatches)
//    {
//      patch = tSLInformation.sDriverPatches(&ucLength);
//
//      if (patch)
//      {
//        hci_patch_send(HCI_EVENT_PATCHES_DRV_REQ,
//                       tSLInformation.pucTxCommandBuffer, patch, ucLength);
//        return;
//      }
//    }
//
//    // Send 0 length Patches response event
//    hci_patch_send(HCI_EVENT_PATCHES_DRV_REQ,
//                   tSLInformation.pucTxCommandBuffer, 0, 0);
//    break;
//
//  case HCI_EVENT_PATCHES_FW_REQ:
//
//    if (tSLInformation.sFWPatches)
//    {
//      patch = tSLInformation.sFWPatches(&ucLength);
//
//      // Build and send a patch
//      if (patch)
//      {
//        hci_patch_send(HCI_EVENT_PATCHES_FW_REQ,
//                       tSLInformation.pucTxCommandBuffer, patch, ucLength);
//        return;
//      }
//    }
//
//    // Send 0 length Patches response event
//    hci_patch_send(HCI_EVENT_PATCHES_FW_REQ,
//                   tSLInformation.pucTxCommandBuffer, 0, 0);
//    break;
//
//  case HCI_EVENT_PATCHES_BOOTLOAD_REQ:
//
//    if (tSLInformation.sBootLoaderPatches)
//    {
//      patch = tSLInformation.sBootLoaderPatches(&ucLength);
//
//      if (patch)
//      {
//        hci_patch_send(HCI_EVENT_PATCHES_BOOTLOAD_REQ,
//                       tSLInformation.pucTxCommandBuffer, patch, ucLength);
//        return;
//      }
//    }
//
//    // Send 0 length Patches response event
//    hci_patch_send(HCI_EVENT_PATCHES_BOOTLOAD_REQ,
//                   tSLInformation.pucTxCommandBuffer, 0, 0);
//    break;
//  }
//}


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
hci_dispatch_event(uint8_t* event_hdr, uint16_t event_size)
{
  char * data = NULL;
  uint32_t retValue32;

  uint16_t opcode = STREAM_TO_UINT16(event_hdr, HCI_EVENT_OPCODE_OFFSET);
  uint16_t usLength = STREAM_TO_UINT8(event_hdr, HCI_DATA_LENGTH_OFFSET);
  uint8_t* pucReceivedParams = event_hdr + HCI_EVENT_HEADER_SIZE;

  if (usLength + HCI_EVENT_HEADER_SIZE - 1 > event_size) {
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
      if (tSLInformation.sWlanCB) {
        tSLInformation.sWlanCB(opcode, 0, 0);
      }
      break;

    case HCI_EVNT_WLAN_UNSOL_DHCP:
      {
        uint8_t  params[NETAPP_IPCONFIG_MAC_OFFSET + 1];  // extra byte is for the status
        uint8_t *recParams = params;

        data = (char*)(event_hdr) + HCI_EVENT_HEADER_SIZE;

        //Read IP address
        memcpy(recParams, data, NETAPP_IPCONFIG_IP_LENGTH);
        data += 4;
        //Read subnet
        memcpy(recParams, data, NETAPP_IPCONFIG_IP_LENGTH);
        data += 4;
        //Read default GW
        memcpy(recParams, data, NETAPP_IPCONFIG_IP_LENGTH);
        data += 4;
        //Read DHCP server
        memcpy(recParams, data, NETAPP_IPCONFIG_IP_LENGTH);
        data += 4;
        //Read DNS server
        memcpy(recParams, data, NETAPP_IPCONFIG_IP_LENGTH);
        // read the status
        *recParams = STREAM_TO_UINT8(event_hdr, HCI_EVENT_STATUS_OFFSET);


        if (tSLInformation.sWlanCB) {
          tSLInformation.sWlanCB(opcode, (char *)params, sizeof(params));
        }
      }
      break;

    case HCI_EVNT_WLAN_ASYNC_PING_REPORT:
      {
        netapp_pingreport_args_t params;
        data = (char*)(event_hdr) + HCI_EVENT_HEADER_SIZE;
        params.packets_sent = STREAM_TO_UINT32(data, NETAPP_PING_PACKETS_SENT_OFFSET);
        params.packets_received = STREAM_TO_UINT32(data, NETAPP_PING_PACKETS_RCVD_OFFSET);
        params.min_round_time = STREAM_TO_UINT32(data, NETAPP_PING_MIN_RTT_OFFSET);
        params.max_round_time = STREAM_TO_UINT32(data, NETAPP_PING_MAX_RTT_OFFSET);
        params.avg_round_time = STREAM_TO_UINT32(data, NETAPP_PING_AVG_RTT_OFFSET);

        if (tSLInformation.sWlanCB) {
          tSLInformation.sWlanCB(opcode, (char*)&params, sizeof(params));
        }
      }
      break;
    case HCI_EVNT_BSD_TCP_CLOSE_WAIT:
      {
        int32_t sd;
        data = (char*)(event_hdr) + HCI_EVENT_HEADER_SIZE;
        sd = STREAM_TO_UINT32(data, 0);
        if (tSLInformation.sWlanCB) {
          tSLInformation.sWlanCB(opcode, (char *)&sd, sizeof(sd));
        }
      }
      break;

    case HCI_EVNT_SEND:
    case HCI_EVNT_SENDTO:
      {
        *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT32(pucReceivedParams,SL_RECEIVE_SD_OFFSET);
        tSLInformation.pRetParams = ((char *)tSLInformation.pRetParams) + 4;
        *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT32(pucReceivedParams,SL_RECEIVE_NUM_BYTES_OFFSET);
        tSLInformation.pRetParams = ((char *)tSLInformation.pRetParams) + 4;

        break;
      }
      /* Intentional fall-through */

    case HCI_EVNT_WRITE:
    {
      char *pArg;
      int32_t status;

      pArg = M_BSD_RESP_PARAMS_OFFSET(event_hdr);
      status = STREAM_TO_UINT32(pArg, BSD_RSP_PARAMS_STATUS_OFFSET);

      if (ERROR_SOCKET_INACTIVE == status) {
        // The only synchronous event that can come from SL device in form of
        // command complete is "Command Complete" on data sent, in case SL device
        // was unable to transmit
        tSLInformation.slTransmitDataError = STREAM_TO_UINT8(event_hdr, HCI_EVENT_STATUS_OFFSET);
        update_socket_active_status(M_BSD_RESP_PARAMS_OFFSET(event_hdr));
      }
      break;
    }

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
      (*(uint8_t *)tSLInformation.pRetParams) = STREAM_TO_UINT8(event_hdr, HCI_EVENT_STATUS_OFFSET);
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
      *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT32(pucReceivedParams, 0);
      break;

    case HCI_EVNT_READ_SP_VERSION:
      (*(uint8_t *)tSLInformation.pRetParams) = STREAM_TO_UINT8(event_hdr, HCI_EVENT_STATUS_OFFSET);
      tSLInformation.pRetParams = ((char *)tSLInformation.pRetParams) + 1;
      retValue32 = STREAM_TO_UINT32(pucReceivedParams, 0);
      UINT32_TO_STREAM((uint8_t *)tSLInformation.pRetParams, retValue32);
      break;

    case HCI_EVNT_BSD_GETHOSTBYNAME:

      *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT32(pucReceivedParams, GET_HOST_BY_NAME_RETVAL_OFFSET);
      tSLInformation.pRetParams = ((char *)tSLInformation.pRetParams) + 4;
      *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT32(pucReceivedParams, GET_HOST_BY_NAME_ADDR_OFFSET);
      break;

    case HCI_EVNT_ACCEPT:
      {
        *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT32(pucReceivedParams, ACCEPT_SD_OFFSET);
        tSLInformation.pRetParams = ((char *)tSLInformation.pRetParams) + 4;
        *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT32(pucReceivedParams, ACCEPT_RETURN_STATUS_OFFSET);
        tSLInformation.pRetParams = ((char *)tSLInformation.pRetParams) + 4;

        //This argument returns in network order
        memcpy((uint8_t *)tSLInformation.pRetParams,
            pucReceivedParams + ACCEPT_ADDRESS__OFFSET, sizeof(sockaddr));
        break;
      }

    case HCI_EVNT_RECV:
    case HCI_EVNT_RECVFROM:
      {
        *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT32(pucReceivedParams,SL_RECEIVE_SD_OFFSET);
        tSLInformation.pRetParams = ((char *)tSLInformation.pRetParams) + 4;
        *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT32(pucReceivedParams,SL_RECEIVE_NUM_BYTES_OFFSET);
        tSLInformation.pRetParams = ((char *)tSLInformation.pRetParams) + 4;
        *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT32(pucReceivedParams,SL_RECEIVE__FLAGS__OFFSET);

        if(((tBsdReadReturnParams *)tSLInformation.pRetParams)->iNumberOfBytes == ERROR_SOCKET_INACTIVE)
        {
          set_socket_active_status(((tBsdReadReturnParams *)tSLInformation.pRetParams)->iSocketDescriptor,SOCKET_STATUS_INACTIVE);
        }
        break;
      }

    case HCI_EVNT_SELECT:
      {
        *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT32(pucReceivedParams,SELECT_STATUS_OFFSET);
        tSLInformation.pRetParams = ((char *)tSLInformation.pRetParams) + 4;
        *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT32(pucReceivedParams,SELECT_READFD_OFFSET);
        tSLInformation.pRetParams = ((char *)tSLInformation.pRetParams) + 4;
        *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT32(pucReceivedParams,SELECT_WRITEFD_OFFSET);
        tSLInformation.pRetParams = ((char *)tSLInformation.pRetParams) + 4;
        *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT32(pucReceivedParams,SELECT_EXFD_OFFSET);
        break;
      }

    case HCI_CMND_GETSOCKOPT:
      ((tBsdGetSockOptReturnParams *)tSLInformation.pRetParams)->iStatus = STREAM_TO_UINT8(event_hdr, HCI_EVENT_STATUS_OFFSET);
      //This argument returns in network order
      memcpy((uint8_t *)tSLInformation.pRetParams, pucReceivedParams, 4);
      break;

    case HCI_CMND_WLAN_IOCTL_GET_SCAN_RESULTS:

      *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT32(pucReceivedParams,GET_SCAN_RESULTS_TABlE_COUNT_OFFSET);
      tSLInformation.pRetParams = ((char *)tSLInformation.pRetParams) + 4;
      *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT32(pucReceivedParams,GET_SCAN_RESULTS_SCANRESULT_STATUS_OFFSET);
      tSLInformation.pRetParams = ((char *)tSLInformation.pRetParams) + 4;
      *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT16(pucReceivedParams,GET_SCAN_RESULTS_ISVALID_TO_SSIDLEN_OFFSET);
      tSLInformation.pRetParams = ((char *)tSLInformation.pRetParams) + 2;
      *(uint32_t *)tSLInformation.pRetParams = STREAM_TO_UINT16(pucReceivedParams,GET_SCAN_RESULTS_FRAME_TIME_OFFSET);
      tSLInformation.pRetParams = ((char *)tSLInformation.pRetParams) + 2;
      memcpy((uint8_t *)tSLInformation.pRetParams, (char *)(pucReceivedParams + GET_SCAN_RESULTS_FRAME_TIME_OFFSET + 2), GET_SCAN_RESULTS_SSID_MAC_LENGTH);
      break;

    case HCI_CMND_SIMPLE_LINK_START:
      break;

    case HCI_NETAPP_IPCONFIG:
      //Read IP address
      memcpy(tSLInformation.pRetParams, pucReceivedParams, NETAPP_IPCONFIG_IP_LENGTH);
      pucReceivedParams += NETAPP_IPCONFIG_IP_LENGTH;

      //Read subnet
      memcpy(tSLInformation.pRetParams, pucReceivedParams, NETAPP_IPCONFIG_IP_LENGTH);
      pucReceivedParams += NETAPP_IPCONFIG_IP_LENGTH;

      //Read default GW
      memcpy(tSLInformation.pRetParams, pucReceivedParams, NETAPP_IPCONFIG_IP_LENGTH);
      pucReceivedParams += NETAPP_IPCONFIG_IP_LENGTH;

      //Read DHCP server
      memcpy(tSLInformation.pRetParams, pucReceivedParams, NETAPP_IPCONFIG_IP_LENGTH);
      pucReceivedParams += NETAPP_IPCONFIG_IP_LENGTH;

      //Read DNS server
      memcpy(tSLInformation.pRetParams, pucReceivedParams, NETAPP_IPCONFIG_IP_LENGTH);
      pucReceivedParams += NETAPP_IPCONFIG_IP_LENGTH;

      //Read Mac address
      memcpy(tSLInformation.pRetParams, pucReceivedParams, NETAPP_IPCONFIG_MAC_LENGTH);
      pucReceivedParams += NETAPP_IPCONFIG_MAC_LENGTH;

      //Read SSID
      memcpy(tSLInformation.pRetParams, pucReceivedParams, NETAPP_IPCONFIG_SSID_LENGTH);
      break;

    default:
      break;
  }

  // TODO can't transit from the SPI I/O thread because we will deadlock...
  // Since we are going to TX - we need to handle this event after the
  // ResumeSPi since we need interrupts
//    if (opcode == HCI_EVNT_PATCHES_REQ))
//      hci_unsol_handle_patch_request((char *)pucReceivedData);

  // If this is the opcode that the app was waiting for, signal them
  if (opcode == tSLInformation.usRxEventOpcode) {
    tSLInformation.usRxEventOpcode = 0;
    chSemSignal(&tSLInformation.sem_recv);
  }
}

//*****************************************************************************
//
//!  set_socket_active_status
//!
//!  @param Sd
//!   @param Status
//!  @return         none
//!
//!  @brief          Check if the socket ID and status are valid and set 
//!                  accordingly  the global socket status
//
//*****************************************************************************
void
set_socket_active_status(int32_t sd, int32_t status)
{
  if(M_IS_VALID_SD(sd) && M_IS_VALID_STATUS(status)) {
    socket_active_status &= ~(1 << sd);      /* clean socket's mask */
    socket_active_status |= (status << sd); /* set new socket's mask */
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
hci_event_unsol_flowcontrol_handler(uint8_t* pEvent)
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
//!  get_socket_active_status
//!
//!  @param  Sd  Socket IS
//!  @return     Current status of the socket.   
//!
//!  @brief  Retrieve socket status
//
//*****************************************************************************
int32_t
get_socket_active_status(int32_t sd)
{
  if(M_IS_VALID_SD(sd)) {
    return (socket_active_status & (1 << sd)) ? SOCKET_STATUS_INACTIVE : SOCKET_STATUS_ACTIVE;
  }
  return SOCKET_STATUS_INACTIVE;
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
void
update_socket_active_status(char *resp_params)
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
//!  SimpleLinkWaitEvent
//!
//!  @param  usOpcode      command operation code
//!  @param  pRetParams    command return parameters
//!
//!  @return               none
//!
//
//*****************************************************************************
void 
SimpleLinkWaitEvent(uint16_t usOpcode, void *pRetParams)
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
//!  SimpleLinkWaitData
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
SimpleLinkWaitData(uint8_t *pBuf, uint8_t *from, uint8_t *fromlen)
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
