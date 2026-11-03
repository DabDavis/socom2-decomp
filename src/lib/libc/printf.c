/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdio/printf.c (MAIN 0x196C20), the toolchain's libc: printf as newlib wrote it. */

#include <stdarg.h>
#include "newlib.h"

extern int vfprintf(FILE *fp, const char *fmt, va_list ap);

int
printf (const char *fmt, ...)
{
  int ret;
  va_list ap;

  va_start (ap, fmt);
  _stdout_r (_REENT)->_data = _REENT;
  ret = vfprintf (_stdout_r (_REENT), fmt, ap);
  va_end (ap);
  return ret;
}
