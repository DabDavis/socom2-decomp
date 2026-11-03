/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's string/strrchr.c (MAIN 0x1999C0), the toolchain's libc (README "The GCC front"): one
 * object, as in the demo. */

#include "newlib.h"

char *strrchr(const char *s, int i)
{
	const char *last = NULL;
	char c = i;

	while (*s) {
		if (*s == c)
			last = s;
		s++;
	}
	if (*s == c)
		last = s;
	return (char *)last;
}
