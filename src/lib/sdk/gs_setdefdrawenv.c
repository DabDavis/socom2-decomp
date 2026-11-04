/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libgraph's sceGsSetDefDrawEnv (MAIN 0x1A1B88): the demo's object 0x14E710 (C), one function.
 * Fills a drawing environment for context 1, eight A+D pairs: FRAME_1 (the frame buffer at page 0,
 * its width in 64-pixel units), ZBUF_1 (the depth buffer after it, sceGszbufaddr's page count --
 * masked from writes when there is no depth test), XYOFFSET_1 (the drawing area centred on 2048),
 * SCISSOR_1, PRMODECONT's AC, COLCLAMP's CLAMP, DTHE (dithering for a 16-bit format) and TEST_1
 * (the depth test, ALWAYS when 0); returns their count, after a sync.l. The offset is computed in
 * 64 bits (the target's dsubu: `2048L - ...` and `(u_long)2048 - ...` compile alike; an int
 * difference converted afterwards is 0x1D4 bytes). Name: SOCOM 1's (s1:exact). */
#include "libgraph.h"

int sceGsSetDefDrawEnv(sceGsDrawEnv1 *dp, short psm, short w, short h, short ztest, short zpsm)
{
	dp->frame1addr = SCE_GS_FRAME_1;
	dp->frame1 = SCE_GS_SET_FRAME(0, ((w + 63) >> 6) & 0x3f, psm & 0xf, 0);
	dp->zbuf1addr = SCE_GS_ZBUF_1;
	if (ztest == 0)
		dp->zbuf1 = SCE_GS_SET_ZBUF(sceGszbufaddr(psm, w, h), zpsm & 0xf, 1);
	else
		dp->zbuf1 = SCE_GS_SET_ZBUF(sceGszbufaddr(psm, w, h), zpsm & 0xf, 0);
	dp->xyoffset1addr = SCE_GS_XYOFFSET_1;
	dp->xyoffset1 = SCE_GS_SET_XYOFFSET((2048 - (u_long)(w >> 1)) << 4, (2048 - (u_long)(h >> 1)) << 4);
	dp->scissor1addr = SCE_GS_SCISSOR_1;
	dp->scissor1 = SCE_GS_SET_SCISSOR(0, w - 1, 0, h - 1);
	dp->prmodecontaddr = SCE_GS_PRMODECONT;
	dp->prmodecont.AC = 1;
	dp->colclampaddr = SCE_GS_COLCLAMP;
	dp->colclamp.CLAMP = 1;
	dp->dtheaddr = SCE_GS_DTHE;
	if (psm & 2)
		dp->dthe.DTHE = 1;
	else
		dp->dthe.DTHE = 0;
	dp->test1addr = SCE_GS_TEST_1;
	if (ztest)
		dp->test1 = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ztest & 3);
	else
		dp->test1 = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1);
	__asm__ __volatile__("sync.l");
	return 8;
}
