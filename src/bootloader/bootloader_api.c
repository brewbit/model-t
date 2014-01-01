
#include "bootloader_api.h"


static const char* bootloader_get_version(void);


__attribute__ ((section("bootloader_api")))
const bootloader_api_t _bootloader_api = {
    .get_version = bootloader_get_version
};


static const char*
bootloader_get_version()
{
  return VERSION_STR;
}
