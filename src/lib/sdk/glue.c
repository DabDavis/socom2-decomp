/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's newlib glue (MAIN 0x1A3FA0-0x1A4370): Sony's SDK, the demo's object 0x15B9A0 (C),
 * its sixteen functions in the same order -- the console's lazy init flag, the vertical-blank
 * waits (the INTC's VBLANK_S bit acknowledged with interrupts masked, then polled), the system
 * calls newlib's reentrant wrappers reach (write and read through the DECI2 console, sbrk over
 * EndOfHeap with interrupts masked, the rest stubs: ENOSYS-like errors, isatty and getpid 1,
 * kill(1) exits). Names are SOCOM 1's (s1:exact, s1:order); the statics' the demo's labels
 * (ttyinit, sbrk's heap_ptr.30: this compile numbers the function-local static .30 as well).
 *
 * Shapes, measured: sbrk masks interrupts with the goto loop dintr.c's notes found (GCC 2.9's loop
 * optimiser hoists only from while/do/for: the target reloads the 0x10000 mask each turn) and
 * computes the new break once, before EndOfHeap, rereading heap_ptr after the call; VSync2's
 * flag and count are written by the vertical-blank handler SetVSyncFlag registers, so both are
 * read volatile (the count's load stays out of the last branch's delay slot). */

typedef unsigned int u_int;
typedef unsigned long u_long;

struct stat;

extern int DIntr(void);
extern int EIntr(void);
extern int sceTtyInit(void);
extern int sceTtyWrite(const char *buf, int len);
extern int sceTtyRead(char *buf, int len);
extern int *__errno(void);
extern void *EndOfHeap(void);
extern void Exit(int);
extern void SetVSyncFlag(int *flag, u_long *count);
extern char _end[];

static int ttyinit = 0;

void sceResetttyinit(void)
{
	ttyinit = 0;
}

void VSync(void)
{
	int eie;

	eie = DIntr();
	*(volatile u_int *)0x1000F000 = 4;
	asm volatile ("sync");
	if (eie)
		EIntr();
	while (!(*(volatile u_int *)0x1000F000 & 4))
		;
	eie = DIntr();
	*(volatile u_int *)0x1000F000 = 4;
	asm volatile ("sync");
	if (eie)
		EIntr();
}

u_long VSync2(void)
{
	int flag;
	u_long count;
	int eie;

	flag = 0;
	SetVSyncFlag(&flag, &count);
	eie = DIntr();
	*(volatile u_int *)0x1000F000 = 4;
	asm volatile ("sync");
	if (eie)
		EIntr();
	while (!(*(volatile u_int *)0x1000F000 & 4) && !*(volatile int *)&flag)
		;
	eie = DIntr();
	*(volatile u_int *)0x1000F000 = 4;
	asm volatile ("sync");
	if (eie)
		EIntr();
	return *(volatile u_long *)&count;
}

int write(int fd, const char *buf, int len)
{
	if (fd == 1 || fd == 2) {
		if (!ttyinit) {
			if (!sceTtyInit())
				return -1;
			ttyinit = 1;
		}
		return sceTtyWrite(buf, len);
	}
	return -1;
}

int read(int fd, char *buf, int len)
{
	if (fd == 0) {
		if (!ttyinit) {
			if (!sceTtyInit())
				return -1;
			ttyinit = 1;
		}
		return sceTtyRead(buf, len);
	}
	return -1;
}

int open(const char *name, int flags, int mode)
{
	*__errno() = 5;
	return -1;
}

int close(int fd)
{
	return -1;
}

int ioctl(int fd, int req, void *arg)
{
	return -1;
}

int lseek(int fd, int offset, int whence)
{
	return -1;
}

void *sbrk(int incr)
{
	static char *heap_ptr = _end;
	char *prev, *end;
	int eie, sr;

	asm volatile ("mfc0 %0, $12" : "=r" (eie));
	eie &= 0x10000;
	if (eie) {
again:
		asm volatile ("di");
		asm volatile ("sync.p");
		asm volatile ("mfc0 %0, $12" : "=r" (sr));
		if (sr & 0x10000)
			goto again;
	}
	end = heap_ptr + incr;
	if ((char *)EndOfHeap() < end) {
		*__errno() = 12;
		if (eie)
			asm volatile ("ei");
		return (void *)-1;
	}
	prev = heap_ptr;
	heap_ptr = end;
	if (eie)
		asm volatile ("ei");
	return prev;
}

int isatty(int fd)
{
	return 1;
}

int fstat(int fd, struct stat *st)
{
	((int *)st)[1] = 0x2000;
	*(long *)((char *)st + 72) = 0;
	return 0;
}

int getpid(void)
{
	return 1;
}

int kill(int pid, int sig)
{
	if (pid == 1)
		Exit(sig);
	return 0;
}

int stat(const char *name, struct stat *st)
{
	*__errno() = 5;
	return -1;
}

int unlink(const char *name)
{
	*__errno() = 5;
	return -1;
}
