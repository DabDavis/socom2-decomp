/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libgraph's sceGsSetDefClear (MAIN 0x1A1D70): the demo's object 0x14E8F8 (C), one function.
 * Fills a GIF A+D block of six register pairs that clears a rectangle -- TEST_1 with the depth test
 * on and ALWAYS, a SPRITE PRIM, RGBAQ the colour with Q 1.0, the sprite's two XYZ2 corners in
 * 12.4 fixed point at depth z, and TEST_1 again with the caller's depth test (ALWAYS when 0) --
 * and returns their count, after a sync.l. Name: SOCOM 1's (s1:exact). */
#include "libgraph.h"

int sceGsSetDefClear(sceGsClear *cp, short ztest, short x, short y, short w, short h,
		     u_char r, u_char g, u_char b, u_char a, u_int z)
{
	cp->testaaddr = SCE_GS_TEST_1;
	cp->testa = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1);
	cp->prim = SCE_GS_SET_PRIM(SCE_GS_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0);
	cp->primaddr = SCE_GS_PRIM;
	cp->rgbaqaddr = SCE_GS_RGBAQ;
	cp->rgbaq = SCE_GS_SET_RGBAQ(r, g, b, a, 0x3f800000);
	cp->xyz2a = SCE_GS_SET_XYZ(x << 4, y << 4, z);
	cp->xyz2aaddr = SCE_GS_XYZ2;
	cp->xyz2baddr = SCE_GS_XYZ2;
	cp->xyz2b = SCE_GS_SET_XYZ((x + w) << 4, (y + h) << 4, z);
	cp->testbaddr = SCE_GS_TEST_1;
	if (ztest)
		cp->testb = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ztest & 3);
	else
		cp->testb = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1);
	__asm__ __volatile__("sync.l");
	return 6;
}
