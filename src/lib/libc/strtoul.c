/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdlib/strtoul.c (MAIN 0x19AD50-0x19AFAC), the toolchain's libc: _strtoul_r and strtoul (the map's func_0019AF80, named here by its body),
 * newlib's text (BSD's strtoul). long is 64-bit on the EE, so the cutoff divides through libgcc's
 * __udivdi3/__umoddi3. */

#include "newlib.h"

#define	isalpha(c)	((_ctype_+1)[(unsigned)(c)]&(_U|_L))
#define	isdigit(c)	((_ctype_+1)[(unsigned)(c)]&_N)
#define	isspace(c)	((_ctype_+1)[(unsigned)(c)]&_S)

#define ULONG_MAX	0xffffffffffffffffUL
#define ERANGE		34

unsigned long
_strtoul_r (struct _reent *rptr, const char *nptr, char **endptr, int base)
{
	register const char *s = nptr;
	register unsigned long acc;
	register int c;
	register unsigned long cutoff;
	register int neg = 0, any, cutlim;

	/*
	 * See strtol for comments as to the logic used.
	 */
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;
	cutoff = (unsigned long)ULONG_MAX / (unsigned long)base;
	cutlim = (unsigned long)ULONG_MAX % (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
               if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = ULONG_MAX;
		rptr->_errno = ERANGE;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *) (any ? s - 1 : nptr);
	return (acc);
}

unsigned long
strtoul (const char *s, char **ptr, int base)
{
	return _strtoul_r (_REENT, s, ptr, base);
}
