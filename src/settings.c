
#include "ch.h"
#include "settings.h"
#include "message.h"
#include "iflash.h"
#include "common.h"


/* Settings stored in flash */
__attribute__ ((section("app_cfg")))
device_settings_t settings_stored;

/* Local RAM copy of settings */
static device_settings_t settings;


void
settings_init()
{
  settings = settings_stored;
}

const device_settings_t*
settings_get()
{
  return &settings;
}

void
settings_set(const device_settings_t* s)
{
  settings = *s;

  int ret = flashSectorErase(1);
  if (ret != FLASH_RETURN_SUCCESS)
    chprintf(stdout, "flash erase failed\r\n");
  else
    chprintf(stdout, "flash erase success\r\n");

  ret = flashWrite((flashaddr_t)&settings_stored, (char*)&settings, sizeof(settings));
  if (ret != FLASH_RETURN_SUCCESS)
    chprintf(stdout, "flash write failed\r\n");
  else
    chprintf(stdout, "flash write success\r\n");

  msg_broadcast(MSG_SETTINGS, &settings);
}
