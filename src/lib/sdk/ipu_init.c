/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* Sony's libipu, its initialisation object (MAIN 0x1A3448-0x1A36E4): the demo's object 0x15ADA8 (C),
 * its two functions in the same order -- its own static setD4_CHCR (this one always calls EIntr,
 * the demo's shape; `setD4_CHCR@ipu_init` in the map) and sceIpuInit: channel 4 stopped, the IPU
 * reset and its bit pointer cleared, the default intra and non-intra quantiser matrices fed
 * through the input FIFO (iqval: MPEG's default intra matrix in zigzag order, then a row of flat
 * 16s sent four times) and set (SETIQ 0x50000000, 0x58000000), the VQ CLUT (vqval, sixteen RGB555
 * colours, SETVQ) and the threshold (SETTH), then reset and cleared again, each command waited
 * on. Each FIFO write reads its quadword afresh (`lq v1,64(a1)` four times in the target: through
 * a volatile quadword -- a plain read is CSE'd into one load, 0x228 against 0x238). Names: SOCOM
 * 1's (s1:exact; iqval and vqval the demo's LOCAL labels, data.txt's rows). */

#include <eetypes.h>	/* u_char ... u_long, and the one 128-bit type */

#define IPU_CMD		((volatile u_int *)0x10002000)
#define IPU_CTRL	((volatile u_int *)0x10002010)
#define IPU_in_FIFO	((volatile u_long128 *)0x10007010)
#define D4_CHCR		((volatile u_int *)0x1000B400)
#define D_ENABLER	((volatile u_int *)0x1000F520)
#define D_ENABLEW	((volatile u_int *)0x1000F590)

#define FIFO_PUT(p)	(*IPU_in_FIFO = *(volatile u_long128 *)(p))

extern int DIntr(void);
extern int EIntr(void);

static u_char iqval[80] __attribute__((aligned(16))) = {
	8, 16, 16, 19, 16, 19, 22, 22, 22, 22, 22, 22, 26, 24, 26, 27,
	27, 27, 26, 26, 26, 26, 27, 27, 27, 29, 29, 29, 34, 34, 34, 29,
	29, 29, 27, 27, 29, 29, 32, 32, 34, 34, 37, 38, 37, 35, 35, 34,
	35, 38, 38, 40, 40, 40, 48, 48, 46, 46, 56, 56, 58, 69, 69, 83,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
};

static u_short vqval[16] __attribute__((aligned(16))) = {
	0x0000, 0x0421, 0x0842, 0x03e0, 0x1084, 0x14a5, 0x18c6, 0x1ce7,
	0x001f, 0x2529, 0x294a, 0x7c00, 0x318c, 0x35ad, 0x7fff, 0x39ce,
};

static void setD4_CHCR(u_int val)
{
	DIntr();
	*D_ENABLEW = *D_ENABLER | 0x10000;
	*D4_CHCR = val;
	*D_ENABLEW = *D_ENABLER & ~0x10000;
	EIntr();
}

void sceIpuInit(void)
{
	setD4_CHCR(1);
	*IPU_CTRL = 0x40000000;
	while ((int)*IPU_CTRL < 0)
		;
	*IPU_CMD = 0;
	while ((int)*IPU_CTRL < 0)
		;
	FIFO_PUT(&iqval[0]);
	FIFO_PUT(&iqval[16]);
	FIFO_PUT(&iqval[32]);
	FIFO_PUT(&iqval[48]);
	FIFO_PUT(&iqval[64]);
	FIFO_PUT(&iqval[64]);
	FIFO_PUT(&iqval[64]);
	FIFO_PUT(&iqval[64]);
	*IPU_CMD = 0x50000000;
	while ((int)*IPU_CTRL < 0)
		;
	*IPU_CMD = 0x58000000;
	while ((int)*IPU_CTRL < 0)
		;
	FIFO_PUT(&vqval[0]);
	FIFO_PUT(&vqval[8]);
	*IPU_CMD = 0x60000000;
	while ((int)*IPU_CTRL < 0)
		;
	*IPU_CMD = 0x90000000;
	while ((int)*IPU_CTRL < 0)
		;
	*IPU_CTRL = 0x40000000;
	while ((int)*IPU_CTRL < 0)
		;
	*IPU_CMD = 0;
	while ((int)*IPU_CTRL < 0)
		;
}
