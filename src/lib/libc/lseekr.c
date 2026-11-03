/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's reent/lseekr.c (_lseek_r, MAIN 0x194848), the toolchain's libc (README "The GCC front"): one object,
 * as in the demo. The system layer's errno is the global one (newlib undefines the macro here), copied
 * into the reentrancy block when the call fails and set it. */

#include "newlib.h"

#undef errno
extern int errno;

struct stat;
typedef long off_t;
typedef int _ssize_t;
typedef int ptrdiff_t;

extern off_t lseek(int fd, off_t pos, int whence);

off_t _lseek_r(struct _reent *ptr, int fd, off_t pos, int whence)
{
	off_t ret;

	errno = 0;
	if ((ret = lseek(fd, pos, whence)) == -1 && errno != 0)
		ptr->_errno = errno;
	return ret;
}
