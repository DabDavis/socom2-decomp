/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdlib/mlock.c as the EE's newlib has it (MAIN 0x195A78-0x195B44, between memset's and
 * mprec's objects in newlib's link order): stock newlib's __malloc_lock and __malloc_unlock are
 * empty (the SOCOM 1 demo's are, 8 bytes each); this toolchain's take the runtime's semaphore
 * (`__sce_sema_id`, libkernl's initsys.c creates it) and count the owning thread's recursive
 * holds -- newlib calls them recursively (malloc's wrappers lock, then _malloc_r locks again).
 * Its two words follow mallocr.o's state in .data: the owner (-1, no thread) and the count.
 * Names: the rows' (by body, the integrator). */

struct _reent;

extern int GetThreadId(void);
extern int WaitSema(int sema_id);
extern int SignalSema(int sema_id);
extern int __sce_sema_id;

static int lock_owner = -1;
static int lock_count = 0;

void __malloc_lock(struct _reent *ptr)
{
	int tid = GetThreadId();

	if (lock_owner == tid) {
		lock_count++;
	} else {
		WaitSema(__sce_sema_id);
		lock_owner = tid;
		lock_count++;
	}
}

void __malloc_unlock(struct _reent *ptr)
{
	if (--lock_count == 0) {
		lock_owner = -1;
		SignalSema(__sce_sema_id);
	}
}
