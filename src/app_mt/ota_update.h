
#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

typedef enum {
  OU_IDLE,
  OU_CHECKING,
  OU_UPDATE_AVAILABLE,
  OU_UPDATE_NOT_AVAILABLE,
  OU_PREPARING,
  OU_STARTING_DOWNLOAD,
  OU_DOWNLOADING,
  OU_COMPLETE,
  OU_FAILED
} ota_update_state_t;

typedef struct {
  ota_update_state_t state;
  char update_ver[16];
  uint32_t update_size;
  uint32_t update_downloaded;
} ota_update_status_t;

void
ota_update_init(void);

ota_update_status_t*
ota_update_get_status(void);

#endif
