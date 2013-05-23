
#ifndef WEB_API_H
#define WEB_API_H

typedef struct {
  char* email;
  char* name;
} bbtc_acct_info_t;


char* bbtc_get_token(void);
bbtc_acct_info_t* bbtc_get_acct_info(char* token);

#endif
