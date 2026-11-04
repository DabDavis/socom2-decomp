/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's malignr.o: mallocr.c built with DEFINE_MEMALIGN -- _memalign_r (README "The GCC front"). */
#define DEFINE_MEMALIGN
#include "mallocr.h"

#ifdef DEFINE_MEMALIGN

/*

  memalign algorithm:

    memalign requests more than enough space from malloc, finds a spot
    within that chunk that meets the alignment request, and then
    possibly frees the leading and trailing space. 

    The alignment argument must be a power of two. This property is not
    checked by memalign, so misuse may result in random runtime errors.

    8-byte alignment is guaranteed by normal malloc calls, so don't
    bother calling memalign with an argument of 8 or less.

    Overreliance on memalign is a sure way to fragment space.

*/


#if __STD_C
Void_t* mEMALIGn(RARG size_t alignment, size_t bytes)
#else
Void_t* mEMALIGn(RARG alignment, bytes) RDECL size_t alignment; size_t bytes;
#endif
{
  INTERNAL_SIZE_T    nb;      /* padded  request size */
  char*     m;                /* memory returned by malloc call */
  mchunkptr p;                /* corresponding chunk */
  char*     brk;              /* alignment point within p */
  mchunkptr newp;             /* chunk to return */
  INTERNAL_SIZE_T  newsize;   /* its size */
  INTERNAL_SIZE_T  leadsize;  /* leading space befor alignment point */
  mchunkptr remainder;        /* spare room at end to split off */
  long      remainder_size;   /* its size */

  /* If need less alignment than we give anyway, just relay to malloc */

  if (alignment <= MALLOC_ALIGNMENT) return mALLOc(RCALL bytes);

  /* Otherwise, ensure that it is at least a minimum chunk size */
  
  if (alignment <  MINSIZE) alignment = MINSIZE;

  /* Call malloc with worst case padding to hit alignment. */

  nb = request2size(bytes);
  m  = (char*)(mALLOc(RCALL nb + alignment + MINSIZE));

  if (m == 0) return 0; /* propagate failure */

  MALLOC_LOCK;

  p = mem2chunk(m);

  if ((((unsigned long)(m)) % alignment) == 0) /* aligned */
  {
#if HAVE_MMAP
    if(chunk_is_mmapped(p))
    {
      MALLOC_UNLOCK;
      return chunk2mem(p); /* nothing more to do */
    }
#endif
  }
  else /* misaligned */
  {
    /* 
      Find an aligned spot inside chunk.
      Since we need to give back leading space in a chunk of at 
      least MINSIZE, if the first calculation places us at
      a spot with less than MINSIZE leader, we can move to the
      next aligned spot -- we've allocated enough total room so that
      this is always possible.
    */

    brk = (char*)mem2chunk(((unsigned long)(m + alignment - 1)) & -alignment);
    if ((long)(brk - (char*)(p)) < (long)MINSIZE) brk = brk + alignment;

    newp = (mchunkptr)brk;
    leadsize = brk - (char*)(p);
    newsize = chunksize(p) - leadsize;

#if HAVE_MMAP
    if(chunk_is_mmapped(p)) 
    {
      newp->prev_size = p->prev_size + leadsize;
      set_head(newp, newsize|IS_MMAPPED);
      MALLOC_UNLOCK;
      return chunk2mem(newp);
    }
#endif

    /* give back leader, use the rest */

    set_head(newp, newsize | PREV_INUSE);
    set_inuse_bit_at_offset(newp, newsize);
    set_head_size(p, leadsize);
    fREe(RCALL chunk2mem(p));
    p = newp;

    assert (newsize >= nb && (((unsigned long)(chunk2mem(p))) % alignment) == 0);
  }

  /* Also give back spare room at the end */

  remainder_size = long_sub_size_t(chunksize(p), nb);

  if (remainder_size >= (long)MINSIZE)
  {
    remainder = chunk_at_offset(p, nb);
    set_head(remainder, remainder_size | PREV_INUSE);
    set_head_size(p, nb);
    fREe(RCALL chunk2mem(remainder));
  }

  check_inuse_chunk(p);
  MALLOC_UNLOCK;
  return chunk2mem(p);

}

#endif /* DEFINE_MEMALIGN */
