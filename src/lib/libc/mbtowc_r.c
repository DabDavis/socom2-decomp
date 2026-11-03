/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdlib/mbtowc_r.c (MAIN 0x195650), the toolchain's libc: the "C" locale only (no
 * MB_CAPABLE), one byte one character. */

#include "newlib.h"

typedef int wchar_t;

int
_mbtowc_r (r, pwc, s, n, state)
     struct _reent *r;
     wchar_t       *pwc;
     const char    *s;
     size_t         n;
     int           *state;
{
  wchar_t dummy;
  unsigned char *t = (unsigned char *)s;

  if (pwc == NULL)
    pwc = &dummy;

  if (s != NULL && n == 0)
    return -1;

  /* otherwise this must be the "C" locale or unknown locale */
  if (s == NULL)
    return 0;  /* not state-dependent */

  *pwc = (wchar_t)*t;

  return (*t != '\0');
}
