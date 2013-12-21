
#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

typedef enum {
  OU_PREPARING,
  OU_REQUESTING,
  OU_DOWNLOADING,
  OU_COMPLETE,
  OU_FAILED
} ota_update_state_t;

typedef struct {
  ota_update_state_t state;
} ota_update_status_t;

void
ota_update_init(void);

void
ota_update_start(void);

#endif
