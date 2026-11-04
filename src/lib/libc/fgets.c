/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdio/fgets.c (MAIN 0x192EC0), the toolchain's libc. */

#include "newlib.h"

char *
fgets (buf, n, fp)
     char *buf;
     int n;
     FILE * fp;
{
  size_t len;
  char *s;
  unsigned char *p, *t;

  if (n < 2)			/* sanity check */
    return 0;

  s = buf;
  n--;				/* leave space for NUL */
  do
    {
      /*
       * If the buffer is empty, refill it.
       */
      if ((len = fp->_r) <= 0)
	{
	  if (__srefill (fp))
	    {
	      /* EOF: stop with partial or no line */
	      if (s == buf)
		return 0;
	      break;
	    }
	  len = fp->_r;
	}
      p = fp->_p;

      /*
       * Scan through at most n bytes of the current buffer,
       * looking for '\n'.  If found, copy up to and including
       * newline, and stop.  Otherwise, copy entire chunk
       * and loop.
       */
      if (len > n)
	len = n;
      t = (unsigned char *) memchr ((void *) p, '\n', len);
      if (t != 0)
	{
	  len = ++t - p;
	  fp->_r -= len;
	  fp->_p = t;
	  (void) memcpy ((void *) s, (void *) p, len);
	  s[len] = 0;
	  return (buf);
	}
      fp->_r -= len;
      fp->_p += len;
      (void) memcpy ((void *) s, (void *) p, len);
      s += len;
    }
  while ((n -= len) != 0);
  *s = 0;
  return buf;
}
