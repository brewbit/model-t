
#include <ch.h>
#include <hal.h>

#include "ota_update.h"
#include "message.h"
#include "bbmt.pb.h"
#include "web_api.h"
#include "sxfs.h"
#include "dfuse.h"
#include "bootloader_api.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


static void
set_state(ota_update_state_t state);

static void
ota_update_dispatch(msg_id_t id, void* msg_data, void* listener_data, void* sub_data);

static void
dispatch_ota_update_start(void);

static void
dispatch_update_check_response(FirmwareUpdateCheckResponse* response);

static void
dispatch_fw_chunk(FirmwareDownloadResponse* update_chunk);

static void
firmware_download_request(void);


static ota_update_status_t status;


void
ota_update_init()
{
  status.state = OU_IDLE;

  msg_listener_t* l = msg_listener_create("ota_update", 2048, ota_update_dispatch, NULL);

  msg_subscribe(l, MSG_OTAU_CHECK, NULL);
  msg_subscribe(l, MSG_OTAU_START, NULL);
  msg_subscribe(l, MSG_API_FW_UPDATE_CHECK_RESPONSE, NULL);
  msg_subscribe(l, MSG_API_FW_CHUNK, NULL);
}

ota_update_status_t*
ota_update_get_status(void)
{
  return &status;
}

static void
set_state(ota_update_state_t state)
{
  status.state = state;
  msg_send(MSG_OTAU_STATUS, &status);
}

static void
ota_update_dispatch(msg_id_t id, void* msg_data, void* listener_data, void* sub_data)
{
  (void)listener_data;
  (void)sub_data;

  switch (id) {
  case MSG_OTAU_CHECK:
    msg_post(MSG_API_FW_UPDATE_CHECK, NULL);
    set_state(OU_CHECKING);
    break;

  case MSG_OTAU_START:
    dispatch_ota_update_start();
    break;

  case MSG_API_FW_UPDATE_CHECK_RESPONSE:
    dispatch_update_check_response(msg_data);
    break;

  case MSG_API_FW_CHUNK:
    dispatch_fw_chunk(msg_data);
    break;

  default:
    break;
  }
}

static void
dispatch_ota_update_start()
{
  set_state(OU_PREPARING);

  printf("part clear\r\n");

  if (!sxfs_erase(SP_UPDATE_IMG)) {
    printf("part clear failed\r\n");
    set_state(OU_FAILED);
    return;
  }

  set_state(OU_STARTING_DOWNLOAD);
  firmware_download_request();
}

static void
dispatch_update_check_response(FirmwareUpdateCheckResponse* response)
{
  if (response->update_available) {
    status.update_downloaded = 0;
    status.update_size = response->binary_size;
    strncpy(status.update_ver, response->version, sizeof(status.update_ver));
    set_state(OU_UPDATE_AVAILABLE);
    printf("update available %s %u\r\n", status.update_ver, (unsigned int)response->binary_size);
  }
  else {
    set_state(OU_UPDATE_NOT_AVAILABLE);
    printf("no update available\r\n");
  }
}

static void
dispatch_fw_chunk(FirmwareDownloadResponse* update_chunk)
{
  static uint32_t last_update_downloaded;
  static uint8_t invalid_chunk_count;

  status.update_downloaded += update_chunk->data.size;

  printf("downloaded %u / %u (%lu%%)\r\n",
      (unsigned int)status.update_downloaded,
      (unsigned int)status.update_size,
      (unsigned int)(100 * status.update_downloaded) / status.update_size);

  set_state(OU_DOWNLOADING);

  if (!sxfs_write(SP_UPDATE_IMG,
      update_chunk->offset,
      update_chunk->data.bytes,
      update_chunk->data.size)) {
    set_state(OU_FAILED);
    return;
  }

  if (status.update_downloaded >= status.update_size) {
    // Verify the integrity of the image that we just downloaded
    dfu_parse_result_t result = dfuse_verify(SP_UPDATE_IMG);
    if (result == DFU_PARSE_OK) {
      printf("image verified resetting to apply update...\r\n");

      set_state(OU_COMPLETE);
      msg_send(MSG_SHUTDOWN, NULL);

      chThdSleepSeconds(1);

      bootloader_load_update_img();
    }
    else {
      printf("dfuse verify failed %d\r\n", result);
      set_state(OU_FAILED);
    }
  }
  else if (status.update_downloaded < last_update_downloaded + 1) {
    if (++invalid_chunk_count > 20) {
      invalid_chunk_count = 0;
      set_state(OU_FAILED);
      printf("OTA update receive timeout\r\n");
    }
    firmware_download_request();
  }
  else
    firmware_download_request();

  last_update_downloaded = status.update_downloaded;
}

static void
firmware_download_request(void)
{
  firmware_update_t* firmware_data = malloc(sizeof(firmware_data));

  firmware_data->version = status.update_ver;
  firmware_data->offset = status.update_downloaded;
  firmware_data->size = MIN(1024, (status.update_size - status.update_downloaded));

  msg_post(MSG_API_FW_DNLD_RQST, firmware_data);
}
