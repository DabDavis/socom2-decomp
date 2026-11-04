/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libgraph's sceGsSetDefLoadImage (MAIN 0x1A2520): the demo's object 0x14F0A8 (C), one function.
 * Builds the packet that sends an image from the host into GS local memory: a GIF tag of four A+D
 * pairs -- BITBLTBUF (the destination buffer), TRXPOS, TRXREG, TRXDIR 0 (host to local) -- and an
 * IMAGE tag whose NLOOP is the image's size in quadwords, which the pixel format sets (a switch
 * over the PSM, its case bodies in the source's order); a size past NLOOP's 15 bits is refused
 * with a message. Returns the packet's size in quadwords, after a sync.l. Name: SOCOM 1's
 * (s1:exact). */
#include "libgraph.h"

int sceGsSetDefLoadImage(sceGsLoadImage *lp, short dbp, short dbw, short dpsm,
			 short x, short y, short w, short h)
{
	int size = 0;

	switch (dpsm) {
	case SCE_GS_PSMCT32:
	case SCE_GS_PSMZ32:
		size = w * h >> 2;
		break;
	case SCE_GS_PSMCT24:
	case SCE_GS_PSMZ24:
		size = w * h * 3 >> 4;
		break;
	case SCE_GS_PSMCT16:
	case SCE_GS_PSMCT16S:
	case SCE_GS_PSMZ16:
	case SCE_GS_PSMZ16S:
		size = w * h >> 3;
		break;
	case SCE_GS_PSMT8:
	case SCE_GS_PSMT8H:
		size = w * h >> 4;
		break;
	case SCE_GS_PSMT4:
	case SCE_GS_PSMT4HL:
	case SCE_GS_PSMT4HH:
		size = w * h >> 5;
		break;
	}
	if (size > 32767) {
		scePrintf("sceGsSetDefLoadImage: too big size\r\n");
		return 0;
	}
	SCE_GIF_CLEAR_TAG(&lp->giftag0);
	lp->giftag0.NLOOP = 4;
	lp->giftag0.NREG = 1;
	lp->giftag0.REGS0 = SCE_GIF_PACKED_AD;
	lp->bitbltbuf = SCE_GS_SET_BITBLTBUF(0, 0, 0, dbp, dbw, dpsm);
	lp->bitbltbufaddr = SCE_GS_BITBLTBUF;
	lp->trxpos = SCE_GS_SET_TRXPOS(0, 0, x, y, 0);
	lp->trxposaddr = SCE_GS_TRXPOS;
	lp->trxreg = SCE_GS_SET_TRXREG(w, h);
	lp->trxregaddr = SCE_GS_TRXREG;
	lp->trxdir = SCE_GS_SET_TRXDIR(0);
	lp->trxdiraddr = SCE_GS_TRXDIR;
	SCE_GIF_CLEAR_TAG(&lp->giftag1);
	lp->giftag1.NLOOP = size;
	lp->giftag1.EOP = 1;
	lp->giftag1.FLG = SCE_GIF_IMAGE;
	__asm__ __volatile__("sync.l");
	return sizeof(sceGsLoadImage) >> 4;
}
