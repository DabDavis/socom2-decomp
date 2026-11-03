/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdio/vfiprintf.c (MAIN 0x19B260-0x19B3D8 of it), the toolchain's libc: the statics
 * __sprint and __sbprintf (rows __sprint@vfiprintf, __sbprintf@vfiprintf: vfprintf.c defines
 * its own pair) and vfiprintf, newlib's text built with INTEGER_ONLY. Not written: _vfiprintf_r and its
 * helpers after it. */
#define VFPRINTF vfiprintf
#define _VFPRINTF_R _vfiprintf_r

#include <stdarg.h>
#include "newlib.h"

extern int __sfvwrite(FILE *, struct __suio *);
extern int fflush(FILE *);
extern void __sinit(struct _reent *);
extern int _VFPRINTF_R(struct _reent *, FILE *, const char *, va_list);
int VFPRINTF(FILE *fp, const char *fmt0, va_list ap);

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

