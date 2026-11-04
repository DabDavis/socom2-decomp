/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdio/rget.c (MAIN 0x197FC8), the toolchain's libc: getc's slow path. */

#include "newlib.h"

int
__srget (fp)
     register FILE *fp;
{
  if (__srefill (fp) == 0)
    {
      fp->_r--;
      return *fp->_p++;
    }
  return EOF;
}
