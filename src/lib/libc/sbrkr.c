/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's reent/sbrkr.c (_sbrk_r, MAIN 0x1982C8), the toolchain's libc (README "The GCC front"): one object,
 * as in the demo. The system layer's errno is the global one (newlib undefines the macro here), copied
 * into the reentrancy block when the call fails and set it. */

#include "newlib.h"

#undef errno
extern int errno;

struct stat;
typedef long off_t;
typedef int _ssize_t;
typedef int ptrdiff_t;

extern void *sbrk(ptrdiff_t incr);

void * _sbrk_r(struct _reent *ptr, ptrdiff_t incr)
{
	void * ret;

	errno = 0;
	if ((ret = sbrk(incr)) == (void *)-1 && errno != 0)
		ptr->_errno = errno;
	return ret;
}
