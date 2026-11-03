/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libgraph's sceGsSyncV (MAIN 0x1A2110): the demo's object 0x14EC98 (C), one function. Waits for
 * the vertical blank -- through VSync, or VSync2 (which returns CSR) when a VSync callback owns the
 * interrupt -- and returns the field being displayed (CSR bit 13) when interlaced, else 1. Name:
 * SOCOM 1's (s1:exact). */
#include "libgraph.h"

extern int VSync(void);
extern long VSync2(void);

int sceGsSyncV(int mode)
{
	sceGsGParam *gp = sceGsGetGParam();
	long field;

	if (gp->sceGsVSCfunc == 0) {
		VSync();
		if (gp->sceGsInterMode == 1)
			return (*GS_CSR >> 13) & 1;
	} else {
		field = (VSync2() >> 13) & 1;
		if (gp->sceGsInterMode == 1)
			return field;
	}
	return 1;
}
