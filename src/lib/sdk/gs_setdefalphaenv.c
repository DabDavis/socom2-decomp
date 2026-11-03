/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libgraph's sceGsSetDefAlphaEnv (MAIN 0x1A24C0): the demo's object 0x14F048 (C), one function.
 * Fills a GIF A+D block of four register pairs -- ALPHA_1 blending (Cs - Cd) * As + Cd, PABE from
 * the argument, TEXA (TA0 0x7F, AEM 1, TA1 0x81), FBA_1 0 -- and returns their count, after a
 * sync.l. Name: SOCOM 1's (s1:exact). */
#include "libgraph.h"

int sceGsSetDefAlphaEnv(sceGsAlphaEnv *ap, short pabe)
{
	ap->alpha1addr = SCE_GS_ALPHA_1;
	ap->alpha1 = 0x44;
	ap->pabeaddr = SCE_GS_PABE;
	ap->pabe = pabe;
	ap->texaaddr = SCE_GS_TEXA;
	ap->texa = (0x81UL << 32) | 0x807F;
	ap->fba1addr = SCE_GS_FBA_1;
	ap->fba1 = 0;
	__asm__ __volatile__("sync.l");
	return 4;
}
