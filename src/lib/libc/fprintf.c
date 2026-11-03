/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdio/fprintf.c (MAIN 0x1934B0), the toolchain's libc. */

#include <stdarg.h>
#include "newlib.h"

extern int vfprintf(FILE *fp, const char *fmt, va_list ap);

int
fprintf (FILE * fp, const char *fmt, ...)
{
  int ret;
  va_list ap;

  va_start (ap, fmt);
  ret = vfprintf (fp, fmt, ap);
  va_end (ap);
  return ret;
}
