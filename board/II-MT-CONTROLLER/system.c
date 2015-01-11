#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ch.h"

#ifndef USE_SEMIHOSTING
int _read_r(struct _reent *r, int file, char * ptr, int len)
{
  (void)r;
  (void)file;
  (void)ptr;
  (void)len;
  __errno_r(r) = EINVAL;
  return -1;
}

int _lseek_r(struct _reent *r, int file, int ptr, int dir)
{
  (void)r;
  (void)file;
  (void)ptr;
  (void)dir;

  return 0;
}

int _write_r(struct _reent *r, int file, char * ptr, int len)
{
  (void)r;
  (void)file;
  (void)ptr;
  return len;
}

int _close_r(struct _reent *r, int file)
{
  (void)r;
  (void)file;

  return 0;
}

int _fstat_r(struct _reent *r, int file, struct stat * st)
{
  (void)r;
  (void)file;

  memset(st, 0, sizeof(*st));
  st->st_mode = S_IFCHR;
  return 0;
}

int _isatty_r(struct _reent *r, int fd)
{
  (void)r;
  (void)fd;

  return 1;
}
#endif

caddr_t _sbrk_r(struct _reent *r, int incr)
{
  void *p;

  if (incr <= 0) {
    return (caddr_t)-1;
  }

  (void)r;
  p = chCoreAlloc((size_t)incr);
  if (p == NULL) {
    __errno_r(r) = ENOMEM;
    return (caddr_t)-1;
  }
  return (caddr_t)p;
}
