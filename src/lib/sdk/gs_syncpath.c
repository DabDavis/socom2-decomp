/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libgraph's sceGsSyncPath (MAIN 0x1A21A8): the demo's object 0x14ED30 (C), one function.
 * Mode 0 waits for every path to the GS to stop -- DMA channels 1 (VIF1) and 2 (GIF), VIF1, VU1
 * (VPU_STAT's VBS1, read with cfc2) and the GIF's PATH3/transfer bits -- one count across the five
 * waits; past 0x1000000 polls it names the unit that did not stop, dumps the channels' and units'
 * registers and returns -1 (each wait's failure block its own: cross-jumping keeps one dump).
 * Any other mode returns a bit per busy unit (1 DMA 1, 2 DMA 2, 4 VIF1, 8 VU1, 16 the GIF). The
 * timeout argument is not read. scePrintf is void (the address after each call is formed in v0).
 * Name: SOCOM 1's (s1:exact). */
#include "libgraph.h"

#define	D1_CHCR		((volatile u_int *)0x10009000)
#define	D1_MADR		((volatile u_int *)0x10009010)
#define	D1_QWC		((volatile u_int *)0x10009020)
#define	D1_TADR		((volatile u_int *)0x10009030)
#define	D2_CHCR		((volatile u_int *)0x1000A000)
#define	D2_MADR		((volatile u_int *)0x1000A010)
#define	D2_QWC		((volatile u_int *)0x1000A020)
#define	D2_TADR		((volatile u_int *)0x1000A030)
#define	VIF1_STAT	((volatile u_int *)0x10003C00)
#define	GIF_STAT	((volatile u_int *)0x10003020)

/* VPU_STAT, read before the VU1 wait and again at the end of its body: the target's loop has its
 * exit test copied before it (as every wait's), which jump.c's duplicate_loop_exit_test refuses
 * for a test holding an asm with operands -- read in the test, the loop is entered by a jump (8
 * bytes short); through a `$6` register variable, the test's AND is tied to a2 (4 words) */
#define	VPU_STAT(s)	__asm__ __volatile__("cfc2 %0, $vi29" : "=r"(s))

#define	TIMEOUT(msg) { \
		scePrintf(msg); \
		scePrintf("\t<D1_CHCR=%08x:", *D1_CHCR); \
		scePrintf("D1_TADR=%08x:", *D1_TADR); \
		scePrintf("D1_MADR=%08x:", *D1_MADR); \
		scePrintf("D1_QWC=%08x>\r\n", *D1_QWC); \
		scePrintf("\t<D2_CHCR=%08x:", *D2_CHCR); \
		scePrintf("D2_TADR=%08x:", *D2_TADR); \
		scePrintf("D2_MADR=%08x:", *D2_MADR); \
		scePrintf("D2_QWC=%08x>\r\n", *D2_QWC); \
		scePrintf("\t<VIF1_STAT=%08x:", *VIF1_STAT); \
		scePrintf("GIF_STAT=%08x>\r\n", *GIF_STAT); \
		return -1; \
	}

int sceGsSyncPath(int mode, u_short timeout)
{
	u_int stat;
	u_int count = 0;
	int ret;

	if (mode == 0) {
		while (*D1_CHCR & 0x100)
			if (count++ > 0x1000000)
				TIMEOUT("sceGsSyncPath: DMA Ch.1 does not terminate\r\n")
		while (*D2_CHCR & 0x100)
			if (count++ > 0x1000000)
				TIMEOUT("sceGsSyncPath: DMA Ch.2 does not terminate\r\n")
		while (*VIF1_STAT & 0x1f000003)
			if (count++ > 0x1000000)
				TIMEOUT("sceGsSyncPath: VIF1 does not terminate\r\n")
		VPU_STAT(stat);
		while (stat & 0x100) {
			if (count++ > 0x1000000)
				TIMEOUT("sceGsSyncPath: VU1 does not terminate\r\n")
			VPU_STAT(stat);
		}
		while (*GIF_STAT & 0xc00)
			if (count++ > 0x1000000)
				TIMEOUT("sceGsSyncPath: GIF does not terminate\r\n")
		return 0;
	}
	ret = 0;
	if (*D1_CHCR & 0x100)
		ret |= 1;
	if (*D2_CHCR & 0x100)
		ret |= 2;
	if (*VIF1_STAT & 0x1f000003)
		ret |= 4;
	VPU_STAT(stat);
	if (stat & 0x100)
		ret |= 8;
	if (*GIF_STAT & 0xc00)
		ret |= 0x10;
	return ret;
}
