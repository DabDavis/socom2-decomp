/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdio/sprintf.c (MAIN 0x1988D0), the toolchain's libc: a string FILE on the stack. */

#include <stdarg.h>
#include "newlib.h"

extern int vfprintf(FILE *fp, const char *fmt, va_list ap);

int
sprintf (char *str, const char *fmt, ...)
{
  int ret;
  va_list ap;
  FILE f;

  f._flags = __SWR | __SSTR;
  f._bf._base = f._p = (unsigned char *) str;
  f._bf._size = f._w = INT_MAX;
  f._data = _REENT;
  va_start (ap, fmt);
  ret = vfprintf (&f, fmt, ap);
  va_end (ap);
  *f._p = 0;
  return (ret);
}
