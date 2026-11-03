/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* audio.c: the voice library's session (OVL2 0x60FF00-0x6126A0), SCEA's rt_audio
 * (/projects/rtime/CVS/rt_audio/src/audio.c,v 1.67 2003/10/10, its $Header and audio.h's first in
 * the libraries' .rodata), prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"); built
 * with sibling calls (0x6113F8 and 0x611388 end in `j` under a vestigial frame). SOCOM 1 has no
 * voice library: every name here is a placeholder or ours.
 *
 * The session (0xD8 bytes, allocated zeroed) holds three message counters, the codec's context,
 * the groups (grid.c's ring), the peers (the list object at 0x629DA0, each record's id at 0x34),
 * its flags, this client's id and the group it talks in, the UDP socket and six handles. */

extern void *memset(void *p, int c, unsigned int n);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);

typedef struct GRID GRID;               /* grid.c's group */

typedef struct GRID_HEADER {            /* grid.c's message header */
    unsigned char nType;                /* 0x00: the type, 0x80 set */
    unsigned char nGrid;                /* 0x01 */
    unsigned short nLen;                /* 0x02 */
    unsigned short nSrc;                /* 0x04 */
    unsigned short nSeq;                /* 0x06 */
} GRID_HEADER;

typedef struct GRID_ENTRY {             /* a group as grid.c reports it */
    unsigned short nId;                 /* 0x00 */
    short nQueued;                      /* 0x02 */
    char szName[60];                    /* 0x04 */
} GRID_ENTRY;                           /* 0x40 */

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

typedef struct PEER {                   /* a peer's record (the list object at 0x629DA0) */
    struct PEER *pNext;                 /* 0x00 */
    struct PEER *pPrev;                 /* 0x04 */
    int x08;                            /* 0x08 */
    unsigned int nState;                /* 0x0C */
    char x10[0x10];                     /* 0x10 */
    PEER_INFO info;                     /* 0x20 */
    char x40[0x808];                    /* 0x40 */
    void *pIn;                          /* 0x848 */
    void *pOut;                         /* 0x84C */
} PEER;

typedef struct AUDIO {
    int nSeq;                           /* 0x00: every message's sequence number */
    int nSeqA;                          /* 0x04: type 0x2F's */
    int nSeqB;                          /* 0x08: type 0x30's */
    char x0C[0x3C];                     /* 0x0C */
    int (*pfnRead)(struct AUDIO *pAudio, int *pnType, char *pBuf, int nSize); /* 0x48 */
    char x4C[0x0C];                     /* 0x4C */
    int nLastRead;                      /* 0x58 */
    void *pCodec;                       /* 0x5C */
    void *pFrame;                       /* 0x60: the codec's, while talking */
    GRID *pGrids;                       /* 0x64 */
    PEER *pSelf;                     /* 0x68 */
    PEER *pPeers;                    /* 0x6C */
    unsigned int nFlags;                /* 0x70 */
    int x74;                            /* 0x74 */
    int nId;                            /* 0x78: this client's */
    char x7C[0x10];                     /* 0x7C */
    void *pSocket;                      /* 0x8C */
    int x90;                            /* 0x90 */
    int x94;                            /* 0x94 */
    int nGrid;                          /* 0x98: the group spoken in, if positive */
    char x9C[8];                        /* 0x9C */
    void *pHandles[6];                  /* 0xA4 */
    int bConnecting;                    /* 0xBC */
    char szHost[16];                    /* 0xC0 */
    int nPort;                          /* 0xD0 */
    int bConnected;                     /* 0xD4 */
} AUDIO;                                /* 0xD8 */

extern GRID *func_00624570(AUDIO *pAudio, GRID *pList, int nId, int nMaxMembers);
extern void func_00624728(GRID *p);
extern GRID *func_00624780(GRID *p);
extern GRID *func_00624788(GRID *p);
extern GRID *func_00624790(GRID *pList, int nId);
extern void func_006247C8(GRID *p);
extern void func_00624B88(GRID *p, GRID_ENTRY *pEntry);
extern void func_00624BE0(GRID *p, GRID_HEADER *h, int nType, int nLen);
extern int func_00626110(GRID *p);
extern int func_006264C0(GRID *p, void *p1, int n2);
extern void func_00626410(GRID *p);
extern void func_00626A38(GRID *p);

extern PEER *func_00629EB8(PEER *pList, int nId);
extern void func_00629E40(PEER **ppList, int nId);
extern void func_00626788(GRID *p, int nId);
extern void func_0062A8C8(void *pCodec);
extern void func_0062B090(void *pCodec, void *pIn);
extern void func_0062AE10(void *pCodec, void *pFrame);
extern void func_0063DEA0(void *pSocket);
extern void func_006456D8(void *pHandle);

void func_00610318(AUDIO *pAudio);
int func_00610C20(AUDIO *pAudio, int nFrom, int nPort, char *pBuf, int nLen);
int func_00610FA0(AUDIO *pAudio, int nTo, GRID_HEADER *h, void *pData, int nLen);
int func_00611250(AUDIO *pAudio, char *pBuf, int nSize, int *pnFrom, int *pnPort);
GRID *func_00611388(AUDIO *pAudio, int nId);
void func_00611A40(AUDIO *pAudio, int nGrid);
void func_00612138(AUDIO *pAudio, int nId);
GRID *func_006113A0(AUDIO *pAudio, GRID *p);

AUDIO *func_0060FF00(void)
{
    AUDIO *p = 0;

    rt_memory_allocate_init_zero((void **)&p, sizeof(AUDIO));
    if (p != 0) {
        memset(p, 0, sizeof(AUDIO));
        p->x90 = 0;
        p->x94 = 0;
        p->bConnecting = 0;
        p->bConnected = 0;
        p->x74 = 0;
    }
    return p;
}

void func_0060FF70(AUDIO *pAudio)
{
    if (pAudio->pFrame != 0) {
        func_0062AE10(pAudio->pCodec, pAudio->pFrame);
        pAudio->pFrame = 0;
        pAudio->x90 = 0;
    }
    func_00610318(pAudio);
}

void *func_0060FFB0(AUDIO *pAudio)
{
    return pAudio->pCodec;
}

void func_0060FFB8(AUDIO *pAudio)
{
    pAudio->nSeqA = 0;
}

void func_0060FFC0(AUDIO *pAudio)
{
    pAudio->nSeqB = 0;
}

int func_0060FFC8(AUDIO *pAudio)
{
    return pAudio->nSeqA++;
}

int func_0060FFE0(AUDIO *pAudio)
{
    return pAudio->nSeqB++;
}

int func_0060FFF8(AUDIO *pAudio)
{
    return pAudio->nSeq++;
}

void func_00610318(AUDIO *pAudio)
{
    if (pAudio != 0) {
        while (pAudio->pGrids != 0)
            func_00624728(func_006113A0(pAudio, func_00624780(pAudio->pGrids)));
        while (pAudio->pPeers != 0)
            func_00612138(pAudio, pAudio->pPeers->info.nId);
        if (pAudio->pCodec != 0) {
            func_0062A8C8(pAudio->pCodec);
            pAudio->pCodec = 0;
        }
        if (pAudio->pSocket != 0) {
            func_0063DEA0(pAudio->pSocket);
            pAudio->pSocket = 0;
        }
        if (pAudio->pHandles[0] != 0) {
            func_006456D8(pAudio->pHandles[0]);
            pAudio->pHandles[0] = 0;
        }
        if (pAudio->pHandles[1] != 0) {
            func_006456D8(pAudio->pHandles[1]);
            pAudio->pHandles[1] = 0;
        }
        if (pAudio->pHandles[2] != 0) {
            func_006456D8(pAudio->pHandles[2]);
            pAudio->pHandles[2] = 0;
        }
        if (pAudio->pHandles[3] != 0) {
            func_006456D8(pAudio->pHandles[3]);
            pAudio->pHandles[3] = 0;
        }
        if (pAudio->pHandles[4] != 0) {
            func_006456D8(pAudio->pHandles[4]);
            pAudio->pHandles[4] = 0;
        }
        if (pAudio->pHandles[5] != 0) {
            func_006456D8(pAudio->pHandles[5]);
            pAudio->pHandles[5] = 0;
        }
        pAudio->nFlags = 0;
        rt_memory_free((void **)&pAudio);
    }
}

void func_00610598(AUDIO *pAudio)
{
    if (pAudio->nGrid > 0)
        func_00611A40(pAudio, pAudio->nGrid);
    if (pAudio->pGrids != 0)
        func_00626410(pAudio->pGrids);
    pAudio->nFlags &= ~2;
}

int func_006105F0(AUDIO *pAudio)
{
    char buf[1280];
    int nFrom;
    int nPort = 0;
    int n;
    int result = 0;

    for (;;) {
        n = func_00611250(pAudio, buf, sizeof(buf), &nFrom, &nPort);
        if (n < 0) {
            result = -1;
            break;
        }
        if (n == 0)
            break;
        if (func_00610C20(pAudio, nFrom, nPort, buf, n) != 0) {
            result = -1;
            break;
        }
    }
    return result;
}

GRID *func_00611348(AUDIO *pAudio, int nId, int nMaxMembers)
{
    GRID *p = func_00624570(pAudio, pAudio->pGrids, nId, nMaxMembers);

    if (pAudio->pGrids == 0)
        pAudio->pGrids = p;
    return p;
}

GRID *func_00611388(AUDIO *pAudio, int nId)
{
    return func_00624790(pAudio->pGrids, nId);
}

GRID *func_006113A0(AUDIO *pAudio, GRID *p)
{
    if (p != 0) {
        if (p == pAudio->pGrids)
            pAudio->pGrids = 0;
        else
            func_006247C8(p);
    }
    return p;
}

PEER *func_006113E8(AUDIO *pAudio)
{
    return pAudio->pSelf;
}

PEER *func_006113F0(AUDIO *pAudio)
{
    return pAudio->pPeers;
}

PEER *func_006113F8(AUDIO *pAudio, int nId)
{
    return func_00629EB8(pAudio->pPeers, nId);
}

void func_00611410(AUDIO *pAudio, unsigned int nFlags)
{
    pAudio->nFlags |= nFlags;
}

void func_00611420(AUDIO *pAudio, unsigned int nFlags)
{
    pAudio->nFlags &= ~nFlags;
}

/* 0x611438, 0x6115C0, 0x6117F0: not written */

/* The group spoken in, if any (and, while not talking, if it takes the voice). */
GRID *func_00611898(AUDIO *pAudio)
{
    GRID *p = 0;

    if (pAudio->nGrid > 0) {
        p = func_00611388(pAudio, pAudio->nGrid);
        if (p != 0 && pAudio->pFrame == 0) {
            if (func_00626110(p) != 0)
                p = 0;
        }
    }
    return p;
}

GRID *func_00611900(AUDIO *pAudio)
{
    GRID *p = 0;

    if (pAudio->nGrid > 0)
        p = func_00611388(pAudio, pAudio->nGrid);
    return p;
}

/* 0x611938, 0x611A40: not written */

/* Each group reported to the callback until it returns 0. */
int func_00611B08(AUDIO *pAudio, int (*pfn)(AUDIO *pAudio, void *pUser, GRID_ENTRY *pEntry), void *pUser)
{
    GRID_ENTRY entry;
    GRID *p = pAudio->pGrids;
    int result = -1;

    if (pfn != 0) {
        while (p != 0) {
            func_00624B88(p, &entry);
            if (pfn(pAudio, pUser, &entry) == 0)
                break;
            p = func_00624788(p);
            if (p == pAudio->pGrids)
                p = 0;
        }
        result = 0;
    }
    return result;
}

/* 0x611BA0, 0x611CB0: not written */

int func_00611DE0(AUDIO *pAudio, void *p1, int n2, int nGrid)
{
    GRID *p = func_00611388(pAudio, nGrid);

    if (p1 == 0 || p == 0)
        return -1;
    return func_006264C0(p, p1, n2);
}

int func_00611E48(AUDIO *pAudio, int nId)
{
    pAudio->nId = nId;
    pAudio->pSelf->info.nId = nId;
    return 0;
}

int func_00611E60(AUDIO *pAudio, int *pnId)
{
    *pnId = pAudio->nId;
    return 0;
}

int func_00611E70(AUDIO *pAudio)
{
    return pAudio->nGrid;
}

GRID *func_00611E78(AUDIO *pAudio)
{
    return pAudio->pGrids;
}

int func_00611E80(AUDIO *pAudio, int nId, PEER_INFO *pInfo)
{
    if (func_006113F8(pAudio, nId) == 0)
        return -1;
    memcpy(pInfo, &pAudio->pSelf->info, sizeof(PEER_INFO));
    return 0;
}

/* 0x611F08: not written */

int func_00611FB0(AUDIO *pAudio, int x)
{
    if (pAudio->x74 != 0 && x == 0) {
        if (pAudio->nGrid > 0)
            func_00626A38(func_00611388(pAudio, pAudio->nGrid));
    }
    pAudio->x74 = x;
    return 0;
}

/* 0x612010: not written */

void func_00612138(AUDIO *pAudio, int nId)
{
    PEER *p = func_00629EB8(pAudio->pPeers, nId);
    GRID *g;

    if (p == 0)
        return;
    if (p->info.nGrid > 0) {
        g = func_00611388(pAudio, p->info.nGrid);
        if (g != 0)
            func_00626788(g, p->info.nId);
    }
    if (pAudio->pGrids != 0)
        func_00626788(pAudio->pGrids, p->info.nId);
    if (p->pIn != 0) {
        func_0062B090(func_0060FFB0(pAudio), p->pIn);
        p->pIn = 0;
    }
    if (p->pOut != 0) {
        func_0062AE10(func_0060FFB0(pAudio), p->pOut);
        p->pOut = 0;
    }
    func_00629E40(&pAudio->pPeers, p->info.nId);
}

unsigned int func_00612210(AUDIO *pAudio)
{
    if (pAudio->nGrid > 0)
        func_00611388(pAudio, pAudio->nGrid);
    if (func_00611898(pAudio) != 0)
        pAudio->nFlags |= 8;
    else
        pAudio->nFlags &= ~8;
    return pAudio->nFlags;
}

/* 0x612270, 0x612460, 0x612518: not written */

void func_006125D0(AUDIO *pAudio, short n1, int nTo, short n2)
{
    GRID_HEADER h;
    short data[2];

    data[0] = n1;
    data[1] = n2;
    func_00624BE0(pAudio->pGrids, &h, 0x12, sizeof(data));
    func_00610FA0(pAudio, nTo, &h, data, sizeof(data));
}

void func_00612630(AUDIO *pAudio, int nTo)
{
    GRID_HEADER h;
    unsigned short nGrid;

    nGrid = pAudio->nGrid;
    h.nType = 0x81;
    h.nLen = sizeof(nGrid);
    h.nSeq = func_0060FFF8(pAudio);
    h.nSrc = pAudio->nId;
    h.nGrid = 0;
    func_00610FA0(pAudio, nTo, &h, &nGrid, sizeof(nGrid));
}
