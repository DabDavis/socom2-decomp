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
 * smtDestruct (over its draft). Round 28c: Smoothing (0x45C; SOCOM 1's 0x380 is the same source,
 * its last loop not unrolled) -- plain loops, not the gotos m2c read: the restart is a do-while on
 * the dropped knot's index, the back-substitution a for with a break; the first term stored as
 * the inner for's init (`for (*py = d; (pr += r->rows) < end; )`), which keeps that loop's test
 * below its body, and cross-jumping then merges the init's store into the body's -- the target's
 * entry jump to the store; `k` a copy of r->cols of its own, the x pointer's offset subtracted in
 * the for's init BEFORE `i = k` (after it, CSE rewrites `k` to `i`); the diagonal as `... + i`
 * then `pr--` (the target's `addu a1,v0,t0; addiu a1,a1,-4`: an index form reverses the sum); the
 * curve's loop on pointers of its own (shared with the back-substitution the pointers are one
 * long-lived pseudo, global-alloc's first; local to the unrolled loop, the unroller renames them
 * per copy, as the target's); the three sizes reset in one chain, innermost the bcopy's. */

typedef struct ASRENGINE {
    char pad[80];
    void *heap;
} ASRENGINE;

/* the mtx object's matrices (mtx.c): column-major, `n` = rows * cols */
typedef struct MTX {
    unsigned int rows;                  /* +0 */
    unsigned int cols;                  /* +4 */
    unsigned int n;                     /* +8 */
    float *data;                        /* +12 */
    float *end;                         /* +16 */
} MTX;

typedef struct IMTX {
    unsigned int rows;                  /* +0 */
    unsigned int cols;                  /* +4 */
    unsigned int n;                     /* +8 */
    int *data;                          /* +12 */
    int *end;                           /* +16 */
} IMTX;

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
extern void mtxFillCopy(MTX *to, MTX *from);
extern float mtxMax(MTX *m);
extern void mtxCompress(MTX *m, float c);
extern int QrPreMult(MTX *r, MTX *c, MTX *x);
extern int QrDeleteCol(MTX *r, MTX *x, unsigned int col);
extern void imtxDeleteCol(IMTX *m, unsigned int col);
extern void mtxFillX(MTX *m, float v);

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

/* x (n points) smoothed in place: its peak compressed under the level, fitted by least squares on
 * the knots' basis (QR), each fitted coefficient back-substituted from the last; a positive one at
 * the third knot or later drops its knot and the fit restarts; the fit's curve written back */
int Smoothing(SMOOTHER *s, float *x)
{
    MTX *r = s->bcopy;
    MTX *xm = s->x;
    MTX *c = s->c;
    MTX *a = s->a;
    MTX *y = s->y;
    IMTX *iy = s->iy;
    unsigned int i, j, k;
    float *px, *py, *pr, *q, *end, d, diag, *col, *out;
    int *idx;

    mtxFillCopy(r, s->b);
    xm->data = x;
    xm->end = x + xm->n;
    mtxCompress(xm, mtxMax(xm) - s->level);
    if (QrPreMult(r, c, xm) != 0)
        return 1;
    for (i = iy->cols; i != 0; ) {
        i--;
        iy->data[i] = i;
    }
    do {
        k = r->cols;
        py = y->data + y->rows;
        end = r->data + r->rows * k;
        px = xm->data + xm->rows;
        for (px -= xm->rows - k, i = k; i != 0; i--) {
            d = *--px;
            q = --py;
            pr = r->data + (i - 1) * r->rows + i;
            pr--;
            diag = *pr;
            for (*py = d; (pr += r->rows) < end; )
                *py -= *pr * *++q;
            *py /= diag;
            if (*py > 0 && i >= 3)
                break;
        }
        if (i != 0) {
            if (QrDeleteCol(r, xm, i - 1) != 0)
                return 1;
            y->rows--;
            imtxDeleteCol(iy, i - 1);
        }
    } while (i >= 3);
    mtxFillX(xm, 0.0f);
    idx = iy->data;
    for (py = y->data, end = py + y->rows; py < end; py++) {
        col = a->data + a->rows * *idx++;
        out = xm->data;
        for (j = 0; j < xm->n; j++)
            *out++ += *col++ * *py;
    }
    iy->cols = y->rows = r->cols = a->cols;
    return 0;
}

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
