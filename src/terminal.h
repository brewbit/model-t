#ifndef TERMINAL_H__
#define TERMINAL_H__

#include <stdint.h>

void
terminal_clear();
void
terminal_write(char* str);
void
terminal_write_int(uint32_t num);
void
terminal_write_hex(uint32_t num);

#endif
