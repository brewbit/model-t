
#ifndef SETTINGS_H
#define SETTINGS_H

#include "types.h"


typedef struct {
  temperature_unit_t unit;
} device_settings_t;


void
settings_init(void);

const device_settings_t*
settings_get(void);

void
settings_set(const device_settings_t* s);

#endif
