/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdio/fclose.c (MAIN 0x192CA8), the toolchain's libc. */

#include "newlib.h"

int
fclose (fp)
     register FILE * fp;
{
  int r;

  if (fp == NULL)
    return (0);			/* on NULL */

  CHECK_INIT (fp);

  if (fp->_flags == 0)		/* not open! */
    return (0);
  r = fp->_flags & __SWR ? fflush (fp) : 0;
  if (fp->_close != NULL && (*fp->_close) (fp->_cookie) < 0)
    r = EOF;
  if (fp->_flags & __SMBF)
    _free_r (fp->_data, (char *) fp->_bf._base);
  if (HASUB (fp))
    FREEUB (fp);
  if (HASLB (fp))
    FREELB (fp);
  fp->_flags = 0;		/* release this FILE for reuse */
  return (r);
}
