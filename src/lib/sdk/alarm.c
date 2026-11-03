/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's alarm patch (MAIN 0x1ACF48-0x1AD084): Sony's SDK 2.7, the demo's object 0x164848 (C),
 * its five functions in the same order. Unless timer 3 is already counting (T3_MODE's CUE bit), it
 * copies the alarm code image (srcfile) to 0x80076000 and a null EE program (eenull) to 0x82000 in
 * kernel mode, and re-points the alarm syscalls (0xFC-0xFF), the INTC 12 handler entry (0x12C) and
 * syscall 8 at the new code. Names are SOCOM 1's (s1:exact; its statics by position, `setup@alarm`:
 * README "The GCC front"). */

typedef unsigned int u_int;

struct SyscallData {
	int syscall;
	void *function;
};

extern void FlushCache(int op);

extern unsigned char srcfile[0x740];    /* the alarm's kernel-mode code image (this object's .data) */
extern unsigned char eenull[0x28];      /* a null EE program */

static int kCopy(u_int *dest, u_int *src, u_int size);

static struct SyscallData SysEntry[] = {
	{ 0x5A, kCopy },
	{ 0x5B, (void *)0x80076000 },
	{ 0xFC, 0 },                    /* SetAlarm */
	{ 0xFE, 0 },                    /* ReleaseAlarm */
	{ 0xFD, 0 },                    /* iSetAlarm */
	{ 0xFF, 0 },                    /* iReleaseAlarm */
	{ 0x12C, 0 },                   /* the INTC 12 handler entry */
	{ 0x08, 0 },
};

#define T3_MODE ((volatile u_int *)0x10001810)

static void setup(int syscall, void *function)
{
	asm volatile ("li $3, 0x74\n\tsyscall");
}

static int Copy(void *dest, void *src, int size)
{
	asm volatile ("li $3, 0x5a\n\tsyscall");
}

static int kCopy(u_int *dest, u_int *src, u_int size)
{
	u_int i;

	size >>= 2;
	for (i = 0; i < size; i++)
		*dest++ = *src++;
	return 0;
}

static void *GetEntryAddress(int syscall)
{
	asm volatile ("li $3, 0x5b\n\tsyscall");
}

void InitAlarm(void)
{
	u_int i;

	if ((*T3_MODE & 0x100) == 0) {
		setup(SysEntry[0].syscall, SysEntry[0].function);
		Copy((void *)0x80076000, srcfile, sizeof(srcfile));
		Copy((void *)0x00082000, eenull, sizeof(eenull));
		FlushCache(0);
		FlushCache(2);
		setup(SysEntry[1].syscall, SysEntry[1].function);
		for (i = 2; i < 8; i++)
			setup(SysEntry[i].syscall, GetEntryAddress(SysEntry[i].syscall));
	}
}
