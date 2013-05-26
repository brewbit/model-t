
#include "txn.h"

#include <stdlib.h>
#include <string.h>



txn_cache_t*
txn_cache_new()
{
  txn_cache_t* tc = malloc(sizeof(txn_cache_t));
  memset(tc, 0, sizeof(txn_cache_t));
  return tc;
}


txn_t*
txn_new(txn_cache_t* tc, txn_callback_t callback, void* callback_data)
{
  txn_t* txn = malloc(sizeof(txn_t));

  txn->txn_id = tc->last_id++;
  txn->callback = callback;
  txn->callback_data = callback_data;
  txn->next = tc->txn_list;
  tc->txn_list = txn;

  return txn;
}

txn_t*
txn_find(txn_cache_t* tc, uint16_t txn_id)
{
  txn_t* txn;

  for (txn = tc->txn_list; txn != NULL; txn = txn->next) {
    if (txn->txn_id == txn_id)
      return txn;
  }

  return NULL;
}

void
txn_free(txn_cache_t* tc, txn_t* txn)
{
  txn_t* t;

  if (txn == NULL)
    return;

  for (t = tc->txn_list; t != NULL; t = t->next) {
    if (t->next == txn) {
      t->next = txn->next;
      break;
    }
  }

  free(txn);
}

void
txn_callback(txn_t* txn, void* response_data)
{
  if (txn == NULL)
    return;

  if (txn->callback == NULL)
    return;

  txn->callback(txn->callback_data, response_data);
}

// finds a transaction and calls the callback
void
txn_update(txn_cache_t* tc, uint16_t txn_id, void* response_data)
{
  txn_t* txn = txn_find(tc, txn_id);
  txn_callback(txn, response_data);
}

// same as txn_update, but also deletes the transaction
void
txn_complete(txn_cache_t* tc, uint16_t txn_id, void* response_data)
{
  txn_t* txn = txn_find(tc, txn_id);
  txn_callback(txn, response_data);
  txn_free(tc, txn);
}
