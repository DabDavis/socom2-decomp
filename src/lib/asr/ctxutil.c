/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* Small recogniser accessors (OVL2 0x4D52F8, 0x4D5AB8, 0x4E5950): ScanSoft's recogniser, prebuilt
 * with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing (README "The GCC front"). Names are SOCOM 1's
 * (s1:exact; CreateActiveWordsCtxTree s1:align, SOCOM 1's size and body). CreateCtxTree makes no
 * sibling call: 2.96 declines one where the return types differ (as mtxConstruct).
 * _ConvPhenomesToStates, once here, is the language object's (language.c: a static in
 * _LangVirtualTable). CreateCtxTree and CreateActiveWordsCtxTree are SOCOM 1's context-tree
 * object's (0x343B58, OVL2 0x4D3318-0x4D5B18), MakeUserWordsActiveDist another's. */

typedef struct UWD {
    char pad[88];
    unsigned short dist;                /* +88 */
} UWD;

extern int intCreateCtxTree(void *a, void *b, int c, int d);

long CreateCtxTree(void *a, void *b)
{
    return intCreateCtxTree(a, b, 0, 0);
}

/* a context's accessors as contextv2.c fills them (21 words: the data pointer, then 20) */
typedef struct CTXV2TABLE {
    void *data;
    void *fn[20];
} CTXV2TABLE;

/* the casr object as far as this reads it (casr.h's CASR: the activated context at +572) */
typedef struct CTXIMPORT {
    void *data;
} CTXIMPORT;

typedef struct CASRCTX {
    char pad[572];
    CTXIMPORT *ctx;                     /* +572 */
} CASRCTX;

extern void FillContextV2VirtualTable(CTXV2TABLE *vt);

/* the tree of the activated context's words: a table of the context accessors on the stack,
 * over the activated context's data (a call, not a sibling one: the table is a stack slot) */
int CreateActiveWordsCtxTree(CASRCTX *c, int a, int b)
{
    CTXV2TABLE vt;

    FillContextV2VirtualTable(&vt);
    vt.data = c->ctx->data;
    return intCreateCtxTree(c, &vt, a, b);
}

unsigned short MakeUserWordsActiveDist(UWD *u, unsigned short *dist)
{
    return *dist = u->dist;
}
