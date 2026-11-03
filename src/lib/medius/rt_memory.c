/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-optimize-sibling-calls */
/* rt_memory: the RT library's allocator (OVL2 0x6322C8-0x6325B8, SOCOM 1's object at 0x405E28:
 * rt_memory_allocate .. rt_memory_get_aligned_address), SCEA's Medius/DME network stack, prebuilt
 * with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"); built, as rt_time is, without sibling calls
 * (rt_memory_align_alloc returns rt_memory_get_aligned_address's result through a jal). The host
 * may hand it a malloc and a free (NetRegisterMallocFreeCallbacks); newlib's are the default.
 * Sizes are 64-bit here and narrowed for malloc and memset. Names are SOCOM 1's (s1:exact). */

typedef struct rt_aligned_memory {
    void *pAllocated;                   /* what rt_memory_allocate gave */
    void *pAligned;                     /* the first aligned address in it */
} rt_aligned_memory;

typedef void *(*rt_malloc_callback)(unsigned int nSize);
typedef void (*rt_free_callback)(void *pMemory);

extern rt_malloc_callback gpfMallocCallback;
extern rt_free_callback gpfFreeCallback;

extern void *malloc(unsigned int nSize);
extern void free(void *pMemory);
extern void *memset(void *p, int c, unsigned int n);

int rt_memory_get_aligned_address(void **ppAligned, void *pMemory, unsigned short nAlignment);

int rt_memory_allocate(void **ppMemory, unsigned long nSize)
{
    if (gpfMallocCallback == 0) {
        *ppMemory = malloc(nSize);
        if (*ppMemory == 0)
            return 1;
    } else {
        *ppMemory = gpfMallocCallback(nSize);
        if (*ppMemory == 0)
            return 1;
    }
    return 0;
}

int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize)
{
    int result;

    if (ppMemory == 0)
        return 1;
    *ppMemory = 0;
    result = rt_memory_allocate(ppMemory, nSize);
    if (result != 0)
        return result;
    if (*ppMemory != 0) {
        memset(*ppMemory, 0, nSize);
        return 0;
    }
    return 1;
}

int rt_memory_free(void **ppMemory)
{
    if (*ppMemory != 0) {
        if (gpfFreeCallback == 0)
            free(*ppMemory);
        else
            gpfFreeCallback(*ppMemory);
        *ppMemory = 0;
    }
    return 0;
}

int rt_memory_align_alloc(rt_aligned_memory *pMemory, unsigned int nSize, unsigned short nAlignment)
{
    int result;

    if (pMemory == 0 || nSize == 0)
        return 1;
    pMemory->pAllocated = 0;
    pMemory->pAligned = 0;
    if (nAlignment >= 2) {
        nSize += nAlignment - 1;
        nSize += nAlignment - nSize % nAlignment;
    }
    result = rt_memory_allocate(&pMemory->pAllocated, nSize);
    if (result != 0)
        return result;
    return rt_memory_get_aligned_address(&pMemory->pAligned, pMemory->pAllocated, nAlignment);
}

int rt_memory_align_alloc_init_zero(rt_aligned_memory *pMemory, unsigned int nSize,
                                    unsigned short nAlignment)
{
    int result;

    if (pMemory == 0 || nSize == 0)
        return 1;
    pMemory->pAllocated = 0;
    pMemory->pAligned = 0;
    result = rt_memory_align_alloc(pMemory, nSize, nAlignment);
    if (result != 0)
        return result;
    if (pMemory->pAligned != 0) {
        memset(pMemory->pAligned, 0, nSize);
        return 0;
    }
    return 1;
}

int rt_memory_align_free(rt_aligned_memory *pMemory)
{
    int result;

    if (pMemory == 0)
        return 1;
    if (pMemory->pAllocated != 0) {
        result = rt_memory_free(&pMemory->pAllocated);
        if (result != 0)
            return result;
        pMemory->pAllocated = 0;
    }
    pMemory->pAligned = 0;
    return 0;
}

int rt_memory_register_callbacks(rt_malloc_callback pfMalloc, rt_free_callback pfFree)
{
    gpfMallocCallback = pfMalloc;
    gpfFreeCallback = pfFree;
    return 0;
}

int rt_memory_get_aligned_address(void **ppAligned, void *pMemory, unsigned short nAlignment)
{
    unsigned int nRemainder;

    if (ppAligned == 0 || pMemory == 0)
        return 1;
    *ppAligned = pMemory;
    if (nAlignment >= 2) {
        nRemainder = (unsigned int)pMemory % nAlignment;
        if (nRemainder != 0)
            *ppAligned = (char *)pMemory + (nAlignment - nRemainder);
    }
    return 0;
}
