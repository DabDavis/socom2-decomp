/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's reent/closer.c (_close_r, MAIN 0x1916E8), the toolchain's libc (README "The GCC front"): one object,
 * as in the demo. The system layer's errno is the global one (newlib undefines the macro here), copied
 * into the reentrancy block when the call fails and set it. */

#include "newlib.h"

#undef errno
extern int errno;

struct stat;
typedef long off_t;
typedef int _ssize_t;
typedef int ptrdiff_t;

extern int close(int fd);

int _close_r(struct _reent *ptr, int fd)
{
	int ret;

	errno = 0;
	if ((ret = close(fd)) == -1 && errno != 0)
		ptr->_errno = errno;
	return ret;
}
