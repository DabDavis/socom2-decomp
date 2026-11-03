/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdio/printf.c (MAIN 0x196BC0-0x196C8C), the toolchain's libc: _printf_r and printf as
 * newlib wrote them. */

#include <stdarg.h>
#include "newlib.h"

extern int vfprintf(FILE *fp, const char *fmt, va_list ap);
extern int _vfprintf_r(struct _reent *, FILE *, const char *, va_list);

int
_printf_r (struct _reent *ptr, const char *fmt, ...)
{
  int ret;
  va_list ap;

  va_start (ap, fmt);
  ret = _vfprintf_r (ptr, _stdout_r (ptr), fmt, ap);
  va_end (ap);
  return ret;
}

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
