/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdlib/malign.c as the EE's newlib has it (MAIN 0x194A00): memalign, _memalign_r on the
 * global reentrancy block -- inside the malloc lock here (stock newlib and the SOCOM 1 demo's,
 * 0x2C, call straight through; mlock.c's lock is recursive). */

#include <stddef.h>

struct _reent;
extern struct _reent *_impure_ptr;
extern void __malloc_lock(struct _reent *);
extern void __malloc_unlock(struct _reent *);
extern void *_memalign_r(struct _reent *, size_t, size_t);

void *memalign(size_t align, size_t nbytes)
{
	void *p;

	__malloc_lock(_impure_ptr);
	p = _memalign_r(_impure_ptr, align, nbytes);
	__malloc_unlock(_impure_ptr);
	return p;
}
