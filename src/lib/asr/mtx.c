/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* mtx: the ScanSoft recogniser's float matrices (OVL2 0x4F7010-0x4F7300), prebuilt with
 * ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing (README "The GCC front"). The engine allocates from its own heap
 * (OVL1's heap_*, 0x2430F0). mtxConstruct hands back the allocator's result but is no sibling
 * call: 2.96 refuses one when the return types differ, so the handle is a long. Names are
 * SOCOM 1's where it has them (s1:exact/align). */

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
