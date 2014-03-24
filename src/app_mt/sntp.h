
#ifndef SNTP_H
#define SNTP_H

#include "types.h"
#include "wifi/wlan.h"


void
sntp_init(void);

bool
sntp_time_available(void);

time_t
sntp_get_time(void);

#endif
