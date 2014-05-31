
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
#include "net.h"
#include "app_cfg.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#define CHUNK_TIMEOUT S2ST(30)

// Write a checkpoint after every 64KB downloaded
#define UPDATE_BLOCK_SIZE 0x10000


typedef enum {
  OU_ERR_ERASE = -1,
  OU_ERR_ERASE_VERIFY = -2,
  OU_ERR_WRITE = -3,
  OU_ERR_WRITE_VERIFY = -4
} ota_update_error_t;


typedef struct {
  systime_t chunk_request_time;
  bool download_in_progress;
  ota_update_state_t state;
  char update_ver[16];
  uint32_t last_block_offset;
  uint32_t update_size;
  uint32_t update_downloaded;
  int error_code;
} ota_update_t;


static void
set_state(ota_update_state_t state);

static void
write_checkpoint(void);

static void
ota_update_dispatch(msg_id_t id, void* msg_data, void* listener_data, void* sub_data);

static void
dispatch_idle(void);

static void
dispatch_api_status(api_status_t* ns);

static void
dispatch_ota_update_start(void);

static void
dispatch_update_check_response(FirmwareUpdateCheckResponse* response);

static void
dispatch_chunk(FirmwareDownloadResponse* update_chunk);

static void
firmware_download_request(uint32_t offset);


static ota_update_t update;


void
ota_update_init()
{
  const ota_update_checkpoint_t* checkpoint = app_cfg_get_ota_update_checkpoint();

  update.state = OU_WAIT_API_CONN;
  update.download_in_progress = checkpoint->download_in_progress;
  update.update_size = checkpoint->update_size;
  update.last_block_offset = checkpoint->last_block_offset;
  update.update_downloaded = checkpoint->last_block_offset;
  update.error_code = 0;
  strncpy(update.update_ver, checkpoint->update_ver, sizeof(update.update_ver));

  msg_listener_t* l = msg_listener_create("ota_update", 2048, ota_update_dispatch, NULL);
  msg_listener_set_idle_timeout(l, 1000);

  msg_subscribe(l, MSG_API_STATUS, NULL);
  msg_subscribe(l, MSG_OTAU_CHECK, NULL);
  msg_subscribe(l, MSG_OTAU_START, NULL);
  msg_subscribe(l, MSG_API_FW_UPDATE_CHECK_RESPONSE, NULL);
  msg_subscribe(l, MSG_API_FW_CHUNK, NULL);
}

ota_update_status_t
ota_update_get_status(void)
{
  ota_update_status_t status = {
      .state = update.state,
      .update_size = update.update_size,
      .update_downloaded = update.update_downloaded,
      .error_code = update.error_code
  };
  strncpy(status.update_ver, update.update_ver, sizeof(status.update_ver));
  return status;
}

static void
set_state(ota_update_state_t state)
{
  update.state = state;

  ota_update_status_t status = ota_update_get_status();
  msg_send(MSG_OTAU_STATUS, &status);
}

static void
ota_update_dispatch(msg_id_t id, void* msg_data, void* listener_data, void* sub_data)
{
  (void)listener_data;
  (void)sub_data;

  switch (id) {
  case MSG_API_STATUS:
    dispatch_api_status(msg_data);
    break;

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
    dispatch_chunk(msg_data);
    break;

  case MSG_IDLE:
    dispatch_idle();
    break;

  default:
    break;
  }
}

static void
dispatch_idle()
{
  if ((update.state == OU_DOWNLOADING) &&
      ((chTimeNow() - update.chunk_request_time) > CHUNK_TIMEOUT)) {
    firmware_download_request(update.last_block_offset);
    set_state(OU_CHUNK_TIMEOUT);
  }
}

static void
write_checkpoint()
{
  ota_update_checkpoint_t checkpoint = {
      .download_in_progress = update.download_in_progress,
      .update_size = update.update_size,
      .last_block_offset = update.last_block_offset,
  };
  strncpy(checkpoint.update_ver, update.update_ver, sizeof(checkpoint.update_ver));

  app_cfg_set_ota_update_checkpoint(&checkpoint);
  app_cfg_flush();
}

static void
dispatch_api_status(api_status_t* as)
{
  if (as->state == AS_CONNECTED) {
    if (update.download_in_progress) {
      set_state(OU_DOWNLOADING);
      firmware_download_request(update.last_block_offset);
    }
    else {
      set_state(OU_IDLE);
    }
  }
  else {
    if (update.state != OU_WAIT_API_CONN)
      set_state(OU_WAIT_API_CONN);
  }
}

static void
dispatch_ota_update_start()
{
  firmware_download_request(0);
}

static void
dispatch_update_check_response(FirmwareUpdateCheckResponse* response)
{
  if (response->update_available) {
    update.update_downloaded = 0;
    update.update_size = response->binary_size;
    strncpy(update.update_ver, response->version, sizeof(update.update_ver));
    set_state(OU_UPDATE_AVAILABLE);
  }
  else {
    set_state(OU_UPDATE_NOT_AVAILABLE);
  }
}

static void
dispatch_chunk(FirmwareDownloadResponse* update_chunk)
{
  update.update_downloaded = update_chunk->offset + update_chunk->data.size;

  if ((update_chunk->offset & (UPDATE_BLOCK_SIZE - 1)) == 0) {
    update.last_block_offset = update_chunk->offset;
    if (!sxfs_erase(SP_UPDATE_IMG, update.last_block_offset, UPDATE_BLOCK_SIZE)) {
      update.error_code = OU_ERR_ERASE;
      set_state(OU_FAILED);
      return;
    }

    if (!sxfs_is_erased(SP_UPDATE_IMG, update.last_block_offset, UPDATE_BLOCK_SIZE)) {
      update.error_code = OU_ERR_ERASE_VERIFY;
      set_state(OU_FAILED);
      return;
    }

    write_checkpoint();
  }

  if (!sxfs_write(SP_UPDATE_IMG,
      update_chunk->offset,
      update_chunk->data.bytes,
      update_chunk->data.size)) {
    update.error_code = OU_ERR_WRITE;
    set_state(OU_FAILED);
    return;
  }

  uint8_t* tmp = malloc(update_chunk->data.size);

  sxfs_read(SP_UPDATE_IMG, update_chunk->offset, tmp, update_chunk->data.size);
  if (memcmp(update_chunk->data.bytes, tmp, update_chunk->data.size)) {
    update.error_code = OU_ERR_WRITE_VERIFY;
    set_state(OU_FAILED);
    free(tmp);
    return;
  }

  free(tmp);

  if (update.update_downloaded >= update.update_size) {
    update.download_in_progress = false;
    update.update_size = 0;
    update.last_block_offset = 0;
    memset(update.update_ver, 0, sizeof(update.update_ver));
    write_checkpoint();

    // Verify the integrity of the image that we just downloaded
    dfu_parse_result_t result = dfuse_verify(SP_UPDATE_IMG);
    if (result == DFU_PARSE_OK) {
      set_state(OU_COMPLETE);
      msg_send(MSG_SHUTDOWN, NULL);

      chThdSleepSeconds(1);

      bootloader_load_update_img();
    }
    else {
      update.error_code = result;
      set_state(OU_FAILED);
    }
  }
  else {
    firmware_download_request(update_chunk->offset + update_chunk->data.size);
  }
}

static void
firmware_download_request(uint32_t offset)
{
  firmware_update_t* firmware_data = malloc(sizeof(firmware_data));

  firmware_data->version = update.update_ver;
  firmware_data->offset = offset;
  firmware_data->size = MIN(1024, (update.update_size - offset));

  update.chunk_request_time = chTimeNow();

  msg_post(MSG_API_FW_DNLD_RQST, firmware_data);
  set_state(OU_DOWNLOADING);
}
