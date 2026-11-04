/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libgraph's sceGsExecLoadImage (MAIN 0x1A2848): the demo's object 0x14F3D0 (C), one function.
 * Sends a sceGsSetDefLoadImage packet and then the image through DMA channel 2 (GIF): waits for
 * the channel to stop (one timeout count across both waits), sends the packet's six quadwords,
 * waits again and sends the image (the IMAGE tag's NLOOP quadwords); a scratchpad address is
 * given to the DMAC with SPR set. Returns 0, or -1 with a message when the channel does not stop.
 * Each wait has its own failure block (cross-jumping keeps one, after the second transfer's SPR
 * arm, as the target lays it out: one shared `goto` puts it last), and the MADR store is in each
 * arm of the scratchpad test (a conditional expression's one store is formed through $at).
 * Name: SOCOM 1's (s1:exact). */
#include "libgraph.h"

#define	D2_CHCR		((volatile u_int *)0x1000A000)
#define	D2_MADR		((volatile u_int *)0x1000A010)
#define	D2_QWC		((volatile u_int *)0x1000A020)

/* a scratchpad address goes to the DMAC with SPR (bit 31) set: one store in each arm */
#define	SET_MADR(a) \
		if (((u_int)(a) & 0x70000000) == 0x70000000) \
			*D2_MADR = ((u_int)(a) & 0x0fffffff) | 0x80000000; \
		else \
			*D2_MADR = (u_int)(a) & 0x0fffffff

int sceGsExecLoadImage(sceGsLoadImage *lp, u_long128 *srcaddr)
{
	u_int count = 0;

	while (*D2_CHCR & 0x100) {
		if (count++ > 0x1000000) {
			scePrintf("sceGsExecLoadImage: DMA Ch.2 does not terminate\r\n");
			return -1;
		}
	}
	*D2_QWC = sizeof(sceGsLoadImage) >> 4;
	SET_MADR(lp);
	*D2_CHCR = 0x101;
	while (*D2_CHCR & 0x100) {
		if (count++ > 0x1000000) {
			scePrintf("sceGsExecLoadImage: DMA Ch.2 does not terminate\r\n");
			return -1;
		}
	}
	*D2_QWC = lp->giftag1.NLOOP;
	SET_MADR(srcaddr);
	*D2_CHCR = 0x101;
	return 0;
}
