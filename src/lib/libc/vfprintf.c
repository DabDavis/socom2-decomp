/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdio/vfprintf.c (MAIN 0x19BFD0-0x19DA54), the toolchain's libc: the statics __sprint and
 * __sbprintf (rows __sprint@vfprintf, __sbprintf@vfprintf: vfiprintf.c defines its own pair),
 * vfprintf, _vfprintf_r and its statics cvt and exponent -- newlib 1.9.0's text with the
 * FLOATING_POINT conversions, but for the toolchain snapshot's older lines, each read off the
 * target: `char *xdigs;` (no `= NULL` store), the argument a plain `double _double` (1.9.0's union
 * is initialised, a `sd zero` the target does not make), `int expsize;` (no store either) and
 * exponent's `char expbuf[MAXEXP]` (a 320-byte frame; 1.9.0's 40 gives 48). Built against GCC
 * 2.96's builtin stdarg (gccstdarg.h), _NO_LONGLONG and _NO_LONGDBL (newlib's defaults).
 *
 * The whole object byte-exact (GCC round 23a): _vfprintf_r and cvt under the runtime's -g (open
 * since round 22a: `jal dpcmp; move a1,zero` against `nop; jal; nop` at each compare of the
 * argument with 0.0, cvt's `move a0,zero; jal dpsub; nop`) -- GCC gives a DFmode register move a
 * length of two instructions, so reorg leaves a slot whose only candidate is one to GNU as, which
 * fills it -- but not under -g, which GNU as reads as -O0 on MIPS; the runtime's objects were built
 * with GCC's CFLAGS_FOR_TARGET, -g -O2 (SCE's patch appends -G0). cc1's output is the same with and
 * without -g -- and with newlib 1.10.0's `if (expt || ndig)` in the 'f' conversion's fraction
 * (1.9.0's `if (ndig)` is three words short): the snapshot is not one release's text. cvt calls
 * dtoa.c's _dtoa_r (0x191970, named by this call). */
#define VFPRINTF vfprintf
#define _VFPRINTF_R _vfprintf_r
#define FLOATING_POINT
#define _NO_LONGLONG
#define _NO_LONGDBL

#include "gccstdarg.h"
#include "newlib.h"

extern int __sfvwrite(FILE *, struct __suio *);
extern int fflush(FILE *);
extern void __sinit(struct _reent *);
extern int _VFPRINTF_R(struct _reent *, FILE *, const char *, va_list);
int VFPRINTF(FILE *fp, const char *fmt0, va_list ap);

#define _DEFUN(name, arglist, args) name(args)
#define _AND ,
#define _CONST const
#define _PARAMS(x) x
#define _POINTER_INT long
#define _LONG_DOUBLE long double
typedef unsigned long u_long;
typedef unsigned int u_int;
typedef unsigned short u_short;
typedef int wchar_t;
extern int __mb_cur_max;
#define MB_CUR_MAX __mb_cur_max
extern int _mbtowc_r(struct _reent *, wchar_t *, const char *, size_t, int *);
extern size_t strlen(const char *);
#define __sferror(p) (((p)->_flags & __SERR) != 0)

/* locale.h, math.h and floatio.h, what this file reads of them */
struct lconv { char *decimal_point; };
extern struct lconv *localeconv(void);
extern int isinf(double);
extern int isnan(double);
#define	MAXEXP		308
#define	MAXFRACT	39

/* vfieeefp.h's, for the EE (IEEE_8087: little-endian words) */
union double_union { double d; unsigned int i[2]; };
#define word0(x) (x.i[1])
#define word1(x) (x.i[0])
#define Sign_bit 0x80000000

#ifdef FLOATING_POINT

#define	BUF		(MAXEXP+MAXFRACT+1)	/* + decimal point */
#define	DEFPREC		6

#ifdef _NO_LONGDBL
static char *cvt _PARAMS((struct _reent *, double, int, int, char *, int *, int, int *));
#else
static char *cvt _PARAMS((struct _reent *, _LONG_DOUBLE, int, int, char *, int *, int, int *));
extern int  _ldcheck _PARAMS((_LONG_DOUBLE *));
#endif

static int exponent _PARAMS((char *, int, int));

#else /* no FLOATING_POINT */

#define	BUF		40

#endif /* FLOATING_POINT */

#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	((n) + '0')

/*
 * Flags used during conversion.
 */
#define	ALT		0x001		/* alternate form */
#define	HEXPREFIX	0x002		/* add 0x or 0X prefix */
#define	LADJUST		0x004		/* left adjustment */
#define	LONGDBL		0x008		/* long double */
#define	LONGINT		0x010		/* long integer */
#define	QUADINT		0x020		/* quad integer */
#define	SHORTINT	0x040		/* short integer */
#define	ZEROPAD		0x080		/* zero (as opposed to blank) pad */
#define FPT		0x100		/* Floating point number */

/*
 * Flush out all the vectors defined by the given uio,
 * then reset it so that it can be reused.
 */
static int
__sprint(FILE *fp, register struct __suio *uio)
{
	register int err;

	if (uio->uio_resid == 0) {
		uio->uio_iovcnt = 0;
		return (0);
	}
	err = __sfvwrite(fp, uio);
	uio->uio_resid = 0;
	uio->uio_iovcnt = 0;
	return (err);
}

/*
 * Helper function for `fprintf to unbuffered unix file': creates a
 * temporary buffer.  We only work on write-only files; this avoids
 * worries about ungetc buffers and so forth.
 */
static int
__sbprintf(register FILE *fp, const char *fmt, va_list ap)
{
	int ret;
	FILE fake;
	unsigned char buf[BUFSIZ];

	/* copy the important variables */
	fake._data = fp->_data;
	fake._flags = fp->_flags & ~__SNBF;
	fake._file = fp->_file;
	fake._cookie = fp->_cookie;
	fake._write = fp->_write;

	/* set up the buffer */
	fake._bf._base = fake._p = buf;
	fake._bf._size = fake._w = sizeof(buf);
	fake._lbfsize = 0;	/* not actually used, but Just In Case */

	/* do the work, then copy any error status */
	ret = VFPRINTF(&fake, fmt, ap);
	if (ret >= 0 && fflush(&fake))
		ret = EOF;
	if (fake._flags & __SERR)
		fp->_flags |= __SERR;
	return (ret);
}

int
VFPRINTF (FILE * fp, const char *fmt0, va_list ap)
{
  CHECK_INIT (fp);
  return _VFPRINTF_R (fp->_data, fp, fmt0, ap);
}

int 
_DEFUN (_VFPRINTF_R, (data, fp, fmt0, ap),
	struct _reent *data _AND
	FILE * fp _AND
	_CONST char *fmt0 _AND
	va_list ap)
{
	register char *fmt;	/* format string */
	register int ch;	/* character from fmt */
	register int n, m;	/* handy integers (short term usage) */
	register char *cp;	/* handy char pointer (short term usage) */
	register struct __siov *iovp;/* for PRINT macro */
	register int flags;	/* flags as above */
	int ret;		/* return value accumulator */
	int width;		/* width from format (%8d), or 0 */
	int prec;		/* precision from format (%.3d), or -1 */
	char sign;		/* sign prefix (' ', '+', '-', or \0) */
	wchar_t wc;
#ifdef FLOATING_POINT
	char *decimal_point = localeconv()->decimal_point;
	char softsign;		/* temporary negative sign for floats */
#ifdef _NO_LONGDBL
	double _double;		/* double precision arguments %[eEfgG] */
	#define _fpvalue _double
#else
	union { int i; _LONG_DOUBLE ld; } _long_double_ = {0};
	#define _fpvalue (_long_double_.ld)
	int tmp;  
#endif
	int expt;		/* integer value of exponent */
	int expsize;		/* character count for expstr */
	int ndig;		/* actual number of digits returned by cvt */
	char expstr[7];		/* buffer for exponent string */
#endif

#ifndef _NO_LONGLONG
#define	quad_t	  long long
#define	u_quad_t  unsigned long long
#endif

#ifndef _NO_LONGLONG
	u_quad_t _uquad;	/* integer arguments %[diouxX] */
#else
	u_long _uquad;
#endif
	enum { OCT, DEC, HEX } base;/* base for [diouxX] conversion */
	int dprec;		/* a copy of prec if [diouxX], 0 otherwise */
	int realsz;		/* field size expanded by dprec */
	int size;		/* size of converted field or string */
	char *xdigs;	/* digits for [xX] conversion */
#define NIOV 8
	struct __suio uio;	/* output information: summary */
	struct __siov iov[NIOV];/* ... and individual io vectors */
	char buf[BUF];		/* space for %c, %[diouxX], %[eEfgG] */
	char ox[2];		/* space for 0x hex-prefix */
        int state = 0;          /* mbtowc calls from library must not change state */

	/*
	 * Choose PADSIZE to trade efficiency vs. size.  If larger printf
	 * fields occur frequently, increase PADSIZE and make the initialisers
	 * below longer.
	 */
#define	PADSIZE	16		/* pad chunk size */
	static _CONST char blanks[PADSIZE] =
	 {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
	static _CONST char zeroes[PADSIZE] =
	 {'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0'};

	/*
	 * BEWARE, these `goto error' on error, and PAD uses `n'.
	 */
#define	PRINT(ptr, len) { \
	iovp->iov_base = (ptr); \
	iovp->iov_len = (len); \
	uio.uio_resid += (len); \
	iovp++; \
	if (++uio.uio_iovcnt >= NIOV) { \
		if (__sprint(fp, &uio)) \
			goto error; \
		iovp = iov; \
	} \
}
#define	PAD(howmany, with) { \
	if ((n = (howmany)) > 0) { \
		while (n > PADSIZE) { \
			PRINT(with, PADSIZE); \
			n -= PADSIZE; \
		} \
		PRINT(with, n); \
	} \
}
#define	FLUSH() { \
	if (uio.uio_resid && __sprint(fp, &uio)) \
		goto error; \
	uio.uio_iovcnt = 0; \
	iovp = iov; \
}

	/*
	 * To extend shorts properly, we need both signed and unsigned
	 * argument extraction methods.
	 */
#ifndef _NO_LONGLONG
#define	SARG() \
	(flags&QUADINT ? va_arg(ap, quad_t) : \
	    flags&LONGINT ? va_arg(ap, long) : \
	    flags&SHORTINT ? (long)(short)va_arg(ap, int) : \
	    (long)va_arg(ap, int))
#define	UARG() \
	(flags&QUADINT ? va_arg(ap, u_quad_t) : \
	    flags&LONGINT ? va_arg(ap, u_long) : \
	    flags&SHORTINT ? (u_long)(u_short)va_arg(ap, int) : \
	    (u_long)va_arg(ap, u_int))
#else
#define	SARG() \
	(flags&LONGINT ? va_arg(ap, long) : \
	    flags&SHORTINT ? (long)(short)va_arg(ap, int) : \
	    (long)va_arg(ap, int))
#define	UARG() \
	(flags&LONGINT ? va_arg(ap, u_long) : \
	    flags&SHORTINT ? (u_long)(u_short)va_arg(ap, int) : \
	    (u_long)va_arg(ap, u_int))
#endif

	/* sorry, fprintf(read_only_file, "") returns EOF, not 0 */
	if (cantwrite(fp))
		return (EOF);

	/* optimise fprintf(stderr) (and other unbuffered Unix files) */
	if ((fp->_flags & (__SNBF|__SWR|__SRW)) == (__SNBF|__SWR) &&
	    fp->_file >= 0)
		return (__sbprintf(fp, fmt0, ap));

	fmt = (char *)fmt0;
	uio.uio_iov = iovp = iov;
	uio.uio_resid = 0;
	uio.uio_iovcnt = 0;
	ret = 0;

	/*
	 * Scan the format for conversions (`%' character).
	 */
	for (;;) {
	        cp = fmt;
	        while ((n = _mbtowc_r(_REENT, &wc, fmt, MB_CUR_MAX, &state)) > 0) {
			fmt += n;
			if (wc == '%') {
				fmt--;
				break;
			}
		}
		if ((m = fmt - cp) != 0) {
			PRINT(cp, m);
			ret += m;
		}
		if (n <= 0)
			goto done;
		fmt++;		/* skip over '%' */

		flags = 0;
		dprec = 0;
		width = 0;
		prec = -1;
		sign = '\0';

rflag:		ch = *fmt++;
reswitch:	switch (ch) {
		case ' ':
			/*
			 * ``If the space and + flags both appear, the space
			 * flag will be ignored.''
			 *	-- ANSI X3J11
			 */
			if (!sign)
				sign = ' ';
			goto rflag;
		case '#':
			flags |= ALT;
			goto rflag;
		case '*':
			/*
			 * ``A negative field width argument is taken as a
			 * - flag followed by a positive field width.''
			 *	-- ANSI X3J11
			 * They don't exclude field widths read from args.
			 */
			if ((width = va_arg(ap, int)) >= 0)
				goto rflag;
			width = -width;
			/* FALLTHROUGH */
		case '-':
			flags |= LADJUST;
			goto rflag;
		case '+':
			sign = '+';
			goto rflag;
		case '.':
			if ((ch = *fmt++) == '*') {
				n = va_arg(ap, int);
				prec = n < 0 ? -1 : n;
				goto rflag;
			}
			n = 0;
			while (is_digit(ch)) {
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			}
			prec = n < 0 ? -1 : n;
			goto reswitch;
		case '0':
			/*
			 * ``Note that 0 is taken as a flag, not as the
			 * beginning of a field width.''
			 *	-- ANSI X3J11
			 */
			flags |= ZEROPAD;
			goto rflag;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = 0;
			do {
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			} while (is_digit(ch));
			width = n;
			goto reswitch;
#ifdef FLOATING_POINT
		case 'L':
			flags |= LONGDBL;
			goto rflag;
#endif
		case 'h':
			flags |= SHORTINT;
			goto rflag;
		case 'l':
			if (*fmt == 'l') {
				fmt++;
				flags |= QUADINT;
			} else {
				flags |= LONGINT;
			}
			goto rflag;
		case 'q':
			flags |= QUADINT;
			goto rflag;
		case 'c':
			*(cp = buf) = va_arg(ap, int);
			size = 1;
			sign = '\0';
			break;
		case 'D':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'd':
		case 'i':
			_uquad = SARG();
#ifndef _NO_LONGLONG
			if ((quad_t)_uquad < 0)
#else
			if ((long) _uquad < 0)
#endif
			{

				_uquad = -_uquad;
				sign = '-';
			}
			base = DEC;
			goto number;
#ifdef FLOATING_POINT
		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':
			if (prec == -1) {
				prec = DEFPREC;
			} else if ((ch == 'g' || ch == 'G') && prec == 0) {
				prec = 1;
			}

#ifdef _NO_LONGDBL
			if (flags & LONGDBL) {
				_fpvalue = (double) va_arg(ap, _LONG_DOUBLE);
			} else {
				_fpvalue = va_arg(ap, double);
			}

			/* do this before tricky precision changes */
			if (isinf(_fpvalue)) {
				if (_fpvalue < 0)
					sign = '-';
				cp = "Inf";
				size = 3;
				break;
			}
			if (isnan(_fpvalue)) {
				cp = "NaN";
				size = 3;
				break;
			}

#else /* !_NO_LONGDBL */
			
			if (flags & LONGDBL) {
				_fpvalue = va_arg(ap, _LONG_DOUBLE);
			} else {
				_fpvalue = (_LONG_DOUBLE)va_arg(ap, double);
			}

			/* do this before tricky precision changes */
			tmp = _ldcheck (&_fpvalue);
			if (tmp == 2) {
				if (_fpvalue < 0)
					sign = '-';
				cp = "Inf";
				size = 3;
				break;
			}
			if (tmp == 1) {
				cp = "NaN";
				size = 3;
				break;
			}
#endif /* !_NO_LONGDBL */

			flags |= FPT;

			cp = cvt(data, _fpvalue, prec, flags, &softsign,
				&expt, ch, &ndig);

			if (ch == 'g' || ch == 'G') {
				if (expt <= -4 || expt > prec)
					ch = (ch == 'g') ? 'e' : 'E';
				else
					ch = 'g';
			} 
			if (ch <= 'e') {	/* 'e' or 'E' fmt */
				--expt;
				expsize = exponent(expstr, expt, ch);
				size = expsize + ndig;
				if (ndig > 1 || flags & ALT)
					++size;
			} else if (ch == 'f') {		/* f fmt */
				if (expt > 0) {
					size = expt;
					if (prec || flags & ALT)
						size += prec + 1;
				} else	/* "0.X" */
					size = prec + 2;
			} else if (expt >= ndig) {	/* fixed g fmt */
				size = expt;
				if (flags & ALT)
					++size;
			} else
				size = ndig + (expt > 0 ?
					1 : 2 - expt);

			if (softsign)
				sign = '-';
			break;
#endif /* FLOATING_POINT */
		case 'n':
#ifndef _NO_LONGLONG
			if (flags & QUADINT)
				*va_arg(ap, quad_t *) = ret;
			else 
#endif
			if (flags & LONGINT)
				*va_arg(ap, long *) = ret;
			else if (flags & SHORTINT)
				*va_arg(ap, short *) = ret;
			else
				*va_arg(ap, int *) = ret;
			continue;	/* no output */
		case 'O':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'o':
			_uquad = UARG();
			base = OCT;
			goto nosign;
		case 'p':
			/*
			 * ``The argument shall be a pointer to void.  The
			 * value of the pointer is converted to a sequence
			 * of printable characters, in an implementation-
			 * defined manner.''
			 *	-- ANSI X3J11
			 */
			/* NOSTRICT */
			_uquad = (u_long)(unsigned _POINTER_INT)va_arg(ap, void *);
			base = HEX;
			xdigs = "0123456789abcdef";
			flags |= HEXPREFIX;
			ch = 'x';
			goto nosign;
		case 's':
			if ((cp = va_arg(ap, char *)) == NULL)
				cp = "(null)";
			if (prec >= 0) {
				/*
				 * can't use strlen; can only look for the
				 * NUL in the first `prec' characters, and
				 * strlen() will go further.
				 */
				char *p = memchr(cp, 0, prec);

				if (p != NULL) {
					size = p - cp;
					if (size > prec)
						size = prec;
				} else
					size = prec;
			} else
				size = strlen(cp);
			sign = '\0';
			break;
		case 'U':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'u':
			_uquad = UARG();
			base = DEC;
			goto nosign;
		case 'X':
			xdigs = "0123456789ABCDEF";
			goto hex;
		case 'x':
			xdigs = "0123456789abcdef";
hex:			_uquad = UARG();
			base = HEX;
			/* leading 0x/X only if non-zero */
			if (flags & ALT && _uquad != 0)
				flags |= HEXPREFIX;

			/* unsigned conversions */
nosign:			sign = '\0';
			/*
			 * ``... diouXx conversions ... if a precision is
			 * specified, the 0 flag will be ignored.''
			 *	-- ANSI X3J11
			 */
number:			if ((dprec = prec) >= 0)
				flags &= ~ZEROPAD;

			/*
			 * ``The result of converting a zero value with an
			 * explicit precision of zero is no characters.''
			 *	-- ANSI X3J11
			 */
			cp = buf + BUF;
			if (_uquad != 0 || prec != 0) {
				/*
				 * Unsigned mod is hard, and unsigned mod
				 * by a constant is easier than that by
				 * a variable; hence this switch.
				 */
				switch (base) {
				case OCT:
					do {
						*--cp = to_char(_uquad & 7);
						_uquad >>= 3;
					} while (_uquad);
					/* handle octal leading 0 */
					if (flags & ALT && *cp != '0')
						*--cp = '0';
					break;

				case DEC:
					/* many numbers are 1 digit */
					while (_uquad >= 10) {
						*--cp = to_char(_uquad % 10);
						_uquad /= 10;
					}
					*--cp = to_char(_uquad);
					break;

				case HEX:
					do {
						*--cp = xdigs[_uquad & 15];
						_uquad >>= 4;
					} while (_uquad);
					break;

				default:
					cp = "bug in vfprintf: bad base";
					size = strlen(cp);
					goto skipsize;
				}
			}
			size = buf + BUF - cp;
		skipsize:
			break;
		default:	/* "%?" prints ?, unless ? is NUL */
			if (ch == '\0')
				goto done;
			/* pretend it was %c with argument ch */
			cp = buf;
			*cp = ch;
			size = 1;
			sign = '\0';
			break;
		}

		/*
		 * All reasonable formats wind up here.  At this point, `cp'
		 * points to a string which (if not flags&LADJUST) should be
		 * padded out to `width' places.  If flags&ZEROPAD, it should
		 * first be prefixed by any sign or other prefix; otherwise,
		 * it should be blank padded before the prefix is emitted.
		 * After any left-hand padding and prefixing, emit zeroes
		 * required by a decimal [diouxX] precision, then print the
		 * string proper, then emit zeroes required by any leftover
		 * floating precision; finally, if LADJUST, pad with blanks.
		 *
		 * Compute actual size, so we know how much to pad.
		 * size excludes decimal prec; realsz includes it.
		 */
		realsz = dprec > size ? dprec : size;
		if (sign)
			realsz++;
		else if (flags & HEXPREFIX)
			realsz+= 2;

		/* right-adjusting blank padding */
		if ((flags & (LADJUST|ZEROPAD)) == 0)
			PAD(width - realsz, blanks);

		/* prefix */
		if (sign) {
			PRINT(&sign, 1);
		} else if (flags & HEXPREFIX) {
			ox[0] = '0';
			ox[1] = ch;
			PRINT(ox, 2);
		}

		/* right-adjusting zero padding */
		if ((flags & (LADJUST|ZEROPAD)) == ZEROPAD)
			PAD(width - realsz, zeroes);

		/* leading zeroes from decimal precision */
		PAD(dprec - size, zeroes);

		/* the string or number proper */
#ifdef FLOATING_POINT
		if ((flags & FPT) == 0) {
			PRINT(cp, size);
		} else {	/* glue together f_p fragments */
			if (ch >= 'f') {	/* 'f' or 'g' */
				if (_fpvalue == 0) {
					/* kludge for __dtoa irregularity */
					PRINT("0", 1);
					if (expt < ndig || (flags & ALT) != 0) {
						PRINT(decimal_point, 1);
						PAD(ndig - 1, zeroes);
					}
				} else if (expt <= 0) {
					PRINT("0", 1);
					if(expt || ndig) {	/* 1.10.0's test: the target's expt, then ndig */
						PRINT(decimal_point, 1);
						PAD(-expt, zeroes);
						PRINT(cp, ndig);
					}
				} else if (expt >= ndig) {
					PRINT(cp, ndig);
					PAD(expt - ndig, zeroes);
					if (flags & ALT)
						PRINT(".", 1);
				} else {
					PRINT(cp, expt);
					cp += expt;
					PRINT(".", 1);
					PRINT(cp, ndig-expt);
				}
			} else {	/* 'e' or 'E' */
				if (ndig > 1 || flags & ALT) {
					ox[0] = *cp++;
					ox[1] = '.';
					PRINT(ox, 2);
                                       if (_fpvalue) {
						PRINT(cp, ndig-1);
					} else	/* 0.[0..] */
						/* __dtoa irregularity */
						PAD(ndig - 1, zeroes);
				} else	/* XeYYY */
					PRINT(cp, 1);
				PRINT(expstr, expsize);
			}
		}
#else
		PRINT(cp, size);
#endif
		/* left-adjusting padding (always blank) */
		if (flags & LADJUST)
			PAD(width - realsz, blanks);

		/* finally, adjust ret */
		ret += width > realsz ? width : realsz;

		FLUSH();	/* copy out the I/O vectors */
	}
done:
	FLUSH();
error:
	return (__sferror(fp) ? EOF : ret);
	/* NOTREACHED */
}

#ifdef FLOATING_POINT

#ifdef _NO_LONGDBL
extern char *_dtoa_r _PARAMS((struct _reent *, double, int,
			      int, int *, int *, char **));
#else
extern char *_ldtoa_r _PARAMS((struct _reent *, _LONG_DOUBLE, int,
			      int, int *, int *, char **));
#undef word0
#define word0(x) ldword0(x)
#endif

static char *
cvt(data, value, ndigits, flags, sign, decpt, ch, length)
	struct _reent *data;
#ifdef _NO_LONGDBL
	double value;
#else
	_LONG_DOUBLE value;
#endif
	int ndigits, flags, *decpt, ch, *length;
	char *sign;
{
	int mode, dsgn;
	char *digits, *bp, *rve;
#ifdef _NO_LONGDBL
        union double_union tmp;
#else
        struct ldieee *ldptr;
#endif

	if (ch == 'f') {
		mode = 3;		/* ndigits after the decimal point */
	} else {
		/* To obtain ndigits after the decimal point for the 'e' 
		 * and 'E' formats, round to ndigits + 1 significant 
		 * figures.
		 */
		if (ch == 'e' || ch == 'E') {
			ndigits++;
		}
		mode = 2;		/* ndigits significant digits */
	}

#ifdef _NO_LONGDBL
        tmp.d = value;

	if (word0(tmp) & Sign_bit) { /* this will check for < 0 and -0.0 */
		value = -value;
		*sign = '-';
        } else
		*sign = '\000';

	digits = _dtoa_r(data, value, mode, ndigits, decpt, &dsgn, &rve);
#else /* !_NO_LONGDBL */
	ldptr = (struct ldieee *)&value;
	if (ldptr->sign) { /* this will check for < 0 and -0.0 */
		value = -value;
		*sign = '-';
        } else
		*sign = '\000';

	digits = _ldtoa_r(data, value, mode, ndigits, decpt, &dsgn, &rve);
#endif /* !_NO_LONGDBL */

	if ((ch != 'g' && ch != 'G') || flags & ALT) {	/* Print trailing zeros */
		bp = digits + ndigits;
		if (ch == 'f') {
			if (*digits == '0' && value)
				*decpt = -ndigits + 1;
			bp += *decpt;
		}
		if (value == 0)	/* kludge for __dtoa irregularity */
			rve = bp;
		while (rve < bp)
			*rve++ = '0';
	}
	*length = rve - digits;
	return (digits);
}

static int
exponent(p0, exp, fmtch)
	char *p0;
	int exp, fmtch;
{
	register char *p, *t;
	char expbuf[MAXEXP];

	p = p0;
	*p++ = fmtch;
	if (exp < 0) {
		exp = -exp;
		*p++ = '-';
	}
	else
		*p++ = '+';
	t = expbuf + MAXEXP;
	if (exp > 9) {
		do {
			*--t = to_char(exp % 10);
		} while ((exp /= 10) > 9);
		*--t = to_char(exp);
		for (; t < expbuf + MAXEXP; *p++ = *t++);
	}
	else {
		*p++ = '0';
		*p++ = to_char(exp);
	}
	return (p - p0);
}
#endif /* FLOATING_POINT */
