/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libcdvd's first object (MAIN 0x18DB90-0x18F178): Sony's SDK, the demo's object 0x16E490 (C), its
 * functions in the demo's order -- the delay thread, the N-command callback and its callback
 * thread, the power-off callback, the search, the N- and S-command pre-checks, sync, init and
 * disk-ready. Prebuilt with the SDK's ee-gcc 2.9-991111-01, the dtls13010 build (README "The GCC
 * front": cdvd_exit's broken sibling call is that build's). Names are SOCOM 1's (s1:exact and the
 * demo's globals, data.txt); the callback number is read back after each store (volatile), the
 * semaphore ids and flags are plain. */

typedef unsigned int u_int;
typedef unsigned short u_short;

struct SemaParam {
	int currentCount;
	int maxCount;
	int initCount;
	int numWaitThreads;
	u_int attr;
	u_int option;
};

extern int CreateSema(struct SemaParam *sema);
extern int DeleteSema(int sema_id);
extern int WaitSema(int sema_id);
extern int iSignalSema(int sema_id);
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

extern int SetAlarm(u_short time, void (*handler)(int, u_short, void *), void *common);
extern int DIntr(void);
extern int EIntr(void);
extern int sceCdSync(int mode);
extern int scePrintf(const char *fmt, ...);
extern void ExitDeleteThread(void);
extern int GetThreadId(void);
extern int ReferThreadStatus(int thread_id, struct ThreadParam *info);
extern int CreateThread(struct ThreadParam *thread);
extern int StartThread(int thread_id, void *arg);
extern int ChangeThreadPriority(int thread_id, int priority);

extern volatile int sceCdCbfunc_num;            /* the demo's globals (data.txt) */
extern volatile int sceCdCbfunc_number;
extern volatile int cb_semid;
extern volatile int _sceCd_ncmd_semid;
extern volatile int _sceCd_c_cb_sem;
extern int data_001CA854;                              /* the callback thread is running */
extern void (*sceCdCbfunc)(int);
extern int data_001D5F84;                       /* the caller's $gp, for the callback thread */
extern int SCE_CD_debug;
extern int my_thid;
extern volatile int data_001D5F94;
extern struct ThreadParam data_001D5FA0;        /* the caller's status */
extern struct ThreadParam data_001D5FD0;        /* the callback thread's parameters */

static void CB_DelayTh(int id, u_short time, void *arg)
{
	iSignalSema((int)arg);
	__asm__ volatile ("sync.l; ei");
}

void sceCdDelayThread(u_short usec)
{
	struct SemaParam sema;
	int sid;

	sema.initCount = 0;
	sema.maxCount = 1;
	sema.option = 0;
	sid = CreateSema(&sema);
	SetAlarm(usec, CB_DelayTh, (void *)sid);
	WaitSema(sid);
	DeleteSema(sid);
}

void *sceCdCallback(void (*func)(int))
{
	void *old;
	int oldi;

	if (sceCdSync(1) != 0)
		return 0;
	oldi = DIntr();
	old = sceCdCbfunc;
	sceCdCbfunc = func;
	__asm__ volatile ("sw $28, %0" : "=m" (data_001D5F84));
	if (oldi)
		EIntr();
	return old;
}

void _sceCd_cd_callback(int *data)
{
	sceCdCbfunc_num = data[0];
	sceCdCbfunc_number = sceCdCbfunc_num;
	if (sceCdCbfunc_num == 11) {
		sceCdCbfunc_num = 0;
		_sceCd_c_cb_sem = 0;
		return;
	}
	iSignalSema(_sceCd_ncmd_semid);
	if (data_001CA854 != 0 && sceCdCbfunc != 0)
		iSignalSema(cb_semid);
	else
		_sceCd_c_cb_sem = 0;
	sceCdCbfunc_num = 0;
}

static void _Cdvd_cbLoop(void)
{
	int old_gp;

	for (;;) {
		WaitSema(cb_semid);
		if (sceCdCbfunc_num == -1) {
			_sceCd_c_cb_sem = 0;
			sceCdCbfunc_num = 0;
			data_001CA854 = 0;
			data_001D5F94 = 0;
			ExitDeleteThread();
		}
		if (SCE_CD_debug > 0)
			scePrintf("sceCdCbfunc= %d sceCdCbfunc_num= %d\n", sceCdCbfunc, sceCdCbfunc_number);
		if (sceCdCbfunc != 0 && sceCdCbfunc_number != 0) {
			__asm__ volatile ("move %0, $28" : "=r" (old_gp));
			__asm__ volatile ("lw $28, %0" : : "m" (data_001D5F84));
			sceCdCbfunc(sceCdCbfunc_number);
			__asm__ volatile ("move $28, %0" : : "r" (old_gp));
		}
		_sceCd_c_cb_sem = 0;
	}
}

int sceCdInitEeCB(int prio, void *stack, int stacksize)
{
	int ret = 1;

	if (data_001CA854 == 0) {
		my_thid = GetThreadId();
		ReferThreadStatus(my_thid, &data_001D5FA0);
		data_001D5FD0.stackSize = stacksize;
		data_001D5FD0.entry = _Cdvd_cbLoop;
		data_001D5FD0.stack = stack;
		data_001D5FD0.initPriority = prio;
		data_001D5FD0.gpReg = 0;
		data_001CA854 = CreateThread(&data_001D5FD0);
		StartThread(data_001CA854, 0);
	} else {
		ChangeThreadPriority(data_001CA854, prio);
		ret = 0;
	}
	return ret;
}
