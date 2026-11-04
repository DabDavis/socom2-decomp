/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's stdio/puts.c (MAIN 0x196C90-0x196D34), the toolchain's libc: _puts_r and puts, the
 * string and its newline as two iovecs to __sfvwrite. */

#include "newlib.h"

extern size_t strlen(const char *);
extern int __sfvwrite(FILE *, struct __suio *);

int
_puts_r (ptr, s)
     struct _reent *ptr;
     const char * s;
{
  size_t c = strlen (s);
  struct __suio uio;
  struct __siov iov[2];

  iov[0].iov_base = s;
  iov[0].iov_len = c;
  iov[1].iov_base = "\n";
  iov[1].iov_len = 1;
  uio.uio_resid = c + 1;
  uio.uio_iov = &iov[0];
  uio.uio_iovcnt = 2;

  return (__sfvwrite (_stdout_r (ptr), &uio) ? EOF : '\n');
}

int
puts (s)
     char const * s;
{
  return _puts_r (_REENT, s);
}
