/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's reent/writer.c (_write_r, MAIN 0x19E9A8), the toolchain's libc (README "The GCC front"): one object,
 * as in the demo. The system layer's errno is the global one (newlib undefines the macro here), copied
 * into the reentrancy block when the call fails and set it. */

#include "newlib.h"

#undef errno
extern int errno;

struct stat;
typedef long off_t;
typedef long _ssize_t;
typedef int ptrdiff_t;

extern int write(int fd, const void *buf, size_t cnt);

_ssize_t _write_r(struct _reent *ptr, int fd, const void *buf, size_t cnt)
{
	_ssize_t ret;

	errno = 0;
	if ((ret = (_ssize_t)write(fd, buf, cnt)) == -1 && errno != 0)
		ptr->_errno = errno;
	return ret;
}
