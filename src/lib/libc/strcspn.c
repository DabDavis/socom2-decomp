/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's string/strcspn.c (MAIN 0x199178), the toolchain's libc. */

#include "newlib.h"

size_t
strcspn (s1, s2)
     const char *s1;
     const char *s2;
{
  const char *s = s1;
  const char *c;

  while (*s1)
    {
      for (c = s2; *c; c++)
	{
	  if (*s1 == *c)
	    break;
	}
      if (*c)
	break;
      s1++;
    }

  return s1 - s;
}
