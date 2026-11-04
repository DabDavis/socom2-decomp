/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's string/strcasecmp.c (MAIN 0x198B80), the toolchain's libc: one object, as in the demo.
 * tolower is newlib's ctype macro (the _ctype_ table's _U bit). */

#include "newlib.h"

int strcasecmp(const char *s1, const char *s2)
{
	while (*s1 != '\0' && tolower(*s1) == tolower(*s2)) {
		s1++;
		s2++;
	}
	return tolower(*(unsigned char *)s1) - tolower(*(unsigned char *)s2);
}
