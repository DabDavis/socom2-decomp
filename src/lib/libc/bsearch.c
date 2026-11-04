/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdlib/bsearch.c (MAIN 0x1914F8), the toolchain's libc. */

#include "newlib.h"

void *
bsearch (key, base, nmemb, size, compar)
     const void * key;
     const void * base;
     size_t nmemb;
     size_t size;
     int (*compar) (const void *, const void *);
{
  void * current;
  size_t lower = 0;
  size_t upper = nmemb;
  size_t index;
  int result;

  if (nmemb == 0 || size == 0)
    return NULL;

  while (lower < upper)
    {
      index = (lower + upper) / 2;
      current = (void *) (((char *) base) + (index * size));

      result = compar (key, current);

      if (result < 0)
        upper = index;
      else if (result > 0)
        lower = index + 1;
      else
	return current;
    }

  return NULL;
}
