/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's string/strstr.c (MAIN 0x199A10), the toolchain's libc: one object, as in the demo. */

#include "newlib.h"

char *strstr(const char *searchee, const char *lookfor)
{
	if (*searchee == 0) {
		if (*lookfor)
			return (char *)NULL;
		return (char *)searchee;
	}
	while (*searchee) {
		size_t i;

		i = 0;
		while (1) {
			if (lookfor[i] == 0)
				return (char *)searchee;
			if (lookfor[i] != searchee[i])
				break;
			i++;
		}
		searchee++;
	}
	return (char *)NULL;
}
