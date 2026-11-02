/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* Small recogniser accessors (OVL2 0x4D52F8, 0x4E5950, 0x509700): ScanSoft's recogniser, prebuilt
 * with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing (README "The GCC front"). Names are SOCOM 1's
 * (s1:exact). CreateCtxTree makes no sibling call: 2.96 declines one where the return types
 * differ (as mtxConstruct). */

typedef struct UWD {
    char pad[88];
    unsigned short dist;                /* +88 */
} UWD;

extern int intCreateCtxTree(void *a, void *b, int c, int d);

long CreateCtxTree(void *a, void *b)
{
    return intCreateCtxTree(a, b, 0, 0);
}

unsigned short MakeUserWordsActiveDist(UWD *u, unsigned short *dist)
{
    return *dist = u->dist;
}

unsigned short _ConvPhenomesToStates(unsigned short *tab, unsigned short ph, unsigned short **states)
{
    *states = tab + tab[ph];
    return tab[ph + 1] - tab[ph];
}
