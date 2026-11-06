/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_list.c: the RT library's message list in a ring (OVL2 0x6313D8-0x6322C8), SCEA's RT library,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). No $Header and no SOCOM 1
 * counterpart: the file name is ours, placed between rt_crypt.c and rt_memory.c as the alphabetical
 * link order puts it, and the function names are placeholders. rt_udp.c keeps a list per endpoint
 * and two per connection (send and receive).
 *
 * A list (0x18 bytes) is a ring buffer of messages, each an 8-byte head -- its length and the next
 * message -- and its bytes, word-aligned; the first message is kept apart (pFirst), and an empty
 * message always ends the chain (func_00631ED0 writes it at the end). Error codes are the values
 * returned: 1 not found, 2 a bad argument or list, 3 no room.
 *
 * The end marker's writer returns a narrow type (`unsigned char`): the clear and init return its
 * result through a jal, not a sibling call. OPEN: func_006313D8 (the init, one word long since
 * round 21b -- guards to the end, `return func_00631ED0(pList)` (d 27 -> 14): the allocation's
 * `return 3` tests the call's v0 itself, so if-conversion cannot hoist it and the target branches
 * to it with `li v0,3` in the slot where this compile lays it out as the fall-through with a `b`;
 * pList takes s0 here, s1 there; a positive allocation test is 8-16 bytes longer).
 * Round 14b: the store after a message (0x631508: its room test a positive `if (nFree >= nLength)
 * {...} else return 3;`, as rt_circ_buf's), the store at the head, the removal, the compaction and
 * the resize (the header copied out as a struct -- ldl/ldr -- and rebuilt message by message), the
 * end's recomputation. Not written: 0x631848. */

extern void *memset(void *p, int c, unsigned int n);
extern int rt_memory_allocate(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);

typedef struct RT_LIST_MSG {
    unsigned int nLength;               /* 0x00 */
    struct RT_LIST_MSG *pNext;          /* 0x04 */
    char data[1];                       /* 0x08 */
} RT_LIST_MSG;

typedef struct RT_LIST {
    unsigned int nStart;                /* 0x00: the first used byte */
    unsigned int nEnd;                  /* 0x04: the first free byte */
    unsigned int nSize;                 /* 0x08 */
    unsigned int nUsed;                 /* 0x0C */
    char *pBuf;                         /* 0x10 */
    RT_LIST_MSG *pFirst;                /* 0x14 */
} RT_LIST;                              /* 0x18 */

int func_00631B10(RT_LIST *pList, RT_LIST_MSG **ppMsg);
unsigned int func_00631E70(RT_LIST_MSG *pMsg);
RT_LIST_MSG *func_00631E88(RT_LIST_MSG *pMsg);
int func_00631EA0(RT_LIST *pList);
unsigned char func_00631ED0(RT_LIST *pList);
int func_00631F08(RT_LIST *pList, unsigned int *pnFree);
int func_00631F30(RT_LIST *pList, unsigned int *pnFree);
int func_00631F80(RT_LIST *pList, unsigned int *pnFree);
int func_00631B60(RT_LIST *pList, RT_LIST_MSG **ppMsg);
int func_00631FD8(RT_LIST *pList, RT_LIST_MSG **ppMsg, unsigned int nNeed);
int func_006320A8(RT_LIST *pList, RT_LIST_MSG *pMsg, RT_LIST_MSG **ppPrev);
int func_00632140(RT_LIST *pList);
int func_006321B8(RT_LIST *pList);
char *func_00631E58(RT_LIST_MSG *pMsg);
int func_00631BC0(RT_LIST *pList, RT_LIST_MSG **ppMsg);
extern void *memcpy(void *dst, const void *src, unsigned int n);

int func_006313D8(RT_LIST *pList, unsigned int nSize)
{
    if (pList == 0)
        return 2;
    if (nSize < 9)
        return 2;
    memset(pList, 0, sizeof(RT_LIST));
    if (rt_memory_allocate((void **)&pList->pBuf, nSize) != 0)
        return 3;
    if (pList->pBuf == 0)
        return 0;
    pList->nSize = nSize;
    return func_00631ED0(pList);
}

int func_00631460(RT_LIST *pList)
{
    if (func_00631EA0(pList) == 0)
        return 2;
    if (pList->pBuf != 0)
        rt_memory_free((void **)&pList->pBuf);
    memset(pList, 0, sizeof(RT_LIST));
    return 0;
}

int func_006314C0(RT_LIST *pList)
{
    if (func_00631EA0(pList) == 0)
        return 2;
    pList->nStart = 0;
    pList->nEnd = 0;
    pList->nUsed = 0;
    return func_00631ED0(pList);
}

/* a message stored after pAfter: written over the end marker when pAfter is it (the room at the
 * ring's end checked), else in a new place found for its head and bytes and linked in after it */
int func_00631508(RT_LIST *pList, RT_LIST_MSG *pAfter, const void *pData, unsigned int nLength)
{
    unsigned int nFree = 0;
    RT_LIST_MSG *pMsg;
    int result;

    if (func_00631EA0(pList) == 0)
        return 2;
    if (pAfter == 0)
        return 2;
    if (pData == 0)
        return 2;
    if (nLength == 0)
        return 2;
    if (func_00631E70(pAfter) == 0) {
        result = func_00631F30(pList, &nFree);
        if (result != 0)
            return result;
        if (nFree >= nLength) {
            pAfter->nLength = nLength;
            pAfter->pNext = 0;
            memcpy(pAfter->data, pData, nLength);
            pList->nEnd += (nLength + 3) & ~3;
            pList->nUsed += (nLength + 3) & ~3;
        } else
            return 3;
    } else {
        result = func_00631FD8(pList, &pAfter, nLength + 8);
        if (result != 0)
            return result;
        pMsg = (RT_LIST_MSG *)(pList->pBuf + pList->nEnd);
        pMsg->pNext = pAfter->pNext;
        pMsg->nLength = nLength;
        memcpy(pMsg->data, pData, nLength);
        pAfter->pNext = pMsg;
        pList->nEnd += ((nLength + 3) & ~3) + 8;
        pList->nUsed += ((nLength + 3) & ~3) + 8;
    }
    if (pList->nEnd == pList->nSize)
        pList->nEnd = 0;
    return 0;
}

/* a message stored at the head: after the first, then swapped in front of it */
int func_00631670(RT_LIST *pList, const void *pData, unsigned int nLength)
{
    RT_LIST_MSG *pFirst = 0;
    RT_LIST_MSG *pMsg;
    int result;

    if (func_00631EA0(pList) == 0)
        return 2;
    if (pData == 0)
        return 2;
    if (nLength == 0)
        return 2;
    result = func_00631B10(pList, &pFirst);
    if (result != 0)
        return result;
    result = func_00631508(pList, pFirst, pData, nLength);
    if (result != 0)
        return result;
    result = func_00631B10(pList, &pFirst);
    if (result != 0)
        return result;
    pMsg = func_00631E88(pFirst);
    if (pMsg != 0) {
        pList->pFirst = pMsg;
        pFirst->pNext = pMsg->pNext;
        pMsg->pNext = pFirst;
    }
    if (pList->nEnd == pList->nSize)
        pList->nEnd = 0;
    return 0;
}

/* a message removed: unlinked from the one before it (the first's successor made first), its
 * bytes given back, the start found again and the end moved to the last message's */
int func_00631748(RT_LIST *pList, RT_LIST_MSG *pMsg)
{
    RT_LIST_MSG *pPrev = 0;
    int result;

    if (func_00631EA0(pList) == 0)
        return 2;
    if (pMsg != pList->pFirst) {
        result = func_006320A8(pList, pMsg, &pPrev);
        if (result != 0)
            return result;
        pPrev->pNext = func_00631E88(pMsg);
        pList->nUsed = pList->nUsed - func_00631E70(pMsg) - 8;
    } else if (func_00631E88(pMsg) == 0) {
        pList->nStart = 0;
        pList->nEnd = 0;
        pList->nUsed = 0;
        result = func_00631ED0(pList);
        if (result != 0)
            return result;
    } else {
        pList->nUsed = pList->nUsed - func_00631E70(pMsg) - 8;
        pList->pFirst = func_00631E88(pMsg);
    }
    result = func_00632140(pList);
    if (result != 0)
        return result;
    return func_006321B8(pList);
}

int func_00631B10(RT_LIST *pList, RT_LIST_MSG **ppMsg)
{
    if (func_00631EA0(pList) == 0)
        return 2;
    *ppMsg = pList->pFirst;
    return 0;
}

int func_00631B60(RT_LIST *pList, RT_LIST_MSG **ppMsg)
{
    if (func_00631EA0(pList) == 0)
        return 2;
    *ppMsg = pList->pFirst;
    while (func_00631E88(*ppMsg) != 0)
        *ppMsg = func_00631E88(*ppMsg);
    return 0;
}

/* the list compacted: rebuilt from its own messages in a new buffer of the same size, the caller's
 * message pointer moved with its message */
int func_00631BC0(RT_LIST *pList, RT_LIST_MSG **ppMsg)
{
    RT_LIST old;
    RT_LIST_MSG *pLast = 0;
    RT_LIST_MSG *p;
    int result;

    if (func_00631EA0(pList) == 0)
        return 2;
    old = *pList;
    memset(pList, 0, sizeof(RT_LIST));
    result = func_006313D8(pList, old.nSize);
    if (result != 0) {
        *pList = old;
        return result;
    }
    for (p = old.pFirst; p != 0; p = func_00631E88(p)) {
        func_00631B60(pList, &pLast);
        func_00631508(pList, pLast, func_00631E58(p), func_00631E70(p));
        if (ppMsg != 0 && p == *ppMsg)
            func_00631B60(pList, ppMsg);
    }
    func_00631460(&old);
    return 0;
}

/* the list resized: as the compaction, into a buffer of the new size (not below the bytes used) */
int func_00631D18(RT_LIST *pList, unsigned int nSize)
{
    RT_LIST old;
    RT_LIST_MSG *pLast = 0;
    RT_LIST_MSG *p;
    int result;

    if (func_00631EA0(pList) == 0)
        return 2;
    if (nSize < pList->nUsed)
        return 1;
    old = *pList;
    memset(pList, 0, sizeof(RT_LIST));
    result = func_006313D8(pList, nSize);
    if (result != 0) {
        *pList = old;
        return result;
    }
    for (p = old.pFirst; p != 0; p = func_00631E88(p)) {
        func_00631B60(pList, &pLast);
        func_00631508(pList, pLast, func_00631E58(p), func_00631E70(p));
    }
    func_00631460(&old);
    return 0;
}

char *func_00631E58(RT_LIST_MSG *pMsg)
{
    if (pMsg == 0)
        return 0;
    return pMsg->data;
}

unsigned int func_00631E70(RT_LIST_MSG *pMsg)
{
    if (pMsg == 0)
        return 0;
    return pMsg->nLength;
}

RT_LIST_MSG *func_00631E88(RT_LIST_MSG *pMsg)
{
    if (pMsg == 0)
        return 0;
    return pMsg->pNext;
}

int func_00631EA0(RT_LIST *pList)
{
    if (pList == 0)
        return 0;
    if (pList->pBuf == 0)
        return 0;
    if (pList->pFirst == 0)
        return 0;
    return pList->nSize != 0;
}

unsigned char func_00631ED0(RT_LIST *pList)
{
    RT_LIST_MSG *pMsg = (RT_LIST_MSG *)(pList->pBuf + pList->nEnd);

    pMsg->nLength = 0;
    pMsg->pNext = 0;
    pList->pFirst = pMsg;
    pList->nEnd += 8;
    pList->nUsed += 8;
    return 0;
}

int func_00631F08(RT_LIST *pList, unsigned int *pnFree)
{
    if (pnFree == 0)
        return 2;
    *pnFree = pList->nSize - pList->nUsed;
    return 0;
}

int func_00631F30(RT_LIST *pList, unsigned int *pnFree)
{
    if (pnFree == 0)
        return 2;
    if (pList->nEnd > pList->nStart)
        *pnFree = pList->nSize - pList->nEnd;
    else if (pList->nEnd < pList->nStart)
        *pnFree = pList->nStart - pList->nEnd;
    else
        *pnFree = 0;
    return 0;
}

int func_00631F80(RT_LIST *pList, unsigned int *pnFree)
{
    if (pnFree == 0)
        return 2;
    if (pList->nStart < pList->nEnd)
        *pnFree = pList->nSize - pList->nEnd + pList->nStart;
    else if (pList->nEnd < pList->nStart)
        *pnFree = pList->nStart - pList->nEnd;
    else
        *pnFree = 0;
    return 0;
}

int func_00631FD8(RT_LIST *pList, RT_LIST_MSG **ppMsg, unsigned int nNeed)
{
    unsigned int nFreeEnd = 0;
    unsigned int nFree = 0;
    int result;

    result = func_00631F08(pList, &nFree);
    if (result != 0)
        return result;
    if (nFree < nNeed)
        return 3;
    result = func_00631F30(pList, &nFreeEnd);
    if (result != 0)
        return result;
    if (nFreeEnd < nNeed) {
        result = func_00631F80(pList, &nFree);
        if (result != 0)
            return result;
        if (nFree - nFreeEnd >= nNeed)
            pList->nEnd = 0;
        else {
            result = func_00631BC0(pList, ppMsg);
            if (result != 0)
                return result;
        }
    }
    return 0;
}

int func_006320A8(RT_LIST *pList, RT_LIST_MSG *pMsg, RT_LIST_MSG **ppPrev)
{
    RT_LIST_MSG *p = 0;
    int result;

    if (pMsg == 0)
        return 2;
    if (ppPrev == 0)
        return 2;
    *ppPrev = 0;
    result = func_00631B10(pList, &p);
    if (result != 0)
        return result;
    while (func_00631E88(p) != 0) {
        if (func_00631E88(p) == pMsg) {
            *ppPrev = p;
            return 0;
        }
        p = func_00631E88(p);
    }
    return 1;
}

int func_00632140(RT_LIST *pList)
{
    RT_LIST_MSG *p = 0;
    unsigned int nStart = pList->nSize;
    unsigned int nOffset;
    int result;

    result = func_00631B10(pList, &p);
    if (result != 0)
        return result;
    while (p != 0) {
        nOffset = (char *)p - pList->pBuf;
        if (nOffset < nStart)
            nStart = nOffset;
        p = func_00631E88(p);
    }
    pList->nStart = nStart;
    return 0;
}

/* the end found again: past the furthest message (an empty one's head), word-aligned, wrapped */
int func_006321B8(RT_LIST *pList)
{
    RT_LIST_MSG *p = 0;
    unsigned int nEnd = 0;
    int result;

    result = func_00631B10(pList, &p);
    if (result != 0)
        return result;
    while (p != 0) {
        if (func_00631E58(p) == 0) {
            if (nEnd < (char *)p - pList->pBuf + 8)
                nEnd = (char *)p - pList->pBuf + 8;
        } else if (nEnd < func_00631E58(p) + func_00631E70(p) - pList->pBuf)
            nEnd = func_00631E58(p) + func_00631E70(p) - pList->pBuf;
        p = func_00631E88(p);
    }
    pList->nEnd = (nEnd + 3) & ~3;
    if (pList->nEnd == pList->nSize)
        pList->nEnd = 0;
    return 0;
}
