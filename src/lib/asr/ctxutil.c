/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* A small recogniser accessor (OVL2 0x4E5950): ScanSoft's recogniser, prebuilt with ee-gcc 2.96 at
 * -O2 -G0 -fno-strict-aliasing (README "The GCC front"). The name is SOCOM 1's (s1:exact).
 * _ConvPhenomesToStates, once here, is the language object's (language.c: a static in
 * _LangVirtualTable); CreateCtxTree and CreateActiveWordsCtxTree, once here too, are the
 * context-tree object's (ctxtree.c: SOCOM 1's 0x343B58, OVL2 0x4D3318-0x4D5B18, round 19c).
 * MakeUserWordsActiveDist is another object's. */

typedef struct UWD {
    char pad[88];
    unsigned short dist;                /* +88 */
} UWD;

unsigned short MakeUserWordsActiveDist(UWD *u, unsigned short *dist)
{
    return *dist = u->dist;
}
