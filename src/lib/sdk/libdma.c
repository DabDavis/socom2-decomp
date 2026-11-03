/* sc: compiler=ee-gcc2.9-991111; flags=-O2 -G0 */
/* libdma (PsIIlibdma 2700): Sony's SDK 2.7 DMA library, MAIN 0x190BC8-0x191410, prebuilt with
 * ee-gcc 2.9-ee-991111 at -O2 -G0 (README "The GCC front": memclr's short-loop nops are that
 * assembler's, and 2.96's register choices miss sceDmaGetChan). The demo's object 0x14FBE0 (C),
 * its nineteen functions in the same order (the first four static). Names are SOCOM 1's
 * (s1:exact). The DMAC's registers are volatile u_ints; a channel's own registers are its
 * sceDmaChan, not volatile, but for the waits' re-reads and sceDmaPause's.
 *
 * What the object says about the source: a transfer sets CHCR's fields one by one (tD_CHCR's
 * bitfields: MOD, DIR, STR -- the compiler merges the three into one load and store, its masks
 * signed, `li -13`), where an explicit `& ~0x100` on a u_int is a zero-extended 0xfffffeff (lui/
 * ori) -- sceDmaPause clears STR so; the madr/tadr guard compares with an unsigned 0xffffffff;
 * D_STAT's acknowledge stores its second value through a plain pointer (it fills memclr's delay
 * slot, which a volatile store may not); sceDmaReset's six stores in the one order that schedules
 * as the target's (tools/ccperm.py, one of 720). */

typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned char u_char;

typedef struct {
	unsigned DIR: 1;	/* direction */
	unsigned p0: 1;
	unsigned MOD: 2;	/* mode */
	unsigned ASP: 2;	/* address stack pointer */
	unsigned TTE: 1;	/* tag transfer enable */
	unsigned TIE: 1;	/* tag interrupt enable */
	unsigned STR: 1;	/* start */
	unsigned p1: 7;
	unsigned TAG: 16;
} tD_CHCR;

typedef struct {
	tD_CHCR chcr;	u_int p0[3];		/* channel control */
	void *madr;	u_int p1[3];		/* memory address */
	u_int qwc;	u_int p2[3];		/* transfer count */
	void *tadr;	u_int p3[3];		/* tag address */
	void *as0;	u_int p4[3];		/* address stack */
	void *as1;	u_int p5[3];
	u_int p6[4];
	u_int p7[4];
	void *sadr;	u_int p8[3];		/* scratchpad address */
} sceDmaChan;

typedef struct {
	u_char sts;		/* stall source */
	u_char std;		/* stall drain */
	u_char mfd;		/* MFIFO drain */
	u_char rcycle;		/* release cycle */
	u_short express;	/* express channel */
	u_short notify;		/* notify channel */
	u_short sqwc;		/* tag skip quad word count */
	u_short tqwc;		/* tag transfer quad word count */
	void *rbadr;		/* ring buffer address */
	u_int rbmsk;		/* ring buffer mask */
} sceDmaEnv;

#define D_CTRL		((volatile u_int *)0x1000E000)
#define D_STAT		((volatile u_int *)0x1000E010)
#define D_PCR		((volatile u_int *)0x1000E020)
#define D_SQWC		((volatile u_int *)0x1000E030)
#define D_RBSR		((volatile u_int *)0x1000E040)
#define D_RBOR		((volatile u_int *)0x1000E050)
#define D_STADR		((u_int *)0x1000E060)	/* not volatile: the store fills the return's delay slot */
#define D_ENABLER	((volatile u_int *)0x1000F520)
#define D_ENABLEW	((volatile u_int *)0x1000F590)

extern int scePrintf(const char *fmt, ...);
extern int DIntr(void);
extern int EIntr(void);
u_int sceDmaPause(sceDmaChan *d);
int sceDmaPutEnv(sceDmaEnv *env);

extern sceDmaChan *dch[10];		/* the ten channels' register blocks (data.txt) */
extern int isclr[10];			/* which channels sceDmaReset clears */
extern u_char data_001D0870[10];	/* sts -> D_CTRL.STS */
extern u_char stdtbl[10];		/* std -> D_CTRL.STD */
extern u_char mfdtbl[10];		/* mfd -> D_CTRL.MFD */
extern sceDmaEnv sceDmaCurrentEnv;

static void WaitDma(sceDmaChan *d)
{
	int i = 0x1000000;

	while (*(volatile u_int *)&d->chcr & 0x100) {
		if (--i < 0) {
			scePrintf("libdma: sync timeout\n");
			sceDmaPause(d);
		}
	}
}

static void WatchDma(sceDmaChan *d, void *addr, int n)
{
	int i = n ? n : 0x1000000;

	while (*(volatile u_int *)&d->madr < (u_int)addr) {
		if (--i < 0) {
			scePrintf("libdma: sync timeout\n");
			sceDmaPause(d);
		}
	}
}

void *CheckAddress(void *addr)
{
	u_int a = (u_int)addr;

	if ((a >> 28) == 7) {               /* the scratchpad: a DMA tag's SPR bit */
		a &= 0x0FFFFFFF;                /* two statements: one expression computes in v0 */
		a |= 0x80000000;
	}
	return (void *)a;
}

void memclr(void *p, int size)
{
	char *q = p;
	while (size--)
		*q++ = 0;
}

sceDmaChan *sceDmaGetChan(int id)
{
	return ((u_int)id < 10) ? dch[id] : 0;
}

int sceDmaReset(int mode)
{
	sceDmaEnv env;
	int i;
	int old = *D_CTRL & 1;

	for (i = 0; i < 10; i++) {
		if (isclr[i]) {
			sceDmaChan *d = dch[i];
			*(u_int *)&d->chcr = 0;
			d->tadr = 0;
			d->madr = 0;
			d->as1 = 0;
			d->as0 = 0;
			d->sadr = 0;
		}
	}
	*D_STAT = 0xff1f;
	*(u_int *)D_STAT = *D_STAT & 0xff1f0000;
	memclr(&env, sizeof(env));
	sceDmaPutEnv(&env);
	if (mode == 1)
		*D_CTRL |= 1;
	return old;
}

int sceDmaPutEnv(sceDmaEnv *env)
{
	u_int ctrl = *D_CTRL;
	u_int pcr = *D_PCR;
	u_int sqwc = *D_SQWC;
	u_int rbor = *D_RBOR;
	u_int rbsr = *D_RBSR;

	if (env->sts >= 10)
		return -1;
	if (env->std >= 10)
		return -2;
	if (env->mfd >= 10)
		return -3;
	if (env->rcycle >= 7)
		return -4;
	ctrl = (ctrl & ~0x30) | (data_001D0870[env->sts] << 4);
	ctrl = (ctrl & ~0xc0) | (stdtbl[env->std] << 6);
	ctrl = (ctrl & ~0x0c) | (mfdtbl[env->mfd] << 2);
	if (env->rcycle) {
		ctrl |= 2;
		ctrl = (ctrl & ~0x300) | ((env->rcycle - 1) << 8);
	} else
		ctrl &= ~2;
	pcr = (env->express << 16) | env->notify;
	sqwc = (env->tqwc << 16) | env->sqwc;
	rbor = (u_int)env->rbadr;
	rbsr = env->rbmsk;
	*D_CTRL = ctrl;
	*D_PCR = pcr;
	*D_SQWC = sqwc;
	*D_RBOR = rbor;
	*D_RBSR = rbsr;
	sceDmaCurrentEnv = *env;
	return 0;
}

sceDmaEnv *sceDmaGetEnv(sceDmaEnv *env)
{
	*env = sceDmaCurrentEnv;
	return env;
}

void *sceDmaPutStallAddr(void *addr)
{
	void *old = (void *)*D_STADR;
	*D_STADR = (u_int)addr;
	return old;
}

void sceDmaSend(sceDmaChan *d, void *tag)
{
	tag = CheckAddress(tag);
	WaitDma(d);
	if ((u_int)d->tadr != 0xffffffff)
		d->tadr = tag;
	d->qwc = 0;
	d->chcr.MOD = 1;
	d->chcr.DIR = 1;
	d->chcr.STR = 1;
}

void sceDmaSendN(sceDmaChan *d, void *addr, int size)
{
	addr = CheckAddress(addr);
	WaitDma(d);
	if ((u_int)d->madr != 0xffffffff)
		d->madr = addr;
	d->qwc = size;
	d->chcr.MOD = 0;
	d->chcr.DIR = 1;
	d->chcr.STR = 1;
}

void sceDmaSendI(sceDmaChan *d, void *addr, int size)
{
	addr = CheckAddress(addr);
	WaitDma(d);
	if ((u_int)d->madr != 0xffffffff)
		d->madr = addr;
	d->qwc = size;
	d->chcr.MOD = 2;
	d->chcr.DIR = 1;
	d->chcr.STR = 1;
}

void sceDmaRecv(sceDmaChan *d)
{
	WaitDma(d);
	d->qwc = 0;
	d->chcr.MOD = 1;
	d->chcr.DIR = 0;
	d->chcr.STR = 1;
}

void sceDmaRecvN(sceDmaChan *d, void *addr, int size)
{
	addr = CheckAddress(addr);
	WaitDma(d);
	if ((u_int)d->madr != 0xffffffff)
		d->madr = addr;
	d->qwc = size;
	d->chcr.MOD = 0;
	d->chcr.DIR = 0;
	d->chcr.STR = 1;
}

void sceDmaRecvI(sceDmaChan *d, void *addr, int size)
{
	addr = CheckAddress(addr);
	WaitDma(d);
	if ((u_int)d->madr != 0xffffffff)
		d->madr = addr;
	d->qwc = size;
	d->chcr.MOD = 2;
	d->chcr.DIR = 0;
	d->chcr.STR = 1;
}

int sceDmaSync(sceDmaChan *d, int mode, int timeout)
{
	if (mode == 1)
		return d->chcr.STR;
	WaitDma(d);
	return 0;
}

int sceDmaWatch(sceDmaChan *d, void *addr, int mode, int timeout)
{
	if (mode == 1)
		return (u_int)d->madr < (u_int)addr;
	WatchDma(d, addr, timeout);
	return 0;
}

u_int sceDmaPause(sceDmaChan *d)
{
	int eie = DIntr();
	u_int enabler = *D_ENABLER;
	u_int chcr;

	if (!(enabler & 0x10000))
		*D_ENABLEW = enabler | 0x10000;
	*D_CTRL;
	chcr = *(volatile u_int *)&d->chcr;
	*(u_int *)&d->chcr = chcr & ~0x100;
	*D_ENABLEW = enabler;
	if (eie)
		EIntr();
	return chcr;
}

int sceDmaRestart(sceDmaChan *d, u_int chcr)
{
	u_int old = *(volatile u_int *)d;
	*(volatile u_int *)d = chcr;
	return (old >> 8) & 1;
}
