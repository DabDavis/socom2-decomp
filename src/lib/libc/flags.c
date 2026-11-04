/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdio/flags.c (MAIN 0x1932F0), the toolchain's libc: fopen's mode string to the
 * stream's flags and open(2)'s. */

#include "newlib.h"

#define	EINVAL 22
#define	O_RDONLY	0
#define	O_WRONLY	1
#define	O_RDWR		2
#define	O_APPEND	0x0008
#define	O_CREAT		0x0200
#define	O_TRUNC		0x0400

int
__sflags (ptr, mode, optr)
     struct _reent *ptr;
     register char *mode;
     int *optr;
{
  register int ret, m, o;

  switch (mode[0])
    {
    case 'r':			/* open for reading */
      ret = __SRD;
      m = O_RDONLY;
      o = 0;
      break;

    case 'w':			/* open for writing */
      ret = __SWR;
      m = O_WRONLY;
      o = O_CREAT | O_TRUNC;
      break;

    case 'a':			/* open for appending */
      ret = __SWR | __SAPP;
      m = O_WRONLY;
      o = O_CREAT | O_APPEND;
      break;
    default:			/* illegal mode */
      ptr->_errno = EINVAL;
      return (0);
    }
  if (mode[1] && (mode[1] == '+' || mode[2] == '+'))
    {
      ret = __SRW;
      m = O_RDWR;
    }
  *optr = m | o;
  return ret;
}
