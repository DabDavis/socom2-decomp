/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's string/strtok_r.c (MAIN 0x19AA20), the toolchain's libc (BSD's strtok_r): one object, as
 * in the demo. */

#include "newlib.h"

char *strtok_r(register char *s, register const char *delim, char **lasts)
{
	register char *spanp;
	register int c, sc;
	char *tok;

	if (s == NULL && (s = *lasts) == NULL)
		return (NULL);
cont:
	c = *s++;
	for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
		if (c == sc)
			goto cont;
	}
	if (c == 0) {
		*lasts = NULL;
		return (NULL);
	}
	tok = s - 1;
	for (;;) {
		c = *s++;
		spanp = (char *)delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*lasts = s;
				return (tok);
			}
		} while (sc != 0);
	}
}
