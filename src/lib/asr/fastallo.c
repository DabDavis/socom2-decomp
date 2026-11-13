/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* fastallo: the recogniser's small-block allocator (OVL2 0x50D560-0x50D810, SOCOM 1's object at
 * 0x377C70), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops (README "The GCC front"). SOCOM 1's three functions,
 * paired by their bodies (s1:align): SOCOM II defines them in another order (the allocation
 * first, then Init and Terminate). Blocks of up to 64 words are cut from chunks taken from the
 * engine's heap; a chunk's unused tail goes on a free list when a new chunk is started. */

typedef struct FASTALLO {
    void *heap;                         /* +0 */
    unsigned short chunk_size;          /* +4: bytes */
    unsigned short left;                /* +6: words left in the current chunk */
    int nchunks;                        /* +8 */
    int **chunks;                       /* +12 */
    int *next;                          /* +16: the current chunk's first free word */
    int *free[65];                      /* +20 */
} FASTALLO;

extern void *func_002430F0(void *heap, void *p, int size);        /* the heap's realloc */
extern void *func_00243298(void *heap, int size);                  /* the heap's malloc */
extern void heap_Free(void *heap, void *p);
extern void *memset(void *p, int c, unsigned int n);

/* a block of n words (1..64) cut from the current chunk; when too few are left, their tail goes
 * on the free list of its size and a new chunk is started (the chunk table grown by one). Byte-exact
 * since GCC round 75 (long OPEN, 6 words short): the count is read from the field at
 * every use, no local -- CSE then keeps one HImode load for the store's `left - n` and a zero-
 * extended copy (`andi`) for the tests and the free list's index (`+ 3u`: srl); the chunk counter
 * is bumped in the table's index (`i++`, in place: the target's s1). A local `left` of any type
 * loses the copy and the extension. */
int *fastallo_AllocateMemoryFromChunk(FASTALLO *fa, unsigned short n)
{
    int *p = 0;
    int i;
    int **list;
    int **chunks;
    int *chunk;

    if (n == 0)
        goto out;
    if (n > 64)
        goto out;
    if (fa->left < n) {
        i = fa->nchunks;
        if (fa->left != 0) {
            list = &fa->free[(fa->left + 3u) >> 2];
            *(int **)fa->next = *list;
            *list = fa->next;
        }
        chunks = func_002430F0(fa->heap, fa->chunks, (i + 1) * sizeof(int *));
        if (chunks == 0)
            goto out;
        fa->chunks = chunks;
        chunk = func_00243298(fa->heap, fa->chunk_size);
        if (chunk == 0)
            goto out;
        fa->chunks[i++] = chunk;
        fa->nchunks = i;
        fa->next = chunk;
        fa->left = fa->chunk_size >> 2;
    }
    p = fa->next;
    fa->left -= n;
    fa->next = p + n;
out:
    return p;
}

void fastallo_Init(FASTALLO *fa, void *heap, unsigned short chunk_size)
{
    memset(fa, 0, sizeof(FASTALLO));
    fa->chunk_size = chunk_size;            /* this order schedules as the target's */
    fa->heap = heap;
}

void fastallo_Terminate(FASTALLO *fa)
{
    unsigned int n = fa->nchunks;
    unsigned int i;

    for (i = 0; i < n; i++)
        heap_Free(fa->heap, fa->chunks[i]);
    if (fa->chunks != 0)
        heap_Free(fa->heap, fa->chunks);
}
