/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdio/stdio.c (MAIN 0x198A10-0x198B7C), the toolchain's libc: the four cookie
 * functions a FILE's _read/_write/_seek/_close point at (the map's func_00198A10 ... func_00198B60,
 * named here by their bodies). */

#include "newlib.h"

extern long _read_r(struct _reent *, int, void *, size_t);
extern long _write_r(struct _reent *, int, const void *, size_t);
extern _off_t _lseek_r(struct _reent *, int, _off_t, int);
extern int _close_r(struct _reent *, int);

#define SEEK_END 2

int
__sread (void *cookie, char *buf, int n)
{
  register FILE *fp = (FILE *) cookie;
  register int ret;

  ret = _read_r (fp->_data, (int) fp->_file, buf, n);

  /* If the read succeeded, update the current offset.  */

  if (ret >= 0)
    fp->_offset += ret;
  else
    fp->_flags &= ~__SOFF;	/* paranoia */
  return ret;
}

int
__swrite (void *cookie, char const *buf, int n)
{
  register FILE *fp = (FILE *) cookie;

  if (fp->_flags & __SAPP)
    (void) _lseek_r (fp->_data, fp->_file, (_off_t) 0, SEEK_END);
  fp->_flags &= ~__SOFF;	/* in case O_APPEND mode is set */
  return _write_r (fp->_data, fp->_file, buf, n);
}

_fpos_t
__sseek (void *cookie, _fpos_t offset, int whence)
{
  register FILE *fp = (FILE *) cookie;
  register _off_t ret;

  ret = _lseek_r (fp->_data, fp->_file, (_off_t) offset, whence);
  if (ret == -1L)
    fp->_flags &= ~__SOFF;
  else
    {
      fp->_flags |= __SOFF;
      fp->_offset = ret;
    }
  return ret;
}

int
__sclose (void *cookie)
{
  FILE *fp = (FILE *) cookie;

  return _close_r (fp->_data, fp->_file);
}
