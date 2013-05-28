#ifndef TOUCH_H__
#define TOUCH_H__

#include "common.h"
#include "stdint.h"
#include "calibrate.h"


void
touch_init(void);
void
touch_exec(void);

void
touch_poll(void);

void
calibrate(void);

#endif
