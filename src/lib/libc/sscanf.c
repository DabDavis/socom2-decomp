/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdio/sscanf.c (MAIN 0x198960-0x198A10), the toolchain's libc: eofread (static) and
 * sscanf, into __svfscanf (0x19DA58). */

#include <stdarg.h>
#include "newlib.h"

extern size_t strlen(const char *);
extern int __svfscanf(FILE *fp, const char *fmt, va_list ap);

/* ARGSUSED */
static int
eofread (void *cookie, char *buf, int len)
{
  return 0;
}

int
sscanf (const char *str, const char *fmt, ...)
{
  int ret;
  va_list ap;
  FILE f;

  f._flags = __SRD;
  f._bf._base = f._p = (unsigned char *) str;
  f._bf._size = f._r = strlen (str);
  f._read = eofread;
  f._ub._base = NULL;
  f._lb._base = NULL;
  f._data = _REENT;
  va_start (ap, fmt);
  ret = __svfscanf (&f, fmt, ap);
  va_end (ap);
  return ret;
}
