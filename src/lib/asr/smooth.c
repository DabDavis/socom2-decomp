/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* smooth: the ScanSoft recogniser's smoothing object (OVL2 0x4F7A48-0x4F812C: SOCOM 1's object at
 * 0x3644C0 -- smtConstruct, smtDestruct, Smoothing -- here smtConstruct, Smoothing, smtDestruct),
 * prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC
 * front"). The acoustic normaliser (acne.c) builds one over SOCOM 1's Smoother1600Info11kHz tables
 * and smooths its mean through it. A smoother is seven matrices of the mtx object (mtx.c) and a
 * level. Names are SOCOM 1's. Round 27c: smtConstruct (SOCOM 1's body: each failing step frees
 * the smoother and returns 0 on its own -- cross-jumping merges the nine blocks into the last one,
 * which so sits before the success return, the target's layout; with one `goto fail` label after
 * the success return the fail block follows it and the allocation's registers swap, 52 words) and
 * smtDestruct (over its draft). Smoothing (0x45C, 0x380 in SOCOM 1: an active-set fit with
 * irreducible gotos) is not written: declared, its row open. */

typedef struct ASRENGINE {
    char pad[80];
    void *heap;
} ASRENGINE;

typedef struct MTX MTX;
typedef struct IMTX IMTX;

/* one table of the info: its data and its shape */
typedef struct SMTINFO {
    float *data;
    unsigned short rows;                /* +4 */
    unsigned short cols;                /* +6 */
} SMTINFO;

typedef struct SMOOTHER {
    MTX *a;                             /* +0: the info's first table */
    MTX *b;                             /* +4: its second */
    MTX *bcopy;                         /* +8: a copy of b */
    MTX *c;                             /* +12: its third */
    MTX *x;                             /* +16: n x 1, the input (associated per call) */
    MTX *y;                             /* +20: (k + 2) x 1 */
    IMTX *iy;                           /* +24: 1 x (k + 2) */
    float level;                        /* +28 */
} SMOOTHER;                             /* 32 */

extern void *heap_Calloc(void *heap, unsigned int n, unsigned int size);
extern void heap_Free(void *heap, void *p);
extern MTX *mtxConstruct(ASRENGINE *eng);
extern void mtxAssociateAll(MTX *m, int rows, int cols, float *data);
extern MTX *mtxInit(MTX *m, int rows, int cols, ASRENGINE *eng);
extern MTX *mtxInitCopy(MTX *m, MTX *from, ASRENGINE *eng);
extern void func_004F7038(MTX *m, ASRENGINE *eng);              /* mtxDestruct */
extern IMTX *imtxConstruct(ASRENGINE *eng);
extern IMTX *imtxInit(IMTX *m, int rows, int cols, ASRENGINE *eng);
extern void func_004F76A0(IMTX *m, ASRENGINE *eng);             /* imtxDestruct */

/* the smoother for n points at rate `rate` (k = (n - 1) / rate, rounded: the knots), its level */
SMOOTHER *smtConstruct(ASRENGINE *eng, unsigned int n, SMTINFO *info, float rate, float level)
{
    SMOOTHER *s = heap_Calloc(eng->heap, 1, sizeof(SMOOTHER));
    unsigned int k;

    if (s == 0)
        return 0;
    if ((s->a = mtxConstruct(eng)) == 0) {
        heap_Free(eng->heap, s);
        return 0;
    }
    mtxAssociateAll(s->a, info[0].rows, info[0].cols, info[0].data);
    if ((s->b = mtxConstruct(eng)) == 0) {
        heap_Free(eng->heap, s);
        return 0;
    }
    mtxAssociateAll(s->b, info[1].rows, info[1].cols, info[1].data);
    if ((s->c = mtxConstruct(eng)) == 0) {
        heap_Free(eng->heap, s);
        return 0;
    }
    mtxAssociateAll(s->c, info[2].rows, info[2].cols, info[2].data);
    k = (n - 1) / rate + 0.5;
    if ((s->x = mtxConstruct(eng)) == 0) {
        heap_Free(eng->heap, s);
        return 0;
    }
    mtxAssociateAll(s->x, n, 1, 0);
    if ((s->y = mtxConstruct(eng)) == 0) {
        heap_Free(eng->heap, s);
        return 0;
    }
    if (mtxInit(s->y, k + 2, 1, eng) == 0) {
        heap_Free(eng->heap, s);
        return 0;
    }
    if ((s->iy = imtxConstruct(eng)) == 0) {
        heap_Free(eng->heap, s);
        return 0;
    }
    if (imtxInit(s->iy, 1, k + 2, eng) == 0) {
        heap_Free(eng->heap, s);
        return 0;
    }
    if ((s->bcopy = mtxConstruct(eng)) == 0) {
        heap_Free(eng->heap, s);
        return 0;
    }
    if (mtxInitCopy(s->bcopy, s->b, eng) == 0) {
        heap_Free(eng->heap, s);
        return 0;
    }
    s->level = level;
    return s;
}

int Smoothing(SMOOTHER *s, float *x);

/* the smoother's matrices and itself freed */
void smtDestruct(SMOOTHER *s, ASRENGINE *eng)
{
    func_004F7038(s->a, eng);
    func_004F7038(s->b, eng);
    func_004F7038(s->c, eng);
    func_004F7038(s->x, eng);
    func_004F7038(s->y, eng);
    func_004F76A0(s->iy, eng);
    func_004F7038(s->bcopy, eng);
    heap_Free(eng->heap, s);
}
