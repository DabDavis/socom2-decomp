/* newlib 1.9's stdlib/mallocr.c (Doug Lea's malloc 2.6.4, public domain, as newlib configures it):
 * everything before its functions, comments stripped. Each unit defining one DEFINE_* macro and
 * including this is one of newlib's objects built from mallocr.c (mallocr.o, freer.o, reallocr.o,
 * malignr.o), the function text after it verbatim -- byte-exact as newlib wrote it, under the
 * EE's configuration (what newlib's sys/config.h and reent.h give the r5900, inlined here for the
 * three system includes): 16-byte alignment, size_t (a word) smaller than long (64-bit: every
 * remainder is long_sub_size_t's conditional form), _sbrk_r the MORECORE. */
#ifndef SC_MALLOCR_H
#define SC_MALLOCR_H

#define INTERNAL_NEWLIB
#define MALLOC_ALIGNMENT 16
#define SIZE_T_SMALLER_THAN_LONG
#define _POINTER_INT long

struct _reent;
extern void *_sbrk_r(struct _reent *, int);
extern struct _reent *_impure_ptr;

#ifndef __STD_C
#ifdef __STDC__
#define __STD_C     1
#else
#if __cplusplus
#define __STD_C     1
#else
#define __STD_C     0
#endif
#endif
#endif

#ifndef Void_t
#if __STD_C
#define Void_t      void
#else
#define Void_t      char
#endif
#endif

#if __STD_C
#include <stddef.h>
#else
#include <sys/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#ifdef INTERNAL_NEWLIB

#define POINTER_UINT unsigned _POINTER_INT
#define SEPARATE_OBJECTS
#define HAVE_MMAP 0
#define MORECORE(size) _sbrk_r(reent_ptr, (size))
#define MORECORE_CLEARS 0
#define MALLOC_LOCK __malloc_lock(reent_ptr)
#define MALLOC_UNLOCK __malloc_unlock(reent_ptr)

#ifndef _WIN32
#ifdef SMALL_MEMORY
#define malloc_getpagesize (128)
#else
#define malloc_getpagesize (4096)
#endif
#endif

#if __STD_C
extern void __malloc_lock(struct _reent *);
extern void __malloc_unlock(struct _reent *);
#else
extern void __malloc_lock();
extern void __malloc_unlock();
#endif

#if __STD_C
#define RARG struct _reent *reent_ptr,
#define RONEARG struct _reent *reent_ptr
#else
#define RARG reent_ptr
#define RONEARG reent_ptr
#define RDECL struct _reent *reent_ptr;
#endif

#define RCALL reent_ptr,
#define RONECALL reent_ptr

#else

#define POINTER_UINT unsigned long
#define RARG
#define RONEARG
#define RDECL
#define RCALL
#define RONECALL

#endif

#if DEBUG
#include <assert.h>
#else
#define assert(x) ((void)0)
#endif

#ifndef SEPARATE_OBJECTS
#define DEFINE_MALLOC
#define DEFINE_FREE
#define DEFINE_REALLOC
#define DEFINE_CALLOC
#define DEFINE_CFREE
#define DEFINE_MEMALIGN
#define DEFINE_VALLOC
#define DEFINE_PVALLOC
#define DEFINE_MALLINFO
#define DEFINE_MALLOC_STATS
#define DEFINE_MALLOC_USABLE_SIZE
#define DEFINE_MALLOPT

#define STATIC static
#else
#define STATIC
#endif

#ifndef MALLOC_LOCK
#define MALLOC_LOCK
#endif

#ifndef MALLOC_UNLOCK
#define MALLOC_UNLOCK
#endif

#ifndef INTERNAL_SIZE_T
#define INTERNAL_SIZE_T size_t
#endif

#ifdef SIZE_T_SMALLER_THAN_LONG
#define long_sub_size_t(x, y) ( (x < y) ? -((long)(y - x)) : (x - y) );
#else
#define long_sub_size_t(x, y) ( (long)(x - y) )
#endif

#ifdef WIN32
#define MORECORE wsbrk
#define HAVE_MMAP 0
#endif

#define HAVE_MEMCPY

#ifndef USE_MEMCPY
#ifdef HAVE_MEMCPY
#define USE_MEMCPY 1
#else
#define USE_MEMCPY 0
#endif
#endif

#if (__STD_C || defined(HAVE_MEMCPY))

#if __STD_C
void* memset(void*, int, size_t);
void* memcpy(void*, const void*, size_t);
#else
Void_t* memset();
Void_t* memcpy();
#endif
#endif

#if USE_MEMCPY

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

#define MALLOC_COPY(dest,src,nbytes)                                          \
do {                                                                          \
  INTERNAL_SIZE_T mcsz = (nbytes);                                            \
  if(mcsz <= 9*sizeof(mcsz)) {                                                \
    INTERNAL_SIZE_T* mcsrc = (INTERNAL_SIZE_T*) (src);                        \
    INTERNAL_SIZE_T* mcdst = (INTERNAL_SIZE_T*) (dest);                       \
    if(mcsz >= 5*sizeof(mcsz)) {     *mcdst++ = *mcsrc++;                     \
                                     *mcdst++ = *mcsrc++;                     \
      if(mcsz >= 7*sizeof(mcsz)) {   *mcdst++ = *mcsrc++;                     \
                                     *mcdst++ = *mcsrc++;                     \
        if(mcsz >= 9*sizeof(mcsz)) { *mcdst++ = *mcsrc++;                     \
                                     *mcdst++ = *mcsrc++; }}}                 \
                                     *mcdst++ = *mcsrc++;                     \
                                     *mcdst++ = *mcsrc++;                     \
                                     *mcdst   = *mcsrc  ;                     \
  } else memcpy(dest, src, mcsz);                                             \
} while(0)

#else

#define MALLOC_ZERO(charp, nbytes)                                            \
do {                                                                          \
  INTERNAL_SIZE_T* mzp = (INTERNAL_SIZE_T*)(charp);                           \
  long mctmp = (nbytes)/sizeof(INTERNAL_SIZE_T), mcn;                         \
  if (mctmp < 8) mcn = 0; else { mcn = (mctmp-1)/8; mctmp %= 8; }             \
  switch (mctmp) {                                                            \
    case 0: for(;;) { *mzp++ = 0;                                             \
    case 7:           *mzp++ = 0;                                             \
    case 6:           *mzp++ = 0;                                             \
    case 5:           *mzp++ = 0;                                             \
    case 4:           *mzp++ = 0;                                             \
    case 3:           *mzp++ = 0;                                             \
    case 2:           *mzp++ = 0;                                             \
    case 1:           *mzp++ = 0; if(mcn <= 0) break; mcn--; }                \
  }                                                                           \
} while(0)

#define MALLOC_COPY(dest,src,nbytes)                                          \
do {                                                                          \
  INTERNAL_SIZE_T* mcsrc = (INTERNAL_SIZE_T*) src;                            \
  INTERNAL_SIZE_T* mcdst = (INTERNAL_SIZE_T*) dest;                           \
  long mctmp = (nbytes)/sizeof(INTERNAL_SIZE_T), mcn;                         \
  if (mctmp < 8) mcn = 0; else { mcn = (mctmp-1)/8; mctmp %= 8; }             \
  switch (mctmp) {                                                            \
    case 0: for(;;) { *mcdst++ = *mcsrc++;                                    \
    case 7:           *mcdst++ = *mcsrc++;                                    \
    case 6:           *mcdst++ = *mcsrc++;                                    \
    case 5:           *mcdst++ = *mcsrc++;                                    \
    case 4:           *mcdst++ = *mcsrc++;                                    \
    case 3:           *mcdst++ = *mcsrc++;                                    \
    case 2:           *mcdst++ = *mcsrc++;                                    \
    case 1:           *mcdst++ = *mcsrc++; if(mcn <= 0) break; mcn--; }       \
  }                                                                           \
} while(0)

#endif

#ifndef HAVE_MMAP
#define HAVE_MMAP 1
#endif

#ifndef HAVE_MREMAP
#ifdef INTERNAL_LINUX_C_LIB
#define HAVE_MREMAP 1
#else
#define HAVE_MREMAP 0
#endif
#endif

#if HAVE_MMAP

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif

#endif

#ifndef malloc_getpagesize
#  ifdef _SC_PAGESIZE
#    ifndef _SC_PAGE_SIZE
#      define _SC_PAGE_SIZE _SC_PAGESIZE
#    endif
#  endif
#  ifdef _SC_PAGE_SIZE
#    define malloc_getpagesize sysconf(_SC_PAGE_SIZE)
#  else
#    if defined(BSD) || defined(DGUX) || defined(HAVE_GETPAGESIZE)
       extern size_t getpagesize();
#      define malloc_getpagesize getpagesize()
#    else
#      include <sys/param.h>
#      ifdef EXEC_PAGESIZE
#        define malloc_getpagesize EXEC_PAGESIZE
#      else
#        ifdef NBPG
#          ifndef CLSIZE
#            define malloc_getpagesize NBPG
#          else
#            define malloc_getpagesize (NBPG * CLSIZE)
#          endif
#        else
#          ifdef NBPC
#            define malloc_getpagesize NBPC
#          else
#            ifdef PAGESIZE
#              define malloc_getpagesize PAGESIZE
#            else
#              define malloc_getpagesize (4096)
#            endif
#          endif
#        endif
#      endif
#    endif
#  endif
#endif

#if HAVE_USR_INCLUDE_MALLOC_H
#include "/usr/include/malloc.h"
#else

struct mallinfo {
  int arena;
  int ordblks;
  int smblks;
  int hblks;
  int hblkhd;
  int usmblks;
  int fsmblks;
  int uordblks;
  int fordblks;
  int keepcost;
};

#define M_MXFAST  1
#define M_NLBLKS  2
#define M_GRAIN   3
#define M_KEEP    4

#endif

#define M_TRIM_THRESHOLD    -1
#define M_TOP_PAD           -2
#define M_MMAP_THRESHOLD    -3
#define M_MMAP_MAX          -4

#ifndef DEFAULT_TRIM_THRESHOLD
#define DEFAULT_TRIM_THRESHOLD (128L * 1024L)
#endif

#ifndef DEFAULT_TOP_PAD
#define DEFAULT_TOP_PAD        (0)
#endif

#ifndef DEFAULT_MMAP_THRESHOLD
#define DEFAULT_MMAP_THRESHOLD (128 * 1024)
#endif

#ifndef DEFAULT_MMAP_MAX
#if HAVE_MMAP
#define DEFAULT_MMAP_MAX       (64)
#else
#define DEFAULT_MMAP_MAX       (0)
#endif
#endif

#ifdef INTERNAL_LINUX_C_LIB

#if __STD_C

Void_t * __default_morecore_init (ptrdiff_t);
Void_t *(*__morecore)(ptrdiff_t) = __default_morecore_init;

#else

Void_t * __default_morecore_init ();
Void_t *(*__morecore)() = __default_morecore_init;

#endif

#define MORECORE (*__morecore)
#define MORECORE_FAILURE 0
#define MORECORE_CLEARS 1

#else

#ifndef INTERNAL_NEWLIB
#if __STD_C
extern Void_t*     sbrk(ptrdiff_t);
#else
extern Void_t*     sbrk();
#endif
#endif

#ifndef MORECORE
#define MORECORE sbrk
#endif

#ifndef MORECORE_FAILURE
#define MORECORE_FAILURE -1
#endif

#ifndef MORECORE_CLEARS
#define MORECORE_CLEARS 1
#endif

#endif

#if defined(INTERNAL_LINUX_C_LIB) && defined(__ELF__)

#define cALLOc		__libc_calloc
#define fREe		__libc_free
#define mALLOc		__libc_malloc
#define mEMALIGn	__libc_memalign
#define rEALLOc		__libc_realloc
#define vALLOc		__libc_valloc
#define pvALLOc		__libc_pvalloc
#define mALLINFo	__libc_mallinfo
#define mALLOPt		__libc_mallopt

#pragma weak calloc = __libc_calloc
#pragma weak free = __libc_free
#pragma weak cfree = __libc_free
#pragma weak malloc = __libc_malloc
#pragma weak memalign = __libc_memalign
#pragma weak realloc = __libc_realloc
#pragma weak valloc = __libc_valloc
#pragma weak pvalloc = __libc_pvalloc
#pragma weak mallinfo = __libc_mallinfo
#pragma weak mallopt = __libc_mallopt

#else

#ifdef INTERNAL_NEWLIB

#define cALLOc		_calloc_r
#define fREe		_free_r
#define mALLOc		_malloc_r
#define mEMALIGn	_memalign_r
#define rEALLOc		_realloc_r
#define vALLOc		_valloc_r
#define pvALLOc		_pvalloc_r
#define mALLINFo	_mallinfo_r
#define mALLOPt		_mallopt_r

#define malloc_stats			_malloc_stats_r
#define malloc_trim			_malloc_trim_r
#define malloc_usable_size		_malloc_usable_size_r

#define malloc_update_mallinfo		__malloc_update_mallinfo

#define malloc_av_			__malloc_av_
#define malloc_current_mallinfo		__malloc_current_mallinfo
#define malloc_max_sbrked_mem		__malloc_max_sbrked_mem
#define malloc_max_total_mem		__malloc_max_total_mem
#define malloc_sbrk_base		__malloc_sbrk_base
#define malloc_top_pad			__malloc_top_pad
#define malloc_trim_threshold		__malloc_trim_threshold

#else

#define cALLOc		calloc
#define fREe		free
#define mALLOc		malloc
#define mEMALIGn	memalign
#define rEALLOc		realloc
#define vALLOc		valloc
#define pvALLOc		pvalloc
#define mALLINFo	mallinfo
#define mALLOPt		mallopt

#endif
#endif

#if __STD_C

Void_t* mALLOc(RARG size_t);
void    fREe(RARG Void_t*);
Void_t* rEALLOc(RARG Void_t*, size_t);
Void_t* mEMALIGn(RARG size_t, size_t);
Void_t* vALLOc(RARG size_t);
Void_t* pvALLOc(RARG size_t);
Void_t* cALLOc(RARG size_t, size_t);
void    cfree(Void_t*);
int     malloc_trim(RARG size_t);
size_t  malloc_usable_size(RARG Void_t*);
void    malloc_stats(RONEARG);
int     mALLOPt(RARG int, int);
struct mallinfo mALLINFo(RONEARG);
#else
Void_t* mALLOc();
void    fREe();
Void_t* rEALLOc();
Void_t* mEMALIGn();
Void_t* vALLOc();
Void_t* pvALLOc();
Void_t* cALLOc();
void    cfree();
int     malloc_trim();
size_t  malloc_usable_size();
void    malloc_stats();
int     mALLOPt();
struct mallinfo mALLINFo();
#endif

#ifdef __cplusplus
};
#endif

#ifdef WIN32

#define AlignPage(add) (((add) + (malloc_getpagesize-1)) &
~(malloc_getpagesize-1))

#define RESERVED_SIZE (1024*1024*64)
#define NEXT_SIZE (2048*1024)
#define TOP_MEMORY ((unsigned long)2*1024*1024*1024)

struct GmListElement;
typedef struct GmListElement GmListElement;

struct GmListElement
{
	GmListElement* next;
	void* base;
};

static GmListElement* head = 0;
static unsigned int gNextAddress = 0;
static unsigned int gAddressBase = 0;
static unsigned int gAllocatedSize = 0;

static
GmListElement* makeGmListElement (void* bas)
{
	GmListElement* this;
	this = (GmListElement*)(void*)LocalAlloc (0, sizeof (GmListElement));
	ASSERT (this);
	if (this)
	{
		this->base = bas;
		this->next = head;
		head = this;
	}
	return this;
}

void gcleanup ()
{
	BOOL rval;
	ASSERT ( (head == NULL) || (head->base == (void*)gAddressBase));
	if (gAddressBase && (gNextAddress - gAddressBase))
	{
		rval = VirtualFree ((void*)gAddressBase,
							gNextAddress - gAddressBase,
							MEM_DECOMMIT);
        ASSERT (rval);
	}
	while (head)
	{
		GmListElement* next = head->next;
		rval = VirtualFree (head->base, 0, MEM_RELEASE);
		ASSERT (rval);
		LocalFree (head);
		head = next;
	}
}

static
void* findRegion (void* start_address, unsigned long size)
{
	MEMORY_BASIC_INFORMATION info;
	while ((unsigned long)start_address < TOP_MEMORY)
	{
		VirtualQuery (start_address, &info, sizeof (info));
		if (info.State != MEM_FREE)
			start_address = (char*)info.BaseAddress + info.RegionSize;
		else if (info.RegionSize >= size)
			return start_address;
		else
			start_address = (char*)info.BaseAddress + info.RegionSize;
	}
	return NULL;

}

void* wsbrk (long size)
{
	void* tmp;
	if (size > 0)
	{
		if (gAddressBase == 0)
		{
			gAllocatedSize = max (RESERVED_SIZE, AlignPage (size));
			gNextAddress = gAddressBase =
				(unsigned int)VirtualAlloc (NULL, gAllocatedSize,
											MEM_RESERVE, PAGE_NOACCESS);
		} else if (AlignPage (gNextAddress + size) > (gAddressBase +
gAllocatedSize))
		{
			long new_size = max (NEXT_SIZE, AlignPage (size));
			void* new_address = (void*)(gAddressBase+gAllocatedSize);
			do
			{
				new_address = findRegion (new_address, new_size);

				if (new_address == 0)
					return (void*)-1;

				gAddressBase = gNextAddress =
					(unsigned int)VirtualAlloc (new_address, new_size,
												MEM_RESERVE, PAGE_NOACCESS);
				// repeat in case of race condition
				// The region that we found has been snagged
				// by another thread
			}
			while (gAddressBase == 0);

			ASSERT (new_address == (void*)gAddressBase);

			gAllocatedSize = new_size;

			if (!makeGmListElement ((void*)gAddressBase))
				return (void*)-1;
		}
		if ((size + gNextAddress) > AlignPage (gNextAddress))
		{
			void* res;
			res = VirtualAlloc ((void*)AlignPage (gNextAddress),
								(size + gNextAddress -
								 AlignPage (gNextAddress)),
								MEM_COMMIT, PAGE_READWRITE);
			if (res == 0)
				return (void*)-1;
		}
		tmp = (void*)gNextAddress;
		gNextAddress = (unsigned int)tmp + size;
		return tmp;
	}
	else if (size < 0)
	{
		unsigned int alignedGoal = AlignPage (gNextAddress + size);

		if (alignedGoal >= gAddressBase)
		{
			VirtualFree ((void*)alignedGoal, gNextAddress - alignedGoal,
						 MEM_DECOMMIT);
			gNextAddress = gNextAddress + size;
			return (void*)gNextAddress;
		}
		else
		{
			VirtualFree ((void*)gAddressBase, gNextAddress - gAddressBase,
						 MEM_DECOMMIT);
			gNextAddress = gAddressBase;
			return (void*)-1;
		}
	}
	else
	{
		return (void*)gNextAddress;
	}
}

#endif

struct malloc_chunk
{
  INTERNAL_SIZE_T prev_size;
  INTERNAL_SIZE_T size;
  struct malloc_chunk* fd;
  struct malloc_chunk* bk;
};

typedef struct malloc_chunk* mchunkptr;

#define SIZE_SZ                (sizeof(INTERNAL_SIZE_T))
#ifndef MALLOC_ALIGNMENT
#define MALLOC_ALIGN           8
#define MALLOC_ALIGNMENT       (SIZE_SZ + SIZE_SZ)
#else
#define MALLOC_ALIGN           MALLOC_ALIGNMENT
#endif
#define MALLOC_ALIGN_MASK      (MALLOC_ALIGNMENT - 1)
#define MINSIZE                (sizeof(struct malloc_chunk))

#define chunk2mem(p)   ((Void_t*)((char*)(p) + 2*SIZE_SZ))
#define mem2chunk(mem) ((mchunkptr)((char*)(mem) - 2*SIZE_SZ))

#define request2size(req) \
 (((long)((req) + (SIZE_SZ + MALLOC_ALIGN_MASK)) < \
  (long)(MINSIZE + MALLOC_ALIGN_MASK)) ? ((MINSIZE + MALLOC_ALIGN_MASK) & ~(MALLOC_ALIGN_MASK)) : \
   (((req) + (SIZE_SZ + MALLOC_ALIGN_MASK)) & ~(MALLOC_ALIGN_MASK)))

#define aligned_OK(m)    (((unsigned long)((m)) & (MALLOC_ALIGN_MASK)) == 0)

#define PREV_INUSE 0x1

#define IS_MMAPPED 0x2

#define SIZE_BITS (PREV_INUSE|IS_MMAPPED)

#define next_chunk(p) ((mchunkptr)( ((char*)(p)) + ((p)->size & ~PREV_INUSE) ))

#define prev_chunk(p)\
   ((mchunkptr)( ((char*)(p)) - ((p)->prev_size) ))

#define chunk_at_offset(p, s)  ((mchunkptr)(((char*)(p)) + (s)))

#define inuse(p)\
((((mchunkptr)(((char*)(p))+((p)->size & ~PREV_INUSE)))->size) & PREV_INUSE)

#define prev_inuse(p)  ((p)->size & PREV_INUSE)

#define chunk_is_mmapped(p) ((p)->size & IS_MMAPPED)

#define set_inuse(p)\
((mchunkptr)(((char*)(p)) + ((p)->size & ~PREV_INUSE)))->size |= PREV_INUSE

#define clear_inuse(p)\
((mchunkptr)(((char*)(p)) + ((p)->size & ~PREV_INUSE)))->size &= ~(PREV_INUSE)

#define inuse_bit_at_offset(p, s)\
 (((mchunkptr)(((char*)(p)) + (s)))->size & PREV_INUSE)

#define set_inuse_bit_at_offset(p, s)\
 (((mchunkptr)(((char*)(p)) + (s)))->size |= PREV_INUSE)

#define clear_inuse_bit_at_offset(p, s)\
 (((mchunkptr)(((char*)(p)) + (s)))->size &= ~(PREV_INUSE))

#define chunksize(p)          ((p)->size & ~(SIZE_BITS))

#define set_head_size(p, s)   ((p)->size = (((p)->size & PREV_INUSE) | (s)))

#define set_head(p, s)        ((p)->size = (s))

#define set_foot(p, s)   (((mchunkptr)((char*)(p) + (s)))->prev_size = (s))

#ifdef SEPARATE_OBJECTS
#define av_ malloc_av_
#endif

#define NAV             128

typedef struct malloc_chunk* mbinptr;

#define bin_at(i)      ((mbinptr)((char*)&(av_[2*(i) + 2]) - 2*SIZE_SZ))
#define next_bin(b)    ((mbinptr)((char*)(b) + 2 * sizeof(mbinptr)))
#define prev_bin(b)    ((mbinptr)((char*)(b) - 2 * sizeof(mbinptr)))

#define top            (bin_at(0)->fd)
#define last_remainder (bin_at(1))

#define initial_top    ((mchunkptr)(bin_at(0)))

#define IAV(i)  bin_at(i), bin_at(i)

#ifdef DEFINE_MALLOC
STATIC mbinptr av_[NAV * 2 + 2] = {
 0, 0,
 IAV(0),   IAV(1),   IAV(2),   IAV(3),   IAV(4),   IAV(5),   IAV(6),   IAV(7),
 IAV(8),   IAV(9),   IAV(10),  IAV(11),  IAV(12),  IAV(13),  IAV(14),  IAV(15),
 IAV(16),  IAV(17),  IAV(18),  IAV(19),  IAV(20),  IAV(21),  IAV(22),  IAV(23),
 IAV(24),  IAV(25),  IAV(26),  IAV(27),  IAV(28),  IAV(29),  IAV(30),  IAV(31),
 IAV(32),  IAV(33),  IAV(34),  IAV(35),  IAV(36),  IAV(37),  IAV(38),  IAV(39),
 IAV(40),  IAV(41),  IAV(42),  IAV(43),  IAV(44),  IAV(45),  IAV(46),  IAV(47),
 IAV(48),  IAV(49),  IAV(50),  IAV(51),  IAV(52),  IAV(53),  IAV(54),  IAV(55),
 IAV(56),  IAV(57),  IAV(58),  IAV(59),  IAV(60),  IAV(61),  IAV(62),  IAV(63),
 IAV(64),  IAV(65),  IAV(66),  IAV(67),  IAV(68),  IAV(69),  IAV(70),  IAV(71),
 IAV(72),  IAV(73),  IAV(74),  IAV(75),  IAV(76),  IAV(77),  IAV(78),  IAV(79),
 IAV(80),  IAV(81),  IAV(82),  IAV(83),  IAV(84),  IAV(85),  IAV(86),  IAV(87),
 IAV(88),  IAV(89),  IAV(90),  IAV(91),  IAV(92),  IAV(93),  IAV(94),  IAV(95),
 IAV(96),  IAV(97),  IAV(98),  IAV(99),  IAV(100), IAV(101), IAV(102), IAV(103),
 IAV(104), IAV(105), IAV(106), IAV(107), IAV(108), IAV(109), IAV(110), IAV(111),
 IAV(112), IAV(113), IAV(114), IAV(115), IAV(116), IAV(117), IAV(118), IAV(119),
 IAV(120), IAV(121), IAV(122), IAV(123), IAV(124), IAV(125), IAV(126), IAV(127)
};
#else
extern mbinptr av_[NAV * 2 + 2];
#endif

#define first(b) ((b)->fd)
#define last(b)  ((b)->bk)

#define bin_index(sz)                                                          \
(((((unsigned long)(sz)) >> 9) ==    0) ?       (((unsigned long)(sz)) >>  3): \
 ((((unsigned long)(sz)) >> 9) <=    4) ?  56 + (((unsigned long)(sz)) >>  6): \
 ((((unsigned long)(sz)) >> 9) <=   20) ?  91 + (((unsigned long)(sz)) >>  9): \
 ((((unsigned long)(sz)) >> 9) <=   84) ? 110 + (((unsigned long)(sz)) >> 12): \
 ((((unsigned long)(sz)) >> 9) <=  340) ? 119 + (((unsigned long)(sz)) >> 15): \
 ((((unsigned long)(sz)) >> 9) <= 1364) ? 124 + (((unsigned long)(sz)) >> 18): \
                                          126)

#define MAX_SMALLBIN_SIZE   512
#define SMALLBIN_WIDTH        8
#define SMALLBIN_WIDTH_BITS   3
#define MAX_SMALLBIN        (MAX_SMALLBIN_SIZE / SMALLBIN_WIDTH) - 1

#define smallbin_index(sz)  (((unsigned long)(sz)) >> SMALLBIN_WIDTH_BITS)

#define is_small_request(nb) (nb < MAX_SMALLBIN_SIZE - SMALLBIN_WIDTH)

#define BINBLOCKWIDTH     4

#define binblocks      (bin_at(0)->size)

#define idx2binblock(ix)    ((unsigned long)1 << (ix / BINBLOCKWIDTH))
#define mark_binblock(ii)   (binblocks |= idx2binblock(ii))
#define clear_binblock(ii)  (binblocks &= ~(idx2binblock(ii)))

#ifdef SEPARATE_OBJECTS
#define trim_threshold		malloc_trim_threshold
#define top_pad			malloc_top_pad
#define n_mmaps_max		malloc_n_mmaps_max
#define mmap_threshold		malloc_mmap_threshold
#define sbrk_base		malloc_sbrk_base
#define max_sbrked_mem		malloc_max_sbrked_mem
#define max_total_mem		malloc_max_total_mem
#define current_mallinfo	malloc_current_mallinfo
#define n_mmaps			malloc_n_mmaps
#define max_n_mmaps		malloc_max_n_mmaps
#define mmapped_mem		malloc_mmapped_mem
#define max_mmapped_mem		malloc_max_mmapped_mem
#endif

#ifdef DEFINE_MALLOC

STATIC unsigned long trim_threshold   = DEFAULT_TRIM_THRESHOLD;
STATIC unsigned long top_pad          = DEFAULT_TOP_PAD;
#if HAVE_MMAP
STATIC unsigned int  n_mmaps_max      = DEFAULT_MMAP_MAX;
STATIC unsigned long mmap_threshold   = DEFAULT_MMAP_THRESHOLD;
#endif

STATIC char* sbrk_base = (char*)(-1);

STATIC unsigned long max_sbrked_mem = 0;

STATIC unsigned long max_total_mem = 0;

STATIC struct mallinfo current_mallinfo = {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

#if HAVE_MMAP

STATIC unsigned int n_mmaps = 0;
STATIC unsigned int max_n_mmaps = 0;
STATIC unsigned long mmapped_mem = 0;
STATIC unsigned long max_mmapped_mem = 0;

#endif

#else

extern unsigned long trim_threshold;
extern unsigned long top_pad;
#if HAVE_MMAP
extern unsigned int  n_mmaps_max;
extern unsigned long mmap_threshold;
#endif
extern char* sbrk_base;
extern unsigned long max_sbrked_mem;
extern unsigned long max_total_mem;
extern struct mallinfo current_mallinfo;
#if HAVE_MMAP
extern unsigned int n_mmaps;
extern unsigned int max_n_mmaps;
extern unsigned long mmapped_mem;
extern unsigned long max_mmapped_mem;
#endif

#endif

#define sbrked_mem  (current_mallinfo.arena)

#if DEBUG

#if __STD_C
static void do_check_chunk(mchunkptr p)
#else
static void do_check_chunk(p) mchunkptr p;
#endif
{
  INTERNAL_SIZE_T sz = p->size & ~PREV_INUSE;

  assert(!chunk_is_mmapped(p));

  assert((char*)p >= sbrk_base);
  if (p != top)
    assert((char*)p + sz <= (char*)top);
  else
    assert((char*)p + sz <= sbrk_base + sbrked_mem);

}

#if __STD_C
static void do_check_free_chunk(mchunkptr p)
#else
static void do_check_free_chunk(p) mchunkptr p;
#endif
{
  INTERNAL_SIZE_T sz = p->size & ~PREV_INUSE;
  mchunkptr next = chunk_at_offset(p, sz);

  do_check_chunk(p);

  assert(!inuse(p));

  if ((long)sz >= (long)MINSIZE)
  {
    assert((sz & MALLOC_ALIGN_MASK) == 0);
    assert(aligned_OK(chunk2mem(p)));

    assert(next->prev_size == sz);

    assert(prev_inuse(p));
    assert (next == top || inuse(next));

    assert(p->fd->bk == p);
    assert(p->bk->fd == p);
  }
  else
    assert(sz == SIZE_SZ);
}

#if __STD_C
static void do_check_inuse_chunk(mchunkptr p)
#else
static void do_check_inuse_chunk(p) mchunkptr p;
#endif
{
  mchunkptr next = next_chunk(p);
  do_check_chunk(p);

  assert(inuse(p));

  if (!prev_inuse(p))
  {
    mchunkptr prv = prev_chunk(p);
    assert(next_chunk(prv) == p);
    do_check_free_chunk(prv);
  }
  if (next == top)
  {
    assert(prev_inuse(next));
    assert(chunksize(next) >= MINSIZE);
  }
  else if (!inuse(next))
    do_check_free_chunk(next);

}

#if __STD_C
static void do_check_malloced_chunk(mchunkptr p, INTERNAL_SIZE_T s)
#else
static void do_check_malloced_chunk(p, s) mchunkptr p; INTERNAL_SIZE_T s;
#endif
{
  INTERNAL_SIZE_T sz = p->size & ~PREV_INUSE;
  long room = long_sub_size_t(sz, s);

  do_check_inuse_chunk(p);

  assert((long)sz >= (long)MINSIZE);
  assert((sz & MALLOC_ALIGN_MASK) == 0);
  assert(room >= 0);
  assert(room < (long)MINSIZE);

  assert(aligned_OK(chunk2mem(p)));

  assert(prev_inuse(p));

}

#define check_free_chunk(P)  do_check_free_chunk(P)
#define check_inuse_chunk(P) do_check_inuse_chunk(P)
#define check_chunk(P) do_check_chunk(P)
#define check_malloced_chunk(P,N) do_check_malloced_chunk(P,N)
#else
#define check_free_chunk(P)
#define check_inuse_chunk(P)
#define check_chunk(P)
#define check_malloced_chunk(P,N)
#endif

#define frontlink(P, S, IDX, BK, FD)                                          \
{                                                                             \
  if (S < MAX_SMALLBIN_SIZE)                                                  \
  {                                                                           \
    IDX = smallbin_index(S);                                                  \
    mark_binblock(IDX);                                                       \
    BK = bin_at(IDX);                                                         \
    FD = BK->fd;                                                              \
    P->bk = BK;                                                               \
    P->fd = FD;                                                               \
    FD->bk = BK->fd = P;                                                      \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    IDX = bin_index(S);                                                       \
    BK = bin_at(IDX);                                                         \
    FD = BK->fd;                                                              \
    if (FD == BK) mark_binblock(IDX);                                         \
    else                                                                      \
    {                                                                         \
      while (FD != BK && S < chunksize(FD)) FD = FD->fd;                      \
      BK = FD->bk;                                                            \
    }                                                                         \
    P->bk = BK;                                                               \
    P->fd = FD;                                                               \
    FD->bk = BK->fd = P;                                                      \
  }                                                                           \
}

#define unlink(P, BK, FD)                                                     \
{                                                                             \
  BK = P->bk;                                                                 \
  FD = P->fd;                                                                 \
  FD->bk = BK;                                                                \
  BK->fd = FD;                                                                \
}                                                                             \

#define link_last_remainder(P)                                                \
{                                                                             \
  last_remainder->fd = last_remainder->bk =  P;                               \
  P->fd = P->bk = last_remainder;                                             \
}

#define clear_last_remainder \
  (last_remainder->fd = last_remainder->bk = last_remainder)

#if HAVE_MMAP

#ifdef DEFINE_MALLOC

#if __STD_C
static mchunkptr mmap_chunk(size_t size)
#else
static mchunkptr mmap_chunk(size) size_t size;
#endif
{
  size_t page_mask = malloc_getpagesize - 1;
  mchunkptr p;

#ifndef MAP_ANONYMOUS
  static int fd = -1;
#endif

  if(n_mmaps >= n_mmaps_max) return 0;

  size = (size + SIZE_SZ + page_mask) & ~page_mask;

#ifdef MAP_ANONYMOUS
  p = (mchunkptr)mmap(0, size, PROT_READ|PROT_WRITE,
		      MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
#else
  if (fd < 0)
  {
    fd = open("/dev/zero", O_RDWR);
    if(fd < 0) return 0;
  }
  p = (mchunkptr)mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
#endif

  if(p == (mchunkptr)-1) return 0;

  n_mmaps++;
  if (n_mmaps > max_n_mmaps) max_n_mmaps = n_mmaps;

  assert(aligned_OK(chunk2mem(p)));

  p->prev_size = 0;
  set_head(p, size|IS_MMAPPED);

  mmapped_mem += size;
  if ((unsigned long)mmapped_mem > (unsigned long)max_mmapped_mem)
    max_mmapped_mem = mmapped_mem;
  if ((unsigned long)(mmapped_mem + sbrked_mem) > (unsigned long)max_total_mem)
    max_total_mem = mmapped_mem + sbrked_mem;
  return p;
}

#endif

#ifdef SEPARATE_OBJECTS
#define munmap_chunk malloc_munmap_chunk
#endif

#ifdef DEFINE_FREE

#if __STD_C
STATIC void munmap_chunk(mchunkptr p)
#else
STATIC void munmap_chunk(p) mchunkptr p;
#endif
{
  INTERNAL_SIZE_T size = chunksize(p);
  int ret;

  assert (chunk_is_mmapped(p));
  assert(! ((char*)p >= sbrk_base && (char*)p < sbrk_base + sbrked_mem));
  assert((n_mmaps > 0));
  assert(((p->prev_size + size) & (malloc_getpagesize-1)) == 0);

  n_mmaps--;
  mmapped_mem -= (size + p->prev_size);

  ret = munmap((char *)p - p->prev_size, size + p->prev_size);

  assert(ret == 0);
}

#else

#if __STD_C
extern void munmap_chunk(mchunkptr);
#else
extern void munmap_chunk();
#endif

#endif

#if HAVE_MREMAP

#ifdef DEFINE_REALLOC

#if __STD_C
static mchunkptr mremap_chunk(mchunkptr p, size_t new_size)
#else
static mchunkptr mremap_chunk(p, new_size) mchunkptr p; size_t new_size;
#endif
{
  size_t page_mask = malloc_getpagesize - 1;
  INTERNAL_SIZE_T offset = p->prev_size;
  INTERNAL_SIZE_T size = chunksize(p);
  char *cp;

  assert (chunk_is_mmapped(p));
  assert(! ((char*)p >= sbrk_base && (char*)p < sbrk_base + sbrked_mem));
  assert((n_mmaps > 0));
  assert(((size + offset) & (malloc_getpagesize-1)) == 0);

  new_size = (new_size + offset + SIZE_SZ + page_mask) & ~page_mask;

  cp = (char *)mremap((char *)p - offset, size + offset, new_size, 1);

  if (cp == (char *)-1) return 0;

  p = (mchunkptr)(cp + offset);

  assert(aligned_OK(chunk2mem(p)));

  assert((p->prev_size == offset));
  set_head(p, (new_size - offset)|IS_MMAPPED);

  mmapped_mem -= size + offset;
  mmapped_mem += new_size;
  if ((unsigned long)mmapped_mem > (unsigned long)max_mmapped_mem)
    max_mmapped_mem = mmapped_mem;
  if ((unsigned long)(mmapped_mem + sbrked_mem) > (unsigned long)max_total_mem)
    max_total_mem = mmapped_mem + sbrked_mem;
  return p;
}

#endif

#endif

#endif

#endif
