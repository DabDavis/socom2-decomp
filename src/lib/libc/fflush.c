/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdio/fflush.c (MAIN 0x192DA8), the toolchain's libc: a NULL stream flushes every
 * stream (_fwalk with itself). */

#include "newlib.h"

int
fflush (fp)
     register FILE * fp;
{
  register unsigned char *p;
  register int n, t;

  if (fp == NULL)
    return _fwalk (_REENT, fflush);

  CHECK_INIT (fp);

  t = fp->_flags;
  if ((t & __SWR) == 0 || (p = fp->_bf._base) == NULL)
    return 0;
  n = fp->_p - p;		/* write this much */

  /*
   * Set these immediately to avoid problems with longjmp
   * and to allow exchange buffering (via setvbuf) in user
   * write function.
   */
  fp->_p = p;
  fp->_w = t & (__SLBF | __SNBF) ? 0 : fp->_bf._size;

  while (n > 0)
    {
      t = (*fp->_write) (fp->_cookie, (char *) p, n);
      if (t <= 0)
	{
	  fp->_flags |= __SERR;
	  return EOF;
	}
      p += t;
      n -= t;
    }
  return 0;
}
