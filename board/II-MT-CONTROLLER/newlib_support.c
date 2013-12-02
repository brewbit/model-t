/**
* @file    newlib_support.c
* @brief   Recursive mutexes macros and structures. Newlib locking functions.
*/

#include <reent.h>

#include "ch.h"

/**
* @brief   Recursive mutex structure.
*/
typedef struct recursive_mutex_s {
  Mutex mutex;
  uint32_t count;
} recursive_mutex_t;

/**
* @brief   Static recursive mutex initializer.
* @details Statically initialized recursive mutexes require no explicit initialization.
*
* @param[in] name      the name of the recursive mutex variable
*/
#define EXT_MUTEX_DECL(name) recursive_mutex_t name = { _MUTEX_DATA(name.mutex), 0 }

/**
* @brief   Locks the specified recursive mutex.
*
* @param[in] rmp        pointer to the @p recursive_mutex_t structure
*/
static void lock_recursive(recursive_mutex_t *rmp) {
  chSysLock();
  if (chThdSelf() != rmp->mutex.m_owner) {
    // Not owned. Lock.
    chMtxLockS(&rmp->mutex);
  }
  rmp->count++;
  chSysUnlock();
}

/**
* @brief   Unlocks the specified recursive mutex.
*
* @param[in] rmp        pointer to the @p recursive_mutex_t structure
*/
static void unlock_recursive(recursive_mutex_t *rmp) {
  chSysLock();
  if (--rmp->count == 0) {
    // Last owned. Unlock.
    chMtxUnlockS();
  }
  chSysUnlock();
}

/**
* @brief   Recursive mutex for __malloc_lock.
*/
static EXT_MUTEX_DECL(malloc_recursive_mutex);

/**
* @brief   Implements __malloc_lock from newlib.
*
* @param[in] reent        pointer to the @p _reent structure
*/
void __malloc_lock (struct _reent *reent) {
  (void)reent;
  lock_recursive(&malloc_recursive_mutex);
}

/**
* @brief   Implements __malloc_unlock from newlib.
*
* @param[in] reent        pointer to the @p _reent structure
*/
void __malloc_unlock (struct _reent *reent) {
  (void)reent;
  unlock_recursive(&malloc_recursive_mutex);
}

/**
* @brief   Recursive mutex for __env_lock.
*/
static EXT_MUTEX_DECL(env_recursive_mutex);

/**
* @brief   Implements __env_lock from newlib.
*
* @param[in] reent        pointer to the @p _reent structure
*/
void __env_lock(struct _reent *reent) {
  (void)reent;
  lock_recursive(&env_recursive_mutex);
}

/**
* @brief   Implements __env_unlock from newlib.
*
* @param[in] reent        pointer to the @p _reent structure
*/
void __env_unlock (struct _reent *reent) {
  (void)reent;
  unlock_recursive(&env_recursive_mutex);
}

/**
* @brief   Mutex for __tz_lock.
*/
static MUTEX_DECL(tz_mutex);

void __tz_lock (void) {
  chMtxLock(&tz_mutex);
}

void __tz_unlock (void) {
  chMtxUnlock();
}

void
__assert_func(const char* file, int line, const char* func, const char* cond)
{
  (void)file;
  (void)line;
  (void)func;
  (void)cond;

  chDbgPanic("assertion failed");
}
