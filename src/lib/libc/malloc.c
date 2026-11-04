/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdlib/malloc.c as the EE's newlib has it (MAIN 0x194C30-0x194CCC): malloc and free on
 * the global reentrancy block, each inside the malloc lock (stock newlib calls straight through;
 * mlock.c's lock is recursive, so _malloc_r's own lock nests). */

#include <stddef.h>

struct _reent;
extern struct _reent *_impure_ptr;
extern void __malloc_lock(struct _reent *);
extern void __malloc_unlock(struct _reent *);
extern void *_malloc_r(struct _reent *, size_t);
extern void _free_r(struct _reent *, void *);

void *malloc(size_t nbytes)
{
	void *p;

	__malloc_lock(_impure_ptr);
	p = _malloc_r(_impure_ptr, nbytes);
	__malloc_unlock(_impure_ptr);
	return p;
}

void free(void *aptr)
{
	__malloc_lock(_impure_ptr);
	_free_r(_impure_ptr, aptr);
	__malloc_unlock(_impure_ptr);
}
