/*****************************************************************************
*
*  socket.c  - CC3000 Host Driver Implementation.
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
//! \addtogroup socket_api
//! @{
//
//*****************************************************************************

#include "hci.h"
#include "socket.h"
#include "cc3000_spi.h"


#define SOCKET_OPEN_PARAMS_LEN              (12)
#define SOCKET_CLOSE_PARAMS_LEN             (4)
#define SOCKET_ACCEPT_PARAMS_LEN            (4)
#define SOCKET_BIND_PARAMS_LEN              (20)
#define SOCKET_LISTEN_PARAMS_LEN            (8)
#define SOCKET_GET_HOST_BY_NAME_PARAMS_LEN  (9)
#define SOCKET_CONNECT_PARAMS_LEN           (20)
#define SOCKET_SELECT_PARAMS_LEN            (44)
#define SOCKET_SET_SOCK_OPT_PARAMS_LEN      (20)
#define SOCKET_GET_SOCK_OPT_PARAMS_LEN      (12)
#define SOCKET_RECV_FROM_PARAMS_LEN         (12)
#define SOCKET_SENDTO_PARAMS_LEN            (24)
#define SOCKET_MDNS_ADVERTISE_PARAMS_LEN    (12)


// The legnth of arguments for the SEND command: sd + buff_offset + len + flags,
// while size of each parameter is 32 bit - so the total length is 16 bytes;

#define HCI_CMND_SEND_ARG_LENGTH    (16)


#define SELECT_TIMEOUT_MIN_MICRO_SECONDS  5000

#define MDNS_DEVICE_SERVICE_MAX_LENGTH  (32)




//*****************************************************************************
//
//! consume_buf
//!
//!  @param  sd  socket descriptor
//!
//!  @return 0 in case there are buffers available,
//!          -1 in case of bad socket
//!          -2 if there are no free buffers present
//!
//*****************************************************************************
static int
consume_buf(int sd)
{
  // In case last transmission failed then we will return the last failure
  // reason here.
  // Note that the buffer will not be allocated in this case
  int err = socket_get_last_error(sd);
  if (err != 0) {
    errno = err;
    return errno;
  }

  if (SOCKET_STATUS_ACTIVE != get_socket_active_status(sd))
    return -1;

  //If there are no available buffers, return -2. It is recommended to use
  // select or receive to see if there is any buffer occupied with received data
  // If so, call receive() to release the buffer.
  if (!hci_claim_buffer())
    return -2;

  return 0;
}

//*****************************************************************************
//
//! socket
//!
//!  @param  domain    selects the protocol family which will be used for
//!                    communication. On this version only AF_INET is supported
//!  @param  type      specifies the communication semantics. On this version
//!                    only SOCK_STREAM, SOCK_DGRAM, SOCK_RAW are supported
//!  @param  protocol  specifies a particular protocol to be used with the
//!                    socket IPPROTO_TCP, IPPROTO_UDP or IPPROTO_RAW are
//!                    supported.
//!
//!  @return  On success, socket handle that is used for consequent socket
//!           operations. On error, -1 is returned.
//!
//!  @brief  create an endpoint for communication
//!          The socket function creates a socket that is bound to a specific
//!          transport service provider. This function is called by the
//!          application layer to obtain a socket handle.
//
//*****************************************************************************

int c_socket(long domain, long type, long protocol)
{
  long ret;
  uint8_t *args;

  ret = EFAIL;
  args = hci_get_cmd_buffer();

  // Fill in HCI packet structure
  args = UINT32_TO_STREAM(args, domain);
  args = UINT32_TO_STREAM(args, type);
  args = UINT32_TO_STREAM(args, protocol);

  // Initiate a HCI command
  hci_command_send(HCI_CMND_SOCKET, SOCKET_OPEN_PARAMS_LEN);

  // Since we are in blocking state - wait for event complete
  hci_wait_for_event(HCI_CMND_SOCKET, &ret);

  // Process the event
  errno = ret;

  return(ret);
}

//*****************************************************************************
//
//! closesocket
//!
//!  @param  sd    socket handle.
//!
//!  @return  On success, zero is returned. On error, -1 is returned.
//!
//!  @brief  The socket function closes a created socket.
//
//*****************************************************************************

long c_closesocket(long sd)
{
  long ret;
  uint8_t *args;

  ret = EFAIL;
  args = hci_get_cmd_buffer();

  // Fill in HCI packet structure
  args = UINT32_TO_STREAM(args, sd);

  // Initiate a HCI command
  hci_command_send(HCI_CMND_CLOSE_SOCKET, SOCKET_CLOSE_PARAMS_LEN);

  // Since we are in blocking state - wait for event complete
  hci_wait_for_event(HCI_CMND_CLOSE_SOCKET, &ret);
  errno = ret;

  return(ret);
}

//*****************************************************************************
//
//! accept
//!
//!  @param[in]   sd      socket descriptor (handle)
//!  @param[out]  addr    the argument addr is a pointer to a sockaddr structure
//!                       This structure is filled in with the address of the
//!                       peer socket, as known to the communications layer.
//!                       determined. The exact format of the address returned
//!                       addr is by the socket's address sockaddr.
//!                       On this version only AF_INET is supported.
//!                       This argument returns in network order.
//!  @param[out] addrlen  the addrlen argument is a value-result argument:
//!                       it should initially contain the size of the structure
//!                       pointed to by addr.
//!
//!  @return  For socket in blocking mode:
//!                   On success, socket handle. on failure negative
//!               For socket in non-blocking mode:
//!                  - On connection establishment, socket handle
//!                  - On connection pending, SOC_IN_PROGRESS (-2)
//!                - On failure, SOC_ERROR  (-1)
//!
//!  @brief  accept a connection on a socket:
//!          This function is used with connection-based socket types
//!          (SOCK_STREAM). It extracts the first connection request on the
//!          queue of pending connections, creates a new connected socket, and
//!          returns a new file descriptor referring to that socket.
//!          The newly created socket is not in the listening state.
//!          The original socket sd is unaffected by this call.
//!          The argument sd is a socket that has been created with socket(),
//!          bound to a local address with bind(), and is  listening for
//!          connections after a listen(). The argument addr is a pointer
//!          to a sockaddr structure. This structure is filled in with the
//!          address of the peer socket, as known to the communications layer.
//!          The exact format of the address returned addr is determined by the
//!          socket's address family. The addrlen argument is a value-result
//!          argument: it should initially contain the size of the structure
//!          pointed to by addr, on return it will contain the actual
//!          length (in bytes) of the address returned.
//!
//! @sa     socket ; bind ; listen
//
//*****************************************************************************
long c_accept(long sd, sockaddr *addr, socklen_t *addrlen)
{
  long ret;
  uint8_t *args;
  tBsdReturnParams tAcceptReturnArguments;

  ret = EFAIL;
  args = hci_get_cmd_buffer();

  // Fill in temporary command buffer
  args = UINT32_TO_STREAM(args, sd);

  // Initiate a HCI command
  hci_command_send(HCI_CMND_ACCEPT, SOCKET_ACCEPT_PARAMS_LEN);

  // Since we are in blocking state - wait for event complete
  hci_wait_for_event(HCI_CMND_ACCEPT, &tAcceptReturnArguments);

  // need specify return parameters!!!
  memcpy(addr, &tAcceptReturnArguments.tSocketAddress, ASIC_ADDR_LEN);
  *addrlen = ASIC_ADDR_LEN;
  errno = tAcceptReturnArguments.iStatus;
  ret = errno;

  return(ret);
}

//*****************************************************************************
//
//! bind
//!
//!  @param[in]   sd      socket descriptor (handle)
//!  @param[out]  addr    specifies the destination address. On this version
//!                       only AF_INET is supported.
//!  @param[out] addrlen  contains the size of the structure pointed to by addr.
//!
//!  @return    On success, zero is returned. On error, -1 is returned.
//!
//!  @brief  assign a name to a socket
//!          This function gives the socket the local address addr.
//!          addr is addrlen bytes long. Traditionally, this is called when a
//!          socket is created with socket, it exists in a name space (address
//!          family) but has no name assigned.
//!          It is necessary to assign a local address before a SOCK_STREAM
//!          socket may receive connections.
//!
//! @sa     socket ; accept ; listen
//
//*****************************************************************************
long c_bind(long sd, const sockaddr *addr, long addrlen)
{
  long ret;
  uint8_t *args;

  ret = EFAIL;
  args = hci_get_cmd_buffer();

  addrlen = ASIC_ADDR_LEN;

  // Fill in temporary command buffer
  args = UINT32_TO_STREAM(args, sd);
  args = UINT32_TO_STREAM(args, 0x00000008);
  args = UINT32_TO_STREAM(args, addrlen);
  ARRAY_TO_STREAM(args, ((uint8_t *)addr), addrlen);

  // Initiate a HCI command
  hci_command_send(HCI_CMND_BIND, SOCKET_BIND_PARAMS_LEN);

  // Since we are in blocking state - wait for event complete
  hci_wait_for_event(HCI_CMND_BIND, &ret);

  errno = ret;

  return(ret);
}

//*****************************************************************************
//
//! listen
//!
//!  @param[in]   sd      socket descriptor (handle)
//!  @param[in]  backlog  specifies the listen queue depth. On this version
//!                       backlog is not supported.
//!  @return    On success, zero is returned. On error, -1 is returned.
//!
//!  @brief  listen for connections on a socket
//!          The willingness to accept incoming connections and a queue
//!          limit for incoming connections are specified with listen(),
//!          and then the connections are accepted with accept.
//!          The listen() call applies only to sockets of type SOCK_STREAM
//!          The backlog parameter defines the maximum length the queue of
//!          pending connections may grow to.
//!
//! @sa     socket ; accept ; bind
//!
//! @note   On this version, backlog is not supported
//
//*****************************************************************************
long c_listen(long sd, long backlog)
{
  long ret;
  uint8_t *args;

  ret = EFAIL;
  args = hci_get_cmd_buffer();

  // Fill in temporary command buffer
  args = UINT32_TO_STREAM(args, sd);
  args = UINT32_TO_STREAM(args, backlog);

  // Initiate a HCI command
  hci_command_send(HCI_CMND_LISTEN, SOCKET_LISTEN_PARAMS_LEN);

  // Since we are in blocking state - wait for event complete
  hci_wait_for_event(HCI_CMND_LISTEN, &ret);
  errno = ret;

  return(ret);
}

//*****************************************************************************
//
//! gethostbyname
//!
//!  @param[in]   hostname     host name
//!  @param[in]   usNameLen    name length
//!  @param[out]  out_ip_addr  This parameter is filled in with host IP address.
//!                            In case that host name is not resolved,
//!                            out_ip_addr is zero.
//!  @return    On success, positive is returned. On error, negative is returned
//!
//!  @brief  Get host IP by name. Obtain the IP Address of machine on network,
//!          by its name.
//!
//!  @note  On this version, only blocking mode is supported. Also note that
//!          the function requires DNS server to be configured prior to its usage.
//
//*****************************************************************************
int
c_gethostbyname(const char * hostname, uint16_t usNameLen, uint32_t* out_ip_addr)
{
  tBsdGethostbynameParams ret;
  uint8_t *args;

  errno = EFAIL;

  if (usNameLen > HOSTNAME_MAX_LENGTH) {
    return errno;
  }

  args = hci_get_cmd_buffer();

  // Fill in HCI packet structure
  args = UINT32_TO_STREAM(args, 8);
  args = UINT32_TO_STREAM(args, usNameLen);
  ARRAY_TO_STREAM(args, hostname, usNameLen);

  // Initiate a HCI command
  hci_command_send(HCI_CMND_GETHOSTNAME, SOCKET_GET_HOST_BY_NAME_PARAMS_LEN + usNameLen - 1);

  // Since we are in blocking state - wait for event complete
  hci_wait_for_event(HCI_EVNT_BSD_GETHOSTBYNAME, &ret);

  errno = ret.retVal;

  (*((long*)out_ip_addr)) = ret.outputAddress;

  return (errno);
}

//*****************************************************************************
//
//! connect
//!
//!  @param[in]   sd       socket descriptor (handle)
//!  @param[in]   addr     specifies the destination addr. On this version
//!                        only AF_INET is supported.
//!  @param[out]  addrlen  contains the size of the structure pointed to by addr
//!  @return    On success, zero is returned. On error, -1 is returned
//!
//!  @brief  initiate a connection on a socket
//!          Function connects the socket referred to by the socket descriptor
//!          sd, to the address specified by addr. The addrlen argument
//!          specifies the size of addr. The format of the address in addr is
//!          determined by the address space of the socket. If it is of type
//!          SOCK_DGRAM, this call specifies the peer with which the socket is
//!          to be associated; this address is that to which datagrams are to be
//!          sent, and the only address from which datagrams are to be received.
//!          If the socket is of type SOCK_STREAM, this call attempts to make a
//!          connection to another socket. The other socket is specified  by
//!          address, which is an address in the communications space of the
//!          socket. Note that the function implements only blocking behavior
//!          thus the caller will be waiting either for the connection
//!          establishment or for the connection establishment failure.
//!
//!  @sa socket
//
//*****************************************************************************
long c_connect(long sd, const sockaddr *addr, long addrlen)
{
  long ret;
  uint8_t *args;

  ret = EFAIL;
  args = hci_get_cmd_buffer();
  addrlen = 8;

  // Fill in temporary command buffer
  args = UINT32_TO_STREAM(args, sd);
  args = UINT32_TO_STREAM(args, 0x00000008);
  args = UINT32_TO_STREAM(args, addrlen);
  ARRAY_TO_STREAM(args, ((uint8_t *)addr), addrlen);

  // Initiate a HCI command
  hci_command_send(HCI_CMND_CONNECT, SOCKET_CONNECT_PARAMS_LEN);

  // Since we are in blocking state - wait for event complete
  hci_wait_for_event(HCI_CMND_CONNECT, &ret);

  errno = ret;

  return ret;
}


//*****************************************************************************
//
//! select
//!
//!  @param[in]   nfds       the highest-numbered file descriptor in any of the
//!                           three sets, plus 1.
//!  @param[out]   writesds   socket descriptors list for write monitoring
//!  @param[out]   readsds    socket descriptors list for read monitoring
//!  @param[out]   exceptsds  socket descriptors list for exception monitoring
//!  @param[in]   timeout     is an upper bound on the amount of time elapsed
//!                           before select() returns. Null means infinity
//!                           timeout. The minimum timeout is 5 milliseconds,
//!                          less than 5 milliseconds will be set
//!                           automatically to 5 milliseconds.
//!  @return    On success, select() returns the number of file descriptors
//!             contained in the three returned descriptor sets (that is, the
//!             total number of bits that are set in readfds, writefds,
//!             exceptfds) which may be zero if the timeout expires before
//!             anything interesting  happens.
//!             On error, -1 is returned.
//!                   *readsds - return the sockets on which Read request will
//!                              return without delay with valid data.
//!                   *writesds - return the sockets on which Write request
//!                                 will return without delay.
//!                   *exceptsds - return the sockets which closed recently.
//!
//!  @brief  Monitor socket activity
//!          Select allow a program to monitor multiple file descriptors,
//!          waiting until one or more of the file descriptors become
//!         "ready" for some class of I/O operation
//!
//!  @Note   If the timeout value set to less than 5ms it will automatically set
//!          to 5ms to prevent overload of the system
//!
//!  @sa socket
//
//*****************************************************************************
int c_select(long nfds, wfd_set *readsds, wfd_set *writesds,
    wfd_set *exceptsds, struct timeval *timeout)
{
  uint8_t *args;
  tBsdSelectRecvParams tParams;
  uint32_t is_blocking;

  if( timeout == NULL) {
    is_blocking = 1; /* blocking , infinity timeout */
  }
  else {
    is_blocking = 0; /* no blocking, timeout */
  }

  // Fill in HCI packet structure
  args = hci_get_cmd_buffer();

  // Fill in temporary command buffer
  args = UINT32_TO_STREAM(args, nfds);
  args = UINT32_TO_STREAM(args, 0x00000014);
  args = UINT32_TO_STREAM(args, 0x00000014);
  args = UINT32_TO_STREAM(args, 0x00000014);
  args = UINT32_TO_STREAM(args, 0x00000014);
  args = UINT32_TO_STREAM(args, is_blocking);
  args = UINT32_TO_STREAM(args, ((readsds) ? *(uint32_t*)readsds : 0));
  args = UINT32_TO_STREAM(args, ((writesds) ? *(uint32_t*)writesds : 0));
  args = UINT32_TO_STREAM(args, ((exceptsds) ? *(uint32_t*)exceptsds : 0));

  if (timeout) {
    if ( 0 == timeout->tv_sec && timeout->tv_usec < SELECT_TIMEOUT_MIN_MICRO_SECONDS) {
      timeout->tv_usec = SELECT_TIMEOUT_MIN_MICRO_SECONDS;
    }
    args = UINT32_TO_STREAM(args, timeout->tv_sec);
    args = UINT32_TO_STREAM(args, timeout->tv_usec);
  }

  // Initiate a HCI command
  hci_command_send(HCI_CMND_BSD_SELECT, SOCKET_SELECT_PARAMS_LEN);

  // Since we are in blocking state - wait for event complete
  hci_wait_for_event(HCI_EVNT_SELECT, &tParams);

  // Update actually read FD
  if (tParams.iStatus >= 0) {
    if (readsds)
      memcpy(readsds, &tParams.uiRdfd, sizeof(tParams.uiRdfd));

    if (writesds)
      memcpy(writesds, &tParams.uiWrfd, sizeof(tParams.uiWrfd));

    if (exceptsds)
      memcpy(exceptsds, &tParams.uiExfd, sizeof(tParams.uiExfd));

    return(tParams.iStatus);
  }
  else {
    errno = tParams.iStatus;
    return(-1);
  }
}

//*****************************************************************************
//
//! setsockopt
//!
//!  @param[in]   sd          socket handle
//!  @param[in]   level       defines the protocol level for this option
//!  @param[in]   optname     defines the option name to Interrogate
//!  @param[in]   optval      specifies a value for the option
//!  @param[in]   optlen      specifies the length of the option value
//!  @return    On success, zero is returned. On error, -1 is returned
//!
//!  @brief  set socket options
//!          This function manipulate the options associated with a socket.
//!          Options may exist at multiple protocol levels; they are always
//!          present at the uppermost socket level.
//!          When manipulating socket options the level at which the option
//!          resides and the name of the option must be specified.
//!          To manipulate options at the socket level, level is specified as
//!          SOL_SOCKET. To manipulate options at any other level the protocol
//!          number of the appropriate protocol controlling the option is
//!          supplied. For example, to indicate that an option is to be
//!          interpreted by the TCP protocol, level should be set to the
//!          protocol number of TCP;
//!          The parameters optval and optlen are used to access optval -
//!          use for setsockopt(). For getsockopt() they identify a buffer
//!          in which the value for the requested option(s) are to
//!          be returned. For getsockopt(), optlen is a value-result
//!          parameter, initially containing the size of the buffer
//!          pointed to by option_value, and modified on return to
//!          indicate the actual size of the value returned. If no option
//!          value is to be supplied or returned, option_value may be NULL.
//!
//!  @Note   On this version the following two socket options are enabled:
//!              The only protocol level supported in this version
//!          is SOL_SOCKET (level).
//!            1. SOCKOPT_RECV_TIMEOUT (optname)
//!               SOCKOPT_RECV_TIMEOUT configures recv and recvfrom timeout
//!           in milliseconds.
//!             In that case optval should be pointer to uint32_t.
//!            2. SOCKOPT_NONBLOCK (optname). sets the socket non-blocking mode on
//!           or off.
//!             In that case optval should be SOCK_ON or SOCK_OFF (optval).
//!
//!  @sa getsockopt
//
//*****************************************************************************
int
c_setsockopt(long sd, long level, long optname, const void *optval, socklen_t optlen)
{
  int ret;
  uint8_t *args;

  args = hci_get_cmd_buffer();

  // Fill in temporary command buffer
  args = UINT32_TO_STREAM(args, sd);
  args = UINT32_TO_STREAM(args, level);
  args = UINT32_TO_STREAM(args, optname);
  args = UINT32_TO_STREAM(args, 0x00000008);
  args = UINT32_TO_STREAM(args, optlen);
  ARRAY_TO_STREAM(args, ((uint8_t *)optval), optlen);

  // Initiate a HCI command
  hci_command_send(HCI_CMND_SETSOCKOPT, SOCKET_SET_SOCK_OPT_PARAMS_LEN  + optlen);

  // Since we are in blocking state - wait for event complete
  hci_wait_for_event(HCI_CMND_SETSOCKOPT, &ret);

  if (ret >= 0) {
    return (0);
  }
  else {
    errno = ret;
    return (errno);
  }
}

//*****************************************************************************
//
//! getsockopt
//!
//!  @param[in]   sd          socket handle
//!  @param[in]   level       defines the protocol level for this option
//!  @param[in]   optname     defines the option name to Interrogate
//!  @param[out]   optval      specifies a value for the option
//!  @param[out]   optlen      specifies the length of the option value
//!  @return    On success, zero is returned. On error, -1 is returned
//!
//!  @brief  set socket options
//!          This function manipulate the options associated with a socket.
//!          Options may exist at multiple protocol levels; they are always
//!          present at the uppermost socket level.
//!          When manipulating socket options the level at which the option
//!          resides and the name of the option must be specified.
//!          To manipulate options at the socket level, level is specified as
//!          SOL_SOCKET. To manipulate options at any other level the protocol
//!          number of the appropriate protocol controlling the option is
//!          supplied. For example, to indicate that an option is to be
//!          interpreted by the TCP protocol, level should be set to the
//!          protocol number of TCP;
//!          The parameters optval and optlen are used to access optval -
//!          use for setsockopt(). For getsockopt() they identify a buffer
//!          in which the value for the requested option(s) are to
//!          be returned. For getsockopt(), optlen is a value-result
//!          parameter, initially containing the size of the buffer
//!          pointed to by option_value, and modified on return to
//!          indicate the actual size of the value returned. If no option
//!          value is to be supplied or returned, option_value may be NULL.
//!
//!  @Note   On this version the following two socket options are enabled:
//!              The only protocol level supported in this version
//!          is SOL_SOCKET (level).
//!            1. SOCKOPT_RECV_TIMEOUT (optname)
//!               SOCKOPT_RECV_TIMEOUT configures recv and recvfrom timeout
//!           in milliseconds.
//!             In that case optval should be pointer to uint32_t.
//!            2. SOCKOPT_NONBLOCK (optname). sets the socket non-blocking mode on
//!           or off.
//!             In that case optval should be SOCK_ON or SOCK_OFF (optval).
//!
//!  @sa setsockopt
//
//*****************************************************************************
int c_getsockopt(long sd, long level, long optname, void *optval,
                         socklen_t *optlen)
{
  uint8_t *args;
  tBsdGetSockOptReturnParams  tRetParams;

  args = hci_get_cmd_buffer();

  // Fill in temporary command buffer
  args = UINT32_TO_STREAM(args, sd);
  args = UINT32_TO_STREAM(args, level);
  args = UINT32_TO_STREAM(args, optname);

  // Initiate a HCI command
  hci_command_send(HCI_CMND_GETSOCKOPT, SOCKET_GET_SOCK_OPT_PARAMS_LEN);

  // Since we are in blocking state - wait for event complete
  hci_wait_for_event(HCI_CMND_GETSOCKOPT, &tRetParams);

  if (((signed char)tRetParams.iStatus) >= 0) {
    *optlen = 4;
    memcpy(optval, tRetParams.ucOptValue, 4);
    return (0);
  }
  else {
    errno = tRetParams.iStatus;
    return (errno);
  }
}

//*****************************************************************************
//
//!  simple_link_recv
//!
//!  @param sd       socket handle
//!  @param buf      read buffer
//!  @param len      buffer length
//!  @param flags    indicates blocking or non-blocking operation
//!  @param from     pointer to an address structure indicating source address
//!  @param fromlen  source address structure size
//!
//!  @return         Return the number of bytes received, or -1 if an error
//!                  occurred
//!
//!  @brief          Read data from socket
//!                  Return the length of the message on successful completion.
//!                  If a message is too long to fit in the supplied buffer,
//!                  excess bytes may be discarded depending on the type of
//!                  socket the message is received from
//
//*****************************************************************************
static int
simple_link_recv(long sd, void *buf, long len, long flags, sockaddr *from,
                socklen_t *fromlen, long opcode)
{
  int ret;
  uint8_t *args;
  tBsdReadReturnParams tSocketReadEvent;

  args = hci_get_cmd_buffer();

  // Fill in HCI packet structure
  args = UINT32_TO_STREAM(args, sd);
  args = UINT32_TO_STREAM(args, len);
  args = UINT32_TO_STREAM(args, flags);

  // Generate the read command, and wait for the
  hci_command_send(opcode,  SOCKET_RECV_FROM_PARAMS_LEN);

  // Since we are in blocking state - wait for event complete
  hci_wait_for_event(opcode, &tSocketReadEvent);

  // In case the number of bytes is more then zero - read data
  if (tSocketReadEvent.iNumberOfBytes > 0) {
    // Wait for the data in a synchronous way. Here we assume that the bug is
    // big enough to store also parameters of receive from too....
    hci_wait_for_data(buf, (uint8_t *)from, (uint8_t *)fromlen);
    errno = 0;
    ret = tSocketReadEvent.iNumberOfBytes;
  }
  else if (tSocketReadEvent.iNumberOfBytes == 0) {
    errno = EAGAIN;
    ret = -1;
  }
  else {
    ret = errno = tSocketReadEvent.iNumberOfBytes;
  }

  return ret;
}

//*****************************************************************************
//
//!  recv
//!
//!  @param[in]  sd     socket handle
//!  @param[out] buf    Points to the buffer where the message should be stored
//!  @param[in]  len    Specifies the length in bytes of the buffer pointed to
//!                     by the buffer argument.
//!  @param[in] flags   Specifies the type of message reception.
//!                     On this version, this parameter is not supported.
//!
//!  @return         Return the number of bytes received, or -1 if an error
//!                  occurred
//!
//!  @brief          function receives a message from a connection-mode socket
//!
//!  @sa recvfrom
//!
//!  @Note On this version, only blocking mode is supported.
//
//*****************************************************************************
int c_recv(long sd, void *buf, long len, long flags)
{
  return(simple_link_recv(sd, buf, len, flags, NULL, NULL, HCI_CMND_RECV));
}

//*****************************************************************************
//
//!  recvfrom
//!
//!  @param[in]  sd     socket handle
//!  @param[out] buf    Points to the buffer where the message should be stored
//!  @param[in]  len    Specifies the length in bytes of the buffer pointed to
//!                     by the buffer argument.
//!  @param[in] flags   Specifies the type of message reception.
//!                     On this version, this parameter is not supported.
//!  @param[in] from   pointer to an address structure indicating the source
//!                    address: sockaddr. On this version only AF_INET is
//!                    supported.
//!  @param[in] fromlen   source address tructure size
//!
//!  @return         Return the number of bytes received, or -1 if an error
//!                  occurred
//!
//!  @brief         read data from socket
//!                 function receives a message from a connection-mode or
//!                 connectionless-mode socket. Note that raw sockets are not
//!                 supported.
//!
//!  @sa recv
//!
//!  @Note On this version, only blocking mode is supported.
//
//*****************************************************************************
int c_recvfrom(long sd, void *buf, long len, long flags, sockaddr *from,
                      socklen_t *fromlen)
{
  return(simple_link_recv(sd, buf, len, flags, from, fromlen,
      HCI_CMND_RECVFROM));
}

//*****************************************************************************
//
//!  simple_link_send
//!
//!  @param sd       socket handle
//!  @param buf      write buffer
//!  @param len      buffer length
//!  @param flags    On this version, this parameter is not supported
//!  @param to       pointer to an address structure indicating destination
//!                  address
//!  @param tolen    destination address structure size
//!
//!  @return         Return the number of bytes transmitted, or -1 if an error
//!                  occurred, or -2 in case there are no free buffers available
//!
//!  @brief          This function is used to transmit a message to another
//!                  socket
//
//*****************************************************************************
static int
simple_link_send(long sd, const void *buf, long len, long flags,
              const sockaddr *to, long tolen, long opcode)
{
  uint8_t uArgSize,  addrlen;
  uint8_t *pDataPtr, *args;
  uint32_t addr_offset = 0;
  int res;
  tBsdReadReturnParams tSocketSendEvent;

  // Check the bsd_arguments
  if ((res = consume_buf(sd)) != 0)
    return res;

  // Allocate a buffer and construct a packet and send it over spi
  args = hci_get_data_buffer();

  // Update the offset of data and parameters according to the command
  switch(opcode) {
    case HCI_CMND_SENDTO:
      addr_offset = len + sizeof(len) + sizeof(len);
      addrlen = 8;
      uArgSize = SOCKET_SENDTO_PARAMS_LEN;
      pDataPtr = args + SOCKET_SENDTO_PARAMS_LEN;
      break;

    case HCI_CMND_SEND:
      tolen = 0;
      to = NULL;
      uArgSize = HCI_CMND_SEND_ARG_LENGTH;
      pDataPtr = args + HCI_CMND_SEND_ARG_LENGTH;
      break;

    default:
      return -1;
  }

  // Fill in temporary command buffer
  args = UINT32_TO_STREAM(args, sd);
  args = UINT32_TO_STREAM(args, uArgSize - sizeof(sd));
  args = UINT32_TO_STREAM(args, len);
  args = UINT32_TO_STREAM(args, flags);

  if (opcode == HCI_CMND_SENDTO) {
    args = UINT32_TO_STREAM(args, addr_offset);
    args = UINT32_TO_STREAM(args, addrlen);
  }

  // Copy the data received from user into the TX Buffer
  ARRAY_TO_STREAM(pDataPtr, ((uint8_t *)buf), len);

  // In case we are using SendTo, copy the to parameters
  if (opcode == HCI_CMND_SENDTO) {
    ARRAY_TO_STREAM(pDataPtr, ((uint8_t *)to), tolen);
  }

  // Initiate a HCI command
  hci_data_send(opcode, uArgSize, len,(uint8_t*)to, tolen);
  if (opcode == HCI_CMND_SENDTO)
    hci_wait_for_event(HCI_EVNT_SENDTO, &tSocketSendEvent);
  else
    hci_wait_for_event(HCI_EVNT_SEND, &tSocketSendEvent);

  return  (len);
}


//*****************************************************************************
//
//!  send
//!
//!  @param sd       socket handle
//!  @param buf      Points to a buffer containing the message to be sent
//!  @param len      message size in bytes
//!  @param flags    On this version, this parameter is not supported
//!
//!  @return         Return the number of bytes transmitted, or -1 if an
//!                  error occurred
//!
//!  @brief          Write data to TCP socket
//!                  This function is used to transmit a message to another
//!                  socket.
//!
//!  @Note           On this version, only blocking mode is supported.
//!
//!  @sa             sendto
//
//*****************************************************************************
int c_send(long sd, const void *buf, long len, long flags)
{
  return(simple_link_send(sd, buf, len, flags, NULL, 0, HCI_CMND_SEND));
}

//*****************************************************************************
//
//!  sendto
//!
//!  @param sd       socket handle
//!  @param buf      Points to a buffer containing the message to be sent
//!  @param len      message size in bytes
//!  @param flags    On this version, this parameter is not supported
//!  @param to       pointer to an address structure indicating the destination
//!                  address: sockaddr. On this version only AF_INET is
//!                  supported.
//!  @param tolen    destination address structure size
//!
//!  @return         Return the number of bytes transmitted, or -1 if an
//!                  error occurred
//!
//!  @brief          Write data to TCP socket
//!                  This function is used to transmit a message to another
//!                  socket.
//!
//!  @Note           On this version, only blocking mode is supported.
//!
//!  @sa             send
//
//*****************************************************************************

int c_sendto(long sd, const void *buf, long len, long flags,
                  const sockaddr *to, socklen_t tolen)
{
  return(simple_link_send(sd, buf, len, flags, to, tolen, HCI_CMND_SENDTO));
}

//*****************************************************************************
//
//!  c_mdns_advertiser
//!
//!  @param[in] mdnsEnabled         flag to enable/disable the mDNS feature
//!  @param[in] deviceServiceName   Service name as part of the published
//!                                 canonical domain name
//!  @param[in] deviceServiceNameLength   Length of the service name
//!
//!
//!  @return   On success, zero is returned, return SOC_ERROR if socket was not
//!            opened successfully, or if an error occurred.
//!
//!  @brief    Set CC3000 in mDNS advertiser mode in order to advertise itself.
//
//*****************************************************************************
int c_mdns_advertiser(uint16_t mdnsEnabled, char * deviceServiceName, uint16_t deviceServiceNameLength)
{
  int ret;
  uint8_t *pArgs;

  if (deviceServiceNameLength > MDNS_DEVICE_SERVICE_MAX_LENGTH) {
    return EFAIL;
  }

  pArgs = hci_get_cmd_buffer();

  // Fill in HCI packet structure
  pArgs = UINT32_TO_STREAM(pArgs, mdnsEnabled);
  pArgs = UINT32_TO_STREAM(pArgs, 8);
  pArgs = UINT32_TO_STREAM(pArgs, deviceServiceNameLength);
  ARRAY_TO_STREAM(pArgs, deviceServiceName, deviceServiceNameLength);

  // Initiate a HCI command
  hci_command_send(HCI_CMND_MDNS_ADVERTISE, SOCKET_MDNS_ADVERTISE_PARAMS_LEN + deviceServiceNameLength);

  // Since we are in blocking state - wait for event complete
  hci_wait_for_event(HCI_EVNT_MDNS_ADVERTISE, &ret);

  return ret;
}
