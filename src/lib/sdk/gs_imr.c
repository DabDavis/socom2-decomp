/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libgraph's GS interrupt-mask pair (MAIN 0x1A3058-0x1A30B4): sceGsPutIMR sets the GS's IMR through
 * the kernel's GsPutIMR and returns the mask it replaced (read first by GsGetIMR); sceGsGetIMR reads
 * it. Not in SOCOM 1 (its libgraph predates them: the demo's sceGsExecStoreImage is the last of its
 * objects there, as here, 0x1A29C8); after it in this image, the last of libgraph before libipu's
 * DMA object (setD3_CHCR 0x1A30B8). Names: Sony's libgraph API (libgraph.h's `u_long
 * sceGsPutIMR(u_long imr)`, `u_long sceGsGetIMR(void)`) by the bodies -- the weapons' round 34 read
 * them: 0x1A3058 calls GsGetIMR, then GsPutIMR(a0), and returns the old value; 0x1A3098 is one call
 * of GsGetIMR. The masks are doublewords, as the syscalls' (eekernel.h; ps2sdk's kernel.h gives
 * both u64): the moves are daddu as they would be for an int. Both matched as drafts before. */

typedef unsigned long u_long;

extern u_long GsGetIMR(void);
extern u_long GsPutIMR(u_long imr);

u_long sceGsPutIMR(u_long imr)
{
	u_long oldimr = GsGetIMR();

	GsPutIMR(imr);
	return oldimr;
}

u_long sceGsGetIMR(void)
{
	return GsGetIMR();
}
