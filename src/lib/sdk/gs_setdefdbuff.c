/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libgraph's sceGsSetDefDBuff (MAIN 0x1A1E78): the demo's object 0x14EA00 (C), one function.
 * Sets up a double buffer: both display environments, both drawing environments and, when asked,
 * both clears (the drawing area's rectangle centred on 2048), each drawing packet's GIF tag (eight
 * A+D pairs, fourteen with the clear); then, unless the display is interlaced by field (where
 * sceGszbufaddr does not double the buffer), the second display and the first drawing buffer move
 * to the page after the first -- half the double buffer's size. Name: SOCOM 1's (s1:exact).
 *
 * OPEN (0x290 against 0x298, the first 122 words the target's -- the GParam pointer spilled to the
 * frame across the calls included): the target halves the page once (`sra a2,a2,1`, the call's
 * result kept in a2) and narrows it to a short again for DISPFB alone (`sll 16; sra 16` before
 * the 0x1ff mask) while FRAME_1 takes the shifted int; here the shift and the narrowing fold into
 * one `sll 15; sra 16` used for both, or the narrowing is dropped under the mask. Twelve shapes
 * measured (build/scratch/gcc/v_dbuff.py: the variable short, int, u_int, u_short; the halving
 * assigned or in each store; the stores swapped; chained; a cast on one store) -- none is longer
 * than 0x290, and the registers differ from the condition's evaluation on (gp in v1, the call's
 * result in a2 there). */
#include "libgraph.h"

void sceGsSetDefDBuff(sceGsDBuff *db, short psm, short w, short h, short ztest, short zpsm,
		      short clear)
{
	sceGsGParam *gp = sceGsGetGParam();
	short zbp;

	sceGsSetDefDispEnv(&db->disp[0], psm, w, h, 0, 0);
	sceGsSetDefDispEnv(&db->disp[1], psm, w, h, 0, 0);
	sceGsSetDefDrawEnv(&db->draw0, psm, w, h, ztest, zpsm);
	sceGsSetDefDrawEnv(&db->draw1, psm, w, h, ztest, zpsm);
	if (clear) {
		sceGsSetDefClear(&db->clear0, ztest, 2048 - (w >> 1), 2048 - (h >> 1), w, h, 0, 0, 0, 0, 0);
		sceGsSetDefClear(&db->clear1, ztest, 2048 - (w >> 1), 2048 - (h >> 1), w, h, 0, 0, 0, 0, 0);
	}
	SCE_GIF_CLEAR_TAG(&db->giftag0);
	db->giftag0.NLOOP = clear ? 14 : 8;
	db->giftag0.EOP = 1;
	db->giftag0.NREG = 1;
	db->giftag0.REGS0 = SCE_GIF_PACKED_AD;
	SCE_GIF_CLEAR_TAG(&db->giftag1);
	db->giftag1.NLOOP = clear ? 14 : 8;
	db->giftag1.EOP = 1;
	db->giftag1.NREG = 1;
	db->giftag1.REGS0 = SCE_GIF_PACKED_AD;
	zbp = sceGszbufaddr(psm, w, h);
	if ((gp->sceGsInterMode == 1 && gp->sceGsFFMode == 1) || gp->sceGsInterMode == 0) {
		zbp >>= 1;
		((sceGsFrame *)&db->draw0.frame1)->FBP = zbp;
		((sceGsDispfb *)&db->disp[1].dispfb)->FBP = zbp;
	}
}
