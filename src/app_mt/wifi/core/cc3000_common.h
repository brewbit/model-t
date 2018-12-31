/*****************************************************************************
*
*  cc3000_common.h  - CC3000 Host Driver Implementation.
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
#ifndef __CC3000_COMMON_H__
#define __CC3000_COMMON_H__

//******************************************************************************
// Include files
//******************************************************************************
#include "ch.h"
#include "hal.h"
#include "error_codes.h"
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef  __cplusplus
extern "C" {
#endif

//*****************************************************************************
//                  ERROR CODES
//*****************************************************************************
#define ESUCCESS        0
#define EFAIL          -1

//*****************************************************************************
//                  COMMON DEFINES
//*****************************************************************************

#define MAC_ADDR_LEN     (6)

  
/*Defines for minimal and maximal RX buffer size. This size includes the spi 
  header and hci header.
  The maximal buffer size derives from:
    MTU + HCI header + SPI header + sendto() agrs size
  The minimum buffer size derives from:
    HCI header + SPI header + max args size

  This buffer is used for receiving events and data.
  The packet can not be longer than MTU size and CC3000 does not support 
  fragmentation. Note that the same buffer is used for reception of the data 
  and events from CC3000. That is why the minimum is defined. 
  The calculation for the actual size of buffer for reception is:
  Given the maximal data size MAX_DATA that is expected to be received by
  application, the required buffer is:
  Using recv() or recvfrom():
 
    max(CC3000_MINIMAL_RX_SIZE, MAX_DATA + SPI_HEADER_SIZE + HCI_DATA_HEADER_SIZE + fromlen
    + ucArgsize + 1)
 
  Using gethostbyname() with minimal buffer size will limit the host name
  returned to 99 bytes only.
  The 1 is used for the overrun detection 

  Buffer size increased to 130 following the add_profile() with WEP security
  which requires TX buffer size of 130 bytes: 
  HEADERS_SIZE_EVNT + WLAN_ADD_PROFILE_WEP_PARAM_LEN + MAX SSID LEN + 4 * MAX KEY LEN = 130
  MAX SSID LEN = 32 
  MAX SSID LEN = 13 (with add_profile only ascii key setting is supported, 
  therfore maximum key size is 13)
*/

#define CC3000_MINIMAL_RX_SIZE      (130 + 1)
#define CC3000_MAXIMAL_RX_SIZE      (1519 + 1)

/*Defines for minimal and maximal TX buffer size.
  This buffer is used for sending events and data.
  The packet can not be longer than MTU size and CC3000 does not support
  fragmentation. Note that the same buffer is used for transmission of the data
  and commands. That is why the minimum is defined.
  The calculation for the actual size of buffer for transmission is:
  Given the maximal data size MAX_DATA, the required buffer is:
  Using Sendto():

   max(CC3000_MINIMAL_TX_SIZE, MAX_DATA + SPI_HEADER_SIZE
   + SOCKET_SENDTO_PARAMS_LEN + HCI_DATA_HEADER_SIZE + 1)

  Using Send():

   max(CC3000_MINIMAL_TX_SIZE, MAX_DATA + SPI_HEADER_SIZE
   + HCI_CMND_SEND_ARG_LENGTH + HCI_DATA_HEADER_SIZE + 1)

  The 1 is used for the overrun detection */

#define  CC3000_MINIMAL_TX_SIZE      (130 + 1)
#define  CC3000_MAXIMAL_TX_SIZE      (1519 + 1)

/*In order to determine your preferred buffer size, 
  change CC3000_MAXIMAL_RX_SIZE and CC3000_MAXIMAL_TX_SIZE to a value between
  the minimal and maximal specified above. 
  Note that the buffers are allocated by SPI.
  In case you change the size of those buffers, you might need also to change
  the linker file, since for example on MSP430 FRAM devices the buffers are
  allocated in the FRAM section that is allocated manually and not by IDE.
*/
  
#define CC3000_RX_BUFFER_SIZE   (CC3000_MAXIMAL_RX_SIZE)
#define CC3000_TX_BUFFER_SIZE   (CC3000_MAXIMAL_TX_SIZE)
#define SP_PORTION_SIZE         512

//*****************************************************************************
//                  Compound Types
//*****************************************************************************
typedef uint32_t clock_t;
typedef long suseconds_t;

//*************************************************************************************
//@@@ Socket Common Header - Start

#define HOSTNAME_MAX_LENGTH (230)  // 230 bytes + header shouldn't exceed 8 bit value

#define IPV4_ADDR(a, b, c, d) (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))

//--------- Special Address --------

#define INADDR_ANY 0

//--------- Address Families --------

#define  AF_INET                2
#define  AF_INET6               23

//------------ Socket Types ------------

#define  SOCK_STREAM            1
#define  SOCK_DGRAM             2
#define  SOCK_RAW               3           // Raw sockets allow new IPv4 protocols to be implemented in user space. A raw socket receives or sends the raw datagram not including link level headers
#define  SOCK_RDM               4
#define  SOCK_SEQPACKET         5

//----------- Socket Protocol ----------

#define IPPROTO_IP              0           // dummy for IP
#define IPPROTO_ICMP            1           // control message protocol
#define IPPROTO_IPV4            IPPROTO_IP  // IP inside IP
#define IPPROTO_TCP             6           // tcp
#define IPPROTO_UDP             17          // user datagram protocol
#define IPPROTO_IPV6            41          // IPv6 in IPv6
#define IPPROTO_NONE            59          // No next header
#define IPPROTO_RAW             255         // raw IP packet
#define IPPROTO_MAX             256

//----------- Socket retunr codes  -----------

#define SOC_ERROR               (-1)        // error
#define SOC_IN_PROGRESS         (-2)        // socket in progress

//----------- Socket Options -----------
#define  SOL_SOCKET             0xffff                  //  socket level

#define  SOCKOPT_RECV_NONBLOCK                 0        // recv non block mode, set SOCK_ON or SOCK_OFF (default block mode)
#define  SOCKOPT_RECV_TIMEOUT                  1        // optname to configure recv and recvfromtimeout
#define  SOCKOPT_ACCEPT_NONBLOCK               2        // accept non block mode, set SOCK_ON or SOCK_OFF (default block mode)

#define  SOCK_ON                0                       // socket non-blocking mode        is enabled
#define  SOCK_OFF               1                       // socket blocking mode is enabled

#define  TCP_NODELAY            0x0001
#define  TCP_BSDURGENT          0x7000

#define  MAX_PACKET_SIZE        1500
#define  MAX_LISTEN_QUEUE       4

#define  IOCTL_SOCKET_EVENTMASK

#define __WFD_SETSIZE            32

#define  ASIC_ADDR_LEN          8

#define NO_QUERY_RECIVED        -3


typedef struct {
  uint32_t s_addr; // load with inet_aton()
} in_addr;

typedef struct {
  uint16_t sa_family;
  uint8_t sa_data[14];
} sockaddr;

typedef struct {
  short sin_family;  // e.g. AF_INET
  uint16_t sin_port; // e.g. htons(3490)
  in_addr sin_addr;  // see struct in_addr, below
  char sin_zero[8];  // zero this if you want to
} sockaddr_in;

typedef uint32_t socklen_t;

// The wfd_set member is required to be an array of longs.
typedef long int __wfd_mask;

// It's easier to assume 8-bit bytes than to get CHAR_BIT.
#define __NWFDBITS               (8 * sizeof (__wfd_mask))
#define __WFDELT(d)              ((d) / __NWFDBITS)
#define __WFDMASK(d)             ((__wfd_mask) 1 << ((d) % __NWFDBITS))

// wfd_set for select and pselect.
typedef struct {
    __wfd_mask wfds_bits[__WFD_SETSIZE / __NWFDBITS];
#define __WFDS_BITS(set)        ((set)->wfds_bits)
} wfd_set;

// We don't use `memset' because this would require a prototype and
//   the array isn't too big.
#define __WFD_ZERO(set)                                \
  do {                                                 \
    unsigned int __i;                                  \
    wfd_set *__arr = (set);                            \
    for (__i = 0; __i < sizeof (wfd_set) / sizeof (__wfd_mask); ++__i) \
      __WFDS_BITS (__arr)[__i] = 0;                    \
  } while (0)
#define __WFD_SET(d, set)       (__WFDS_BITS (set)[__WFDELT (d)] |= __WFDMASK (d))
#define __WFD_CLR(d, set)       (__WFDS_BITS (set)[__WFDELT (d)] &= ~__WFDMASK (d))
#define __WFD_ISSET(d, set)     (__WFDS_BITS (set)[__WFDELT (d)] & __WFDMASK (d))

// Access macros for 'wfd_set'.
#define WFD_SET(wfd, wfdsetp)      __WFD_SET (wfd, wfdsetp)
#define WFD_CLR(wfd, wfdsetp)      __WFD_CLR (wfd, wfdsetp)
#define WFD_ISSET(wfd, wfdsetp)    __WFD_ISSET (wfd, wfdsetp)
#define WFD_ZERO(wfdsetp)         __WFD_ZERO (wfdsetp)

//Use in case of Big Endian only

#define htonl(A)    ((((uint32_t)(A) & 0xff000000) >> 24) | \
                     (((uint32_t)(A) & 0x00ff0000) >> 8) | \
                     (((uint32_t)(A) & 0x0000ff00) << 8) | \
                     (((uint32_t)(A) & 0x000000ff) << 24))

#define ntohl                   htonl

//Use in case of Big Endian only
#define htons(A)     ((((uint32_t)(A) & 0xff00) >> 8) | \
                      (((uint32_t)(A) & 0x00ff) << 8))


#define ntohs                   htons

//@@@ Socket Common Header - End
//**************************************************************************************







//*************************************************************************************
//@@@ HCI Common Header - Start

#define SPI_HEADER_SIZE                             (5)

#define HCI_CMND_HEADER_SIZE                        (4)
#define HCI_DATA_HEADER_SIZE                        (5)
#define HCI_DATA_CMD_HEADER_SIZE                    (5)
#define HCI_PATCH_HEADER_SIZE                       (6)
#define HCI_PATCH_PORTION_HEADER_SIZE               (2)
#define HCI_EVENT_HEADER_SIZE                       (5)


//*****************************************************************************
//
// Values that can be used as HCI Commands and HCI Packet header defines
//
//*****************************************************************************
#define  HCI_TYPE_CMND                                0x1
#define  HCI_TYPE_DATA                                0x2
#define  HCI_TYPE_PATCH                               0x3
#define  HCI_TYPE_EVNT                                0x4


#define HCI_EVENT_PATCHES_DRV_REQ                     (1)
#define HCI_EVENT_PATCHES_FW_REQ                      (2)
#define HCI_EVENT_PATCHES_BOOTLOAD_REQ                (3)


#define  HCI_CMND_WLAN_BASE                           0x0000
#define  HCI_CMND_WLAN_CONNECT                        0x0001
#define  HCI_CMND_WLAN_DISCONNECT                     0x0002
#define  HCI_CMND_WLAN_IOCTL_SET_SCANPARAM            0x0003
#define  HCI_CMND_WLAN_IOCTL_SET_CONNECTION_POLICY    0x0004
#define  HCI_CMND_WLAN_IOCTL_ADD_PROFILE              0x0005
#define  HCI_CMND_WLAN_IOCTL_DEL_PROFILE              0x0006
#define  HCI_CMND_WLAN_IOCTL_GET_SCAN_RESULTS         0x0007
#define  HCI_CMND_EVENT_MASK                          0x0008
#define  HCI_CMND_WLAN_IOCTL_STATUSGET                0x0009
#define  HCI_CMND_WLAN_IOCTL_SIMPLE_CONFIG_START      0x000A
#define  HCI_CMND_WLAN_IOCTL_SIMPLE_CONFIG_STOP       0x000B
#define  HCI_CMND_WLAN_IOCTL_SIMPLE_CONFIG_SET_PREFIX 0x000C
#define  HCI_CMND_WLAN_CONFIGURE_PATCH                0x000D

#define  HCI_CMND_SOCKET_BASE                         0x1000
#define  HCI_CMND_SOCKET                              0x1001
#define  HCI_CMND_BIND                                0x1002
#define  HCI_CMND_RECV                                0x1004
#define  HCI_CMND_ACCEPT                              0x1005
#define  HCI_CMND_LISTEN                              0x1006
#define  HCI_CMND_CONNECT                             0x1007
#define  HCI_CMND_BSD_SELECT                          0x1008
#define  HCI_CMND_SETSOCKOPT                          0x1009
#define  HCI_CMND_GETSOCKOPT                          0x100A
#define  HCI_CMND_CLOSE_SOCKET                        0x100B
#define  HCI_CMND_RECVFROM                            0x100D
#define  HCI_CMND_GETHOSTNAME                         0x1010

#define HCI_DATA_BASE                                 0x80
#define HCI_CMND_SEND                                 (0x01 + HCI_DATA_BASE)
#define HCI_CMND_SENDTO                               (0x03 + HCI_DATA_BASE)
#define HCI_DATA_BSD_RECVFROM                         (0x04 + HCI_DATA_BASE)
#define HCI_DATA_BSD_RECV                             (0x05 + HCI_DATA_BASE)

#define HCI_CMND_NVMEM_CBASE                          0x0200

#define HCI_CMND_NVMEM_CREATE_ENTRY                   0x0203
#define HCI_CMND_NVMEM_SWAP_ENTRY                     0x0205
#define HCI_CMND_NVMEM_READ                           0x0201
#define HCI_CMND_NVMEM_WRITE                          0x0090
#define HCI_CMND_NVMEM_WRITE_PATCH                    0x0204
#define HCI_CMND_READ_SP_VERSION                      0x0207

#define  HCI_CMND_READ_BUFFER_SIZE                    0x400B
#define  HCI_CMND_SIMPLE_LINK_START                   0x4000

#define HCI_CMND_NETAPP_BASE                          0x2000

#define HCI_NETAPP_DHCP                               (0x0001 + HCI_CMND_NETAPP_BASE)
#define HCI_NETAPP_PING_SEND                          (0x0002 + HCI_CMND_NETAPP_BASE)
#define HCI_NETAPP_PING_REPORT                        (0x0003 + HCI_CMND_NETAPP_BASE)
#define HCI_NETAPP_PING_STOP                          (0x0004 + HCI_CMND_NETAPP_BASE)
#define HCI_NETAPP_IPCONFIG                           (0x0005 + HCI_CMND_NETAPP_BASE)
#define HCI_NETAPP_ARP_FLUSH                          (0x0006 + HCI_CMND_NETAPP_BASE)
#define HCI_NETAPP_SET_DEBUG_LEVEL                    (0x0008 + HCI_CMND_NETAPP_BASE)
#define HCI_NETAPP_SET_TIMERS                         (0x0009 + HCI_CMND_NETAPP_BASE)

//*****************************************************************************
//
// Values that can be used as HCI Events defines
//
//*****************************************************************************
#define  HCI_EVNT_WLAN_BASE                     0x0000
#define  HCI_EVNT_WLAN_CONNECT                  0x0001
#define  HCI_EVNT_WLAN_DISCONNECT               0x0002
#define  HCI_EVNT_WLAN_IOCTL_ADD_PROFILE        0x0005

#define  HCI_EVNT_SOCKET                        HCI_CMND_SOCKET
#define  HCI_EVNT_BIND                          HCI_CMND_BIND
#define  HCI_EVNT_RECV                          HCI_CMND_RECV
#define  HCI_EVNT_ACCEPT                        HCI_CMND_ACCEPT
#define  HCI_EVNT_LISTEN                        HCI_CMND_LISTEN
#define  HCI_EVNT_CONNECT                       HCI_CMND_CONNECT
#define  HCI_EVNT_SELECT                        HCI_CMND_BSD_SELECT
#define  HCI_EVNT_CLOSE_SOCKET                  HCI_CMND_CLOSE_SOCKET
#define  HCI_EVNT_RECVFROM                      HCI_CMND_RECVFROM
#define  HCI_EVNT_SETSOCKOPT                    HCI_CMND_SETSOCKOPT
#define  HCI_EVNT_GETSOCKOPT                    HCI_CMND_GETSOCKOPT
#define  HCI_EVNT_BSD_GETHOSTBYNAME             HCI_CMND_GETHOSTNAME

#define  HCI_EVNT_SEND                          0x1003
#define  HCI_EVNT_WRITE                         0x100E
#define  HCI_EVNT_SENDTO                        0x100F

#define HCI_EVNT_PATCHES_REQ                    0x1000

#define HCI_EVNT_UNSOL_BASE                     0x4000

#define HCI_EVNT_WLAN_UNSOL_BASE                (0x8000)

#define HCI_EVNT_WLAN_UNSOL_CONNECT             (0x0001 + HCI_EVNT_WLAN_UNSOL_BASE)
#define HCI_EVNT_WLAN_UNSOL_DISCONNECT          (0x0002 + HCI_EVNT_WLAN_UNSOL_BASE)
#define HCI_EVNT_WLAN_UNSOL_INIT                (0x0004 + HCI_EVNT_WLAN_UNSOL_BASE)
#define HCI_EVNT_WLAN_TX_COMPLETE               (0x0008 + HCI_EVNT_WLAN_UNSOL_BASE)
#define HCI_EVNT_WLAN_UNSOL_DHCP                (0x0010 + HCI_EVNT_WLAN_UNSOL_BASE)
#define HCI_EVNT_WLAN_ASYNC_PING_REPORT         (0x0040 + HCI_EVNT_WLAN_UNSOL_BASE)
#define HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE  (0x0080 + HCI_EVNT_WLAN_UNSOL_BASE)
#define HCI_EVNT_WLAN_KEEPALIVE                 (0x0200 + HCI_EVNT_WLAN_UNSOL_BASE)
#define HCI_EVNT_BSD_TCP_CLOSE_WAIT             (0x0800 + HCI_EVNT_WLAN_UNSOL_BASE)

#define HCI_EVNT_DATA_UNSOL_FREE_BUFF           0x4100

#define HCI_EVNT_NVMEM_CREATE_ENTRY             HCI_CMND_NVMEM_CREATE_ENTRY
#define HCI_EVNT_NVMEM_SWAP_ENTRY               HCI_CMND_NVMEM_SWAP_ENTRY

#define HCI_EVNT_NVMEM_READ                     HCI_CMND_NVMEM_READ
#define HCI_EVNT_NVMEM_WRITE                    (0x0202)

#define HCI_EVNT_READ_SP_VERSION                HCI_CMND_READ_SP_VERSION

#define  HCI_EVNT_INPROGRESS                    0xFFFF

#define HCI_DATA_RECVFROM                       0x84
#define HCI_DATA_RECV                           0x85
#define HCI_DATA_NVMEM                          0x91

#define HCI_EVENT_CC3000_CAN_SHUT_DOWN          0x99

//*****************************************************************************
//
// Prototypes for the structures for APIs.
//
//*****************************************************************************
#define HCI_PACKET_TYPE_OFFSET                  (0)
#define HCI_PACKET_ARGSIZE_OFFSET               (2)
#define HCI_PACKET_LENGTH_OFFSET                (3)


#define HCI_EVENT_OPCODE_OFFSET                 (1)
#define HCI_EVENT_LENGTH_OFFSET                 (3)
#define HCI_EVENT_STATUS_OFFSET                 (4)
#define HCI_DATA_LENGTH_OFFSET                  (3)

//@@@ HCI Common Header - End
//**************************************************************************************


#define AES128_KEY_SIZE     16


//*****************************************************************************
// Prototypes for the APIs.
//*****************************************************************************

//*****************************************************************************
//
//!  UINT32_TO_STREAM_f
//!
//!  \param  p       pointer to the new stream
//!  \param  u32     pointer to the 32 bit
//!
//!  \return               pointer to the new stream
//!
//!  \brief                This function is used for copying 32 bit to stream
//!                        while converting to little endian format.
//
//*****************************************************************************

extern uint8_t* UINT32_TO_STREAM_f (uint8_t *p, uint32_t u32);

//*****************************************************************************
//
//!  UINT16_TO_STREAM_f
//!
//!  \param  p       pointer to the new stream
//!  \param  u32     pointer to the 16 bit
//!
//!  \return               pointer to the new stream
//!
//!  \brief               This function is used for copying 16 bit to stream
//!                       while converting to little endian format.
//
//*****************************************************************************

extern uint8_t* UINT16_TO_STREAM_f (uint8_t *p, uint16_t u16);

//*****************************************************************************
//
//!  STREAM_TO_UINT16_f
//!
//!  \param  p          pointer to the stream
//!  \param  offset     offset in the stream
//!
//!  \return               pointer to the new 16 bit
//!
//!  \brief               This function is used for copying received stream to
//!                       16 bit in little endian format.
//
//*****************************************************************************

extern uint16_t STREAM_TO_UINT16_f(char* p, uint16_t offset);

//*****************************************************************************
//
//!  STREAM_TO_UINT32_f
//!
//!  \param  p          pointer to the stream
//!  \param  offset     offset in the stream
//!
//!  \return               pointer to the new 32 bit
//!
//!  \brief               This function is used for copying received stream to
//!                       32 bit in little endian format.
//
//*****************************************************************************

extern uint32_t STREAM_TO_UINT32_f(char* p, uint16_t offset);


//*****************************************************************************
//                    COMMON MACROs
//*****************************************************************************


//This macro is used for copying 8 bit to stream while converting to little endian format.
#define UINT8_TO_STREAM(_p, _val)  {*(_p)++ = (_val);}
//This macro is used for copying 16 bit to stream while converting to little endian format.
#define UINT16_TO_STREAM(_p, _u16)  (UINT16_TO_STREAM_f(_p, _u16))
//This macro is used for copying 32 bit to stream while converting to little endian format.
#define UINT32_TO_STREAM(_p, _u32)  (UINT32_TO_STREAM_f(_p, _u32))
//This macro is used for copying a specified value length bits (l) to stream while converting to little endian format.
#define ARRAY_TO_STREAM(p, a, l)   {register short _i; for (_i = 0; _i < (short)l; _i++) *(p)++ = ((uint8_t *) a)[_i];}
//This macro is used for copying received stream to 8 bit in little endian format.
#define STREAM_TO_UINT8(_p, _offset)  (uint8_t)(*((char*)_p + _offset))
//This macro is used for copying received stream to 16 bit in little endian format.
#define STREAM_TO_UINT16(_p, _offset)  STREAM_TO_UINT16_f((char*)_p, _offset)
//This macro is used for copying received stream to 32 bit in little endian format.
#define STREAM_TO_UINT32(_p, _offset)  STREAM_TO_UINT32_f((char*)_p, _offset)




//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // __COMMON_H__
