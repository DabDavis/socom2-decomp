/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdio/findfp.c (MAIN 0x192FF8-0x193294), the toolchain's libc: std (static), the glue
 * allocator, __sfp (a free FILE for fopen, its descriptor -1), the exit-time cleanup that flushes
 * every open stream, and __sinit, which sets up stdin/stdout/stderr in the reentrancy block. The
 * demo's object 0x3E0760 holds the same six in the same order. */

#include "newlib.h"

#define	ENOMEM 12
#define	NDYNAMIC 4	/* add four more whenever necessary */

extern int __sread(void *, char *, int);
extern int __swrite(void *, const char *, int);
extern _fpos_t __sseek(void *, _fpos_t, int);
extern int __sclose(void *);

static void
std (fp, flags, file, ptr)
     FILE *fp;
     int flags;
     int file;
     struct _reent *ptr;
{
  fp->_p = 0;
  fp->_r = 0;
  fp->_w = 0;
  fp->_flags = flags;
  fp->_file = file;
  fp->_bf._base = 0;
  fp->_bf._size = 0;
  fp->_lbfsize = 0;
  fp->_cookie = fp;
  fp->_read = __sread;
  fp->_write = __swrite;
  fp->_seek = __sseek;
  fp->_close = __sclose;
  fp->_data = ptr;
}

struct _glue *
__sfmoreglue (d, n)
     struct _reent *d;
     register int n;
{
  struct _glue *g;
  FILE *p;

  g = (struct _glue *) _malloc_r (d, sizeof (*g) + n * sizeof (FILE));
  if (g == NULL)
    return NULL;
  p = (FILE *) (g + 1);
  g->_next = NULL;
  g->_niobs = n;
  g->_iobs = p;
  memset (p, 0, n * sizeof (FILE));
  return g;
}

/*
 * Find a free FILE for fopen et al.
 */

FILE *
__sfp (d)
     struct _reent *d;
{
  FILE *fp;
  int n;
  struct _glue *g;

  if (!d->__sdidinit)
    __sinit (d);
  for (g = &d->__sglue;; g = g->_next)
    {
      for (fp = g->_iobs, n = g->_niobs; --n >= 0; fp++)
	if (fp->_flags == 0)
	  goto found;
      if (g->_next == NULL &&
	  (g->_next = __sfmoreglue (d, NDYNAMIC)) == NULL)
	break;
    }
  d->_errno = ENOMEM;
  return NULL;

found:
  fp->_flags = 1;		/* reserve this slot; caller sets real flags */
  fp->_p = NULL;		/* no current pointer */
  fp->_w = 0;			/* nothing to read or write */
  fp->_r = 0;
  fp->_bf._base = NULL;		/* no buffer */
  fp->_bf._size = 0;
  fp->_lbfsize = 0;		/* not line buffered */
  fp->_file = -1;		/* no file */
  /* fp->_cookie = <any>; */	/* caller sets cookie, _read/_write etc */
  fp->_ub._base = NULL;		/* no ungetc buffer */
  fp->_ub._size = 0;
  fp->_lb._base = NULL;		/* no line buffer */
  fp->_lb._size = 0;
  fp->_data = d;
  return fp;
}

/*
 * exit() calls _cleanup() through *__cleanup, set whenever we
 * open or buffer a file.  This chicanery is done so that programs
 * that do not use stdio need not link it all in.
 *
 * The name `_cleanup' is, alas, fairly well known outside stdio.
 */

void
_cleanup_r (ptr)
     struct _reent *ptr;
{
  /* (void) _fwalk(fclose); */
  (void) _fwalk (ptr, fflush);	/* `cheating' */
}

void
_cleanup ()
{
  _cleanup_r (_REENT);
}

/*
 * __sinit() is called whenever stdio's state variables must be set.
 */

void
__sinit (s)
     struct _reent *s;
{
  /* make sure we clean up on exit */
  s->__cleanup = _cleanup_r;	/* conservative */
  s->__sdidinit = 1;

  std (s->__sf + 0, __SRD, 0, s);
  std (s->__sf + 1, __SWR | __SLBF, 1, s);
  std (s->__sf + 2, __SWR | __SNBF, 2, s);

  /* initialize the global glue */
  s->__sglue._next = NULL;
  s->__sglue._niobs = 3;
  s->__sglue._iobs = &s->__sf[0];
}
