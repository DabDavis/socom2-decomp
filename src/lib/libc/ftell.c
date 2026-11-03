/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdio/ftell.c (MAIN 0x193FA0), the toolchain's libc. */

#include "newlib.h"

#define	ESPIPE 29
#define	SEEK_CUR 1

long
ftell (fp)
     register FILE * fp;
{
  _fpos_t pos;

  /* Ensure stdio is set up.  */

  CHECK_INIT (fp);

  if (fp->_seek == NULL)
    {
      fp->_data->_errno = ESPIPE;
      return -1L;
    }

  /* Find offset of underlying I/O object, then
     adjust for buffered bytes. */

  fflush(fp);           /* may adjust seek offset on append stream */
  if (fp->_flags & __SOFF)
    pos = fp->_offset;
  else
    {
      pos = (*fp->_seek) (fp->_cookie, (_fpos_t) 0, SEEK_CUR);
      if (pos == -1L)
	return pos;
    }
  if (fp->_flags & __SRD)
    {
      /*
       * Reading.  Any unread characters (including
       * those from ungetc) cause the position to be
       * smaller than that in the underlying object.
       */
      pos -= fp->_r;
      if (HASUB (fp))
	pos -= fp->_ur;
    }
  else if (fp->_flags & __SWR && fp->_p != NULL)
    {
      /*
       * Writing.  Any buffered characters cause the
       * position to be greater than that in the
       * underlying object.
       */
      pos += fp->_p - fp->_bf._base;
    }

  return pos;
}
