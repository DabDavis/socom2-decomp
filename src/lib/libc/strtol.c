/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdlib/strtol.c (MAIN 0x19AAE8-0x19AD4C), the toolchain's libc: _strtol_r and strtol,
 * newlib's text (BSD's strtol). long is 64-bit on the EE, so the cutoff divides through libgcc's
 * __udivdi3/__umoddi3. */

#include "newlib.h"

#define	isalpha(c)	((_ctype_+1)[(unsigned)(c)]&(_U|_L))
#define	isdigit(c)	((_ctype_+1)[(unsigned)(c)]&_N)
#define	isspace(c)	((_ctype_+1)[(unsigned)(c)]&_S)

#define LONG_MAX	0x7fffffffffffffffL
#define LONG_MIN	(-LONG_MAX - 1L)
#define ERANGE		34

long
_strtol_r (struct _reent *rptr, const char *nptr, char **endptr, int base)
{
	register const char *s = nptr;
	register unsigned long acc;
	register int c;
	register unsigned long cutoff;
	register int neg = 0, any, cutlim;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
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

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
	cutlim = cutoff % (unsigned long)base;
	cutoff /= (unsigned long)base;
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
		acc = neg ? LONG_MIN : LONG_MAX;
		rptr->_errno = ERANGE;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *) (any ? s - 1 : nptr);
	return (acc);
}

long
strtol (const char *s, char **ptr, int base)
{
	return _strtol_r (_REENT, s, ptr, base);
}
