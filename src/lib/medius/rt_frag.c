/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_frag.c: a buffer of 64 fragment slots (OVL2 0x6451F0-0x6456C0), in SCEA's RT library, prebuilt
 * with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). It follows StreamMedia.c and precedes the
 * Medius client; it has no $Header, no data and no SOCOM 1 counterpart, so the file name and every
 * name are ours. A buffer (0x800C bytes, allocated zeroed) holds two words, a count of the slots
 * in use and 64 slots of 512 bytes, each a sequence word, a length and up to 504 bytes, indexed by
 * the sequence's low six bits; sequences are 29-bit, compared modulo 2^29.
 * Byte-exact: the create (every slot cleared again after the zeroed allocation, the buffer pointer
 * reloaded from its stack slot after each store) and the destroy. func_00645290 byte-exact since GCC
 * round 83 (13 words to 2 in round 81: the age test through a user variable written in two steps, `d = nSeq & 63;
 * d = (d - nIndex) & SEQ; if (d - 1 < ...)` -- its load and its `d - 1` then leave v0 (local-alloc
 * ties no temporary to a user variable), and the 0xFFFFFFD constant takes a0, nIndex a3): the latest
 * test is d again, set twice, `d = nLatest & SEQ; d = (nIndex - d) & 0x40; if (d)` -- d, a global
 * pseudo (two deaths), takes the load, the and and the andi in v0, and the subu between them is a
 * block-local temporary nothing can tie (its input and its user are d), allocated first: v1. Round 26b:
 * func_00645528 (the reassembly into the caller's buffer) byte-exact; func_00645380 (the next complete message's
 * length: a walk from the oldest slot over the 0x20000000 start and 0x40000000 end flags) written, open. */

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
    unsigned int d;

    if (pBuf == 0)
        return;
    pSlot = &pBuf->aSlots[i];
    if (pSlot->nLength != 0) {
        d = pSlot->nSeq & (RT_FRAG_SLOTS - 1);
        d = (d - nIndex) & RT_FRAG_SEQ;
        if (d - 1 < 0x0FFFFFFE)
            return;
        pSlot->nLength = 0;
        pBuf->nCount--;
    }
    if (nSeq & 0x40000000) {
        d = pBuf->nLatest & RT_FRAG_SEQ;
        d = (nIndex - d) & 0x40;
        if (d)
            pBuf->nLatest = nSeq;
    }
    pSlot->nSeq = nSeq;
    if (nLength == 0)
        return;
    if (nLength > RT_FRAG_DATA)
        return;
    memcpy(pSlot->data, pData, nLength);
    pSlot->nLength = nLength;
    pBuf->nCount++;
}
/* the next complete message's length (0x645380): from the oldest sequence's slot -- or, when that does not start a message,
 * the next slot flagged 0x20000000 past it -- over consecutive fragments to one flagged 0x40000000 */
/* Byte-exact since GCC round 82 (62 words to 18 in round 81, registers only): the count a variable of its own (j: nSlot
 * or n as the counter gives the search's break path i's equality with nSlot, recomputing the slot's address from nSlot),
 * the slot's sequence a temporary (`n & SEQ` written at each use: the store takes the and's register), x00 stored before
 * nExpect, nSlot computed before the nCount test (the target's slot), the walk's length and sequence locals of the
 * block, nSeq declared after pSlot (pSlot wins their 30000 tie for v1). The last 18: `nExpect++` written in each arm
 * (jump2 cross-jumps the start arm's copy back onto the other's), so nExpect has four loop-weighted refs more (14 over
 * 51, 8235) and is allocated before nSlot (6857), taking a2 -- written once at the body's end it lost by 6122. */
unsigned int func_00645380(RT_FRAG_BUF *pBuf)
{
    unsigned int nFirst = 0;
    int nTotal = 0;
    unsigned int nHead;
    unsigned int nExpect;
    unsigned int nSlot;
    unsigned int n;
    unsigned int i;
    unsigned int j;

    if (pBuf == 0)
        return 0;
    nHead = pBuf->x00;
    nExpect = nHead & RT_FRAG_SEQ;
    nSlot = nHead & (RT_FRAG_SLOTS - 1);
    if (pBuf->nCount == 0)
        return 0;
    if (pBuf->aSlots[nSlot].nLength == 0 || (pBuf->aSlots[nSlot].nSeq & 0x20000000) == 0) {
        i = nSlot;
        for (;;) {
            i = (i + 1) & (RT_FRAG_SLOTS - 1);
            if (i == nSlot)
                break;
            if (pBuf->aSlots[i].nLength != 0 && (pBuf->aSlots[i].nSeq & 0x20000000) != 0)
                break;
        }
        n = pBuf->aSlots[i].nSeq;
        if (nHead < (n & RT_FRAG_SEQ)) {
            pBuf->x00 = n & RT_FRAG_SEQ;
            nExpect = n & RT_FRAG_SEQ;
            nSlot = n & (RT_FRAG_SLOTS - 1);
        } else
            return 0;
    }
    i = nSlot;
    for (j = 0; j < RT_FRAG_SLOTS; j++, i = (i + 1) & (RT_FRAG_SLOTS - 1)) {
        RT_FRAG_SLOT *pSlot = &pBuf->aSlots[i];
        unsigned int nSeq;
        unsigned int nLength;

        nSeq = pSlot->nSeq;
        if (nSeq & 0x20000000) {
            nFirst = nSeq;
            nTotal = pSlot->nLength;
            nExpect = nFirst & RT_FRAG_SEQ;
            nExpect++;
        } else {
            if (nTotal == 0 || (nLength = pSlot->nLength) == 0 || nExpect != (nSeq & RT_FRAG_SEQ)) {
                nTotal = 0;
                continue;
            }
            nTotal += nLength;
            if (nSeq & 0x40000000)
                break;
            nExpect++;
        }
    }
    if (j == RT_FRAG_SLOTS)
        nTotal = 0;
    if (nTotal != 0)
        pBuf->x00 = nFirst;
    return nTotal;
}

/* the next complete message copied out (0x645528), its slots freed: -1 when it does not fit */
int func_00645528(RT_FRAG_BUF *pBuf, char *pOut, unsigned int nSize)
{
    unsigned int nTotal = func_00645380(pBuf);
    unsigned int nExpect;
    unsigned int nSlot;
    unsigned int nStart;
    unsigned int nHead;
    unsigned int i;

    if (nTotal == 0)
        return nTotal;
    if (nSize < nTotal)
        return -1;
    nHead = pBuf->x00;
    nExpect = nHead & RT_FRAG_SEQ;
    nStart = nHead & (RT_FRAG_SLOTS - 1);
    nTotal = 0;
    if (pBuf->nCount == 0)
        return 0;
    nSlot = nStart;
    for (i = 0; i < RT_FRAG_SLOTS; i++, nSlot = (nSlot + 1) & (RT_FRAG_SLOTS - 1)) {
        RT_FRAG_SLOT *pSlot = &pBuf->aSlots[nSlot];

        if (pSlot->nSeq & 0x20000000) {
            memcpy(pOut + nTotal, pSlot->data, pSlot->nLength);
            nTotal += pSlot->nLength;
            pSlot->nLength = 0;
            nExpect = pSlot->nSeq & RT_FRAG_SEQ;
            pBuf->nCount--;
            nExpect++;
            continue;
        } else {
            if (nTotal == 0 || pSlot->nLength == 0 || nExpect != (pSlot->nSeq & RT_FRAG_SEQ)) {
                nTotal = 0;
                continue;
            }
            memcpy(pOut + nTotal, pSlot->data, pSlot->nLength);
            nTotal += pSlot->nLength;
            pSlot->nLength = 0;
            pBuf->nCount--;
            if (pSlot->nSeq & 0x40000000)
                break;
        }
        nExpect++;
    }
    if (i == RT_FRAG_SLOTS)
        nTotal = 0;
    if (nTotal != 0)
        pBuf->x00 = nExpect + 1;
    return nTotal;
}
