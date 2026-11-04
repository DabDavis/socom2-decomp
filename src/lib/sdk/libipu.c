/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* Sony's libipu, its DMA object (MAIN 0x1A30B8-0x1A3444): the demo's object 0x15AA38 (C), its five
 * functions in the same order -- the statics setD3_CHCR and setD4_CHCR (a channel's CHCR written
 * with the DMA suspended: D_ENABLEW from D_ENABLER with bit 16 set, the write, the bit cleared,
 * all under DIntr), sceIpuStopDMA (both channels stopped and saved with the IPU's BP and CTRL, the
 * input FIFO drained first), sceIpuRestartDMA (the FIFO's unconsumed quadwords given back to
 * channel 4: BP's FP + IFC, the IPU's bit pointer restored by a BCLR) and sceIpuSync. SOCOM II's
 * setD3/setD4 re-enable interrupts only if DIntr found them on (0x74; the demo's, 0x64, always
 * call EIntr as ipu_init.c's copy still does). sceIpuRestartDMA takes channel 4's address before
 * its count (the other order swaps their a0/a1). Names: SOCOM 1's (s1:exact, s1:callee; the static
 * setD4_CHCR two objects define is `setD4_CHCR@libipu` in the map). */

typedef unsigned int u_int;
typedef unsigned short u_short;

typedef struct {
	u_int d4madr;
	u_int d4tadr;
	u_int d4qwc;
	u_int d4chcr;
	u_int d3madr;
	u_int d3qwc;
	u_int d3chcr;
	u_int ipubp;
	u_int ipuctrl;
} sceIpuDmaEnv;

#define IPU_CMD		((volatile u_int *)0x10002000)
#define IPU_CTRL	((volatile u_int *)0x10002010)
#define IPU_BP		((volatile u_int *)0x10002020)
#define D3_CHCR		((volatile u_int *)0x1000B000)
#define D3_MADR		((volatile u_int *)0x1000B010)
#define D3_QWC		((volatile u_int *)0x1000B020)
#define D4_CHCR		((volatile u_int *)0x1000B400)
#define D4_MADR		((volatile u_int *)0x1000B410)
#define D4_QWC		((volatile u_int *)0x1000B420)
#define D4_TADR		((volatile u_int *)0x1000B430)
#define D_ENABLER	((volatile u_int *)0x1000F520)
#define D_ENABLEW	((volatile u_int *)0x1000F590)

extern int DIntr(void);
extern int EIntr(void);

static void setD3_CHCR(u_int val)
{
	int ie;

	ie = DIntr();
	*D_ENABLEW = *D_ENABLER | 0x10000;
	*D3_CHCR = val;
	*D_ENABLEW = *D_ENABLER & ~0x10000;
	if (ie)
		EIntr();
}

static void setD4_CHCR(u_int val)
{
	int ie;

	ie = DIntr();
	*D_ENABLEW = *D_ENABLER | 0x10000;
	*D4_CHCR = val;
	*D_ENABLEW = *D_ENABLER & ~0x10000;
	if (ie)
		EIntr();
}

void sceIpuStopDMA(sceIpuDmaEnv *env)
{
	setD4_CHCR(1);
	env->d4madr = *D4_MADR;
	env->d4tadr = *D4_TADR;
	env->d4qwc = *D4_QWC;
	env->d4chcr = *D4_CHCR;
	while (*IPU_CTRL & 0xf0)
		;
	setD3_CHCR(0);
	env->d3madr = *D3_MADR;
	env->d3qwc = *D3_QWC;
	env->d3chcr = *D3_CHCR;
	env->ipubp = *IPU_BP;
	env->ipuctrl = *IPU_CTRL;
}

void sceIpuRestartDMA(sceIpuDmaEnv *env)
{
	u_int bp, n, qwc, madr;

	bp = env->ipubp & 0x7f;
	n = ((env->ipubp >> 16) & 3) + ((env->ipubp >> 8) & 0xf);
	madr = env->d4madr - n * 16;
	qwc = env->d4qwc + n;
	if (env->d3madr && env->d3qwc) {
		*D3_MADR = env->d3madr;
		*D3_QWC = env->d3qwc;
		setD3_CHCR(env->d3chcr | 0x100);
	}
	while ((int)*IPU_CTRL < 0)
		;
	*IPU_CMD = bp;
	while ((int)*IPU_CTRL < 0)
		;
	if (madr && qwc) {
		*D4_MADR = madr;
		*D4_TADR = env->d4tadr;
		*D4_QWC = qwc;
		setD4_CHCR(env->d4chcr | 0x100);
	}
}

int sceIpuSync(int mode, u_short timeout)
{
	int ret = 0;

	switch (mode) {
	case 0:
		while ((int)*IPU_CTRL < 0)
			;
		ret = 0;
		break;
	case 1:
		ret = *IPU_CTRL >> 31;
		break;
	}
	return ret;
}
