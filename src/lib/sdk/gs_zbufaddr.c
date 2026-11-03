/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libgraph's sceGszbufaddr (MAIN 0x1A1AC0): the demo's object 0x14E648 (C), one function. The size
 * of a frame buffer in GS pages (2048 words): its width in 64-pixel pages, its height in 64-line
 * pages for a 16-bit format (psm bit 1) or 32-line ones, doubled unless the display is interlaced
 * by field. The two GParam shorts are tested together (GCC folds the two field compares into one
 * 64-bit load and mask). Name: SOCOM 1's (s1:exact). */
#include "libgraph.h"

short sceGszbufaddr(short psm, short w, short h)
{
	sceGsGParam *gp = sceGsGetGParam();
	int fbw, fbh;

	fbw = (w + 63) / 64;
	if (psm & 2)
		fbh = (h + 63) / 64;
	else
		fbh = (h + 31) / 32;
	if (gp->sceGsInterMode == 1 && gp->sceGsFFMode == 0)
		return fbw * fbh;
	return fbw * fbh * 2;
}
