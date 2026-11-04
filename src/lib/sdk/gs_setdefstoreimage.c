/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libgraph's sceGsSetDefStoreImage (MAIN 0x1A2708): the demo's object 0x14F290 (C), one function.
 * Builds the packet that reads a rectangle of GS local memory back to the host: VIF1's NOP,
 * MSKPATH3, FLUSHA and DIRECT 6, a GIF tag (NLOOP 5, EOP, one A+D register), BITBLTBUF (the
 * source buffer), TRXPOS, TRXREG, FINISH and TRXDIR 1 (local to host); returns its size in
 * quadwords, after a sync.l. Name: SOCOM 1's (s1:exact). */
#include "libgraph.h"

int sceGsSetDefStoreImage(sceGsStoreImage *sp, short sbp, short sbw, short spsm,
			  short x, short y, short w, short h)
{
	sp->vifcode[0] = 0x00000000;
	sp->vifcode[1] = 0x06008000;
	sp->vifcode[2] = 0x13000000;
	sp->vifcode[3] = 0x50000006;
	SCE_GIF_CLEAR_TAG(&sp->giftag);
	sp->giftag.NLOOP = 5;
	sp->giftag.EOP = 1;
	sp->giftag.NREG = 1;
	sp->giftag.REGS0 = SCE_GIF_PACKED_AD;
	sp->bitbltbuf = SCE_GS_SET_BITBLTBUF(sbp, sbw, spsm, 0, 0, 0);
	sp->bitbltbufaddr = SCE_GS_BITBLTBUF;
	sp->trxpos = SCE_GS_SET_TRXPOS(x, y, 0, 0, 0);
	sp->trxposaddr = SCE_GS_TRXPOS;
	sp->trxreg = SCE_GS_SET_TRXREG(w, h);
	sp->trxregaddr = SCE_GS_TRXREG;
	sp->finish = 0;
	sp->finishaddr = SCE_GS_FINISH;
	sp->trxdir = SCE_GS_SET_TRXDIR(1);
	sp->trxdiraddr = SCE_GS_TRXDIR;
	__asm__ __volatile__("sync.l");
	return sizeof(sceGsStoreImage) >> 4;
}
