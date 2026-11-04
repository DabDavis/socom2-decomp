/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdlib/exit.c (MAIN 0x192BF8), the toolchain's libc: the atexit functions in reverse
 * order, then the stdio cleanup, then crt0's _exit. */

#include "newlib.h"

extern void _exit(int);

void
exit (code)
     int code;
{
  register struct _atexit *p;
  register int n;

  for (p = _REENT->_atexit; p; p = p->_next)
    for (n = p->_ind; --n >= 0;)
      (*p->_fns[n]) ();
  if (_REENT->__cleanup)
    (*_REENT->__cleanup) (_REENT);
  _exit (code);
}
