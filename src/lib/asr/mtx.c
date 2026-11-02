/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing */
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
