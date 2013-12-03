/**
 * @file
 * SNTP client module
 *
 */

/*
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 */

#include "sntp.h"

#include <ch.h>
#include <hal.h>
#include <chprintf.h>

#include <string.h>

#include "net.h"
#include "wifi/wlan.h"
#include "wifi/socket.h"

/** This is an example of a "SNTP" client (with socket API).
 *
 * For a list of some public NTP servers, see this link :
 * http://support.ntp.org/bin/view/Servers/NTPPoolServers
 *
 */

/** SNTP server port */
#ifndef SNTP_PORT
#define SNTP_PORT                   123
#endif

/** SNTP server address as IPv4 address in "u32_t" format */
#ifndef SNTP_SERVER_ADDRESS
#define SNTP_SERVER_ADDRESS         IPV4_ADDR(129,6,15,28) /* pool.ntp.org */
#endif

/** SNTP receive timeout - in milliseconds */
#ifndef SNTP_RECV_TIMEOUT
#define SNTP_RECV_TIMEOUT           3000
#endif

/** SNTP update delay - in milliseconds */
#ifndef SNTP_UPDATE_DELAY
#define SNTP_UPDATE_DELAY           15000
#endif

/** SNTP macro to change system time and/or the update the RTC clock */
#ifndef SNTP_SYSTEM_TIME
#define SNTP_SYSTEM_TIME(t)
#endif

/* SNTP protocol defines */
#define SNTP_MAX_DATA_LEN           48
#define SNTP_RCV_TIME_OFS           32
#define SNTP_LI_NO_WARNING          0x00
#define SNTP_VERSION               (4/* NTP Version 4*/<<3)
#define SNTP_MODE_CLIENT            0x03
#define SNTP_MODE_SERVER            0x04
#define SNTP_MODE_BROADCAST         0x05
#define SNTP_MODE_MASK              0x07

/* number of seconds between 1900 and 1970 */
#define DIFF_SEC_1900_1970         (2208988800)

/**
 * SNTP processing
 */
static void
sntp_process(time_t t)
{
  /* change system time and/or the update the RTC clock */
  SNTP_SYSTEM_TIME(t);

  /* display local time from GMT time */
  chprintf(SD_STDIO, "sntp_process: %d", t);
}

/**
 * SNTP request
 */
static void
sntp_request(void)
{
  int                sock;
  sockaddr_in local;
  sockaddr_in to;
  socklen_t   tolen;
  int         size;
  int         timeout;
  uint8_t     sntp_request [SNTP_MAX_DATA_LEN];
  uint8_t     sntp_response[SNTP_MAX_DATA_LEN];
  uint32_t    sntp_server_address = 0;
  uint32_t    timestamp;
  time_t      t;

  chprintf(SD_STDIO, "making SNTP request\r\n");

  /* Lookup SNTP server address */
  const char* server_name = "pool.ntp.org";
  gethostbyname(server_name, strlen(server_name), &sntp_server_address);

  /* if we got a valid SNTP server address... */
  if (sntp_server_address != 0) {
    /* create new socket */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock >= 0) {
      /* prepare local address */
      memset(&local, 0, sizeof(local));
      local.sin_family      = AF_INET;
      local.sin_port        = htons(INADDR_ANY);
      local.sin_addr.s_addr = htonl(INADDR_ANY);

      chprintf(SD_STDIO, "binding\r\n");

      /* bind to local address */
      if (bind(sock, (sockaddr *)&local, sizeof(local)) == 0) {

        chprintf(SD_STDIO, "setsockopt\r\n");

        /* set recv timeout */
        timeout = SNTP_RECV_TIMEOUT;
        setsockopt(sock, SOL_SOCKET, SOCKOPT_RECV_TIMEOUT, (char *)&timeout, sizeof(timeout));

        /* prepare SNTP request */
        memset(sntp_request, 0, sizeof(sntp_request));
        sntp_request[0] = SNTP_LI_NO_WARNING | SNTP_VERSION | SNTP_MODE_CLIENT;

        /* prepare SNTP server address */
        memset(&to, 0, sizeof(to));
        to.sin_family      = AF_INET;
        to.sin_port        = htons(SNTP_PORT);
        to.sin_addr.s_addr = htonl(sntp_server_address);

        chprintf(SD_STDIO, "sendto\r\n");
        /* send SNTP request to server */
        if (sendto(sock, sntp_request, sizeof(sntp_request), 0, (sockaddr *)&to, sizeof(to)) >= 0) {
          chprintf(SD_STDIO, "recvfrom\r\n");

          /* receive SNTP server response */
          tolen = sizeof(to);
          size  = recvfrom(sock, sntp_response, sizeof(sntp_response), 0, (sockaddr *)&to, &tolen);

          chprintf(SD_STDIO, "recvfrom %d\r\n", size);

          /* if the response size is good */
          if (size == SNTP_MAX_DATA_LEN) {
            /* if this is a SNTP response... */
            if (((sntp_response[0] & SNTP_MODE_MASK) == SNTP_MODE_SERVER) || ((sntp_response[0] & SNTP_MODE_MASK) == SNTP_MODE_BROADCAST)) {
              /* extract GMT time from response */
              memcpy(&timestamp, (sntp_response+SNTP_RCV_TIME_OFS), sizeof(timestamp));
              t = (ntohl(timestamp) - DIFF_SEC_1900_1970);

              /* do time processing */
              sntp_process(t);
            }
            else {
              chprintf(SD_STDIO, "sntp_request: not response frame code\r\n");
            }
          }
          else {
            chprintf(SD_STDIO, "sntp_request: not recvfrom==%i\r\n", errno);
          }
        }
        else {
          chprintf(SD_STDIO, "sntp_request: not sendto==%i\r\n", errno);
        }
      }
      /* close the socket */
      closesocket(sock);
    }
  }
}

/**
 * SNTP thread
 */
static msg_t
sntp_thread(void *arg)
{
  (void)arg;

  while(1)
  {
    chThdSleepMilliseconds(SNTP_UPDATE_DELAY);
    sntp_request();
  }

  return 0;
}

void
sntp_init(void)
{
  chThdCreateFromHeap(NULL, 512, NORMALPRIO, sntp_thread, NULL);
}
