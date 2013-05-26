
#ifndef TXN_H
#define TXN_H

#include <stdint.h>

typedef void (*txn_callback_t)(void* callback_data, void* response_data);

typedef struct http_request_txn_s {
  uint32_t txn_id;
  txn_callback_t callback;
  void* callback_data;

  struct http_request_txn_s* next;
} txn_t;

typedef struct {
  uint32_t last_id;
  txn_t* txn_list;
} txn_cache_t;


txn_cache_t*
txn_cache_new(void);

txn_t*
txn_new(txn_cache_t* tc, txn_callback_t callback, void* callback_data);

txn_t*
txn_find(txn_cache_t* tc, uint32_t txn_id);

void
txn_free(txn_cache_t* tc, txn_t* txn);

void
txn_callback(txn_t* txn, void* response_data);

void
txn_update(txn_cache_t* tc, uint32_t txn_id, void* response_data);

void
txn_complete(txn_cache_t* tc, uint32_t txn_id, void* response_data);


#endif
