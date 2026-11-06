/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* grid.c: the voice library's groups (OVL2 0x6244B0-), SCEA's rt_audio
 * (/projects/rtime/CVS/rt_audio/src/grid.c,v 1.48, its $Header and grid.h's in .rodata after
 * Dme.c's), prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). SOCOM 1 has no voice
 * library, so every name here is a placeholder or ours.
 *
 * Where the file starts: grid.c's own data is first referenced at 0x6250D0, but the functions
 * from Dme.c's end (0x6244B0) up to it are this file's too -- every rt_audio source keeps its
 * $Header (audio.c, codec.c, grid.c, rt_audio.c, each with its headers'), and none sits between
 * Dme.c's and grid.c's; the one static between Dme.c's and LargeInt.c's .data is the random
 * seed below; and these functions and grid.c's call each other both ways (0x624DC0 calls
 * 0x626398 and 0x626788, 0x625690 calls the generator and five of the group functions).
 *
 * A group (0x84 bytes, allocated zeroed) is a ring through pNext/pPrev that audio.c keeps; it
 * has an id, a 64-byte name, a list of queued items (the list object at 0x629DA0) and a table
 * of members -- nMaxMembers of them, or 32 that double when full (nMaxMembers 0). A member is a
 * client id (0xFFFF free), audio.c's peer record for it, a state and a time. */

extern void *memset(void *p, int c, unsigned int n);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern char *strncpy(char *d, const char *s, unsigned int n);
extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);
extern unsigned int func_0062C070(void);

typedef struct RT_LIST_NODE {           /* the list object at 0x629DA0: a ring */
    struct RT_LIST_NODE *pNext;         /* 0x00 */
    struct RT_LIST_NODE *pPrev;         /* 0x04 */
    void *pData;                        /* 0x08: audio.c's peer record */
    unsigned int nIds[4];               /* 0x0C: 0xFFFF when free */
} RT_LIST_NODE;                         /* 0x1C */

typedef struct RT_LIST {
    int nCount;                         /* 0x00 */
    RT_LIST_NODE *pHead;                /* 0x04 */
} RT_LIST;

typedef struct GRID_PEER {              /* audio.c's peer record, as far as grid.c reads it */
    struct GRID_PEER *pNext;            /* 0x00 */
    struct GRID_PEER *pPrev;            /* 0x04 */
    void *pAudio;                       /* 0x08 */
    char x0C[0x10];                     /* 0x0C */
    int tHeard;                         /* 0x1C */
    char x20[0x10];                     /* 0x20 */
    int nGrid;                          /* 0x30 */
    int nId;                            /* 0x34 */
    unsigned int nFlags;                /* 0x38 */
    int x3C;                            /* 0x3C: the clock's offset, averaged */
    char x40[0x804];                    /* 0x40 */
    void *pStream;                      /* 0x844 */
    void *pIn;                          /* 0x848 */
    void *pOut;                         /* 0x84C */
    int x850;                           /* 0x850 */
    int x854;                           /* 0x854: the sequence number next wanted */
    int nPending;                       /* 0x858: a frame received, its length */
    void *pPending;                     /* 0x85C */
} GRID_PEER;

extern int func_0062A2E8(RT_LIST *pList);   /* the count */
extern void func_0062A360(RT_LIST *pList);  /* frees every entry */
extern RT_LIST_NODE *func_0062A298(RT_LIST *pList, unsigned int nId); /* the entry for an id */
extern void func_0062A2F0(RT_LIST *pList, unsigned int nId); /* removes one */

/* audio.c's */
extern int func_0060FFF8(void *pAudio);                 /* the next sequence number */
extern void *func_006113F8(void *pAudio, unsigned int nId); /* the peer record for an id */
extern int func_00611E60(void *pAudio, int *pnId);      /* this client's id */
extern int func_00611E70(void *pAudio);                 /* the group spoken in */
extern void *func_0060FFB0(void *pAudio);               /* the codec */
extern int func_0062AAF8(void *pCodec);
extern int func_0062AB08(void *pCodec);
extern struct GRID *func_00611E78(void *pAudio);        /* the groups */
extern int func_00610FA0(void *pAudio, int nTo, void *h, void *pData, int nLen); /* to one peer */
extern int func_0060FFC8(void *pAudio);                 /* type 0x2F's sequence number */
extern int func_0060FFE0(void *pAudio);                 /* type 0x30's */
extern int func_00611180(void *pAudio, char *pBuf, int nLen, void *pPeer);
extern void *func_006113E8(void *pAudio);               /* the peers' sentinel */
extern void *func_006113F0(void *pAudio);               /* the peers */
extern void func_00611410(void *pAudio, unsigned int nFlags);
extern void func_00611420(void *pAudio, unsigned int nFlags);
extern void func_00611938(void *pAudio, int nGrid);
extern void func_00612138(void *pAudio, unsigned int nId);
extern unsigned int func_0062A130(void *pPeer);         /* peer.c: the peer's flags */
extern void *func_006451F0(void);                       /* rt_frag.c: a fragment buffer */
extern void func_00645290(void *pStream, int x, char *pData, int nLen);
extern int func_00645380(void *pStream);
extern int rt_memory_allocate(void **ppMemory, unsigned long nSize);
extern void *func_0062AF40(void *pCodec);               /* voicebuf.c: a stream */
extern void func_00642A58(void *pIn, int nType, int x, char *pData, int nLen); /* streammedia.c */
extern int func_0062A138(void *pPeer, unsigned int nFlags); /* int here: the block after the call keeps v0 for the return (0x6253B8) */
extern void func_0062A148(void *pPeer, unsigned int nFlags);

typedef struct GRID_MEMBER {
    unsigned int nId;                   /* 0x00: the client's id, 0xFFFF when free */
    void *pPeer;                        /* 0x04: audio.c's record for it */
    int nState;                         /* 0x08 */
    int nTime;                          /* 0x0C */
    unsigned int nTries;                /* 0x10 */
} GRID_MEMBER;                          /* 0x14 */

typedef struct GRID_BODY {              /* a group message's body */
    unsigned short x0;                  /* 0x00 */
    unsigned short x2;                  /* 0x02 */
    unsigned short x4;                  /* 0x04 */
    unsigned short x6;                  /* 0x06 */
    unsigned int nId;                   /* 0x08 */
} GRID_BODY;                            /* 0x0C */

typedef struct GRID {
    struct GRID *pNext;                 /* 0x00 */
    struct GRID *pPrev;                 /* 0x04 */
    void *pAudio;                       /* 0x08 */
    int nId;                            /* 0x0C */
    char szName[64];                    /* 0x10 */
    RT_LIST list;                       /* 0x50 */
    int nMaxMembers;                    /* 0x58: 0 = 32 to start, doubling */
    int nMembers;                       /* 0x5C: the table's slots */
    GRID_MEMBER *pMembers;              /* 0x60 */
    int x64;                            /* 0x64 */
    int x68;                            /* 0x68 */
    unsigned int nTime;                 /* 0x6C: created */
    int x70;                            /* 0x70 */
    unsigned int x74;                   /* 0x74 */
    GRID_BODY x78;                      /* 0x78 */
} GRID;                                 /* 0x84 */

typedef struct GRID_HEADER {            /* a group message's header */
    unsigned char nType;                /* 0x00: the type, 0x80 set */
    unsigned char nGrid;                /* 0x01 */
    unsigned short nLen;                /* 0x02 */
    unsigned short nSrc;                /* 0x04 */
    unsigned short nSeq;                /* 0x06 */
} GRID_HEADER;

typedef struct GRID_INFO {
    unsigned short nSize;               /* 0x00: sizeof (GRID_INFO) */
    unsigned short nId;                 /* 0x02 */
    unsigned short nMaxMembers;         /* 0x04 */
    unsigned short nMaxMembers2;        /* 0x06 */
    char szName[60];                    /* 0x08 */
} GRID_INFO;                            /* 0x44 */

typedef struct GRID_ENTRY {             /* a group as a list reports it */
    unsigned short nId;                 /* 0x00 */
    short nQueued;                      /* 0x02 */
    char szName[60];                    /* 0x04 */
} GRID_ENTRY;                           /* 0x40 */

int func_00624830(GRID *p, int nSrc, GRID_HEADER *h, void *pData, int nLen);
void func_00626140(GRID *p);
void func_00626A38(GRID *p);
int func_00626398(GRID *p);
void func_00626788(GRID *p, unsigned int nId);
int func_00626240(GRID *p, unsigned int nTo, int bForce);
void func_00626510(GRID_MEMBER *m);
void func_00626658(GRID *p, GRID_PEER *pPeer);
void func_00626948(GRID *p, unsigned int nId);
void func_006269A8(GRID *p, unsigned int nId, int nPeer);
extern void func_0062A0A8(void *pPeer, int nGrid);
extern GRID *func_00611388(void *pAudio, int nId);
extern GRID *func_00611348(void *pAudio, int nId, int nMaxMembers);
extern void func_00612630(void *pAudio, unsigned int nTo);
extern void func_0062AE10(void *pCodec, void *pFrame);
extern void func_0062B090(void *pCodec, void *pIn);

static int nSeed = 0;

/* 0x6244B0: the C library's LCG (214013, 2531011), seeded from the clock on first use. */
int func_006244B0(void)
{
    if (nSeed == 0)
        nSeed = func_0062C070();
    nSeed = nSeed * 214013 + 2531011;
    return (nSeed >> 16) & 0x7FFF;
}

void func_00624510(GRID_MEMBER *m)
{
    m->nId = 0xFFFF;
    m->nState = 0;
    m->pPeer = 0;
    m->nTime = 0;
    m->nTries = 0;
}

void func_00624530(void *pAudio, GRID_MEMBER *m, int nState, unsigned int nId)
{
    m->nState = nState;
    m->nId = nId;
    m->pPeer = func_006113F8(pAudio, nId);
    m->nTime = func_0062C070();
    m->nTries = 0;
}

GRID *func_00624570(void *pAudio, GRID *pList, int nId, int nMaxMembers)
{
    GRID *p = 0;
    unsigned int now;
    int i;

    rt_memory_allocate_init_zero((void **)&p, sizeof(GRID));
    if (p != 0) {
        now = func_0062C070();
        p->pAudio = pAudio;
        p->nId = nId;
        p->pNext = 0;
        p->pPrev = 0;
        p->x64 = 0;
        p->x70 = 0;
        p->nTime = now;
        p->x74 = 0;
        p->x68 = 0;
        p->szName[0] = 0;
        p->nMaxMembers = nMaxMembers;
        p->nMembers = p->nMaxMembers ? p->nMaxMembers : 32;
        rt_memory_allocate_init_zero((void **)&p->pMembers, p->nMembers * sizeof(GRID_MEMBER));
        for (i = 0; i < p->nMembers; i++)
            p->pMembers[i].nTime = now;
        memset(&p->x78, 0, sizeof(p->x78));
        p->list.pHead = 0;
        p->list.nCount = 0;
        if (pList != 0) {
            p->pPrev = pList;
            p->pNext = pList->pNext;
            p->pPrev->pNext = p;
            p->pNext->pPrev = p;
        } else {
            p->pPrev = p;
            p->pNext = p;
        }
    }
    return p;
}

void func_00624728(GRID *p)
{
    if (p != 0) {
        func_0062A360(&p->list);
        if (p->pMembers != 0) {
            rt_memory_free((void **)&p->pMembers);
            p->pMembers = 0;
            p->nMembers = 0;
        }
        rt_memory_free((void **)&p);
    }
}

GRID *func_00624780(GRID *p)
{
    return p->pNext;
}

GRID *func_00624788(GRID *p)
{
    return p->pPrev;
}

GRID *func_00624790(GRID *pList, int nId)
{
    GRID *p = pList;

    while (p != 0) {
        if (p->nId == nId)
            break;
        p = p->pPrev;
        if (p == pList) {
            p = 0;
            break;
        }
    }
    return p;
}

void func_006247C8(GRID *p)
{
    p->pNext->pPrev = p->pPrev;
    p->pPrev->pNext = p->pNext;
}

RT_LIST_NODE *func_006247E8(GRID *p)
{
    return p->list.pHead;
}

void func_006247F0(GRID *p, const char *szName)
{
    strncpy(p->szName, szName, sizeof(p->szName));
    p->szName[sizeof(p->szName) - 1] = 0;
}

char *func_00624820(GRID *p)
{
    return p->szName;
}

int func_00624828(GRID *p)
{
    return p->nId;
}

/* 0x624830: the chunked send (504 bytes a message, to every member whose peer takes the type, or to every peer when
 * the group's id is not positive). */
int func_00624830(GRID *p, int nSrc, GRID_HEADER *h, void *pData, int nLen)
{
    char buf[1280];
    unsigned int x;
    int nHeader;
    int nMode = 0;
    int n;
    int i;
    int bFirst = 1;
    char *pSrc = pData;
    int result = 0;
    GRID_PEER *pPeer;
    char *pOut = buf + 8;

    memcpy(buf, h, 8);
next:
    nHeader = 8;
    switch (h->nType) {
    case 32:
    case 33:
    case 34:
    case 35:
    case 47:
        nMode = 1;
        x = func_0060FFC8(p->pAudio);
        nHeader = 12;
        memcpy(pOut, &x, 4);
        break;
    case 48:
        x = func_0060FFE0(p->pAudio);
        if (bFirst)
            x |= 0x20000000;
        if ((unsigned int)nLen < 505)
            x |= 0x40000000;
        memcpy(pOut, &x, 4);
        nMode = 2;
        nHeader = 12;
        break;
    }
    n = nLen;
    if (h->nType == 48) {
        n = nLen < 505 ? nLen : 504;
        ((GRID_HEADER *)buf)->nLen = n;
    }
    memcpy(buf + nHeader, pSrc, n);
    nLen -= n;
    nHeader += n;
    pSrc += n;
    if (p->nId > 0) {
        for (i = 0; i < p->nMembers; i++) {
            pPeer = p->pMembers[i].pPeer;
            if (pPeer != 0) {
                if (p->nMaxMembers == 0) {
                    if (nMode == 1 && (pPeer->nFlags & 0x20000))
                        continue;
                    if (nMode == 2 && (pPeer->nFlags & 0x10000))
                        continue;
                }
                if (p->pMembers[i].nState == 2 && p->pMembers[i].nId != nSrc) {
                    if (nHeader != func_00611180(p->pAudio, buf, nHeader, pPeer))
                        result = -1;
                }
            }
        }
    } else {
        pPeer = func_006113F0(p->pAudio);
        while (func_006113E8(p->pAudio) != (pPeer = pPeer->pPrev)) {
            if (nHeader != func_00611180(p->pAudio, buf, nHeader, pPeer))
                result = -1;
        }
    }
    if (nLen != 0) {
        ((GRID_HEADER *)buf)->nSeq = func_0060FFF8(p->pAudio);
        bFirst = 0;
        goto next;
    }
    return result;
}

void func_00624B38(GRID *p, GRID_INFO *pInfo)
{
    pInfo->nSize = sizeof(GRID_INFO);
    pInfo->nId = p->nId;
    pInfo->nMaxMembers = p->nMaxMembers;
    pInfo->nMaxMembers2 = p->nMaxMembers;
    strncpy(pInfo->szName, p->szName, sizeof(pInfo->szName));
    pInfo->szName[sizeof(pInfo->szName) - 1] = 0;
}

void func_00624B88(GRID *p, GRID_ENTRY *pEntry)
{
    pEntry->nId = p->nId;
    strncpy(pEntry->szName, p->szName, sizeof(pEntry->szName));
    pEntry->szName[sizeof(pEntry->szName) - 1] = 0;
    pEntry->nQueued = func_0062A2E8(&p->list);
}

void func_00624BE0(GRID *p, GRID_HEADER *h, int nType, int nLen)
{
    int nId = 0xFFFF;

    func_00611E60(p->pAudio, &nId);
    h->nType = nType | 0x80;
    h->nLen = nLen;
    h->nSeq = func_0060FFF8(p->pAudio);
    h->nGrid = p->nId;
    h->nSrc = nId;
}

/* Doubles a growing group's member table (-1 for a fixed one, or when out of memory). */
int func_00624C60(GRID *p)
{
    GRID_MEMBER *pOld;
    unsigned int now;
    int i;

    if (p == 0 || p->nMaxMembers != 0)
        return -1;
    now = func_0062C070();
    pOld = p->pMembers;
    rt_memory_allocate_init_zero((void **)&p->pMembers, p->nMembers * 2 * sizeof(GRID_MEMBER));
    if (p->pMembers == 0) {
        p->pMembers = pOld;
        return -1;
    }
    for (i = 0; i < p->nMembers; i++)
        p->pMembers[i] = pOld[i];
    rt_memory_free((void **)&pOld);
    p->nMembers *= 2;
    for (; i < p->nMembers; i++) {
        memset(&p->pMembers[i], 0, sizeof(GRID_MEMBER));
        p->pMembers[i].nTime = now;
    }
    return 0;
}

/* 0x624DC0: the group's clock */
int func_00624DC0(GRID *p)
{
    GRID_HEADER h;
    GRID_HEADER h2;
    int nId;
    struct { int a; int now; } msg;
    short data;
    int now;
    int i;
    int nAlive = 0;
    GRID_MEMBER *m;

    now = func_0062C070();
    if (p->x74 != 0 && now - p->x74 > 2000) {
        for (i = 0; i < p->nMembers; i++) {
            if (p->pMembers[i].nState == 0)
                break;
        }
        if (i < p->nMembers) {
            func_00624530(p->pAudio, &p->pMembers[i], 1, p->x78.nId);
            func_00624BE0(p, &h, 8, 0);
            func_00610FA0(p->pAudio, p->x78.nId, &h, 0, 0);
        }
        p->x74 = 0;
        memset(&p->x78, 0, sizeof(p->x78));
    }
    for (i = 0; i < p->nMembers; i++) {
        m = &p->pMembers[i];
        if (m->nState == 2) {
            if (now - m->nTime >= 1500) {
                if (m->nTries++ < 16) {
                    m->nTime = now;
                    data = func_00611E70(p->pAudio);
                    func_00624BE0(p, &h2, 1, 2);
                    func_00610FA0(p->pAudio, m->nId, &h2, &data, 2);
                } else {
                    if (p->nId > 0)
                        func_00626788(p, m->nId);
                    func_00612138(p->pAudio, m->nId);
                    continue;
                }
            }
            nAlive++;
        }
    }
    if (p->nId > 0) {
        if (nAlive > 0) {
            func_00611410(p->pAudio, 4);
            if (nAlive < p->nMembers && p->nMaxMembers != 0 && now - p->x70 > 6000) {
                func_00626398(p);
                p->x70 = now;
            }
            if (now - p->nTime > 1500) {
                nId = 0xFFFF;
                msg.a = 0;
                msg.now = now;
                func_00611E60(p->pAudio, &nId);
                func_00624BE0(p, &h2, 3, 8);
                func_00624830(p, nId, &h2, &msg, 8);
                p->nTime = now;
            }
        } else {
            func_00611420(p->pAudio, 4);
            if (now - p->x70 > 3000) {
                func_00611938(p->pAudio, p->nId);
                p->x70 = now;
            }
        }
    }
    return 0;
}

/* 0x6250D0: a talk message */
int func_006250D0(GRID *p, GRID_HEADER *h, GRID_PEER *pFrom, char *pData, int nLen)
{
    int x;
    int nCodec;
    int now = func_0062C070();
    RT_LIST_NODE *pNode;
    int nMax;
    int bFull;
    int i, j;
    GRID_MEMBER *m;
    GRID_PEER *pPeer;
    unsigned int nFlags;

    switch (h->nType) {
    case 32:
        nCodec = 33;
        break;
    case 33:
        nCodec = 34;
        break;
    case 34:
        nCodec = 35;
        break;
    case 35:
        nCodec = 36;
        break;
    case 47:
        nCodec = 63;
        break;
    default:
        return -1;
    }
    bFull = 0;
    nMax = p->nMaxMembers;
    if (nMax != 0) {
        nMax++;
        bFull = nMax < func_0062A2E8(&p->list);
    }
    pNode = 0;
    if (bFull)
        pNode = func_0062A298(&p->list, h->nSrc);
    for (i = 0; i < p->nMembers; i++) {
        m = &p->pMembers[i];
        if (m->nState == 2) {
            if (m->nId == pFrom->nId) {
                m->nTime = now;
                m->nTries = 0;
            } else if (m->nId != h->nSrc && bFull) {
                if (pNode != 0) {
                    for (j = 3; j >= 0; j--) {
                        if (pNode->nIds[j] == m->nId)
                            break;
                    }
                    if (j >= 0)
                        continue;
                }
                func_00611180(p->pAudio, pData, nLen, m->pPeer);
            }
        }
    }
    pPeer = func_006113F8(p->pAudio, h->nSrc);
    if (pPeer == 0 || (pPeer->nFlags & 2))
        return 0;
    pData += 8;
    nLen -= 12;
    memcpy(&x, pData, 4);
    pData += 4;
    if (pPeer->pIn == 0) {
        pPeer->pIn = func_0062AF40(func_0060FFB0(p->pAudio));
        if (pPeer->pIn == 0)
            return 0;
    }
    nFlags = func_0062A130(pPeer) & 0x4000;
    if (nLen != 0) {
        pPeer->tHeard = now;
        if (nFlags == 0) {
            func_0062A148(pPeer, 0x1000);
            func_0062A138(pPeer, 0x4000);
        }
    } else {
        pPeer->tHeard = 0;
        func_0062A148(pPeer, 0x4000);
    }
    func_0062A138(pPeer, 0x2000);
    func_00642A58(pPeer->pIn, nCodec, x, pData, nLen);
    return 0;
}

/* 0x6253B8: a video message */
int func_006253B8(GRID *p, GRID_HEADER *h, GRID_PEER *pFrom, char *pData, int nLen)
{
    int x;
    int now = func_0062C070();
    GRID_PEER *pPeer = func_006113F8(p->pAudio, h->nSrc);
    RT_LIST_NODE *pNode;
    int nMax;
    int bFull;
    int i, j;
    GRID_MEMBER *m;
    int nDiff;

    if (pPeer == 0)
        return -1;
    if (h->nType != 48)
        return -1;
    bFull = 0;
    nMax = p->nMaxMembers;
    if (nMax != 0) {
        nMax++;
        bFull = nMax < func_0062A2E8(&p->list);
    }
    pNode = 0;
    if (bFull)
        pNode = func_0062A298(&p->list, h->nSrc);
    for (i = 0; i < p->nMembers; i++) {
        m = &p->pMembers[i];
        if (m->nState == 2) {
            if (m->nId == pFrom->nId) {
                m->nTime = now;
                m->nTries = 0;
            } else if (m->nId != h->nSrc && bFull) {
                if (pNode != 0) {
                    for (j = 3; j >= 0; j--) {
                        if (pNode->nIds[j] == m->nId)
                            break;
                    }
                    if (j >= 0)
                        continue;
                }
                func_00611180(p->pAudio, pData, nLen, m->pPeer);
            }
        }
    }
    if (pPeer->nFlags & 1)
        return 0;
    pData += 8;
    nLen -= 12;
    memcpy(&x, pData, 4);
    pData += 4;
    if ((x & 0x60000000) != 0x60000000) {
        if (pPeer->pStream == 0) {
            pPeer->pStream = func_006451F0();
            if (pPeer->pStream == 0)
                return -1;
        }
    }
    if (pPeer->pStream != 0) {
        func_00645290(pPeer->pStream, x, pData, nLen);
        if (func_00645380(pPeer->pStream) != 0)
            func_0062A138(pPeer, 0x40000000);
    } else {
        nDiff = x - pPeer->x854;
        if (nDiff < 0) {
            if (nDiff >= -16)
                goto done;
        }
        if ((unsigned int)nLen > 2048)
            return -1;
        if (pPeer->pPending == 0) {
            rt_memory_allocate(&pPeer->pPending, 2048);
            if (pPeer->pPending == 0)
                goto done;
        }
        memcpy(pPeer->pPending, pData, nLen);
        pPeer->nPending = nLen;
        func_0062A138(pPeer, 0x40000000);
        pPeer->x854 = x + 1;
    }
done:
    return 0;
}

/* 0x625690: a group message: first what the relay passes on (when the queue holds more than the fixed table), then the
 * members' clocks, then the type's own work. OPEN (7 words, all of them the first test): the target tests the id in v0 and
 * this source in s0 (`lw s0,12(s2)`, which also moves s0's save in the prologue). What the rest needed, each measured:
 * - bForward is in t0, saved round both calls: CALLER_SAVE_PROFITABLE wants more than 8 refs, and it must come after now and the
 *   hoisted 2 in global-alloc's order, so a live length past 486: update_equiv_regs doubles it once for every set of the same
 *   constant in insn order until the first other value. So every case sets it apart: 0, 1 and 2 before case 3 are k=5.
 * - The arm: cross-jump keeps the copy with no label before it (find_cross_jump's --minimum at a CODE_LABEL), so the cases' zeros
 *   all land on the arm in front of the dispatch. It has to be (set t0 0): CSE2 rewrites the zero to `result` (`lw t0,104(sp)`,
 *   d=5, every other word right) unless a label stands between the initialisers and the arm, which this first test gives until
 *   thread_jumps removes it. A conditional for the count gives that label and the target's v0, and leaves a dead `move s0,zero`.
 * - The flag is a global pseudo (`slt s0,s0,v0`, tested after): i, not a local.
 * - Case 6's two and case 7's group are locals, block-local pseudos in s0 and s1 that keep p out of s1; case 10's peer and
 *   case 18's group each their own, the last so that its call cross-jumps with case 14's. */
int func_00625690(GRID *p, GRID_HEADER *h, GRID_PEER *pFrom, char *pData, int nLen)
{
    GRID_HEADER hOut;
    GRID_INFO info;
    unsigned short data1;
    unsigned short data2;
    int now = func_0062C070();
    GRID_BODY *pBody = (GRID_BODY *)(pData + 8);
    int bForward = 0;
    int result = 0;
    int nMax;
    int i;
    GRID_PEER *pPeer;
    GRID *pGrid;
    unsigned int nFlags;

    if ((nMax = p->nId) != 0)
        nMax = p->nMaxMembers;
    if (nMax != 0) {
        nMax++;
        i = nMax < func_0062A2E8(&p->list);
        if (i == 0) {
            bForward = 0;
        } else {
            switch (h->nType) {
            case 0:
                bForward = 0;
                break;
            case 1:
                bForward = 0;
                break;
            case 2:
                bForward = 0;
                break;
            case 3:
                (*(int *)pBody)++;
                bForward = 1;
                break;
            case 4:
                bForward = 0;
                break;
            case 5:
                bForward = 1;
                break;
            case 6:
                bForward = 1;
                break;
            case 7:
                bForward = 0;
                break;
            case 9:
                bForward = 1;
                break;
            case 10:
                bForward = 1;
                break;
            case 15:
                bForward = 1;
                break;
            case 16:
                bForward = 1;
                break;
            }
        }
    }
    for (i = 0; i < p->nMembers; i++) {
        GRID_MEMBER *m = &p->pMembers[i];

        if (m->nState == 2) {
            if (m->nId == pFrom->nId) {
                m->nTime = now;
                m->nTries = 0;
            } else if (m->nId != h->nSrc && bForward) {
                func_00611180(p->pAudio, pData, nLen, m->pPeer);
            }
        }
    }
    switch (h->nType) {
    case 0:
        break;
    case 1:
        data1 = func_00611E70(p->pAudio);
        func_00624BE0(p, &hOut, 2, 2);
        func_00610FA0(p->pAudio, pFrom->nId, &hOut, &data1, 2);
        /* fall through */
    case 2:
        if (pFrom->nGrid != pBody->x0)
            func_0062A0A8(pFrom, pBody->x0);
        break;
    case 3:
        func_00624BE0(p, &hOut, 4, 8);
        func_00610FA0(p->pAudio, h->nSrc, &hOut, pBody, 8);
        break;
    case 4:
        pFrom->x3C -= (pFrom->x3C + ((int *)pBody)[1] - now) >> 3;
        break;
    case 15:
        result = func_00626240(p, h->nSrc, 0);
        break;
    case 5: {
        GRID_MEMBER *m;

        pPeer = func_006113F8(p->pAudio, h->nSrc);
        if (pPeer->nGrid != pBody->x2)
            func_0062A0A8(pPeer, pBody->x2);
        if (p->nId == 0) {
            func_00626658(p, pPeer);
            if (pBody->x2 != 0) {
                pGrid = func_00611388(p->pAudio, pBody->x2);
                if (pGrid == 0) {
                    pGrid = func_00611348(p->pAudio, pBody->x2, 3);
                    data2 = pBody->x2;
                    func_00624BE0(pGrid, &hOut, 7, 2);
                    func_00610FA0(pGrid->pAudio, h->nSrc, &hOut, &data2, 2);
                }
                func_00626658(pGrid, pPeer);
            }
        }
        for (i = 0; i < p->nMembers; i++) {
            m = &p->pMembers[i];
            if (m->nState == 2 && m->nId == h->nSrc)
                break;
        }
        if (i < p->nMembers)
            break;
        for (i = 0; i < p->nMembers; i++) {
            m = &p->pMembers[i];
            if (m->nState == 0 || m->nId == h->nSrc)
                break;
        }
        if (i >= p->nMembers) {
            if (p->nMaxMembers != 0)
                break;
            func_00624C60(p);
            m = &p->pMembers[i];
        }
        if (pBody->x4 != 0) {
            func_00624530(p->pAudio, m, 1, h->nSrc);
            func_00624BE0(p, &hOut, 8, 0);
            func_00610FA0(p->pAudio, h->nSrc, &hOut, 0, 0);
            break;
        }
        p->x74 = now;
        p->x78 = *pBody;
        break;
    }
    case 8: {
        GRID_MEMBER *m;
        int n;

        m = 0;
        for (i = 0; i < p->nMembers; i++) {
            m = &p->pMembers[i];
            if (m->nState != 2 || m->nId == h->nSrc)
                break;
        }
        if (i >= p->nMembers) {
            if (p->nMaxMembers == 0) {
                func_00624C60(p);
                m = &p->pMembers[i];
            }
            if (i >= p->nMembers) {
                m = &p->pMembers[func_006244B0() % p->nMembers];
                n = m->nId;
                func_00624BE0(p, &hOut, 14, 0);
                func_00610FA0(p->pAudio, n, &hOut, 0, 0);
                func_00624510(m);
                if (p->nMaxMembers != 0) {
                    func_00626A38(p);
                    func_00626948(p, n);
                }
            }
        }
        func_00624530(p->pAudio, m, 2, h->nSrc);
        func_00624BE0(p, &hOut, 12, 0);
        func_00610FA0(p->pAudio, h->nSrc, &hOut, 0, 0);
        break;
    }
    case 12: {
        GRID_MEMBER *m;

        for (i = 0; i < p->nMembers; i++) {
            m = &p->pMembers[i];
            if (m->nId == h->nSrc && (m->nState == 1 || m->nState == 2))
                break;
        }
        if (i < p->nMembers) {
            m->nTime = now;
            m->nState = 2;
            m->nTries = 0;
            break;
        }
        func_00624BE0(p, &hOut, 14, 0);
        func_00610FA0(p->pAudio, h->nSrc, &hOut, 0, 0);
        break;
    }
    case 14:
        for (i = 0; i < p->nMembers; i++) {
            GRID_MEMBER *m = &p->pMembers[i];

            if (m->nId == h->nSrc) {
                func_00624510(m);
                if (p->nMaxMembers != 0) { func_00626A38(p); func_00626398(p); func_00626948(p, h->nSrc); } break;
            }
        }
        break;
    case 6:
        if (func_00611388(p->pAudio, pBody->x2) == 0) {
            GRID_PEER *pNew = func_006113F8(p->pAudio, h->nSrc);
            GRID *pThis = func_00611348(p->pAudio, pBody->x2, pBody->x4);

            strncpy(pThis->szName, (char *)pBody + 8, sizeof(pThis->szName));
            pThis->szName[sizeof(pThis->szName) - 1] = 0;
            func_0062A0A8(pNew, pBody->x2);
        }
        break;
    case 7: {
        GRID *pThis = func_00611388(p->pAudio, pBody->x0);

        if (pThis != 0) {
            info.nSize = sizeof(GRID_INFO);
            info.nId = pThis->nId;
            info.nMaxMembers = pThis->nMaxMembers;
            info.nMaxMembers2 = pThis->nMaxMembers;
            strncpy(info.szName, pThis->szName, sizeof(info.szName));
            info.szName[sizeof(info.szName) - 1] = 0;
            func_00624BE0(pThis, &hOut, 11, sizeof(GRID_INFO));
            func_00610FA0(pThis->pAudio, h->nSrc, &hOut, &info, sizeof(GRID_INFO));
        }
        break;
    }
    case 11:
        pGrid = func_00611388(p->pAudio, pBody->x2);
        if (pGrid != 0) {
            strncpy(pGrid->szName, (char *)pBody + 8, sizeof(pGrid->szName));
            if (pBody->x4 == 0 && pGrid->nMaxMembers != 0) {
                pGrid->nMaxMembers = 0;
                func_00624C60(pGrid);
            }
        }
        break;
    case 9:
        if (pBody->x0 != 0) {
            pPeer = func_006113F8(p->pAudio, h->nSrc);
            if (pPeer != 0) {
                if (pPeer->nGrid > 0 && pPeer->nGrid != pBody->x0) {
                    pGrid = func_00611388(p->pAudio, pPeer->nGrid);
                    if (pGrid != 0)
                        func_00626788(pGrid, pPeer->nId);
                }
                pGrid = func_00611388(p->pAudio, pBody->x0);
                if (pGrid != 0) {
                    if (pPeer->nGrid != pGrid->nId)
                        func_0062A0A8(pPeer, pGrid->nId);
                    if (func_00611E70(pGrid->pAudio) == pGrid->nId) {
                        if (pGrid->nMaxMembers != 0)
                            result = func_00626240(pGrid, h->nSrc, 1);
                        else
                            func_00612630(pGrid->pAudio, h->nSrc);
                    }
                }
            }
        }
        break;
    case 10:
        if (pBody->x0 != 0) {
            GRID_PEER *pLeft = func_006113F8(p->pAudio, h->nSrc);

            if (func_00611388(p->pAudio, pBody->x0) != 0 && pLeft != 0) {
                if (pLeft->pOut != 0) {
                    func_0062AE10(func_0060FFB0(pLeft->pAudio), pLeft->pOut);
                    pLeft->pOut = 0;
                }
                if (pLeft->pIn != 0) {
                    func_0062B090(func_0060FFB0(pLeft->pAudio), pLeft->pIn);
                    pLeft->pIn = 0;
                }
                func_0062A0A8(pLeft, 0);
            }
        }
        break;
    case 16:
        func_00612138(p->pAudio, h->nSrc);
        break;
    case 17:
        nFlags = pFrom->nFlags;
        if (*(unsigned int *)pBody & 1)
            nFlags |= 0x10000;
        else
            nFlags &= ~0x10000;
        if (*(unsigned int *)pBody & 2)
            nFlags |= 0x20000;
        else
            nFlags &= ~0x20000;
        pFrom->nFlags = nFlags;
        break;
    case 18:
        if (pBody->x0 != 0) {
            GRID *pThis = func_00611388(p->pAudio, pBody->x0);

            if (pBody->x2 == 0xFFFF)
                func_00626948(pThis, h->nSrc);
            else
                func_006269A8(pThis, h->nSrc, pBody->x2);
        }
        break;
    }
done:
    return result;
}

int func_00626110(GRID *p)
{
    func_00626140(p);
    return p->x68 != 0 ? 0 : -1;
}

/* Whether the codec can take the group's voices (0x68): a member connected, and fewer decoders
 * in use than it has, or no more queued than one over.
 * The queue's count is read before nMax + 1 only with the call on the left (the other order puts
 * nMax + 1 in a saved register before the call). Byte-exact since round 25b: the target lays the 0
 * store out as the second test's fall-through and the 1 store after it, both tests branching forward
 * to it -- SCEA's `goto` past the 0 (2.96 does not reorder blocks: an if/else, a negated `||`, a
 * nested else and returns in either order all put the 1 first, 9 words). */
void func_00626140(GRID *p)
{
    int nId = 0xFFFF;
    int i;
    void *pCodec;
    int nMax;

    func_00611E60(p->pAudio, &nId);
    for (i = 0; i < p->nMembers; i++) {
        if (p->pMembers[i].nState == 2)
            break;
    }
    if (i < p->nMembers) {
        pCodec = func_0060FFB0(p->pAudio);
        nMax = func_0062AAF8(pCodec);
        if (func_0062AB08(pCodec) < nMax || func_0062A2E8(&p->list) <= nMax + 1)
            goto one;
    }
    p->x68 = 0;
    return;
one:
    p->x68 = 1;
}

typedef struct GRID_SLOTS {             /* a type-5 message: the room left in the group */
    unsigned short nSize;               /* 0x00: sizeof (GRID_SLOTS) */
    unsigned short nGrid;               /* 0x02: the group spoken in */
    unsigned short nFree;               /* 0x04: free slots, the receiver's own counted */
    unsigned short x06;
    int nId;                            /* 0x08: this client */
} GRID_SLOTS;                           /* 0x0C */

/* The free member slots to one peer -- its own slot counted free, one for a growing table --
 * unless there are none and it is not forced; the send's result. */
int func_00626240(GRID *p, unsigned int nTo, int bForce)
{
    GRID_HEADER h;
    GRID_SLOTS msg;
    int nId = 0xFFFF;
    int i;

    func_00611E60(p->pAudio, &nId);
    msg.nId = nId;
    msg.nGrid = func_00611E70(p->pAudio);
    msg.nSize = sizeof msg;
    msg.nFree = 0;
    for (i = 0; i < p->nMembers; i++) {
        if (p->pMembers[i].nState == 0 || p->pMembers[i].nId == nTo)
            msg.nFree++;
    }
    if (msg.nFree == 0 && p->nMaxMembers == 0)
        msg.nFree++;
    if (bForce != 0 || msg.nFree != 0) {
        func_00624BE0(p, &h, 5, sizeof msg);
        return func_00610FA0(p->pAudio, nTo, &h, &msg, sizeof msg);
    }
    return 0;
}

GRID_MEMBER *func_00626350(GRID *p, unsigned int nId)
{
    int i;

    for (i = 0; i < p->nMembers; i++) {
        if (p->pMembers[i].nId == nId)
            return &p->pMembers[i];
    }
    return 0;
}

/* An empty message of type 15 to the group. */
int func_00626398(GRID *p)
{
    GRID_HEADER h;
    int nId = 0xFFFF;

    if (p == 0 || p->nId == 0)
        return -1;
    func_00611E60(p->pAudio, &nId);
    func_00624BE0(p, &h, 15, 0);
    return func_00624830(p, nId, &h, 0, 0);
}

/* An empty message of type 16, then the queue and the members cleared. */
void func_00626410(GRID *p)
{
    GRID_HEADER h;
    int nId = 0xFFFF;
    int i = 0;

    func_00611E60(p->pAudio, &nId);
    func_00624BE0(p, &h, 16, 0);
    func_00624830(p, nId, &h, 0, 0);
    func_0062A360(&p->list);
    for (; i < p->nMembers; i++)
        func_00624510(&p->pMembers[i]);
}

typedef struct PEER_INFO {              /* what a peer tells of itself (audio.c's, 32 bytes at 0x20) */
    int nAddr;                          /* 0x00 */
    int nAddr2;                         /* 0x04 */
    int nPort;                          /* 0x08 */
    int nPort2;                         /* 0x0C */
    int nGrid;                          /* 0x10 */
    int nId;                            /* 0x14 */
    unsigned int nFlags;                /* 0x18 */
    int x1C;                            /* 0x1C */
} PEER_INFO;


/* Each joined member's self-description (0x8000 set when its peer's flags hold 0x4000) to the
 * caller's function, until it returns 0 -- audio.c's 0x611BA0 over a group's table. The element's
 * address and its peer each a local (the peer read through the member twice keeps the call's
 * argument out of a0 until after the copy, 23 words; indexing swaps the sum's operands, 1). */
int func_006264C0(GRID *p, int (*pfn)(void *pAudio, void *pUser, PEER_INFO *pInfo), void *pUser)
{
    PEER_INFO info;
    GRID_MEMBER *m;
    void *pPeer;
    int i;

    for (i = 0; i < p->nMembers; i++) {
        m = &p->pMembers[i];
        if (m->nState == 2) {
            pPeer = m->pPeer;
            if (pPeer != 0) {
                memcpy(&info, (char *)pPeer + 0x20, sizeof(info));
                if (func_0062A130(pPeer) & 0x4000)
                    info.nFlags |= 0x8000;
                else
                    info.nFlags &= ~0x8000;
                if (pfn(p->pAudio, pUser, &info) == 0)
                    break;
            }
        }
    }
    return 0;
}

void func_006265F0(GRID *p)
{
    int i;

    for (i = 0; i < p->nMembers; i++)
        func_00624510(&p->pMembers[i]);
}


/* A client leaves the group (and, in a growing group spoken in here, the others are told). */
void func_00626788(GRID *p, unsigned int nId)
{
    GRID_MEMBER *m;

    if (p == 0)
        return;
    if (func_00611E70(p->pAudio) == p->nId) {
        m = func_00626350(p, nId);
        if (m != 0) {
            func_00624510(m);
            if (p->nMaxMembers != 0) {
                func_00626A38(p);
                func_00626398(p);
                func_00626948(p, nId);
            }
        }
    }
    func_0062A2F0(&p->list, nId);
}

int func_00626830(GRID *p)
{
    return func_0062A2E8(&p->list);
}

/* A growing group's members taken from its queue (every peer but this client). */
void func_00626848(GRID *p)
{
    int nId = 0xFFFF;
    int i = 0;
    void *pAudio;
    RT_LIST_NODE *n, *pFirst;
    GRID_MEMBER *m;
    int nPeer;

    if (p == 0 || p->nMaxMembers != 0)
        return;
    pAudio = p->pAudio;
    func_00611E60(pAudio, &nId);
    n = p->list.pHead;
    m = p->pMembers;
    pFirst = n;
    while (n != 0) {
        nPeer = ((GRID_PEER *)n->pData)->nId;
        if (nPeer != nId) {
            if (i >= p->nMembers) {
                func_00624C60(p);
                m = &p->pMembers[i];
            }
            i++;
            func_00624530(pAudio, m, 2, nPeer);
            m++;
        }
        n = n->pPrev;
        if (n == pFirst)
            n = 0;
    }
}

/* A client's id cleared from every queued item. */
void func_00626948(GRID *p, unsigned int nId)
{
    RT_LIST_NODE *n = p->list.pHead;
    RT_LIST_NODE *pFirst = n;
    unsigned int i;

    while (n != 0) {
        for (i = 0; i < 4; i++) {
            if (n->nIds[i] == nId)
                n->nIds[i] = 0xFFFF;
        }
        n = n->pPrev;
        if (n == pFirst)
            n = 0;
    }
}

/* A client's id moved to the first free slot of a peer's queued item. */
void func_006269A8(GRID *p, unsigned int nId, int nPeer)
{
    RT_LIST_NODE *n = p->list.pHead;
    RT_LIST_NODE *pFirst = n;
    int i, nSlot;

    while (n != 0) {
        if (((GRID_PEER *)n->pData)->nId == nPeer)
            break;
        n = n->pPrev;
        if (n == pFirst)
            n = 0;
    }
    if (n != 0) {
        nSlot = -1;
        for (i = 0; i < 4; i++) {
            if (n->nIds[i] == nId)
                n->nIds[i] = 0xFFFF;
            if (n->nIds[i] == 0xFFFF && nSlot < 0) {
                n->nIds[i] = nId;
                nSlot = i;
            }
        }
    }
}

/* The group's size (type 18) to every member connected. */
void func_00626A38(GRID *p)
{
    GRID_HEADER h;
    short data[2];
    int i;

    if (p == 0 || p->nMaxMembers == 0)
        return;
    i = 0;
    data[0] = p->nId;
    data[1] = -1;
    func_00624BE0(func_00611E78(p->pAudio), &h, 18, sizeof(data));
    for (; i < p->nMembers; i++) {
        if (p->pMembers[i].pPeer != 0 && p->pMembers[i].nState == 2)
            func_00610FA0(p->pAudio, ((GRID_PEER *)p->pMembers[i].pPeer)->nId, &h, data, sizeof(data));
    }
}

extern void func_0062A218(RT_LIST *pList, void *pPeer);

/* a peer joins the group's list (0x626658): in the group this client speaks in, unless it is this client or the
 * group is fixed, it also takes a member slot -- its own again, or the first free, the table grown when full.
 * Round 25b, unwritten before. */
void func_00626658(GRID *p, GRID_PEER *pPeer)
{
    int nId = 0xFFFF;
    int nPeerId;
    int i;
    GRID_MEMBER *m;

    if (p == 0)
        return;
    func_00611E60(p->pAudio, &nId);
    func_0062A218(&p->list, pPeer);
    if (p->nId <= 0)
        return;
    if (pPeer->nId == nId)
        return;
    if (p->nId != func_00611E70(p->pAudio))
        return;
    if (p->nMaxMembers != 0)
        return;
    nPeerId = pPeer->nId;
    m = p->pMembers;
    for (i = 0; i < p->nMembers; i++, m++) {
        if (m->nId == nPeerId || m->nState == 0)
            break;
    }
    if (i >= p->nMembers) {
        func_00624C60(p);
        m = &p->pMembers[i];
    }
    func_00624530(p->pAudio, m, 2, nPeerId);
}
