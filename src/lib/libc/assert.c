/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdlib/assert.c (MAIN 0x191440), the toolchain's libc: one object, as in the demo (2.96's
 * sibling call to abort under the torn-down frame). */

#include "newlib.h"

extern int fiprintf(struct __sFILE *fp, const char *fmt, ...);
extern void abort(void);

void __assert(const char *file, int line, const char *failedexpr)
{
	(void)fiprintf(_REENT->_stderr, "assertion \"%s\" failed: file \"%s\", line %d\n", failedexpr, file, line);
	abort();
}
