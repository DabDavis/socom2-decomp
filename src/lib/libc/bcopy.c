/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's string/bcopy.c (MAIN 0x1914D8), the toolchain's libc: memmove with the operands the
 * other way round. */

#include "newlib.h"

extern void *memmove(void *, const void *, size_t);

void
bcopy (b1, b2, length)
     const char *b1;
     char *b2;
     size_t length;
{
  memmove ((void *) b2, (void *) b1, length);
}
