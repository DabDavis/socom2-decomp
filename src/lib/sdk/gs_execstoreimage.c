/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libgraph's sceGsExecStoreImage (MAIN 0x1A29C8): the demo's object 0x14F550 (C), one function.
 * Reads a rectangle of GS local memory back to the host: sends the sceGsSetDefStoreImage packet
 * through DMA channel 1 (VIF1), waits for the GS's FINISH, turns the bus round (BUSDIR) and takes
 * the image from VIF1's FIFO -- by DMA in whole eights of quadwords, the rest of the quadwords by
 * hand, then the partial quadword's bytes and the tail of a rectangle rounded up to the transfer's
 * own block. Each wait has its own failure block, all moved below the transfer's SPR arm by
 * loop.c (find_and_verify_loops: a block ending in a jump out of a loop goes to the last barrier
 * before the jump's target), cross-jumped into the target's shared tails. `if (rem == 0)` first in
 * every arm gives `beqz` to one shared tail and the last arm's `bnez` over it; the size in a
 * variable of its own (`total`) is global-alloc's, so the product's pseudo is not tied to it (`mult
 * v1; sra v0,v1,2`), and qwc8 and qwcr declared in that order with qwcr assigned first set their
 * saved registers by live length. The local is an array or a union: `addu v0,sp,a1` (the pointer
 * cast's `addu v0,a1,sp` is the one word against). Name: SOCOM 1's (s1:exact). */
#include "libgraph.h"

#define	D1_CHCR		((volatile u_int *)0x10009000)
#define	D1_MADR		((volatile u_int *)0x10009010)
#define	D1_QWC		((volatile u_int *)0x10009020)
#define	GIF_CTRL	((volatile u_int *)0x10003000)
#define	VIF1_STAT	((volatile u_int *)0x10003C00)
#define	VIF1_FBRST	((volatile u_int *)0x10003C10)
#define	VIF1_FIFO	((volatile u_long128 *)0x10005000)
#define	GS_BUSDIR	((volatile u_long *)0x12001040)

typedef struct { u_long SBP:14; u_long pad14:2; u_long SBW:6; u_long pad22:2; u_long SPSM:6;
		 u_long pad30:34; } sceGsBitbltbuf;
typedef struct { u_long RRW:12; u_long pad12:20; u_long RRH:12; u_long pad44:20; } sceGsTrxreg;

extern u_long GsGetIMR(void);
extern u_long GsPutIMR(u_long);

/* a scratchpad address goes to the DMAC with SPR (bit 31) set: one store in each arm */
#define	SET_MADR(a) \
		if (((u_int)(a) & 0x70000000) == 0x70000000) \
			*D1_MADR = ((u_int)(a) & 0x0fffffff) | 0x80000000; \
		else \
			*D1_MADR = (u_int)(a) & 0x0fffffff

#define	WAIT_VIF1 \
		while (!(*VIF1_STAT & 0x1f000000)) { \
			if (count++ > 0x1000000) { \
				scePrintf("sceGsExecStoreImage: Enough data does not reach VIF1\n"); \
				RESET_PATH; \
				return -1; \
			} \
		}

#define	RESET_PATH \
		*GS_CSR = 0x100; \
		*GS_BUSDIR = 0; \
		*GIF_CTRL = 1; \
		*VIF1_FBRST = 1

int sceGsExecStoreImage(sceGsStoreImage *sp, u_long128 *dstaddr)
{
	static u_int init_mp3[4] __attribute__((aligned(16))) = { 0x06000000, 0, 0, 0 };
	union { u_long128 q; u_char c[16]; } buf;
	u_long imr;
	int i;
	int extra = 0, rem = 0, qwc8 = 0, qwcr = 0;
	u_int count = 0;
	int w = ((sceGsTrxreg *)&sp->trxreg)->RRW;
	int h = ((sceGsTrxreg *)&sp->trxreg)->RRH;
	int h2 = 0;
	int size;
	int total;

	switch (((sceGsBitbltbuf *)&sp->bitbltbuf)->SPSM) {
	case SCE_GS_PSMCT32:
	case SCE_GS_PSMZ32:
		size = w * h * 4;
		rem = size & 15;
		qwcr = (size >> 4) & 7;
		qwc8 = (size >> 4) & 0xfffffff8;
		if (rem == 0) {
			h2 = h;
			extra = 0;
		} else {
			h2 = (h + 3) & 0x1ffc;
			total = (w * h2) >> 2;
			extra = total - qwc8 - qwcr - 1;
		}
		break;
	case SCE_GS_PSMCT24:
	case SCE_GS_PSMZ24:
		size = w * h * 3;
		rem = size & 15;
		qwcr = (size >> 4) & 7;
		qwc8 = (size >> 4) & 0xfffffff8;
		if (rem == 0) {
			h2 = h;
			extra = 0;
		} else {
			h2 = (h + 15) & 0x1ff0;
			total = (w * h2 * 3) >> 4;
			extra = total - qwc8 - qwcr - 1;
		}
		break;
	case SCE_GS_PSMCT16:
	case SCE_GS_PSMCT16S:
	case SCE_GS_PSMZ16:
	case SCE_GS_PSMZ16S:
		size = w * h * 2;
		rem = size & 15;
		qwcr = (size >> 4) & 7;
		qwc8 = (size >> 4) & 0xfffffff8;
		if (rem == 0) {
			h2 = h;
			extra = 0;
		} else {
			h2 = (h + 7) & 0xfffffff8;
			total = (w * h2) >> 3;
			extra = total - qwc8 - qwcr - 1;
		}
		break;
	case SCE_GS_PSMT8:
	case SCE_GS_PSMT8H:
		size = w * h;
		rem = size & 15;
		qwcr = (size >> 4) & 7;
		qwc8 = (size >> 4) & 0xfffffff8;
		if (rem == 0) {
			h2 = h;
			extra = 0;
		} else {
			h2 = (h + 7) & 0xfffffff8;
			total = (w * h2) >> 4;
			extra = total - qwc8 - qwcr - 1;
		}
		break;
	case SCE_GS_PSMT4:
	case SCE_GS_PSMT4HL:
	case SCE_GS_PSMT4HH:
		size = (w * h) >> 1;
		rem = size & 15;
		qwcr = (size >> 4) & 7;
		qwc8 = (size >> 4) & 0xfffffff8;
		if (rem == 0) {
			h2 = h;
			extra = 0;
		} else {
			h2 = (h + 7) & 0xfffffff8;
			total = (w * h2) >> 5;
			extra = total - qwc8 - qwcr - 1;
		}
		break;
	}

	if (rem)
		*(u_long *)((u_int)&sp->trxreg | 0x20000000) =
			SCE_GS_SET_TRXREG((int)((sceGsTrxreg *)&sp->trxreg)->RRW, h2);

	while (*D1_CHCR & 0x100) {
		if (count++ > 0x1000000) {
			scePrintf("sceGsExecStoreImage: DMA Ch.1 does not terminate\r\n");
			return -1;
		}
	}

	imr = GsPutIMR(GsGetIMR() | 0x200);
	*GS_CSR = 2;
	*D1_QWC = sizeof(sceGsStoreImage) >> 4;
	SET_MADR(sp);
	*D1_CHCR = 0x101;
	while (*D1_CHCR & 0x100) {
		if (count++ > 0x1000000) {
			scePrintf("sceGsExecStoreImage: DMA Ch.1 does not terminate\r\n");
			return -1;
		}
	}
	while (!(*GS_CSR & 2)) {
		if (count++ > 0x1000000) {
			scePrintf("sceGsExecStoreImage: GS does not terminate\r\n");
			*VIF1_FIFO = *(u_long128 *)init_mp3;
			return -1;
		}
	}
	*VIF1_STAT = 0x800000;
	*GS_BUSDIR = 1;

	if (qwc8) {
		*D1_QWC = qwc8;
		SET_MADR(dstaddr);
		*D1_CHCR = 0x100;
		while (*D1_CHCR & 0x100) {
			if (count++ > 0x1000000) {
				scePrintf("sceGsExecStoreImage: DMA Ch.1 (GS->MEM) does not terminate\r\n");
				RESET_PATH;
				return -1;
			}
		}
	}
	for (i = 0; i < qwcr; i++) {
		WAIT_VIF1;
		dstaddr[qwc8 + i] = *VIF1_FIFO;
	}
	if (rem) {
		WAIT_VIF1;
		buf.q = *VIF1_FIFO;
		for (i = 0; i < rem; i++)
			((u_char *)&dstaddr[qwc8 + qwcr])[i] = buf.c[i];
		for (i = 0; i < extra; i++) {
			WAIT_VIF1;
			buf.q = *VIF1_FIFO;
		}
	}
	*VIF1_STAT = 0;
	*GS_BUSDIR = 0;
	GsPutIMR(imr);
	*GS_CSR = 2;
	*VIF1_FIFO = *(u_long128 *)init_mp3;
	return 0;
}
