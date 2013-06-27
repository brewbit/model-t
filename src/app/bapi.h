
#ifndef BAPI_H
#define BAPI_H

typedef struct {
  char* email;
  char* name;
} bapi_acct_info_t;


void bapi_init(void);

#endif
