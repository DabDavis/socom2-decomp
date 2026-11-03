/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdio/fwrite.c (MAIN 0x194520), the toolchain's libc. */

#include "newlib.h"

extern int __sfvwrite(FILE *, struct __suio *);

size_t
fwrite (buf, size, count, fp)
     const void *buf;
     size_t size;
     size_t count;
     FILE * fp;
{
  size_t n;
  struct __suio uio;
  struct __siov iov;

  iov.iov_base = buf;
  uio.uio_resid = iov.iov_len = n = count * size;
  uio.uio_iov = &iov;
  uio.uio_iovcnt = 1;

  /*
   * The usual case is success (__sfvwrite returns 0);
   * skip the divide if this happens, since divides are
   * generally slow and since this occurs whenever size==0.
   */

  if (__sfvwrite (fp, &uio) == 0)
    return count;
  return (n - uio.uio_resid) / size;
}
