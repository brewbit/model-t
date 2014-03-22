
/*
 * Copyright (c) 2013, Per Gantelius
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the copyright holders.
 */

#include <errno.h>
#include <string.h>

#include "wifi/socket.h"
#include "socket.h"

struct stfSocket
{
  int fileDescriptor;
  char* host;
  int port;
  int logErrors;
};

#ifdef DEBUG
static void sn_log(stfSocket* s, const char* fmt, ...)
{
  if (s->logErrors) {
    va_list args;
    va_start(args,fmt);
    vprintf(fmt,args);
    va_end(args);
  }
}
#endif

static int shouldStopOnError(stfSocket* s, int error, int* ignores, int numInores)
{
  (void)s;
  int i;
    
  if (error == 0) {
    //all good
    return 0;
  }
    
  //see if this error code should be ignored
  for (i = 0; i < numInores; i++) {
    if (ignores[i] == error) {
      return 0;
    }
  }

#ifdef DEBUG
  switch (error)
  {
  case EACCES:
  {
    sn_log(s, "errno == EACCES: (For UNIX domain sockets, which are identified by pathname) Write permission is denied on the destination socket file, or search permission is denied for one of the directories the path prefix. (See path_resolution(7).)\n");
    break;
  }
  case EAGAIN: //same as EWOULDBLOCK
  {
    sn_log(s, "errno == EAGAIN || errno == EWOULDBLOCK: The socket is marked nonblocking and the requested operation would block. POSIX.1-2001 allows either error to be returned for this case, and does not require these constants to have the same value, so a portable application should check for both possibilities.\n");
    break;
  }
  case EBADF:
  {
    sn_log(s, "errno == EBADF: An invalid descriptor was specified.\n");
    break;
  }
  case ECONNRESET:
  {
    sn_log(s, "errno == ECONNRESET: Connection reset by peer.\n");
    break;
  }
  case EDESTADDRREQ:
  {
    sn_log(s, "errno == EDESTADDRREQ: The socket is not connection-mode, and no peer address is set.\n");
    break;
  }
  case EFAULT:
  {
    sn_log(s, "errno == EFAULT: An invalid user space address was specified for an argument.\n");
    break;
  }
  case EINTR:
  {
    sn_log(s, "errno == EINTR: A signal occurred before any data was transmitted; see signal(7).\n");
    break;
  }
  case EINVAL:
  {
    sn_log(s, "errno == EINVAL: Invalid argument passed.\n");
    break;
  }
  case EISCONN:
  {
    sn_log(s, "errno == EISCONN: The connection-mode socket was connected already but a recipient was specified. (Now either this error is returned, or the recipient specification is ignored.)\n");
    break;
  }
  case EMSGSIZE:
  {
    sn_log(s, "errno == EMSGSIZE: The socket type requires that message be sent atomically, and the size of the message to be sent made this impossible.\n");
    break;
  }
  case ENOBUFS:
  {
    sn_log(s, "errno == ENOBUFS: The output queue for a network interface was full. This generally indicates that the interface has stopped sending, but may be caused by transient congestion. (Normally, this does not occur in  Linux. Packets are just silently dropped when a device queue overflows.)\n");
    break;
  }
  case ENOMEM:
  {
    sn_log(s, "errno == ENOMEM: No memory available\n");
    break;
  }
  case ENOTCONN:
  {
    sn_log(s, "errno == ENOTCONN: The socket is not connected, and no target has been given.\n");
    break;
  }
  case ENOTSOCK:
  {
    sn_log(s, "errno == ENOTSOCK: The argument sockfd is not a socket.\n");
    break;
  }
  case EOPNOTSUPP:
  {
    sn_log(s, "errno == EOPNOTSUPP: Some bit in the flags argument is inappropriate for the socket type.\n");
    break;
  }
  case EPIPE:
  {
    sn_log(s, "errno == EPIPE: The local end has been shut down on a connection oriented socket. In this case the process will also receive a SIGPIPE unless MSG_NOSIGNAL is set.\n");
    break;
  }
  case ENOPROTOOPT:
  {
    sn_log(s, "errno == ENOPROTOOPT: Protocol not available.\n");
    break;
  }
  case ETIMEDOUT:
  {
    sn_log(s, "errno == ETIMEDOUT\n");
    break;
  }
  default:
  {
    sn_log(s, "errno == %d.\n", error);
  }
}
    
#endif //DEBUG
    
    return 1;
}

stfSocket* stfSocket_new()
{
  stfSocket* newSocket = malloc(sizeof(stfSocket));
  memset(newSocket, 0, sizeof(stfSocket));
  newSocket->logErrors = 1;
  newSocket->fileDescriptor = -1;
  return newSocket;
}

void stfSocket_delete(stfSocket* socket)
{
  if (socket) {
    stfSocket_disconnect(socket);

    if (socket->host) {
      free(socket->host);
    }
    memset(socket, 0, sizeof(stfSocket));
    free(socket);
  }
}

int stfSocket_connect(stfSocket* s,
                      const char* host,
                      int port)
{
  if (s->fileDescriptor != -1) {
    //shut down existing connection
    stfSocket_disconnect(s);
  }

  unsigned long ip_addr;
  if (gethostbyname(host, strlen(host), &ip_addr) < 0)
    return 0;

  s->fileDescriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s->fileDescriptor == -1) {
    return 0;
  }

  //set socket to non-blocking
  unsigned long val = SOCK_ON;
  setsockopt(s->fileDescriptor, SOL_SOCKET, SOCKOPT_RECV_NONBLOCK, &val, sizeof(val));

  sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(ip_addr);
  if (connect(s->fileDescriptor, (sockaddr*)&addr, sizeof(addr)) != 0)
    return 0;

  return 1;
}

void stfSocket_disconnect(stfSocket* socket)
{
    free(socket->host);
    socket->host = 0;
    socket->port = 0;
    closesocket(socket->fileDescriptor);
    
    socket->fileDescriptor = -1;
}

int stfSocket_sendData(stfSocket* s, const char* data, int numBytes, int* numSentBytes,
                       stfSocketCancelCallback cancelCallback, void* callbackData)
{
  (void)numSentBytes;

  int numBytesSentTot = 0;
  while (numBytesSentTot < numBytes) {
    errno = 0;

    //check if we should abort
    if (cancelCallback) {
      if (cancelCallback(callbackData) == 0) {
        return 0;
      }
    }
        
    //try to send all the bytes we have left
    const int chunkSize = numBytes - numBytesSentTot;
    int ret = send(s->fileDescriptor,
        (const void*)(&data[numBytesSentTot]),
        chunkSize,
        0);
        
    if (ret >= 0) {
      numBytesSentTot += ret;
    }
    else {
      //check errors
      int ignores[2] = {EAGAIN, EWOULDBLOCK};
      if (shouldStopOnError(s, errno, ignores, 2)) {
        return 0;
      }
    }

    //printf("sent %d/%d\n", numBytesSentTot, numBytes);
  }

  *numSentBytes = numBytesSentTot;
    
  return 1;
}

int stfSocket_receiveData(stfSocket* s, char* data, int maxNumBytes, int* numBytesReceived)
{
  errno = 0;
//    assert(data);
  int success = 1;
  int bytesRecvd = recv(s->fileDescriptor,
      data,
      maxNumBytes,
      0);
    
  int ignores[2] = {EAGAIN, EWOULDBLOCK};
  if (shouldStopOnError(s, errno, ignores, 2)) {
    success = 0;
  }
    
  *numBytesReceived = bytesRecvd < 0 ? 0 : bytesRecvd;
    
  return success;
}
