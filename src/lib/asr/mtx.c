/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* mtx: the ScanSoft recogniser's float matrices (OVL2 0x4F7010-0x4F77F8: SOCOM 1's mtx object at
 * 0x363888 but its first function, QrDeleteCol, 0x4F6CE0, not written; mtxDestruct and
 * imtxDestruct moved after their constructors), prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing (README "The GCC front"). The engine allocates from its own heap (OVL1's
 * heap_*, 0x2430F0). mtxConstruct hands back the allocator's result but is no sibling call: 2.96
 * refuses one when the return types differ, so the handle is a long. Names are SOCOM 1's where it
 * has them (s1:exact/align/order). Round 16c: mtxCompress, mtxMac and mtxMax -- the loops'
 * bounds in locals (the unroller copies a bound it reloads), the data indexed through a local
 * (mtxCompress: the strength-reduced pointer starts as a copy of it, the target's `move s1,s0`),
 * mtxMax's step `i++, p++` in the for (the counter's increment first in the unrolled copies). */

typedef struct MTX {
    int rows;
    int cols;
    int n;                      /* rows * cols */
    float *data;
    float *end;
    unsigned char owned;        /* data is the matrix's own allocation */
} MTX;

typedef struct ASRENGINE {
    char pad[80];
    void *heap;
} ASRENGINE;

extern void *func_00243398(void *heap, int count, int size);    /* the heap's calloc */
extern void heap_Free(void *heap, void *p);
extern float expf(float x);
extern float logf(float x);

long mtxConstruct(ASRENGINE *eng)
{
    return (long)func_00243398(eng->heap, 1, sizeof(MTX));
}

void func_004F7038(MTX *m, ASRENGINE *eng)     /* mtxDestruct */
{
    if (m->owned && m->data)
        heap_Free(eng->heap, m->data);
    heap_Free(eng->heap, m);
}

void mtxAssociateEntr(MTX *m, float *data)
{
    m->data = data;
    m->end = data + m->n;
}

void mtxAssociateAll(MTX *m, int rows, int cols, float *data)
{
    m->rows = rows;
    m->cols = cols;
    m->n = rows * cols;
    m->data = data;
    m->end = data + m->n;
}

/* (mtxDeleteCol, 0x4F70C8, stands as its draft) */

/* every element x pulled down by log(1 + e^(2(x - c))) / 2: a soft ceiling at c */
void mtxCompress(MTX *m, float c)
{
    float *d = m->data;
    unsigned int n = m->rows * m->cols;
    unsigned int i;

    for (i = 0; i < n; i++)
        d[i] -= (float)(logf(expf((d[i] - c) * 2.0) + 1.0f) * 0.5);
}

/* m += f * x, element by element */
int mtxMac(MTX *m, float *x, float f)
{
    float *p = m->data;
    float *end = p + m->rows * m->cols;

    while (p < end)
        *p++ += *x++ * f;
    return 0;
}

/* (mtxInit, 0x4F7428, and mtxInitCopy, 0x4F74C0, stand as their drafts) */

/* the largest element */
float mtxMax(MTX *m)
{
    unsigned int n = m->n;
    float *p = m->data;
    float max = *p++;
    unsigned int i;

    for (i = 1; i < n; i++, p++)
        if (max < *p)
            max = *p;
    return max;
}

long imtxConstruct(ASRENGINE *eng)              /* the integer matrix: as mtxConstruct */
{
    return (long)func_00243398(eng->heap, 1, 24);
}

/* the integer matrix's data allocated cleared (mtxInit's twin: calloc for alloc and memset); the
 * matrix itself freed when that fails (round 10c) */
typedef struct IMTX {
    int rows;
    int cols;
    int n;
    int *data;
    int *end;
    unsigned char owned;
} IMTX;

IMTX *imtxInit(IMTX *m, int rows, int cols, ASRENGINE *eng)
{
    int n = rows * cols;
    int *p;

    m->rows = rows;
    m->cols = cols;
    m->n = n;
    p = func_00243398(eng->heap, n, sizeof(int));
    m->data = p;
    if (p == 0) {
        heap_Free(eng->heap, m);
        return 0;
    }
    m->owned = 1;
    m->end = p + n;
    return m;
}
