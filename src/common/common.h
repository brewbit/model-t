#ifndef __COMMON_H__
#define __COMMON_H__

#include "ch.h"
#include "hal.h"

#define SETBIT(p, b) ((uint8_t*)(p))[(b) / 8] |= (1 << ((b) % 8))
#define CLRBIT(p, b) ((uint8_t*)(p))[(b) / 8] &= ~(1 << ((b) % 8))
#define ASSIGNBIT(p, b, v) if (v) { SETBIT(p, b); } else { CLRBIT(p, b); }
#define TESTBIT(p, b) ((((uint8_t*)(p))[(b) / 8] & (1 << ((b) % 8))) != 0)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define LIMIT(val, min, max) MIN(MAX(val, min), max)

#endif
