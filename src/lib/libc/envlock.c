/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdlib/envlock.c (MAIN 0x192BD8-0x192BE4), the toolchain's libc: the environment lock's
 * default no-op hooks, which _findenv_r calls around its walk. */

#include "newlib.h"

void
__env_lock (ptr)
     struct _reent *ptr;
{
}

void
__env_unlock (ptr)
     struct _reent *ptr;
{
}
