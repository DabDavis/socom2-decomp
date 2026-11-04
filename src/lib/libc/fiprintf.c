/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdio/fiprintf.c (MAIN 0x193298), the toolchain's libc: the integer-only fprintf,
 * into vfiprintf (0x19B360). */

#include <stdarg.h>
#include "newlib.h"

extern int vfiprintf(FILE *fp, const char *fmt, va_list ap);

int
fiprintf (FILE * fp, const char *fmt, ...)
{
  int ret;
  va_list ap;

  va_start (ap, fmt);
  ret = vfiprintf (fp, fmt, ap);
  va_end (ap);
  return ret;
}
