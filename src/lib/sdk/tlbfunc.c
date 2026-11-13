/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's TLB set-up (MAIN 0x1ABFD8-0x1AC4E4): Sony's SDK 2.7, the demo's object 0x1638D0 (C),
 * its nineteen functions in the same order. The library installs its own kernel-mode code: the
 * kernel's syscall 0x74 (`setup`) points a syscall slot at a handler, syscall 0x5A (`Copy`) runs
 * `kCopy` in kernel mode to copy a code image (`tlbsrc`) to 0x80075000, syscall 0x5B
 * (`GetEntryAddress`) reads a slot back. The TLB calls are the kernel's syscalls 0x55-0x59, their
 * interrupt-context forms the negated numbers. InitTLB32MB writes the wired kernel and default
 * entries and the extended ones on a 32 MB console. Names are SOCOM 1's (s1:exact, its statics by
 * position: `Copy@tlbfunc` in the map, README "The GCC front"). */

typedef unsigned int u_int;

struct SyscallData {
	int syscall;
	void *function;
};

struct TLBEntry {
	u_int PageMask;
	u_int EntryHi;
	u_int EntryLo0;
	u_int EntryLo1;
};

struct TLBInfo {
	int NumKernelTLBEntries;
	int NumDefaultTLBEntries;
	int NumExtendedTLBEntries;
	int NumWiredEntries;
	struct TLBEntry *kernelTLB;
	struct TLBEntry *defaultTLB;
	struct TLBEntry *extendedTLB;
};

extern int SetVTLBRefillHandler(int cause, void *handler);
extern int SetVCommonHandler(int cause, void *handler);
extern void FlushCache(int op);
extern int GetMemorySize(void);
extern int _InitTLB(void);
extern int kprintf(const char *fmt, ...);
extern void Exit(int status);
extern void _kTLBException(void);
extern void _kExitTLBHandler(void);
extern void _kDebugException(void);

extern unsigned char tlbsrc[0x330];     /* the kernel-mode TLB code image (this object's .data) */

static int kCopy(u_int *dest, u_int *src, u_int size);
static int InitTLB32MB(void);

void *_kTLBRefillHandler = 0;
void *_kDebugHandler[16] = { 0 };
char **_kExecArg = 0;

static struct SyscallData SysEntry[] = {
	{ 0x5A, kCopy },
	{ 0x5B, (void *)0x80075000 },
	{ 0x54, _kExitTLBHandler },
	{ 0x55, 0 },                    /* PutTLBEntry */
	{ 0x56, 0 },                    /* SetTLBEntry */
	{ 0x57, 0 },                    /* GetTLBEntry */
	{ 0x58, 0 },                    /* ProbeTLBEntry */
	{ 0x59, 0 },                    /* ExpandScratchPad */
};

static struct TLBEntry kernelTLB[13] = {
	{ 0x00000000, 0x70000000, 0x80000007, 0x00000007 },
	{ 0x00006000, 0xFFFF8000, 0x00001E1F, 0x00001F1F },
	{ 0x00000000, 0x10000000, 0x00400017, 0x00400053 },
	{ 0x00000000, 0x10002000, 0x00400097, 0x004000D7 },
	{ 0x00000000, 0x10004000, 0x00400117, 0x00400157 },
	{ 0x00000000, 0x10006000, 0x00400197, 0x004001D7 },
	{ 0x00000000, 0x10008000, 0x00400217, 0x00400257 },
	{ 0x00000000, 0x1000A000, 0x00400297, 0x004002D7 },
	{ 0x00000000, 0x1000C000, 0x00400313, 0x00400357 },
	{ 0x00000000, 0x1000E000, 0x00400397, 0x004003D7 },
	{ 0x0001E000, 0x11000000, 0x00440017, 0x00440415 },
	{ 0x0001E000, 0x12000000, 0x00480017, 0x00480415 },
	{ 0x01FFE000, 0x1E000000, 0x00780017, 0x007C0017 },
};

static struct TLBEntry defaultTLB[18] = {
	{ 0x0007E000, 0x00080000, 0x0000201F, 0x0000301F },
	{ 0x0007E000, 0x00100000, 0x0000401F, 0x0000501F },
	{ 0x0007E000, 0x00180000, 0x0000601F, 0x0000701F },
	{ 0x001FE000, 0x00200000, 0x0000801F, 0x0000C01F },
	{ 0x001FE000, 0x00400000, 0x0001001F, 0x0001401F },
	{ 0x001FE000, 0x00600000, 0x0001801F, 0x0001C01F },
	{ 0x007FE000, 0x00800000, 0x0002001F, 0x0003001F },
	{ 0x007FE000, 0x01000000, 0x0004001F, 0x0005001F },
	{ 0x007FE000, 0x01800000, 0x0006001F, 0x0007001F },
	{ 0x0007E000, 0x20080000, 0x00002017, 0x00003017 },
	{ 0x0007E000, 0x20100000, 0x00004017, 0x00005017 },
	{ 0x0007E000, 0x20180000, 0x00006017, 0x00007017 },
	{ 0x001FE000, 0x20200000, 0x00008017, 0x0000C017 },
	{ 0x001FE000, 0x20400000, 0x00010017, 0x00014017 },
	{ 0x001FE000, 0x20600000, 0x00018017, 0x0001C017 },
	{ 0x007FE000, 0x20800000, 0x00020017, 0x00030017 },
	{ 0x007FE000, 0x21000000, 0x00040017, 0x00050017 },
	{ 0x007FE000, 0x21800000, 0x00060017, 0x00070017 },
};

static struct TLBEntry extendTLB[8] = {
	{ 0x0007E000, 0x30100000, 0x0000403F, 0x0000503F },
	{ 0x0007E000, 0x30180000, 0x0000603F, 0x0000703F },
	{ 0x001FE000, 0x30200000, 0x0000803F, 0x0000C03F },
	{ 0x001FE000, 0x30400000, 0x0001003F, 0x0001403F },
	{ 0x001FE000, 0x30600000, 0x0001803F, 0x0001C03F },
	{ 0x007FE000, 0x30800000, 0x0002003F, 0x0003003F },
	{ 0x007FE000, 0x31000000, 0x0004003F, 0x0005003F },
	{ 0x007FE000, 0x31800000, 0x0006003F, 0x0007003F },
};

struct TLBInfo TLBInfo = { 13, 18, 8, 0, kernelTLB, defaultTLB, extendTLB };

void *SetTLBHandler(void *handler)
{
	_kTLBRefillHandler = handler;
	SetVTLBRefillHandler(1, _kTLBException);
	SetVTLBRefillHandler(2, _kTLBException);
	SetVTLBRefillHandler(3, _kTLBException);
	return handler;
}

void *SetDebugHandler(int cause, void *handler)
{
	void *old;

	if (cause < 1 || cause > 13)
		return (void *)0xffffffff;
	old = _kDebugHandler[cause];
	_kDebugHandler[cause] = handler;
	if (cause >= 1 && cause <= 3)
		SetVTLBRefillHandler(cause, _kDebugException);
	else
		SetVCommonHandler(cause, _kDebugException);
	return old;
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

static void setup(int syscall, void *function)
{
	asm volatile ("li $3, 0x74\n\tsyscall");
}

void InitTLBFunctions(void)
{
	u_int i;

	setup(SysEntry[0].syscall, SysEntry[0].function);
	Copy((void *)0x80075000, tlbsrc, sizeof(tlbsrc));
	FlushCache(0);
	FlushCache(2);
	setup(SysEntry[1].syscall, SysEntry[1].function);
	setup(SysEntry[2].syscall, SysEntry[2].function);
	for (i = 3; i < 8; i++)
		setup(SysEntry[i].syscall, GetEntryAddress(SysEntry[i].syscall));
	_kExecArg = GetEntryAddress(3);
}

int PutTLBEntry(u_int PageMask, u_int EntryHi, u_int EntryLo0, u_int EntryLo1)
{
	asm volatile ("li $3, 0x55\n\tsyscall");
}

int iPutTLBEntry(u_int PageMask, u_int EntryHi, u_int EntryLo0, u_int EntryLo1)
{
	asm volatile ("li $3, -0x55\n\tsyscall");
}

static int _SetTLBEntry(u_int index, u_int PageMask, u_int EntryHi, u_int EntryLo0, u_int EntryLo1)
{
	asm volatile ("li $3, 0x56\n\tsyscall");
}

int SetTLBEntry(u_int index, u_int PageMask, u_int EntryHi, u_int EntryLo0, u_int EntryLo1)
{
	if (index < 13 || index > 47)
		return -1;
	return _SetTLBEntry(index, PageMask, EntryHi, EntryLo0, EntryLo1);
}

int iSetTLBEntry(u_int index, u_int PageMask, u_int EntryHi, u_int EntryLo0, u_int EntryLo1)
{
	asm volatile ("li $3, -0x56\n\tsyscall");
}

int GetTLBEntry(u_int index, u_int *PageMask, u_int *EntryHi, u_int *EntryLo0, u_int *EntryLo1)
{
	asm volatile ("li $3, 0x57\n\tsyscall");
}

int iGetTLBEntry(u_int index, u_int *PageMask, u_int *EntryHi, u_int *EntryLo0, u_int *EntryLo1)
{
	asm volatile ("li $3, -0x57\n\tsyscall");
}

int ProbeTLBEntry(u_int EntryHi, u_int *PageMask, u_int *EntryLo0, u_int *EntryLo1)
{
	asm volatile ("li $3, 0x58\n\tsyscall");
}

int iProbeTLBEntry(u_int EntryHi, u_int *PageMask, u_int *EntryLo0, u_int *EntryLo1)
{
	asm volatile ("li $3, -0x58\n\tsyscall");
}

int ExpandScratchPad(u_int page)
{
	asm volatile ("li $3, 0x59\n\tsyscall");
}

int InitTLB(void)
{
	if (GetMemorySize() == 0x2000000)
		return InitTLB32MB();
	return _InitTLB();
}

/* The index lives in t9 across the _SetTLBEntry calls, saved nowhere -- a register variable
 * (`register int i asm ("$25")`: GCC gives a pseudo live across a call a saved register) -- and the
 * function returns it shifted (`sll t9,t9,13`, in the exit test's delay slot): `i <<= 13` here.
 * `i = 0` above the first overflow test (the target tests `i < end` after Exit as a real compare),
 * the bounds cached, `i + count`. The last loop is rotated by hand (GCC round 75; OPEN since round
 * 4a, 5 words): its first EntryHi is computed inside the guard, after the exit test -- a `for`
 * computes its init before its own entry test (`lui; sll; slti; beqz; addu`), which rounds 4a-74
 * read as loop.c's giv placement. */
static int InitTLB32MB(void)
{
	register int i asm ("$25");
	int n, end;
	u_int value;
	struct TLBEntry *e;

	kprintf("# TLB spad=0 kernel=1:%d default=%d:%d extended=%d:%d\n",
	        TLBInfo.NumKernelTLBEntries - 1, TLBInfo.NumKernelTLBEntries,
	        TLBInfo.NumKernelTLBEntries + TLBInfo.NumDefaultTLBEntries - 1,
	        TLBInfo.NumKernelTLBEntries + TLBInfo.NumDefaultTLBEntries,
	        TLBInfo.NumKernelTLBEntries + TLBInfo.NumDefaultTLBEntries + TLBInfo.NumExtendedTLBEntries - 1);
	asm volatile ("mtc0 $0, $6\n\tsync.p");
	i = 0;
	end = TLBInfo.NumKernelTLBEntries;
	if (end > 48) {
		kprintf("# TLB over flow (1)");
		Exit(1);
	}
	for (e = TLBInfo.kernelTLB; i < end; i++, e++)
		_SetTLBEntry(i, e->PageMask, e->EntryHi, e->EntryLo0, e->EntryLo1);
	end = i + TLBInfo.NumDefaultTLBEntries;
	if (end > 48) {
		kprintf("# TLB over flow (2)");
		Exit(1);
	}
	for (e = TLBInfo.defaultTLB; i < end; i++, e++)
		_SetTLBEntry(i, e->PageMask, e->EntryHi, e->EntryLo0, e->EntryLo1);
	TLBInfo.NumWiredEntries = i;
	asm volatile ("mtc0 %0, $6\n\tsync.p" : : "r" (i));
	if (TLBInfo.NumExtendedTLBEntries > 0) {
		end = i + TLBInfo.NumExtendedTLBEntries;
		if (end > 48) {
			kprintf("# TLB over flow (3)");
			Exit(1);
		}
		for (e = TLBInfo.extendedTLB; i < end; i++, e++)
			_SetTLBEntry(i, e->PageMask, e->EntryHi, e->EntryLo0, e->EntryLo1);
	}
	n = i;
	i <<= 13;
	if (n < 48) {
		value = 0xE0000000 + i;
		do {
			_SetTLBEntry(n, 0, value, 0, 0);
			n++;
			value += 0x2000;
		} while (n < 48);
	}
	return i;
}
