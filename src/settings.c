
#include "ch.h"
#include "settings.h"
#include "message.h"


static device_settings_t settings;


void
settings_init()
{
  // TODO load settings from flash
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
  msg_broadcast(MSG_SETTINGS, &settings);
}
