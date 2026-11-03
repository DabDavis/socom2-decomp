/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdio/vsprintf.c (MAIN 0x19E7E8), the toolchain's libc. */

#include <stdarg.h>
#include "newlib.h"

extern int vfprintf(FILE *fp, const char *fmt, va_list ap);

int
vsprintf (char *str, const char *fmt, va_list ap)
{
  int ret;
  FILE f;

  f._flags = __SWR | __SSTR;
  f._bf._base = f._p = (unsigned char *) str;
  f._bf._size = f._w = INT_MAX;
  f._data = _REENT;
  ret = vfprintf (&f, fmt, ap);
  *f._p = 0;
  return ret;
}
