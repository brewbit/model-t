/*****************************************************************************
*
*  wlan.c  - CC3000 Host Driver Implementation.
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
//! \addtogroup wlan_api
//! @{
//
//*****************************************************************************
#include "ch.h"
#include "hal.h"
#include "wlan.h"
#include "hci.h"
#include "cc3000_spi.h"
#include "socket.h"
#include "nvmem.h"
#include "security.h"

#include <string.h>
#include <stdio.h>


#define SMART_CONFIG_PROFILE_SIZE       67      // 67 = 32 (max ssid) + 32 (max key) + 1 (SSID length) + 1 (security type) + 1 (key length)

#ifndef CC3000_UNENCRYPTED_SMART_CONFIG
uint8_t key[AES128_KEY_SIZE];
uint8_t profileArray[SMART_CONFIG_PROFILE_SIZE];
#endif //CC3000_UNENCRYPTED_SMART_CONFIG

/* patches type */
#define PATCHES_HOST_TYPE_WLAN_DRIVER   0x01
#define PATCHES_HOST_TYPE_WLAN_FW       0x02
#define PATCHES_HOST_TYPE_BOOTLOADER    0x03

#define SL_SET_SCAN_PARAMS_INTERVAL_LIST_SIZE   (16)
#define SL_SIMPLE_CONFIG_PREFIX_LENGTH (3)
#define ETH_ALEN                                (6)
#define MAXIMAL_SSID_LENGTH                     (32)

#define WLAN_SL_INIT_START_PARAMS_LEN           (1)
#define WLAN_PATCH_PARAMS_LENGTH                (8)
#define WLAN_SET_CONNECTION_POLICY_PARAMS_LEN   (12)
#define WLAN_DEL_PROFILE_PARAMS_LEN             (4)
#define WLAN_SET_MASK_PARAMS_LEN                (4)
#define WLAN_SET_SCAN_PARAMS_LEN                (100)
#define WLAN_GET_SCAN_RESULTS_PARAMS_LEN        (4)
#define WLAN_ADD_PROFILE_NOSEC_PARAM_LEN        (24)
#define WLAN_ADD_PROFILE_WEP_PARAM_LEN          (36)
#define WLAN_ADD_PROFILE_WPA_PARAM_LEN          (44)
#define WLAN_CONNECT_PARAM_LEN                  (29)
#define WLAN_SMART_CONFIG_START_PARAMS_LEN      (4)



//*****************************************************************************
//
//!  wlan_start
//!
//!  @param   patch_load_cmd -  Indicates how the device should load patches.
//!
//!  @return        none
//!
//!  @brief        Start WLAN device. This function asserts the enable pin of
//!                the device (WLAN_EN), starting the HW initialization process.
//!                The function blocked until device Initialization is completed.
//!                Function also configure patches (FW, driver or bootloader)
//!                and calls appropriate device callbacks.
//!
//!  @Note          Prior calling the function wlan_init shall be called.
//!  @Warning       This function must be called after wlan_init and before any
//!                 other wlan API
//!  @sa            wlan_init , wlan_stop
//!
//
//*****************************************************************************
void
c_wlan_start(patch_load_command_t patch_load_cmd)
{
  uint8_t *args;
  uint8_t patch_req_type;

  hci_init();

  spi_open();

  args = hci_get_cmd_buffer();

  UINT8_TO_STREAM(args, patch_load_cmd);

  if (patch_load_cmd == PATCH_LOAD_FROM_HOST) {
    hci_command_send(HCI_CMND_SIMPLE_LINK_START, WLAN_SL_INIT_START_PARAMS_LEN,
        HCI_EVNT_PATCHES_REQ, &patch_req_type);

    hci_patch_send(patch_req_type, 0, 0,
        HCI_EVNT_PATCHES_REQ, &patch_req_type);

    hci_patch_send(patch_req_type, 0, 0,
        HCI_EVNT_PATCHES_REQ, &patch_req_type);

    hci_patch_send(patch_req_type, 0, 0,
        HCI_CMND_SIMPLE_LINK_START, NULL);
  }
  else {
    hci_command_send(HCI_CMND_SIMPLE_LINK_START, WLAN_SL_INIT_START_PARAMS_LEN,
        HCI_CMND_SIMPLE_LINK_START, NULL);
  }


  // Read Buffer's size and finish
  hci_command_send(HCI_CMND_READ_BUFFER_SIZE, 0,
      HCI_CMND_READ_BUFFER_SIZE, 0);
}


//*****************************************************************************
//
//!  wlan_stop
//!
//!  @param         none
//!
//!  @return        none
//!
//!  @brief         Stop WLAN device by putting it into reset state.
//!
//!  @sa            wlan_start
//
//*****************************************************************************
void c_wlan_stop(void)
{
  spi_close();
}


//*****************************************************************************
//
//!  wlan_connect
//!
//!  @param    sec_type   security options:
//!               WLAN_SEC_UNSEC,
//!               WLAN_SEC_WEP (ASCII support only),
//!               WLAN_SEC_WPA or WLAN_SEC_WPA2
//!  @param    ssid       up to 32 bytes and is ASCII SSID of the AP
//!  @param    ssid_len   length of the SSID
//!  @param    bssid      6 bytes specified the AP bssid
//!  @param    key        up to 16 bytes specified the AP security key
//!  @param    key_len    key length
//!
//!  @return     On success, zero is returned. On error, negative is returned.
//!              Note that even though a zero is returned on success to trigger
//!              connection operation, it does not mean that CCC3000 is already
//!              connected. An asynchronous "Connected" event is generated when
//!              actual association process finishes and CC3000 is connected to
//!              the AP. If DHCP is set, An asynchronous "DHCP" event is
//!              generated when DHCP process is finish.
//!
//!
//!  @brief      Connect to AP
//!  @warning    Please Note that when connection to AP configured with security
//!              type WEP, please confirm that the key is set as ASCII and not
//!              as HEX.
//!  @sa         wlan_disconnect
//
//*****************************************************************************
long c_wlan_connect(wlan_security_t ulSecType, const char *ssid, long ssid_len,
                    const uint8_t *bssid, const uint8_t *key, long key_len)
{
    long ret;
    uint8_t *args;
    uint8_t bssid_zero[] = {0, 0, 0, 0, 0, 0};

    ret     = EFAIL;
    args = hci_get_cmd_buffer();

    // Fill in command buffer
    args = UINT32_TO_STREAM(args, 0x0000001c);
    args = UINT32_TO_STREAM(args, ssid_len);
    args = UINT32_TO_STREAM(args, ulSecType);
    args = UINT32_TO_STREAM(args, 0x00000010 + ssid_len);
    args = UINT32_TO_STREAM(args, key_len);
    args = UINT16_TO_STREAM(args, 0);

    // padding shall be zeroed
    if(bssid)
    {
        ARRAY_TO_STREAM(args, bssid, ETH_ALEN);
    }
    else
    {
        ARRAY_TO_STREAM(args, bssid_zero, ETH_ALEN);
    }

    ARRAY_TO_STREAM(args, ssid, ssid_len);

    if(key_len && key)
    {
        ARRAY_TO_STREAM(args, key, key_len);
    }

    // Initiate a HCI command
    hci_command_send(HCI_CMND_WLAN_CONNECT, WLAN_CONNECT_PARAM_LEN + ssid_len + key_len - 1,
        HCI_CMND_WLAN_CONNECT, &ret);
    errno = ret;

    return(ret);
}

//*****************************************************************************
//
//!  wlan_disconnect
//!
//!  @return    0 disconnected done, other CC3000 already disconnected
//!
//!  @brief      Disconnect connection from AP.
//!
//!  @sa         wlan_connect
//
//*****************************************************************************
long c_wlan_disconnect(void)
{
    long ret;

    ret = EFAIL;

    hci_command_send(HCI_CMND_WLAN_DISCONNECT, 0,
        HCI_CMND_WLAN_DISCONNECT, &ret);
    errno = ret;

    return(ret);
}

//*****************************************************************************
//
//!  wlan_ioctl_set_connection_policy
//!
//!  @param    should_connect_to_open_ap  enable(1), disable(0) connect to any
//!            available AP. This parameter corresponds to the configuration of
//!            item # 3 in the brief description.
//!  @param    should_use_fast_connect enable(1), disable(0). if enabled, tries
//!            to connect to the last connected AP. This parameter corresponds
//!            to the configuration of item # 1 in the brief description.
//!  @param    auto_start enable(1), disable(0) auto connect
//!            after reset and periodically reconnect if needed. This
//!            configuration configures option 2 in the above description.
//!
//!  @return     On success, zero is returned. On error, -1 is returned
//!
//!  @brief      When auto is enabled, the device tries to connect according
//!              the following policy:
//!              1) If fast connect is enabled and last connection is valid,
//!                 the device will try to connect to it without the scanning
//!                 procedure (fast). The last connection will be marked as
//!                 invalid, due to adding/removing profile.
//!              2) If profile exists, the device will try to connect it
//!                 (Up to seven profiles).
//!              3) If fast and profiles are not found, and open mode is
//!                 enabled, the device will try to connect to any AP.
//!              * Note that the policy settings are stored in the CC3000 NVMEM.
//!
//!  @sa         wlan_add_profile , wlan_ioctl_del_profile
//
//*****************************************************************************
long c_wlan_ioctl_set_connection_policy(uint32_t should_connect_to_open_ap,
    uint32_t ulShouldUseFastConnect,
    uint32_t ulUseProfiles)
{
    long ret;
    uint8_t *args;

    ret = EFAIL;
    args = hci_get_cmd_buffer();

    // Fill in HCI packet structure
    args = UINT32_TO_STREAM(args, should_connect_to_open_ap);
    args = UINT32_TO_STREAM(args, ulShouldUseFastConnect);
    args = UINT32_TO_STREAM(args, ulUseProfiles);

    // Initiate a HCI command
    hci_command_send(HCI_CMND_WLAN_IOCTL_SET_CONNECTION_POLICY, WLAN_SET_CONNECTION_POLICY_PARAMS_LEN,
        HCI_CMND_WLAN_IOCTL_SET_CONNECTION_POLICY, &ret);

    return(ret);
}

//*****************************************************************************
//
//!  wlan_add_profile
//!
//!  @param    ulSecType  WLAN_SEC_UNSEC,WLAN_SEC_WEP,WLAN_SEC_WPA,WLAN_SEC_WPA2
//!  @param    ucSsid    ssid  SSID up to 32 bytes
//!  @param    ulSsidLen ssid length
//!  @param    ucBssid   bssid  6 bytes
//!  @param    ulPriority ulPriority profile priority. Lowest priority:0.
//!  @param    ulPairwiseCipher_Or_TxKeyLen  key length for WEP security
//!  @param    ulGroupCipher_TxKeyIndex  key index
//!  @param    ulKeyMgmt        KEY management
//!  @param    ucPf_OrKey       security key
//!  @param    ulPassPhraseLen  security key length for WPA\WPA2
//!
//!  @return    On success, zero is returned. On error, -1 is returned
//!
//!  @brief     When auto start is enabled, the device connects to
//!             station from the profiles table. Up to 7 profiles are supported.
//!             If several profiles configured the device choose the highest
//!             priority profile, within each priority group, device will choose
//!             profile based on security policy, signal strength, etc
//!             parameters. All the profiles are stored in CC3000 NVMEM.
//!
//!  @sa        wlan_ioctl_del_profile
//
//*****************************************************************************
long c_wlan_add_profile(
    wlan_security_t ulSecType,
    uint8_t* ucSsid,
    uint32_t ulSsidLen,
    uint8_t *ucBssid,
    uint32_t ulPriority,
    uint32_t ulPairwiseCipher_Or_TxKeyLen,
    uint32_t ulGroupCipher_TxKeyIndex,
    uint32_t ulKeyMgmt,
    uint8_t* ucPf_OrKey,
    uint32_t ulPassPhraseLen)
{
    uint16_t arg_len;
    long ret;
    long i = 0;
    uint8_t *args;
    uint8_t bssid_zero[] = {0, 0, 0, 0, 0, 0};

    args = hci_get_cmd_buffer();

    args = UINT32_TO_STREAM(args, ulSecType);

    // Setup arguments in accordance with the security type
    switch (ulSecType)
    {
        //OPEN
        case WLAN_SEC_UNSEC:
        {
            args = UINT32_TO_STREAM(args, 0x00000014);
            args = UINT32_TO_STREAM(args, ulSsidLen);
            args = UINT16_TO_STREAM(args, 0);
            if(ucBssid)
            {
                ARRAY_TO_STREAM(args, ucBssid, ETH_ALEN);
            }
            else
            {
                ARRAY_TO_STREAM(args, bssid_zero, ETH_ALEN);
            }
            args = UINT32_TO_STREAM(args, ulPriority);
            ARRAY_TO_STREAM(args, ucSsid, ulSsidLen);

            arg_len = WLAN_ADD_PROFILE_NOSEC_PARAM_LEN + ulSsidLen;
        }
        break;

        //WEP
        case WLAN_SEC_WEP:
        {
            args = UINT32_TO_STREAM(args, 0x00000020);
            args = UINT32_TO_STREAM(args, ulSsidLen);
            args = UINT16_TO_STREAM(args, 0);
            if(ucBssid)
            {
                ARRAY_TO_STREAM(args, ucBssid, ETH_ALEN);
            }
            else
            {
                ARRAY_TO_STREAM(args, bssid_zero, ETH_ALEN);
            }
            args = UINT32_TO_STREAM(args, ulPriority);
            args = UINT32_TO_STREAM(args, 0x0000000C + ulSsidLen);
            args = UINT32_TO_STREAM(args, ulPairwiseCipher_Or_TxKeyLen);
            args = UINT32_TO_STREAM(args, ulGroupCipher_TxKeyIndex);
            ARRAY_TO_STREAM(args, ucSsid, ulSsidLen);

            for(i = 0; i < 4; i++)
            {
              uint8_t *p = &ucPf_OrKey[i * ulPairwiseCipher_Or_TxKeyLen];

                ARRAY_TO_STREAM(args, p, ulPairwiseCipher_Or_TxKeyLen);
            }

            arg_len = WLAN_ADD_PROFILE_WEP_PARAM_LEN + ulSsidLen +
                ulPairwiseCipher_Or_TxKeyLen * 4;

        }
        break;

        //WPA
        //WPA2
        case WLAN_SEC_WPA:
        case WLAN_SEC_WPA2:
        {
            args = UINT32_TO_STREAM(args, 0x00000028);
            args = UINT32_TO_STREAM(args, ulSsidLen);
            args = UINT16_TO_STREAM(args, 0);
            if(ucBssid)
            {
                ARRAY_TO_STREAM(args, ucBssid, ETH_ALEN);
            }
            else
            {
                ARRAY_TO_STREAM(args, bssid_zero, ETH_ALEN);
            }
            args = UINT32_TO_STREAM(args, ulPriority);
            args = UINT32_TO_STREAM(args, ulPairwiseCipher_Or_TxKeyLen);
            args = UINT32_TO_STREAM(args, ulGroupCipher_TxKeyIndex);
            args = UINT32_TO_STREAM(args, ulKeyMgmt);
            args = UINT32_TO_STREAM(args, 0x00000008 + ulSsidLen);
            args = UINT32_TO_STREAM(args, ulPassPhraseLen);
            ARRAY_TO_STREAM(args, ucSsid, ulSsidLen);
            ARRAY_TO_STREAM(args, ucPf_OrKey, ulPassPhraseLen);

            arg_len = WLAN_ADD_PROFILE_WPA_PARAM_LEN + ulSsidLen + ulPassPhraseLen;
        }

        break;

        default:
          return -1;
    }

    // Initiate a HCI command
    hci_command_send(HCI_CMND_WLAN_IOCTL_ADD_PROFILE, arg_len,
        HCI_CMND_WLAN_IOCTL_ADD_PROFILE, &ret);

    return(ret);
}

//*****************************************************************************
//
//!  wlan_ioctl_del_profile
//!
//!  @param    index   number of profile to delete
//!
//!  @return    On success, zero is returned. On error, -1 is returned
//!
//!  @brief     Delete WLAN profile
//!
//!  @Note      In order to delete all stored profile, set index to 255.
//!
//!  @sa        wlan_add_profile
//
//*****************************************************************************
long c_wlan_ioctl_del_profile(uint32_t ulIndex)
{
    long ret;
    uint8_t *args;

    args = hci_get_cmd_buffer();

    // Fill in HCI packet structure
    args = UINT32_TO_STREAM(args, ulIndex);
    ret = EFAIL;

    // Initiate a HCI command
    hci_command_send(
        HCI_CMND_WLAN_IOCTL_DEL_PROFILE, WLAN_DEL_PROFILE_PARAMS_LEN,
        HCI_CMND_WLAN_IOCTL_DEL_PROFILE, &ret);

    return(ret);
}

//*****************************************************************************
//
//!  wlan_ioctl_get_scan_results
//!
//!  @param[in]    scan_timeout   parameter not supported
//!  @param[out]   ucResults  scan results (_wlan_full_scan_results_args_t)
//!
//!  @return    On success, zero is returned. On error, -1 is returned
//!
//!  @brief    Gets entry from scan result table.
//!            The scan results are returned one by one, and each entry
//!            represents a single AP found in the area. The following is a
//!            format of the scan result:
//!          - 4 Bytes: number of networks found
//!          - 4 Bytes: The status of the scan: 0 - aged results,
//!                     1 - results valid, 2 - no results
//!          - 42 bytes: Result entry, where the bytes are arranged as  follows:
//!
//!                         - 1 bit isValid - is result valid or not
//!                         - 7 bits rssi - RSSI value;
//!                 - 2 bits: securityMode - security mode of the AP:
//!                           0 - Open, 1 - WEP, 2 WPA, 3 WPA2
//!                         - 6 bits: SSID name length
//!                         - 2 bytes: the time at which the entry has entered into
//!                            scans result table
//!                         - 32 bytes: SSID name
//!                 - 6 bytes:  BSSID
//!
//!  @Note      scan_timeout, is not supported on this version.
//!
//!  @sa        wlan_ioctl_set_scan_params
//
//*****************************************************************************
void
c_wlan_ioctl_get_scan_results(
    uint32_t ulScanTimeout,
    wlan_scan_results_t* results)
{
  uint8_t *args;

  args = hci_get_cmd_buffer();

  // Fill in temporary command buffer
  args = UINT32_TO_STREAM(args, ulScanTimeout);

  // Initiate a HCI command
  hci_command_send(
      HCI_CMND_WLAN_IOCTL_GET_SCAN_RESULTS, WLAN_GET_SCAN_RESULTS_PARAMS_LEN,
      HCI_CMND_WLAN_IOCTL_GET_SCAN_RESULTS, results);
}

//*****************************************************************************
//
//!  wlan_ioctl_set_scan_params
//!
//!  @param    uiEnable - start/stop application scan:
//!            1 = start scan with default interval value of 10 min.
//!            in order to set a different scan interval value apply the value
//!            in milliseconds. minimum 1 second. 0=stop). Wlan reset
//!           (wlan_stop() wlan_start()) is needed when changing scan interval
//!            value. Saved: No
//!  @param   uiMinDwellTime   minimum dwell time value to be used for each
//!           channel, in milliseconds. Saved: yes
//!           Recommended Value: 100 (Default: 20)
//!  @param   uiMaxDwellTime    maximum dwell time value to be used for each
//!           channel, in milliseconds. Saved: yes
//!           Recommended Value: 100 (Default: 30)
//!  @param   uiNumOfProbeRequests  max probe request between dwell time.
//!           Saved: yes. Recommended Value: 5 (Default:2)
//!  @param   uiChannelMask  bitwise, up to 13 channels (0x1fff).
//!           Saved: yes. Default: 0x7ff
//!  @param   uiRSSIThreshold   RSSI threshold. Saved: yes (Default: -80)
//!  @param   uiSNRThreshold    NSR threshold. Saved: yes (Default: 0)
//!  @param   uiDefaultTxPower  probe Tx power. Saved: yes (Default: 205)
//!  @param   aiIntervalList    pointer to array with 16 entries (16 channels)
//!           each entry (uint32_t) holds timeout between periodic scan
//!           (connection scan) - in millisecond. Saved: yes. Default 2000ms.
//!
//!  @return    On success, zero is returned. On error, -1 is returned
//!
//!  @brief    start and stop scan procedure. Set scan parameters.
//!
//!  @Note     uiDefaultTxPower, is not supported on this version.
//!
//!  @sa        wlan_ioctl_get_scan_results
//
//*****************************************************************************
long c_wlan_ioctl_set_scan_params(
    uint32_t uiEnable,
    uint32_t uiMinDwellTime,
    uint32_t uiMaxDwellTime,
    uint32_t uiNumOfProbeRequests,
    uint32_t uiChannelMask,
    long iRSSIThreshold,
    uint32_t uiSNRThreshold,
    uint32_t uiDefaultTxPower,
    const uint32_t *aiIntervalList)
{
  uint32_t  uiRes;
  uint8_t *args;

  args = hci_get_cmd_buffer();

  // Fill in temporary command buffer
  args = UINT32_TO_STREAM(args, 36);
  args = UINT32_TO_STREAM(args, uiEnable);
  args = UINT32_TO_STREAM(args, uiMinDwellTime);
  args = UINT32_TO_STREAM(args, uiMaxDwellTime);
  args = UINT32_TO_STREAM(args, uiNumOfProbeRequests);
  args = UINT32_TO_STREAM(args, uiChannelMask);
  args = UINT32_TO_STREAM(args, iRSSIThreshold);
  args = UINT32_TO_STREAM(args, uiSNRThreshold);
  args = UINT32_TO_STREAM(args, uiDefaultTxPower);
  ARRAY_TO_STREAM(args, aiIntervalList, sizeof(uint32_t) *
      SL_SET_SCAN_PARAMS_INTERVAL_LIST_SIZE);

  // Initiate a HCI command
  hci_command_send(HCI_CMND_WLAN_IOCTL_SET_SCANPARAM,
      WLAN_SET_SCAN_PARAMS_LEN,
      HCI_CMND_WLAN_IOCTL_SET_SCANPARAM, &uiRes);

  return(uiRes);
}

//*****************************************************************************
//
//!  wlan_set_event_mask
//!
//!  @param    mask   mask option:
//!       HCI_EVNT_WLAN_UNSOL_CONNECT connect event
//!       HCI_EVNT_WLAN_UNSOL_DISCONNECT disconnect event
//!       HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE  smart config done
//!       HCI_EVNT_WLAN_UNSOL_INIT init done
//!       HCI_EVNT_WLAN_UNSOL_DHCP dhcp event report
//!       HCI_EVNT_WLAN_ASYNC_PING_REPORT ping report
//!       HCI_EVNT_WLAN_KEEPALIVE keepalive
//!       HCI_EVNT_WLAN_TX_COMPLETE - disable information on end of transmission
//!       Saved: no.
//!
//!  @return    On success, zero is returned. On error, -1 is returned
//!
//!  @brief    Mask event according to bit mask. In case that event is
//!            masked (1), the device will not send the masked event to host.
//
//*****************************************************************************
long c_wlan_set_event_mask(uint32_t ulMask)
{
  long ret;
  uint8_t *args;

  ret = EFAIL;
  args = hci_get_cmd_buffer();

  // Fill in HCI packet structure
  args = UINT32_TO_STREAM(args, ulMask);

  // Initiate a HCI command
  hci_command_send(HCI_CMND_EVENT_MASK,
      WLAN_SET_MASK_PARAMS_LEN,
      HCI_CMND_EVENT_MASK, &ret);

  return(ret);
}

//*****************************************************************************
//
//!  wlan_ioctl_statusget
//!
//!  @param none
//!
//!  @return    WLAN_STATUS_DISCONNECTED, WLAN_STATUS_SCANING,
//!             STATUS_CONNECTING or WLAN_STATUS_CONNECTED
//!
//!  @brief    get wlan status: disconnected, scanning, connecting or connected
//
//*****************************************************************************
long c_wlan_ioctl_statusget(void)
{
    long ret;

    ret = EFAIL;

    hci_command_send(HCI_CMND_WLAN_IOCTL_STATUSGET, 0,
        HCI_CMND_WLAN_IOCTL_STATUSGET, &ret);

    return(ret);
}

//*****************************************************************************
//
//!  wlan_smart_config_start
//!
//!  @param    algoEncryptedFlag indicates whether the information is encrypted
//!
//!  @return   On success, zero is returned. On error, -1 is returned
//!
//!  @brief   Start to acquire device profile. The device acquire its own
//!           profile, if profile message is found. The acquired AP information
//!           is stored in CC3000 EEPROM only in case AES128 encryption is used.
//!           In case AES128 encryption is not used, a profile is created by
//!           CC3000 internally.
//!
//!  @Note    An asynchronous event - Smart Config Done will be generated as soon
//!           as the process finishes successfully.
//!
//!  @sa      wlan_smart_config_set_prefix , wlan_smart_config_stop
//
//*****************************************************************************
long c_wlan_smart_config_start(uint32_t algoEncryptedFlag)
{
  long ret;
  uint8_t *args;

  ret = EFAIL;
  args = hci_get_cmd_buffer();

  // Fill in HCI packet structure
  args = UINT32_TO_STREAM(args, algoEncryptedFlag);
  ret = EFAIL;

  hci_command_send(HCI_CMND_WLAN_IOCTL_SIMPLE_CONFIG_START,
      WLAN_SMART_CONFIG_START_PARAMS_LEN,
      HCI_CMND_WLAN_IOCTL_SIMPLE_CONFIG_START, &ret);

  return(ret);
}

//*****************************************************************************
//
//!  wlan_smart_config_stop
//!
//!  @param    algoEncryptedFlag indicates whether the information is encrypted
//!
//!  @return   On success, zero is returned. On error, -1 is returned
//!
//!  @brief   Stop the acquire profile procedure
//!
//!  @sa      wlan_smart_config_start , wlan_smart_config_set_prefix
//
//*****************************************************************************
long c_wlan_smart_config_stop(void)
{
  long ret;

  ret = EFAIL;

  hci_command_send(HCI_CMND_WLAN_IOCTL_SIMPLE_CONFIG_STOP, 0,
      HCI_CMND_WLAN_IOCTL_SIMPLE_CONFIG_STOP, &ret);

  return(ret);
}

//*****************************************************************************
//
//!  wlan_smart_config_set_prefix
//!
//!  @param   newPrefix  3 bytes identify the SSID prefix for the Smart Config.
//!
//!  @return   On success, zero is returned. On error, -1 is returned
//!
//!  @brief   Configure station ssid prefix. The prefix is used internally
//!           in CC3000. It should always be TTT.
//!
//!  @Note    The prefix is stored in CC3000 NVMEM
//!
//!  @sa      wlan_smart_config_start , wlan_smart_config_stop
//
//*****************************************************************************
long c_wlan_smart_config_set_prefix(char* cNewPrefix)
{
  long ret;
  uint8_t *args;

  ret = EFAIL;
  args = hci_get_cmd_buffer();

  if (cNewPrefix == NULL)
    return ret;
  else {
    *cNewPrefix = 'T';
    *(cNewPrefix + 1) = 'T';
    *(cNewPrefix + 2) = 'T';
  }

  ARRAY_TO_STREAM(args, cNewPrefix, SL_SIMPLE_CONFIG_PREFIX_LENGTH);

  hci_command_send(HCI_CMND_WLAN_IOCTL_SIMPLE_CONFIG_SET_PREFIX, SL_SIMPLE_CONFIG_PREFIX_LENGTH,
      HCI_CMND_WLAN_IOCTL_SIMPLE_CONFIG_SET_PREFIX, &ret);

  return(ret);
}

//*****************************************************************************
//
//!  wlan_smart_config_process
//!
//!  @param   none
//!
//!  @return   On success, zero is returned. On error, -1 is returned
//!
//!  @brief   process the acquired data and store it as a profile. The acquired
//!           AP information is stored in CC3000 EEPROM encrypted.
//!           The encrypted data is decrypted and stored as a profile.
//!           behavior is as defined by connection policy.
//
//*****************************************************************************

#ifndef CC3000_UNENCRYPTED_SMART_CONFIG
long c_wlan_smart_config_process()
{
  signed long returnValue;
  uint32_t ssidLen, keyLen;
  uint8_t *decKeyPtr;
  uint8_t *ssidPtr;

  // read the key from EEPROM - fileID 12
  returnValue = c_aes_read_key(key);

  if (returnValue != 0)
    return returnValue;

  // read the received data from fileID #13 and parse it according to the followings:
  // 1) SSID LEN - not encrypted
  // 2) SSID - not encrypted
  // 3) KEY LEN - not encrypted. always 32 bytes long
  // 4) Security type - not encrypted
  // 5) KEY - encrypted together with true key length as the first byte in KEY
  //   to elaborate, there are two corner cases:
  //      1) the KEY is 32 bytes long. In this case, the first byte does not represent KEY length
  //      2) the KEY is 31 bytes long. In this case, the first byte represent KEY length and equals 31
  returnValue = c_nvmem_read(NVMEM_SHARED_MEM_FILEID, SMART_CONFIG_PROFILE_SIZE, 0, profileArray);
  if (returnValue != 0)
    return returnValue;

  ssidPtr = &profileArray[1];

  ssidLen = profileArray[0];

  decKeyPtr = &profileArray[profileArray[0] + 3];

  c_aes_decrypt(decKeyPtr, key);
  if (profileArray[profileArray[0] + 1] > 16)
    c_aes_decrypt((uint8_t *)(decKeyPtr + 16), key);

  if (*(uint8_t *)(decKeyPtr +31) != 0) {
    if (*decKeyPtr == 31) {
      keyLen = 31;
      decKeyPtr++;
    }
    else {
      keyLen = 32;
    }
  }
  else {
    keyLen = *decKeyPtr;
    decKeyPtr++;
  }

  // add a profile
  switch (profileArray[profileArray[0] + 2]) {
    case WLAN_SEC_UNSEC://None
    {
      returnValue = c_wlan_add_profile(profileArray[profileArray[0] + 2],     // security type
          ssidPtr,                               // SSID
          ssidLen,                               // SSID length
          NULL,                                  // BSSID
          1,                                     // Priority
          0, 0, 0, 0, 0);

      break;
    }

    case WLAN_SEC_WEP://WEP
    {
      returnValue = c_wlan_add_profile(profileArray[profileArray[0] + 2],     // security type
          ssidPtr,                               // SSID
          ssidLen,                               // SSID length
          NULL,                                  // BSSID
          1,                                     // Priority
          keyLen,                                // KEY length
          0,                                     // KEY index
          0,
          decKeyPtr,                             // KEY
          0);
      break;
    }

    case WLAN_SEC_WPA://WPA
    case WLAN_SEC_WPA2://WPA2
    {
      returnValue = c_wlan_add_profile(profileArray[profileArray[0] + 2],     // security type
          ssidPtr,
          ssidLen,
          NULL,                                  // BSSID
          1,                                     // Priority
          0x18,                                  // PairwiseCipher
          0x1e,                                  // GroupCipher
          2,                                     // KEY management
          decKeyPtr,                             // KEY
          keyLen);                               // KEY length
      break;
    }
  }

  return returnValue;
}
#endif //CC3000_UNENCRYPTED_SMART_CONFIG

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
