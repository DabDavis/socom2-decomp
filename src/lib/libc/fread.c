/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdio/fread.c (MAIN 0x193508), the toolchain's libc. */

#include "newlib.h"

size_t
fread (buf, size, count, fp)
     void * buf;
     size_t size;
     size_t count;
     FILE * fp;
{
  register size_t resid;
  register char *p;
  register int r;
  size_t total;

  if ((resid = count * size) == 0)
    return 0;
  if (fp->_r < 0)
    fp->_r = 0;
  total = resid;
  p = buf;
  while (resid > (r = fp->_r))
    {
      (void) memcpy ((void *) p, (void *) fp->_p, (size_t) r);
      fp->_p += r;
      /* fp->_r = 0 ... done in __srefill */
      p += r;
      resid -= r;
      if (__srefill (fp))
	{
	  /* no more input: return partial result */
	  return (total - resid) / size;
	}
    }
  (void) memcpy ((void *) p, (void *) fp->_p, resid);
  fp->_r -= resid;
  fp->_p += resid;
  return count;
}
