
#include "ch.h"
#include "settings.h"
#include "message.h"
#include "iflash.h"
#include "common.h"

#include <string.h>


static msg_t settings_thread(void* arg);


/* Settings stored in flash */
__attribute__ ((section("app_cfg")))
device_settings_t settings_stored;

/* Local RAM copy of settings */
static device_settings_t settings;
Mutex settings_mtx;


void
settings_init()
{
  settings = settings_stored;
  chMtxInit(&settings_mtx);
  chThdCreateFromHeap(NULL, 512, NORMALPRIO, settings_thread, NULL);
}

static msg_t
settings_thread(void* arg)
{
  (void)arg;

  while (1) {
    chMtxLock(&settings_mtx);
    if (memcmp(&settings, &settings_stored, sizeof(settings)) != 0) {
      flashSectorErase(1);
      flashWrite((flashaddr_t)&settings_stored, (char*)&settings, sizeof(settings));
    }
    chMtxUnlock();

    chThdSleepSeconds(5);
  }

  return 0;
}

const device_settings_t*
settings_get()
{
  return &settings;
}

void
settings_set(const device_settings_t* s)
{
  chMtxLock(&settings_mtx);
  settings = *s;
  chMtxUnlock();

  msg_broadcast(MSG_SETTINGS, &settings);
}
