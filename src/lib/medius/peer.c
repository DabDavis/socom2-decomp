/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* peer.c: the voice library's peer records and member lists (OVL2 0x629DA0-0x62A3B0), the "list
 * object at 0x629DA0" audio.c and grid.c call (round 9b), prebuilt with ee-gcc 2.96 at -O2 -G0
 * (README "The GCC front"), sibling calls on (0x629E40 ends in `j`). The object has no `$Header`
 * and no SOCOM 1 counterpart: it links after md5.c and before the RC cipher, so its file's name
 * sorts between theirs -- this one says what it holds; every function name is a placeholder.
 *
 * Two rings: audio.c's peers (0x860-byte records, the ring through each record's first two
 * words, found by the id in the peer's self-description), and grid.c's member lists (a count and
 * a ring of 0x1C-byte nodes, each holding a peer). A ring walk stops when it comes back to the
 * head, the next pointer being the second word. */

extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);

typedef struct PEER_INFO {              /* what a peer tells of itself (32 bytes) */
    int nAddr;                          /* 0x00 */
    int nAddr2;                         /* 0x04 */
    int nPort;                          /* 0x08 */
    int nPort2;                         /* 0x0C */
    int nGrid;                          /* 0x10: the group it talks in */
    int nId;                            /* 0x14 */
    unsigned int nFlags;                /* 0x18 */
    int x1C;                            /* 0x1C */
} PEER_INFO;

typedef struct PEER_SEQ {               /* a message seen, by its header (8 bytes) */
    unsigned char nType;
    unsigned char nGrid;
    unsigned short nLen;
    unsigned short nSrc;                /* 0x04: kept: the value answered for a repeat */
    unsigned short nSeq;                /* 0x06 */
} PEER_SEQ;

typedef struct PEER {
    struct PEER *pPrev;                 /* 0x00 */
    struct PEER *pNext;                 /* 0x04 */
    void *pAudio;                       /* 0x08: audio.c's session */
    unsigned int nState;                /* 0x0C */
    unsigned int tSent;                 /* 0x10 */
    unsigned int nTries;                /* 0x14 */
    int x18;                            /* 0x18 */
    int tHeard;                         /* 0x1C */
    PEER_INFO info;                     /* 0x20 */
    PEER_SEQ aSeen[256];                /* 0x40 */
    void *pFrame;                       /* 0x840 */
    void *pStream;                      /* 0x844 */
    void *pIn;                          /* 0x848: the codec's decoder */
    void *pOut;                         /* 0x84C */
    unsigned int nFlags;                /* 0x850 */
    int x854;
    int x858;
    int x85C;
} PEER;                                 /* 0x860 */

typedef struct LIST_NODE {
    struct LIST_NODE *pPrev;            /* 0x00 */
    struct LIST_NODE *pNext;            /* 0x04 */
    PEER *pPeer;                        /* 0x08 */
    int anId[4];                        /* 0x0C: 0xFFFF each */
} LIST_NODE;                            /* 0x1C */

typedef struct LIST {
    int nCount;                         /* 0x00 */
    LIST_NODE *pHead;                   /* 0x04 */
} LIST;

extern void func_00645700(void *p);
extern void func_00645268(void *p);
extern void *func_0060FFB0(void *pAudio);
extern void func_0062B090(void *pCodec, void *pDecoder);
extern void func_0062AE10(void *pCodec, void *pEncoder);
extern void *func_00611388(void *pAudio, int nGrid);
extern void func_00626788(void *pGrid, int nId);
extern void func_00626658(void *pGrid, PEER *pPeer);

LIST_NODE *func_00629DA0(PEER *pPeer)
{
    LIST_NODE *pNode = 0;
    unsigned int i;

    rt_memory_allocate_init_zero((void **)&pNode, sizeof(LIST_NODE));
    if (pNode != 0) {
        pNode->pPeer = pPeer;
        pNode->pPrev = pNode;
        pNode->pNext = pNode;
        for (i = 0; i < 4; i++)
            pNode->anId[i] = 0xFFFF;
    }
    return pNode;
}

void func_00629E18(LIST_NODE *pNode)
{
    if (pNode != 0)
        rt_memory_free((void **)&pNode);
}

PEER *func_00629EB8(PEER *pHead, int nId);
void func_00629FF0(PEER *pPeer);

void func_00629E40(PEER **ppHead, int nId)
{
    PEER *pPeer = func_00629EB8(*ppHead, nId);

    if (pPeer == 0)
        return;
    if (pPeer == *ppHead)
        *ppHead = (pPeer == pPeer->pNext) ? 0 : pPeer->pNext;
    pPeer->pPrev->pNext = pPeer->pNext;
    pPeer->pNext->pPrev = pPeer->pPrev;
    func_00629FF0(pPeer);
}

PEER *func_00629EB8(PEER *pHead, int nId)
{
    PEER *pPeer;

    for (pPeer = pHead; pPeer != 0; ) {
        if (pPeer->info.nId == nId)
            break;
        pPeer = pPeer->pNext;
        if (pPeer == pHead)
            pPeer = 0;
    }
    return pPeer;
}

int func_00629EE8(PEER *pHead)
{
    PEER *pPeer;
    int n = 0;

    for (pPeer = pHead; pPeer != 0; n++) {
        pPeer = pPeer->pNext;
        if (pPeer == pHead)
            pPeer = 0;
    }
    return n;
}

PEER *func_00629F20(void *pAudio)
{
    PEER *pPeer = 0;

    rt_memory_allocate_init_zero((void **)&pPeer, sizeof(PEER));
    if (pPeer != 0) {
        pPeer->pNext = pPeer->pPrev = pPeer;
        pPeer->pAudio = pAudio;
        pPeer->x18 = 0;
    }
    return pPeer;
}

PEER *func_00629F78(void *pAudio, PEER_INFO *pInfo)
{
    PEER *pPeer = func_00629F20(pAudio);

    if (pPeer != 0) {
        pPeer->info = *pInfo;
        pPeer->nState = 0;
    }
    return pPeer;
}

void func_00629FF0(PEER *pPeer)
{
    if (pPeer == 0)
        return;
    if (pPeer->pFrame != 0) {
        func_00645700(pPeer->pFrame);
        pPeer->pFrame = 0;
    }
    if (pPeer->pStream != 0) {
        func_00645268(pPeer->pStream);
        pPeer->pStream = 0;
    }
    if (pPeer->pIn != 0) {
        func_0062B090(func_0060FFB0(pPeer->pAudio), pPeer->pIn);
        pPeer->pIn = 0;
    }
    if (pPeer->pOut != 0) {
        func_0062AE10(func_0060FFB0(pPeer->pAudio), pPeer->pOut);
        pPeer->pOut = 0;
    }
    rt_memory_free((void **)&pPeer);
}

void func_0062A0A8(PEER *pPeer, int nGrid)
{
    if (pPeer->info.nGrid != nGrid) {
        if (pPeer->info.nGrid > 0)
            func_00626788(func_00611388(pPeer->pAudio, pPeer->info.nGrid), pPeer->info.nId);
        pPeer->info.nGrid = nGrid;
        if (nGrid > 0)
            func_00626658(func_00611388(pPeer->pAudio, nGrid), pPeer);
    }
}

unsigned int func_0062A130(PEER *pPeer)
{
    return pPeer->nFlags;
}

void func_0062A138(PEER *pPeer, unsigned int nFlags)
{
    pPeer->nFlags |= nFlags;
}

void func_0062A148(PEER *pPeer, unsigned int nFlags)
{
    pPeer->nFlags &= ~nFlags;
}

/* a message's sequence number seen before answers what was kept for it; a new one is kept */
unsigned short func_0062A160(PEER *pPeer, PEER_SEQ *pSeq, int nSrc)
{
    PEER_SEQ *pSeen = &pPeer->aSeen[pSeq->nSeq & 0xFF];

    if (pSeen->nSeq == pSeq->nSeq)
        return pSeen->nSrc;
    *pSeen = *pSeq;
    pSeen->nSrc = nSrc;
    return 0xFFFF;
}

LIST *func_0062A1A8(void)
{
    LIST *pList = 0;

    rt_memory_allocate_init_zero((void **)&pList, sizeof(LIST));
    if (pList != 0) {
        pList->nCount = 0;
        pList->pHead = 0;
    }
    return pList;
}

void func_0062A360(LIST *pList);

void func_0062A1E8(LIST *pList)
{
    if (pList != 0) {
        func_0062A360(pList);
        rt_memory_free((void **)&pList);
    }
}

LIST_NODE *func_0062A298(LIST *pList, int nId);

void func_0062A218(LIST *pList, PEER *pPeer)
{
    LIST_NODE *pNode;
    LIST_NODE *pHead;

    if (func_0062A298(pList, pPeer->info.nId) != 0)
        return;
    pNode = func_00629DA0(pPeer);
    pHead = pList->pHead;
    if (pHead == 0) {
        pList->pHead = pNode;
        pHead = pNode;
    }
    pNode->pNext = pHead;
    pList->nCount++;
    pNode->pPrev = pList->pHead->pPrev;
    pHead->pPrev = pNode;
    pNode->pPrev->pNext = pNode;
}

LIST_NODE *func_0062A298(LIST *pList, int nId)
{
    LIST_NODE *pNode = pList->pHead;

    if (pNode != 0) {
        LIST_NODE *pHead = pNode;

        do {
            if (pNode->pPeer->info.nId == nId)
                return pNode;
            pNode = pNode->pNext;
            if (pNode == pHead)
                pNode = 0;
        } while (pNode != 0);
    }
    return 0;
}

LIST_NODE *func_0062A2E0(LIST *pList)
{
    return pList->pHead;
}

int func_0062A2E8(LIST *pList)
{
    return pList->nCount;
}

void func_0062A2F0(LIST *pList, int nId)
{
    LIST_NODE *pNode = func_0062A298(pList, nId);

    if (pNode == 0)
        return;
    if (pNode == pList->pHead)
        pList->pHead = (pNode == pNode->pNext) ? 0 : pNode->pNext;
    pNode->pPrev->pNext = pNode->pNext;
    pNode->pNext->pPrev = pNode->pPrev;
    func_00629E18(pNode);
    pList->nCount--;
}

void func_0062A360(LIST *pList)
{
    while (pList->pHead != 0)
        func_0062A2F0(pList, pList->pHead->pPeer->info.nId);
}
