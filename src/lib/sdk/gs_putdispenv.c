/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libgraph's sceGsPutDispEnv (MAIN 0x1A1A00): the demo's object 0x14E588 (C), one function. It
 * writes the display environment into the GS's privileged registers: on a revision-1 GS the first
 * read circuit's DISPFB1/DISPLAY1 and EXTDATA, without SMODE2; on any other the second's, SMODE2
 * and BGCOLOR. PMODE is loaded once, ahead of the test. Name: SOCOM 1's (s1:exact). */
#include "libgraph.h"

void sceGsPutDispEnv(sceGsDispEnv *disp)
{
	if (sceGsGetGParam()->sceGsVersion == 1) {
		*GS_PMODE = disp->pmode;
		*GS_DISPFB1 = disp->dispfb;
		*GS_DISPLAY1 = disp->display;
		*GS_EXTDATA = disp->bgcolor;
	} else {
		*GS_PMODE = disp->pmode;
		*GS_SMODE2 = disp->smode2;
		*GS_DISPFB2 = disp->dispfb;
		*GS_DISPLAY2 = disp->display;
		*GS_BGCOLOR = disp->bgcolor;
	}
}
