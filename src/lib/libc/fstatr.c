/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's reent/fstatr.c (_fstat_r, MAIN 0x193F40), the toolchain's libc (README "The GCC front"): one object,
 * as in the demo. The system layer's errno is the global one (newlib undefines the macro here), copied
 * into the reentrancy block when the call fails and set it. */

#include "newlib.h"

#undef errno
extern int errno;

struct stat;
typedef long off_t;
typedef int _ssize_t;
typedef int ptrdiff_t;

extern int fstat(int fd, struct stat *pstat);

int _fstat_r(struct _reent *ptr, int fd, struct stat *pstat)
{
	int ret;

	errno = 0;
	if ((ret = fstat(fd, pstat)) == -1 && errno != 0)
		ptr->_errno = errno;
	return ret;
}
