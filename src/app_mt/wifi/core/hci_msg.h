/*****************************************************************************
*
*  hci_msg.h  - CC3000 Host Driver Implementation.
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
#ifndef __HCI_MSG_H__
#define __HCI_MSG_H__
#include "hci.h"
#include "socket.h"


#ifdef  __cplusplus
extern "C" {
#endif


#define NETAPP_IPCONFIG_IP_LENGTH         (4)
#define NETAPP_IPCONFIG_MAC_LENGTH        (6)
#define NETAPP_IPCONFIG_SSID_LENGTH       (32)


typedef struct _bsd_accept_return_t {
  int32_t iSocketDescriptor;
  int32_t iStatus;
  sockaddr tSocketAddress;
} tBsdReturnParams;


typedef struct _bsd_read_return_t {
  int32_t iSocketDescriptor;
  int32_t iNumberOfBytes;
  uint32_t uiFlags;
} tBsdReadReturnParams;

#define BSD_RECV_FROM_FROMLEN_OFFSET  (4)
#define BSD_RECV_FROM_FROM_OFFSET    (16)


typedef struct _bsd_select_return_t {
  int32_t iStatus;
  uint32_t uiRdfd;
  uint32_t uiWrfd;
  uint32_t uiExfd;
} tBsdSelectRecvParams;


typedef struct _bsd_getsockopt_return_t {
  uint8_t ucOptValue[4];
  char iStatus;
} tBsdGetSockOptReturnParams;

typedef struct _bsd_gethostbyname_return_t {
  int32_t retVal;
  int32_t outputAddress;
} tBsdGethostbynameParams;

typedef struct {
  uint8_t status;
  uint8_t ip_addr[NETAPP_IPCONFIG_IP_LENGTH];
  uint8_t subnet_mask[NETAPP_IPCONFIG_IP_LENGTH];
  uint8_t default_gateway[NETAPP_IPCONFIG_IP_LENGTH];
  uint8_t dhcp_server[NETAPP_IPCONFIG_IP_LENGTH];
  uint8_t dns_server[NETAPP_IPCONFIG_IP_LENGTH];
} dhcp_status_t;


#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // __HCI_MSG_H__

