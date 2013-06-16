#ifndef TEMP_INPUT_H
#define TEMP_INPUT_H

#include "onewire.h"
#include <stdint.h>


struct temp_port_s;
typedef struct temp_port_s temp_port_t;


temp_port_t*
temp_input_init(SerialDriver* port);

#endif
