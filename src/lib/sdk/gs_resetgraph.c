/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libgraph's sceGsResetGraph and sceGsGetGParam (MAIN 0x1A13D0-0x1A156C): the demo's object
 * 0x14DF58 (C), two functions. The library's parameters are sceGsGetGParam's local static (the
 * demo's `gp.6`: interlaced, NTSC, frame mode, GS revision 3 until the reset reads CSR). Mode 0
 * resets the GS (CSR RESET), records the display mode and the revision, masks the GS interrupts,
 * drops a VSync callback and sets the CRT mode; mode 1 resets only; mode 5 records and sets the
 * CRT without the reset. The SetGsCrt tail calls are 991111's void-context sibling calls. Names:
 * SOCOM 1's (s1:exact, sceGsGetGParam s1:callee).
 *
 * sceGsResetGraph's mode 0 (open since round 6a, 4 words, registers only): `ffmode != 0` takes v1 and the callback load
 * v0 because the kernel's GsPutIMR returns the old mask -- declared void, the call leaves v0 to the first temporary after
 * it (GCC round 44a, tools/retsweep.py; the Medius lane's round 25b). The callback's two fields are cleared function
 * pointer first. */
#include "libgraph.h"

extern u_long GsPutIMR(u_long);
extern int DisableIntc(int);
extern int RemoveIntcHandler(int, int);
extern void SetGsCrt(short, short, short);

int sceGsResetGraph(short mode, short inter, short omode, short ffmode)
{
	sceGsGParam *gp;

	switch (mode) {
	case 0:
		gp = sceGsGetGParam();
		*GS_CSR = 0x200;
		gp->sceGsInterMode = inter;
		gp->sceGsOutMode = omode;
		gp->sceGsVersion = (*GS_CSR >> 16) & 0xff;
		GsPutIMR(0xff00);
		gp->sceGsFFMode = ffmode != 0;
		if (gp->sceGsVSCfunc != 0) {
			DisableIntc(2);
			RemoveIntcHandler(2, gp->sceGsVSCid);
			gp->sceGsVSCfunc = 0;
			gp->sceGsVSCid = 0;
		}
		SetGsCrt(inter & 1, omode & 0xff, ffmode & 1);
		break;
	case 1:
		*GS_CSR = 0x100;
		break;
	case 5:
		gp = sceGsGetGParam();
		gp->sceGsFFMode = ffmode != 0;
		gp->sceGsInterMode = inter;
		gp->sceGsOutMode = omode;
		gp->sceGsVersion = (*GS_CSR >> 16) & 0xff;
		SetGsCrt(inter & 1, omode & 0xff, ffmode & 1);
		break;
	}
}

sceGsGParam *sceGsGetGParam(void)
{
	static sceGsGParam gp = { 1, 2, 1, 3, 0, 0 };

	return &gp;
}
