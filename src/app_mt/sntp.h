
#ifndef SNTP_H
#define SNTP_H

#include "wifi/wlan.h"


void
sntp_init(void);

time_t
sntp_get_time(void);

#endif
