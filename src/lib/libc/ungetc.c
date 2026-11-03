/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdio/ungetc.c (MAIN 0x19AFF0-0x19B25C), the toolchain's libc: __submore (static: grow
 * the ungetc buffer) and ungetc. The demo's object 0x3E7BD0 holds the same two. */

#include "newlib.h"

static int
__submore (fp)
     register FILE *fp;
{
  register int i;
  register unsigned char *p;

  if (fp->_ub._base == fp->_ubuf)
    {
      /*
       * Get a new buffer (rather than expanding the old one).
       */
      if ((p = (unsigned char *) _malloc_r (fp->_data, (size_t) BUFSIZ)) == NULL)
	return EOF;
      fp->_ub._base = p;
      fp->_ub._size = BUFSIZ;
      p += BUFSIZ - sizeof (fp->_ubuf);
      for (i = sizeof (fp->_ubuf); --i >= 0;)
	p[i] = fp->_ubuf[i];
      fp->_p = p;
      return 0;
    }
  i = fp->_ub._size;
  p = (unsigned char *) _realloc_r (fp->_data, (void *) (fp->_ub._base), i << 1);
  if (p == NULL)
    return EOF;
  (void) memcpy ((void *) (p + i), (void *) p, (size_t) i);
  fp->_p = p + i;
  fp->_ub._base = p;
  fp->_ub._size = i << 1;
  return 0;
}

int
ungetc (c, fp)
     int c;
     register FILE *fp;
{
  if (c == EOF)
    return (EOF);

  /* Ensure stdio has been initialized.
     ??? Might be able to remove this as some other stdio routine should
     have already been called to get the char we are un-getting.  */

  CHECK_INIT (fp);

  /* After ungetc, we won't be at eof anymore */
  fp->_flags &= ~__SEOF;

  if ((fp->_flags & __SRD) == 0)
    {
      /*
       * Not already reading: no good unless reading-and-writing.
       * Otherwise, flush any current write stuff.
       */
      if ((fp->_flags & __SRW) == 0)
	return EOF;
      if (fp->_flags & __SWR)
	{
	  if (fflush (fp))
	    return EOF;
	  fp->_flags &= ~__SWR;
	  fp->_w = 0;
	  fp->_lbfsize = 0;
	}
      fp->_flags |= __SRD;
    }
  c = (unsigned char) c;

  /*
   * If we are in the middle of ungetc'ing, just continue.
   * This may require expanding the current ungetc buffer.
   */

  if (HASUB (fp))
    {
      if (fp->_r >= fp->_ub._size && __submore (fp))
	return EOF;
      *--fp->_p = c;
      fp->_r++;
      return c;
    }

  /*
   * If we can handle this by simply backing up, do so,
   * but never replace the original character.
   * (This makes sscanf() work when scanning `const' data.)
   */

  if (fp->_bf._base != NULL && fp->_p > fp->_bf._base && fp->_p[-1] == c)
    {
      fp->_p--;
      fp->_r++;
      return c;
    }

  /*
   * Create an ungetc buffer.
   * Initially, we will use the `reserve' buffer.
   */

  fp->_ur = fp->_r;
  fp->_up = fp->_p;
  fp->_ub._base = fp->_ubuf;
  fp->_ub._size = sizeof (fp->_ubuf);
  fp->_ubuf[sizeof (fp->_ubuf) - 1] = c;
  fp->_p = &fp->_ubuf[sizeof (fp->_ubuf) - 1];
  fp->_r = 1;
  return c;
}
