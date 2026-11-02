/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's start-up (MAIN 0x1AC8C0-0x1ACB24): Sony's SDK 2.7, the demo's object 0x1641C0 (C), its
 * nine functions in the same order. _InitSys, which crt0 calls, makes the two library semaphores and
 * finds the kernel's syscall table: it installs kFindAddress and kCopy in two syscall slots, then
 * searches kernel memory (syscall 0x83 runs kFindAddress there) for the two pointers it just stored,
 * until both entries agree on the table's base. Then the alarm, thread, ExecPS2 and TLB set-ups.
 * Names are SOCOM 1's (s1:exact; its statics by position, `Copy@initsys`: README "The GCC front"). */

typedef unsigned int u_int;

struct SemaParam {
	int currentCount;
	int maxCount;
	int initCount;
	int numWaitThreads;
	u_int attr;
	u_int option;
};

struct SyscallData {
	int syscall;
	void *function;
};

extern int CreateSema(struct SemaParam *sema);
extern void InitAlarm(void);
extern int InitThread(void);
extern void InitExecPS2(void);
extern void InitTLBFunctions(void);

static u_int *kFindAddress(u_int *p, u_int *end, u_int value);
static int kCopy(u_int *dest, u_int *src, u_int size);

static u_int *_SyscallEntry = 0;

static struct SyscallData SysEntry[] = {
	{ 0x83, kFindAddress },
	{ 0x5A, kCopy },
};

int __sce_sema_id = 0;
int __sce_eh_sema_id = 0;

static void supplement_crt0(void)
{
	struct SemaParam sema, eh_sema;

	sema.maxCount = 1;
	sema.initCount = 1;
	eh_sema.maxCount = 1;
	eh_sema.initCount = 1;
	__sce_sema_id = CreateSema(&sema);
	__sce_eh_sema_id = CreateSema(&eh_sema);
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

static u_int *kFindAddress(u_int *p, u_int *end, u_int value)
{
	while (*p != value && p < end)
		p++;
	if (p < end)
		return p;
	else
		return 0;
}

static u_int *FindAddress(u_int *start, u_int *end, u_int value)
{
	asm volatile ("li $3, 0x83\n\tsyscall");
}

void *GetSystemCallEntry(int syscall)
{
	void *entry;

	Copy(&entry, _SyscallEntry + syscall, 4);
	return entry;
}

static void setup(int syscall, void *function);

static void InitSystemCallTableAddress(void)
{
	u_int *a, *b, *ta, *tb;

	setup(SysEntry[0].syscall, SysEntry[0].function);
	setup(SysEntry[1].syscall, SysEntry[1].function);
	a = FindAddress((u_int *)0x80000000, (u_int *)0x80080000, (u_int)kFindAddress);
	b = FindAddress((u_int *)0x80000000, (u_int *)0x80080000, (u_int)kCopy);
	ta = a - 0x83;
	tb = b - 0x5A;
	while (ta != tb) {
		if (ta < tb) {
			a = FindAddress(a + 1, (u_int *)0x80080000, (u_int)kFindAddress);
			ta = a - 0x83;
		} else {
			b = FindAddress(b + 1, (u_int *)0x80080000, (u_int)kCopy);
			tb = b - 0x5A;
		}
	}
	_SyscallEntry = ta;
}

static void setup(int syscall, void *function)
{
	asm volatile ("li $3, 0x74\n\tsyscall");
}

void _InitSys(void)
{
	supplement_crt0();
	InitSystemCallTableAddress();
	InitAlarm();
	InitThread();
	InitExecPS2();
	InitTLBFunctions();
}
