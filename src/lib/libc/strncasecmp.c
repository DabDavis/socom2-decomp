/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's string/strncasecmp.c (MAIN 0x1993C0), the toolchain's libc: one object, as in the demo. */

#include "newlib.h"

int strncasecmp(const char *s1, const char *s2, size_t n)
{
	if (n == 0)
		return 0;
	while (n-- != 0 && tolower(*s1) == tolower(*s2)) {
		if (n == 0 || *s1 == '\0' || *s2 == '\0')
			break;
		s1++;
		s2++;
	}
	return tolower(*(unsigned char *)s1) - tolower(*(unsigned char *)s2);
}
