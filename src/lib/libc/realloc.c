/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdlib/realloc.c as the EE's newlib has it (MAIN 0x197808): realloc on the global
 * reentrancy block inside the malloc lock (stock newlib calls _realloc_r straight through). */

#include <stddef.h>

struct _reent;
extern struct _reent *_impure_ptr;
extern void __malloc_lock(struct _reent *);
extern void __malloc_unlock(struct _reent *);
extern void *_realloc_r(struct _reent *, void *, size_t);

void *realloc(void *ap, size_t nbytes)
{
	void *p;

	__malloc_lock(_impure_ptr);
	p = _realloc_r(_impure_ptr, ap, nbytes);
	__malloc_unlock(_impure_ptr);
	return p;
}
