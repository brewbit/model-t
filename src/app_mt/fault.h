
#ifndef FAULT_H
#define FAULT_H

#define MAX_FAULT_DATA 256

typedef enum {
  FAULT_NONE,
  HARD_FAULT,
  BUS_FAULT,
  MEM_MANAGE_FAULT,
  USAGE_FAULT
} fault_type_t;

typedef struct {
  fault_type_t type;
  uint8_t data[MAX_FAULT_DATA];
} fault_data_t;

#endif
