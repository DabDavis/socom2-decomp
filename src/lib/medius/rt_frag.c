/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_frag.c: a buffer of 64 fragment slots (OVL2 0x6451F0-0x6456C0), in SCEA's RT library, prebuilt
 * with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). It follows StreamMedia.c and precedes the
 * Medius client; it has no $Header, no data and no SOCOM 1 counterpart, so the file name and every
 * name are ours. A buffer (0x800C bytes, allocated zeroed) holds two words, a count of the slots
 * in use and 64 slots of 512 bytes, each a sequence word, a length and up to 504 bytes, indexed by
 * the sequence's low six bits; sequences are 29-bit, compared modulo 2^29.
 * Byte-exact: the create (every slot cleared again after the zeroed allocation, the buffer pointer
 * reloaded from its stack slot after each store) and the destroy. OPEN: func_00645290 (13 words: the
 * slot index taken first gives the target's order of the parameter copies; the sequence's 29 bits
 * land in a0 where the target reuses a3, nLength's register once copied to s2). Not written:
 * func_00645380 (the next complete message's length: a walk from the oldest slot over the 0x20000000
 * start and 0x40000000 end flags) and func_00645528 (the reassembly into the caller's buffer). */

extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);
extern void *memcpy(void *d, const void *s, unsigned int n);

#define RT_FRAG_SLOTS   64
#define RT_FRAG_DATA    504
#define RT_FRAG_SEQ     0x1FFFFFFF

typedef struct RT_FRAG_SLOT {
    unsigned int nSeq;                  /* 0x00 */
    unsigned int nLength;               /* 0x04: 0 when free */
    char data[RT_FRAG_DATA];            /* 0x08 */
} RT_FRAG_SLOT;                         /* 0x200 */

typedef struct RT_FRAG_BUF {
    unsigned int x00;                   /* 0x00 */
    unsigned int nLatest;               /* 0x04: the newest sequence flagged 0x40000000 */
    int nCount;                         /* 0x08: slots in use */
    RT_FRAG_SLOT aSlots[RT_FRAG_SLOTS]; /* 0x0C */
} RT_FRAG_BUF;                          /* 0x800C */

RT_FRAG_BUF *func_006451F0(void)
{
    RT_FRAG_BUF *pBuf = 0;
    unsigned int i;

    rt_memory_allocate_init_zero((void **)&pBuf, sizeof(RT_FRAG_BUF));
    if (pBuf != 0) {
        pBuf->x00 = 0;
        pBuf->nLatest = 0;
        pBuf->nCount = 0;
        for (i = 0; i < RT_FRAG_SLOTS; i++) {
            pBuf->aSlots[i].nSeq = 0;
            pBuf->aSlots[i].nLength = 0;
        }
    }
    return pBuf;
}

void func_00645268(RT_FRAG_BUF *pBuf)
{
    if (pBuf != 0)
        rt_memory_free((void **)&pBuf);
}

/* keeps a fragment in its sequence's slot, unless the slot holds a newer one */
void func_00645290(RT_FRAG_BUF *pBuf, unsigned int nSeq, void *pData, unsigned int nLength)
{
    unsigned int i = nSeq & (RT_FRAG_SLOTS - 1);
    unsigned int nIndex = nSeq & RT_FRAG_SEQ;
    RT_FRAG_SLOT *pSlot;

    if (pBuf == 0)
        return;
    pSlot = &pBuf->aSlots[i];
    if (pSlot->nLength != 0) {
        if ((((pSlot->nSeq & (RT_FRAG_SLOTS - 1)) - nIndex) & RT_FRAG_SEQ) - 1 < 0x0FFFFFFE)
            return;
        pSlot->nLength = 0;
        pBuf->nCount--;
    }
    if ((nSeq & 0x40000000) && ((nIndex - (pBuf->nLatest & RT_FRAG_SEQ)) & 0x40))
        pBuf->nLatest = nSeq;
    pSlot->nSeq = nSeq;
    if (nLength == 0)
        return;
    if (nLength > RT_FRAG_DATA)
        return;
    memcpy(pSlot->data, pData, nLength);
    pSlot->nLength = nLength;
    pBuf->nCount++;
}
