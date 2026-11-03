/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's reent/signalr.c (_kill_r and _getpid_r, MAIN 0x1987D0-0x198844), the toolchain's libc (README "The GCC front"): one object,
 * as in the demo. The system layer's errno is the global one (newlib undefines the macro here), copied
 * into the reentrancy block when the call fails and set it. */

#include "newlib.h"

#undef errno
extern int errno;

struct stat;
typedef long off_t;
typedef int _ssize_t;
typedef int ptrdiff_t;

extern int kill(int pid, int sig);
extern int getpid(void);

int _kill_r(struct _reent *ptr, int pid, int sig)
{
	int ret;

	errno = 0;
	if ((ret = kill(pid, sig)) == -1 && errno != 0)
		ptr->_errno = errno;
	return ret;
}

int _getpid_r(struct _reent *ptr)
{
	int ret;

	ret = getpid();
	return ret;
}
