/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdio/fwalk.c (MAIN 0x194488), the toolchain's libc. */

#include "newlib.h"

int
_fwalk (ptr, function)
     struct _reent *ptr;
     register int (*function) ();
{
  register FILE *fp;
  register int n, ret = 0;
  register struct _glue *g;

  for (g = &ptr->__sglue; g != NULL; g = g->_next)
    for (fp = g->_iobs, n = g->_niobs; --n >= 0; fp++)
      if (fp->_flags != 0)
	ret |= (*function) (fp);
  return ret;
}
