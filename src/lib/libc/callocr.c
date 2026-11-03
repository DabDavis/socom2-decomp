/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdlib/mallocr.c built with DEFINE_CALLOC (MAIN 0x191630): _calloc_r, Doug Lea's cALLOc
 * as newlib configures it here -- no mmap, MORECORE_CLEARS off (no lock around it: the caller,
 * calloc, holds the malloc lock), MALLOC_ZERO's unrolled clear up to nine words, else memset.
 * INTERNAL_SIZE_T is size_t (a word). Only the pieces cALLOc reads are kept. */

#include "newlib.h"

#define INTERNAL_SIZE_T size_t
#define SIZE_SZ                (sizeof(INTERNAL_SIZE_T))
#define PREV_INUSE 0x1
#define IS_MMAPPED 0x2

struct malloc_chunk
{
  INTERNAL_SIZE_T prev_size; /* Size of previous chunk (if free). */
  INTERNAL_SIZE_T size;      /* Size in bytes, including overhead. */
  struct malloc_chunk* fd;   /* double links -- used only if free. */
  struct malloc_chunk* bk;
};

typedef struct malloc_chunk* mchunkptr;

#define mem2chunk(mem) ((mchunkptr)((char*)(mem) - 2*SIZE_SZ))
#define chunksize(p)          ((p)->size & ~(PREV_INUSE|IS_MMAPPED))

#define MALLOC_ZERO(charp, nbytes)                                            \
do {                                                                          \
  INTERNAL_SIZE_T mzsz = (nbytes);                                            \
  if(mzsz <= 9*sizeof(mzsz)) {                                                \
    INTERNAL_SIZE_T* mz = (INTERNAL_SIZE_T*) (charp);                         \
    if(mzsz >= 5*sizeof(mzsz)) {     *mz++ = 0;                               \
                                     *mz++ = 0;                               \
      if(mzsz >= 7*sizeof(mzsz)) {   *mz++ = 0;                               \
                                     *mz++ = 0;                               \
        if(mzsz >= 9*sizeof(mzsz)) { *mz++ = 0;                               \
                                     *mz++ = 0; }}}                           \
                                     *mz++ = 0;                               \
                                     *mz++ = 0;                               \
                                     *mz   = 0;                               \
  } else memset((charp), 0, mzsz);                                            \
} while(0)

void *
_calloc_r (reent_ptr, n, elem_size)
     struct _reent *reent_ptr;
     size_t n;
     size_t elem_size;
{
  mchunkptr p;
  INTERNAL_SIZE_T csz;

  INTERNAL_SIZE_T sz = n * elem_size;

  void* mem;

  mem = _malloc_r (reent_ptr, sz);

  if (mem == 0)
  {
    return 0;
  }
  else
  {
    p = mem2chunk(mem);

    /* Two optional cases in which clearing not necessary */

    csz = chunksize(p);

    MALLOC_ZERO(mem, csz - SIZE_SZ);
    return mem;
  }
}
