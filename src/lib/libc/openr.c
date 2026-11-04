/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's reent/openr.c (_open_r, MAIN 0x196B60), the toolchain's libc (README "The GCC front"): one object,
 * as in the demo. The system layer's errno is the global one (newlib undefines the macro here), copied
 * into the reentrancy block when the call fails and set it. */

#include "newlib.h"

#undef errno
extern int errno;

struct stat;
typedef long off_t;
typedef int _ssize_t;
typedef int ptrdiff_t;

extern int open(const char *file, int flags, int mode);

int _open_r(struct _reent *ptr, const char *file, int flags, int mode)
{
	int ret;

	errno = 0;
	if ((ret = open(file, flags, mode)) == -1 && errno != 0)
		ptr->_errno = errno;
	return ret;
}
