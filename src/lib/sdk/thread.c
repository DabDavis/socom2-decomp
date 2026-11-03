/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's thread helpers (MAIN 0x1A4820-0x1A4B80): Sony's SDK, the demo's object 0x15C220 (C),
 * in its order. The kernel's interrupt-side calls cannot act on the running thread (or, for the
 * ready queue, change it): these queue the request in topArg's ring for topThread, a thread of
 * priority 0 woken by topSema, which makes the plain call. Each returns the id it queued (the
 * priority, for the rotation), -1 when the helper thread is not running or the id is out of range.
 * Names are SOCOM 1's (s1:exact; topId, topSema, topArg the demo's globals). The helper thread's
 * stack is InitThread's own static (the demo's label stack.6; this compile numbers it stack.9, so
 * its reference stays unchecked); InitThread declares its thread parameters before the semaphore's
 * (their frame slots). Sony's calls differ from ps2sdk's reconstruction: they return the id, test
 * it unsigned, and topThread's default reports "## internel error in libkernl.a!". */

typedef unsigned int u_int;
typedef unsigned char u_char;

struct SemaParam {
	int currentCount;
	int maxCount;
	int initCount;
	int numWaitThreads;
	u_int attr;
	u_int option;
};

struct ThreadParam {
	int status;
	void *entry;
	void *stack;
	int stackSize;
	void *gpReg;
	int initPriority;
	int currentPriority;
	u_int attr;
	u_int option;
	int waitType;
	int waitId;
	int wakeupCount;
};

struct topArg {
	int requestOut;
	int requestIn;
	struct {
		u_char mode;		/* 0 wake up, 1 rotate the ready queue, 2 suspend */
		u_char data;
	} request[512];
};

extern int CreateSema(struct SemaParam *);
extern int DeleteSema(int);
extern int WaitSema(int);
extern int iSignalSema(int);
extern int CreateThread(struct ThreadParam *);
extern int StartThread(int, void *);
extern int GetThreadId(void);
extern int ChangeThreadPriority(int, int);
extern int WakeupThread(int);
extern int RotateThreadReadyQueue(int);
extern int SuspendThread(int);
extern int _iWakeupThread(int);
extern int _iSuspendThread(int);
extern int kprintf(const char *, ...);

extern char _gp[];
extern int topId;
extern int topSema;
extern struct topArg topArg;

/* the running thread's id, from an interrupt handler (syscall -47) */
static __inline__ int _iGetThreadId(void)
{
	register int id asm ("$2");

	__asm__ __volatile__ ("li $3, -47\n\tsyscall" : "=r" (id) : : "$3");
	return id;
}

static void topThread(struct topArg *arg)
{
	int i;

	for (;;) {
		WaitSema(topSema);
		i = arg->requestOut & 0x1FF;
		arg->requestOut = i + 1;
		switch (arg->request[i].mode) {
		case 0:
			WakeupThread(arg->request[i].data);
			break;
		case 1:
			RotateThreadReadyQueue(arg->request[i].data);
			break;
		case 2:
			SuspendThread(arg->request[i].data);
			break;
		default:
			kprintf("## internel error in libkernl.a!\n");
			break;
		}
	}
}

int InitThread(void)
{
	static u_char stack[0x400] __attribute__((aligned(16)));
	struct ThreadParam thread;
	struct SemaParam sema;

	if (topId > 0)
		return -1;
	sema.initCount = 0;
	sema.maxCount = 255;
	if ((topSema = CreateSema(&sema)) < 0)
		return -1;
	thread.entry = topThread;
	thread.stack = stack;
	thread.stackSize = sizeof(stack);
	thread.gpReg = _gp;
	thread.initPriority = 0;
	if ((topId = CreateThread(&thread)) < 0) {
		DeleteSema(topSema);
		return -1;
	}
	topArg.requestOut = 0;
	topArg.requestIn = 0;
	StartThread(topId, &topArg);
	ChangeThreadPriority(GetThreadId(), 1);
	return topId;
}

int iWakeupThread(int tid)
{
	int i;

	if (_iGetThreadId() != tid)
		return _iWakeupThread(tid);
	if ((u_int)tid >= 256 || topId == 0)
		return -1;
	i = topArg.requestIn & 0x1FF;
	topArg.requestIn = i + 1;
	topArg.request[i].mode = 0;
	topArg.request[i].data = tid;
	iSignalSema(topSema);
	return tid;
}

int iRotateThreadReadyQueue(int prio)
{
	int i;

	if ((u_int)prio >= 128 || topId == 0)
		return -1;
	i = topArg.requestIn & 0x1FF;
	topArg.requestIn = i + 1;
	topArg.request[i].mode = 1;
	topArg.request[i].data = prio;
	iSignalSema(topSema);
	return prio;
}

int iSuspendThread(int tid)
{
	int i;

	if (_iGetThreadId() != tid)
		return _iSuspendThread(tid);
	if ((u_int)tid >= 256 || topId == 0)
		return -1;
	i = topArg.requestIn & 0x1FF;
	topArg.requestIn = i + 1;
	topArg.request[i].mode = 2;
	topArg.request[i].data = tid;
	iSignalSema(topSema);
	return tid;
}
