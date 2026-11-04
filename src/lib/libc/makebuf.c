/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdio/makebuf.c (MAIN 0x1948A8), the toolchain's libc: a stream's buffer, BUFSIZ bytes
 * (no st_blksize here: HAVE_BLKSIZE undefined, the optimised-seek block 1024). */

#include "newlib.h"

extern _fpos_t __sseek(void *, _fpos_t, int);
extern void _cleanup_r(struct _reent *);
extern int isatty(int);

void
__smakebuf (fp)
     register FILE * fp;
{
  register size_t size, couldbetty;
  register void *p;
  struct stat st;

  if (fp->_flags & __SNBF)
    {
      fp->_bf._base = fp->_p = fp->_nbuf;
      fp->_bf._size = 1;
      return;
    }
  if (fp->_file < 0 || _fstat_r (fp->_data, fp->_file, &st) < 0)
    {
      couldbetty = 0;
      size = BUFSIZ;
      /* do not try to optimise fseek() */
      fp->_flags |= __SNPT;
    }
  else
    {
      couldbetty = (st.st_mode & S_IFMT) == S_IFCHR;
      size = BUFSIZ;
      /*
       * Optimize fseek() only if it is a regular file.
       * (The test for __sseek is mainly paranoia.)
       */
      if ((st.st_mode & S_IFMT) == S_IFREG && fp->_seek == __sseek)
	{
	  fp->_flags |= __SOPT;
	  fp->_blksize = 1024;
	}
      else
	fp->_flags |= __SNPT;
    }
  if ((p = _malloc_r (fp->_data, size)) == NULL)
    {
      fp->_flags |= __SNBF;
      fp->_bf._base = fp->_p = fp->_nbuf;
      fp->_bf._size = 1;
    }
  else
    {
      fp->_data->__cleanup = _cleanup_r;
      fp->_flags |= __SMBF;
      fp->_bf._base = fp->_p = (unsigned char *) p;
      fp->_bf._size = size;
      if (couldbetty && isatty (fp->_file))
	fp->_flags |= __SLBF;
    }
}
