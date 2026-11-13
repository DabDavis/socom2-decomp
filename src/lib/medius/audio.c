/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* audio.c: the voice library's session (OVL2 0x60FB50-0x6126A0), SCEA's rt_audio
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

typedef struct GRID_INFO {              /* grid.c's */
    unsigned short nSize;               /* 0x00 */
    unsigned short nId;                 /* 0x02 */
    unsigned short nMaxMembers;         /* 0x04 */
    unsigned short nMaxMembers2;        /* 0x06 */
    char szName[60];                    /* 0x08 */
} GRID_INFO;                            /* 0x44 */

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
    unsigned int tSent;                 /* 0x10: the last keep-alive sent */
    unsigned int nTries;                /* 0x14: keep-alives unanswered */
    int x18;                            /* 0x18 */
    int tHeard;                         /* 0x1C: when last heard */
    PEER_INFO info;                     /* 0x20 */
    char x40[0x800];                    /* 0x40 */
    void *pFrame;                       /* 0x840: the decoded frame */
    void *pStream;                      /* 0x844 */
    void *pIn;                          /* 0x848 */
    void *pOut;                         /* 0x84C */
    int x850;                           /* 0x850 */
    int x854;                           /* 0x854 */
    int nPending;                       /* 0x858: a frame received, its length */
    void *pPending;                     /* 0x85C */
} PEER;

typedef struct GRID_NODE {              /* a group's member list (the list object at 0x629DA0) */
    struct GRID_NODE *pNext;            /* 0x00 */
    struct GRID_NODE *pPrev;            /* 0x04 */
    PEER *pPeer;                        /* 0x08 */
} GRID_NODE;

extern GRID_NODE *func_006247E8(GRID *p);

typedef enum AUDIO_CONNECT {           /* the socket's connection, under way or done */
    AUDIO_CONNECT_NONE,
    AUDIO_CONNECT_PENDING,
    AUDIO_CONNECT_DONE
} AUDIO_CONNECT;

typedef struct AUDIO {
    int nSeq;                           /* 0x00: every message's sequence number */
    int nSeqA;                          /* 0x04: type 0x2F's */
    int nSeqB;                          /* 0x08: type 0x30's */
    int x0C;                            /* 0x0C: the channels */
    int x10;
    int x14;
    int x18;
    int x1C;
    int x20;
    int x24;
    unsigned int nIn;                            /* 0x28 */
    unsigned int nOut;                           /* 0x2C */
    unsigned int nInBytes;                       /* 0x30 */
    unsigned int nOutBytes;                      /* 0x34 */
    unsigned int nEncoded;                       /* 0x38 */
    unsigned int nDecoded;                       /* 0x3C */
    unsigned int tEncode;                        /* 0x40: the time spent encoding */
    unsigned int tDecode;                        /* 0x44 */
    int (*pfnRead)(struct AUDIO *pAudio, int *pnType, char *pBuf, int nSize); /* 0x48 */
    void (*pfnDecoded)(struct AUDIO *pAudio, int nType, int nId, char *pBuf, int nLen); /* 0x4C */
    int (*pfnCapture)(struct AUDIO *pAudio, void *p1, void *p2, void *p3); /* 0x50 */
    void (*pfnPlay)(struct AUDIO *pAudio, int nId, void *pFrame); /* 0x54 */
    int nLastRead;                      /* 0x58 */
    void *pCodec;                       /* 0x5C */
    void *pFrame;                       /* 0x60: the codec's, while talking */
    GRID *pGrids;                       /* 0x64 */
    PEER *pSelf;                     /* 0x68 */
    PEER *pPeers;                    /* 0x6C */
    unsigned int nFlags;                /* 0x70 */
    int x74;                            /* 0x74 */
    int nId;                            /* 0x78: this client's */
    int x7C;                            /* 0x7C: this client's address, binary */
    int x80;                            /* 0x80: and its port */
    int nLocalAddr;                     /* 0x84 */
    int nLocalPort;                     /* 0x88 */
    void *pSocket;                      /* 0x8C */
    int x90;                            /* 0x90 */
    int x94;                            /* 0x94 */
    int nGrid;                          /* 0x98: the group spoken in, if positive */
    int nWidth;                         /* 0x9C */
    int nHeight;                        /* 0xA0 */
    void *pHandles[6];                  /* 0xA4: the encoder's three, the decoder's three */
    AUDIO_CONNECT nConnect;             /* 0xBC */
    char szHost[16];                    /* 0xC0 */
    int nPort;                          /* 0xD0 */
    void (*pfnConnect)(struct AUDIO *pAudio, int nAddr, int nPort); /* 0xD4: none = wait */
} AUDIO;                                /* 0xD8 */

extern GRID *func_00624570(AUDIO *pAudio, GRID *pList, int nId, int nMaxMembers);
extern void func_00624728(GRID *p);
extern GRID *func_00624780(GRID *p);
extern GRID *func_00624788(GRID *p);
extern GRID *func_00624790(GRID *pList, int nId);
extern void func_006247C8(GRID *p);
extern void func_00624B88(GRID *p, GRID_ENTRY *pEntry);
extern void func_00624B38(GRID *p, GRID_INFO *pInfo);
extern void func_006247F0(GRID *p, const char *szName);
extern char *func_00624820(GRID *p);
extern void func_00624BE0(GRID *p, GRID_HEADER *h, int nType, int nLen);
extern int func_00626110(GRID *p);
extern int func_006264C0(GRID *p, void *p1, int n2);
extern void func_00626410(GRID *p);
extern void func_00626A38(GRID *p);
extern int func_006250D0(GRID *p, GRID_HEADER *h, PEER *pFrom, char *pData, int nLen);
extern int func_006253B8(GRID *p, GRID_HEADER *h, PEER *pFrom, char *pData, int nLen);
extern int func_00625690(GRID *p, GRID_HEADER *h, PEER *pFrom, char *pData, int nLen);
extern unsigned short func_0062A160(PEER *p, GRID_HEADER *h, int nSrc);

extern PEER *func_00629EB8(PEER *pList, int nId);
extern void func_00629E40(PEER **ppList, int nId);
extern void func_00626788(GRID *p, int nId);
extern int func_00624828(GRID *p);
extern int func_00624830(GRID *p, int nSrc, GRID_HEADER *h, void *pData, int nLen);
extern void func_006265F0(GRID *p);
extern void func_00626658(GRID *p, PEER *pPeer);
extern void func_00626848(GRID *p);
extern void func_0062A0A8(PEER *p, int nGrid);
extern int func_0062BCB8(const char *szAddr);
extern void func_0063F4C8(void *pSocket);
extern void func_0063E4A0(void *pSocket, int nHandle, char *szAddr, unsigned short *pnPort);
extern int func_0063ECC8(void *pSocket, int nHandle, char *pBuf, int nSize, int *pnRead);
extern int strcmp(const char *a, const char *b);
extern unsigned int strlen(const char *s);
extern char *strncpy(char *d, const char *s, unsigned int n);
extern void mcDelayThread(int n);

typedef struct AUDIO_ADDR {             /* the socket's address argument */
    char szHost[16];                    /* 0x00 */
    unsigned short nPort;               /* 0x10 */
} AUDIO_ADDR;                           /* 0x12 */

extern int func_0063E230(void *pSocket, AUDIO_ADDR *pAddr);
extern int data_00676694;               /* bytes received */
extern PEER *func_00629F78(AUDIO *pAudio, PEER_INFO *pInfo);
extern unsigned int func_0062A130(PEER *p);
extern void func_0063E390(void *pSocket, char *szAddr, int *pnPort);
extern void func_0063E830(void *pSocket);
extern void func_0063F270(void *pSocket);
extern void func_0062A8C8(void *pCodec);
extern void func_0062A138(PEER *p, unsigned int nFlags);
extern void func_0062A148(PEER *p, unsigned int nFlags);
extern void func_00624DC0(GRID *p);
extern int func_0062C070(void);         /* the clock, in milliseconds */
extern int func_006456C0(void *pCodecA, void *pCodecB, void *p1, void *p2, void *p3, char *pBuf);
extern void func_006456C8(void *pCodecA, void *pCodecB, void *p1, void *p2, void *p3, void *pData);
extern void *func_006456F8(void *pCodecA, void *pCodecB);
extern void func_00645708(void *p1, void *p2, void *p3, void *pFrame);
extern int func_00645528(void *pStream, char *pBuf, int nSize);
extern int func_006137E8(void *pDecoder);
extern void *func_0062ABE0(void *pCodec, int nType);
extern int func_00613B80(void *pDecoder, char *pIn, int nIn, char *pOut, int nOutSize);
extern int func_00642C48(void *pIn, int *pnType, char *pBuf, int nSize);
void *func_0060FFB0(AUDIO *pAudio);
GRID *func_00611348(AUDIO *pAudio, int nId, int nMaxMembers);
int func_0060FC90(AUDIO *pAudio, PEER *p);
int func_0060FD98(AUDIO *pAudio, PEER *p);
void func_0060FEB8(AUDIO *pAudio);
int func_00611438(AUDIO *pAudio, PEER *p, int nType, char *pBuf, int nLen);
int func_006117F0(AUDIO *pAudio, void *pData, int nLen);
extern void func_0062B090(void *pCodec, void *pIn);
extern void func_0062AE10(void *pCodec, void *pFrame);
extern void func_0063DEA0(void *pSocket);
extern void func_006456D8(void *pHandle);

void func_00610318(AUDIO *pAudio);
int func_00610788(AUDIO *pAudio);
int func_00610938(AUDIO *pAudio);
int func_00610668(AUDIO *pAudio, int nNow);
void func_00612630(AUDIO *pAudio, int nTo);
int func_006115C0(AUDIO *pAudio, int nType, char *pData, int nFlag);
int func_00610C20(AUDIO *pAudio, int nFrom, int nPort, char *pBuf, unsigned int nLen);
int func_00610FA0(AUDIO *pAudio, int nTo, GRID_HEADER *h, void *pData, int nLen);
int func_00611250(AUDIO *pAudio, char *pBuf, int nSize, int *pnFrom, int *pnPort);
GRID *func_00611388(AUDIO *pAudio, int nId);
int func_00611A40(AUDIO *pAudio, int nGrid);
int func_006110D8(AUDIO *pAudio, char *pBuf, int nLen, int nAddr, int nPort);
GRID *func_00611900(AUDIO *pAudio);
void func_0060FFC0(AUDIO *pAudio);
void func_00611420(AUDIO *pAudio, unsigned int nFlags);
void func_00612138(AUDIO *pAudio, int nId);
GRID *func_006113A0(AUDIO *pAudio, GRID *p);

static int nTimerStart = 0;           /* the codec timer's start */

void func_0060FB50(void)
{
    nTimerStart = func_0062C070();
}

int func_0060FB78(void)
{
    return func_0062C070() - nTimerStart;
}

void func_0060FBA0(AUDIO *pAudio, int n, int nBytes)
{
    pAudio->nInBytes += nBytes;
    pAudio->nIn += n;
}

void func_0060FBC0(AUDIO *pAudio, int n, int nBytes)
{
    pAudio->nOutBytes += nBytes;
    pAudio->nOut += n;
}

/* Captures and encodes a frame while this client talks in a group (its length, or 0). */
int func_0060FBE0(AUDIO *pAudio, char *pBuf, int nSize)
{
    int n = 0;

    if (pAudio->pfnCapture != 0
        && pAudio->pfnCapture(pAudio, pAudio->pHandles[0], pAudio->pHandles[1], pAudio->pHandles[2]) > 0
        && pAudio->nGrid > 0) {
        func_0060FB50();
        n = func_006456C0(pAudio->nWidth, pAudio->nHeight, pAudio->pHandles[0], pAudio->pHandles[1],
                          pAudio->pHandles[2], pBuf);
        pAudio->tEncode += func_0060FB78();
        pAudio->nEncoded++;
    }
    return n;
}

/* Decodes a peer's pending (or streamed) frame and plays it. GCC round 74a (open since written, 52s):
 * the length in v1 was control flow, not the length's spelling -- the empty test a guard of its own
 * (its `return 0` set in the slot, v0 then live across the test) and the play under `if (pfnPlay !=
 * 0) {...}` falling to the final return; pData = buf before the read (`move s2,sp` in the jal's slot).
 * The argument and store orders followed. */
int func_0060FC90(AUDIO *pAudio, PEER *p)
{
    char buf[32256];
    void *pData = p->pPending;
    int n = p->nPending;

    if (n != 0) {
        p->nPending = 0;
    } else if (p->pStream != 0) {
        pData = buf;
        n = func_00645528(p->pStream, buf, sizeof(buf));
    }
    if (n == 0)
        return 0;
    if (pAudio->pfnPlay != 0) {
        if (p->pFrame == 0)
            p->pFrame = func_006456F8(pAudio->nWidth, pAudio->nHeight);
        func_0060FB50();
        func_006456C8(pAudio->nWidth, pAudio->nHeight, pAudio->pHandles[3], pAudio->pHandles[4],
                      pAudio->pHandles[5], pData);
        pAudio->tDecode += func_0060FB78();
        pAudio->nDecoded++;
        func_00645708(pAudio->pHandles[3], pAudio->pHandles[4], pAudio->pHandles[5], p->pFrame);
        pAudio->pfnPlay(pAudio, p->info.nId, p->pFrame);
    }
    return 0;
}

/* Drains a peer's incoming stream (flag 0x2000), closing it unless it still talks (0x4000). */
int func_0060FD98(AUDIO *pAudio, PEER *p)
{
    char buf[2048];
    int nType;
    unsigned int nFlags = func_0062A130(p);
    int nTotal = 0;
    int n;

    if (nFlags & 0x2000) {
        int nRead = 0;

        nType = 0;
        do {
            n = func_00642C48(p->pIn, &nType, buf, sizeof(buf));
            nRead += n;
            if (nRead == 0 || n != 0)
                nTotal += func_00611438(pAudio, p, nType, buf, n);
        } while (n != 0);
        if (!(nFlags & 0x4000)) {
            func_0062B090(func_0060FFB0(pAudio), p->pIn);
            p->pIn = 0;
            func_0062A138(p, 0x1000);
        }
        func_0062A148(p, 0x2000);
    } else if (nFlags & 0x1000) {
        func_00611438(pAudio, p, p->x18, buf, 0);
    }
    return nTotal;
}

/* Sends what was captured to the group spoken in. */
void func_0060FEB8(AUDIO *pAudio)
{
    char buf[32256];
    int n = func_0060FBE0(pAudio, buf, sizeof(buf));

    if (n > 0)
        func_006117F0(pAudio, buf, n);
}

AUDIO *func_0060FF00(void)
{
    AUDIO *p = 0;

    rt_memory_allocate_init_zero((void **)&p, sizeof(AUDIO));
    if (p != 0) {
        memset(p, 0, sizeof(AUDIO));
        p->x90 = 0;
        p->x94 = 0;
        p->nConnect = AUDIO_CONNECT_NONE;
        p->pfnConnect = 0;
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

/* Connects the socket to the host; without a callback, waits for the connection. */
int func_00610488(AUDIO *pAudio, const char *szHost, int nPort, void (*pfn)(AUDIO *pAudio, int nAddr, int nPort))
{
    AUDIO_ADDR addr;

    if (pAudio == 0 || szHost == 0 || strlen(szHost) == 0 || nPort == 0)
        return -1;
    strncpy(pAudio->szHost, szHost, sizeof(pAudio->szHost));
    pAudio->nPort = nPort;
    pAudio->pfnConnect = pfn;
    pAudio->nConnect = AUDIO_CONNECT_PENDING;
    memset(&addr, 0, sizeof(addr));
    strncpy(addr.szHost, szHost, sizeof(addr.szHost));
    addr.nPort = nPort;
    if (func_0063E230(pAudio->pSocket, &addr) != 0)
        return -1;
    while (pAudio->nConnect == AUDIO_CONNECT_PENDING && pAudio->pfnConnect == 0) {
        mcDelayThread(20);
        func_00610788(pAudio);
    }
    return 0;
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

/* Reads what the codec has (to the voice, a message at a time), then ends a silence over 1.5 s. */
int func_00610668(AUDIO *pAudio, int nNow)
{
    char buf[2048];
    int nType = 0;
    int nTotal = 0;
    int n;

    for (;;) {
        n = pAudio->pfnRead(pAudio, &nType, buf, sizeof(buf));
        nTotal += n;
        if (n > 0) {
            func_006115C0(pAudio, nType, buf, n);
            pAudio->nLastRead = nNow;
        } else if (nTotal == 0 && pAudio->pFrame != 0) {
            func_006115C0(pAudio, nType, buf, -1);
        }
        if (!((n >= 2048 || (n > 0 && nType == 63)) && n > 0))
            break;
    }
    if (nTotal == 0 && pAudio->pFrame != 0 && nNow - pAudio->nLastRead > 1500)
        func_006115C0(pAudio, pAudio->x90, buf, 0);
    return nTotal;
}

/* The session's update: datagrams, keep-alives and the group spoken in, the codec's output,
 * then each member's flags. */
int func_00610788(AUDIO *pAudio)
{
    int nNow = func_0062C070();
    unsigned int n;
    GRID *g;
    GRID_NODE *node, *pFirst;
    PEER *p;
    unsigned int nFlags;

    if (func_006105F0(pAudio) == 0 && func_00610938(pAudio) == 0 && pAudio->nGrid > 0) {
        g = func_00611388(pAudio, pAudio->nGrid);
        if (g != 0)
            func_00624DC0(g);
    }
    n = 0;
    if (pAudio->pfnRead != 0)
        n = func_00610668(pAudio, nNow);
    if (n <= 8000 && pAudio->pfnCapture != 0)
        func_0060FEB8(pAudio);
    if (pAudio->nGrid != 0) {
        g = func_00611388(pAudio, pAudio->nGrid);
        if (g != 0) {
            node = func_006247E8(g);
            pFirst = node;
            while (node != 0) {
                p = node->pPeer;
                nFlags = func_0062A130(p);
                if (nFlags & 0x40000000) {
                    func_0060FC90(pAudio, p);
                    func_0062A148(p, 0x40000000);
                }
                if (nFlags & 0x3000) {
                    func_0060FD98(pAudio, p);
                } else if ((nFlags & 0x4000) && nNow - p->tHeard > 1500) {
                    func_0062A148(p, 0x4000);
                    func_0062A138(p, 0x2000);
                }
                node = node->pPrev;
                if (node == pFirst)
                    node = 0;
            }
        }
    }
    return 0;
}

/* Pings each peer (every 1.5 s, or 0.2 s while it is new) and drops one unanswered 30 times
 * (100 while new); 1 when the list ends under it. Sets flag 2 once every peer is past new. */
int func_00610938(AUDIO *pAudio)
{
    PEER *p = pAudio->pPeers;
    unsigned int nNow = func_0062C070();
    int bKnown = 0;
    int bNew = 0;
    unsigned int nElapsed;

    while ((p = p->pPrev) != 0 && p != pAudio->pSelf) {
        if (p->nState == 0)
            bNew = 1;
        else
            bKnown = 1;
        nElapsed = nNow - p->tSent;
        if ((p->nState != 0 && nElapsed < 1500) || (p->nState == 0 && nElapsed < 200))
            continue;
        if ((p->nState != 0 && p->nTries < 30) || (p->nState == 0 && p->nTries < 100)) {
            func_00612630(pAudio, p->info.nId);
            p->tSent = nNow;
            p->nTries++;
        } else {
            if (p->info.nGrid > 0)
                func_00626788(func_00611388(pAudio, p->info.nGrid), p->info.nId);
            p = p->pNext;
            if (p != 0 && p->pPrev != 0)
                func_00612138(pAudio, p->pPrev->info.nId);
            else
                return 1;
        }
    }
    if (!(pAudio->nFlags & 2) && bKnown && !bNew)
        pAudio->nFlags |= 2;
    return 0;
}

/* Creates a named group under a free id and moves this client into it (1: no name, or taken). */
int func_00610AA8(AUDIO *pAudio, int nMaxMembers, int *pnId, const char *szName)
{
    GRID_HEADER h;
    GRID_INFO info;
    GRID *p = pAudio->pGrids;
    PEER *pSelf;

    if (pnId == 0 || szName == 0 || szName[0] == 0)
        return 1;
    *pnId = 1;
    while (func_00611388(pAudio, *pnId) != 0)
        (*pnId)++;
    while ((p = func_00624788(p)) != 0 && p != pAudio->pGrids) {
        if (strcmp(func_00624820(p), szName) == 0)
            return 1;
    }
    if (pAudio->nGrid > 0) {
        func_00611A40(pAudio, pAudio->nGrid);
        pAudio->nGrid = 0;
    }
    p = func_00611348(pAudio, *pnId, nMaxMembers);
    func_006247F0(p, szName);
    pSelf = func_00629EB8(pAudio->pPeers, pAudio->nId);
    func_00626658(p, pSelf);
    func_00624B38(p, &info);
    func_00624BE0(pAudio->pGrids, &h, 6, sizeof(info));
    pAudio->nGrid = info.nId;
    func_0062A0A8(pSelf, info.nId);
    return func_00624830(pAudio->pGrids, pAudio->nId, &h, &info, sizeof(info));
}

/* 0x610C20: a message from the socket: the peer it came from (by the sender's id when the header's 0x80 is set, else by address), the
 * length checked, then handed to its group by type. Round 28b, unwritten before. */
int func_00610C20(AUDIO *pAudio, int nFrom, int nPort, char *pBuf, unsigned int nLen)
{
    GRID_HEADER h;
    int now = func_0062C070();
    PEER *p;
    PEER *pSender;
    GRID *pGrid;
    int nId;
    unsigned int nSeen;

    if (nLen < sizeof(h))
        return -1;
    memcpy(&h, pBuf, sizeof(h));
    if (h.nType & 0x80) {
        h.nType &= 0x7F;
        memcpy(pBuf, &h, sizeof(h));
        p = func_00629EB8(pAudio->pPeers, h.nSrc);
        if (p != 0 && p->nState == 0) {
            if (p->info.nAddr == nFrom && p->info.nPort == nPort) {
                p->nState = 2;
            } else if (p->info.nAddr2 == nFrom && p->info.nPort2 == nPort) {
                p->nState = 1;
            } else {
                p = 0;
            }
        }
    } else {
        PEER *pFirst = pAudio->pPeers;

        p = pFirst;
        while (p != 0) {
            if (p->info.nAddr == nFrom && p->info.nPort == nPort) {
                if (p->nState == 0)
                    p->nState = 2;
                break;
            }
            if (p->info.nAddr2 == nFrom && p->info.nPort2 == nPort) {
                if (p->nState == 0)
                    p->nState = 1;
                break;
            }
            p = p->pPrev;
            if (p == pFirst)
                p = 0;
        }
    }
    if (p == 0)
        return 0;
    p->tSent = now;
    p->nTries = 0;
    nId = p->info.nId;
    pSender = func_00629EB8(pAudio->pPeers, h.nSrc);
    if (pSender == 0)
        return 0;
    switch (h.nType) {
    case 32:
    case 33:
    case 34:
    case 35:
    case 47:
    case 48:
        if (h.nLen != nLen - 12)
            return -1;
        break;
    default:
        if (h.nLen != nLen - 8)
            return -1;
        break;
    }
    if (h.nSrc == pAudio->nId)
        return 0;
    if (h.nGrid != 0) {
        if (h.nGrid != pSender->info.nGrid || h.nGrid != pAudio->nGrid) {
            func_00612630(pAudio, pSender->info.nId);
            pSender->tSent = now;
            pSender->nTries++;
            if (h.nGrid != pAudio->nGrid)
                return 0;
        }
    }
    pGrid = func_00611388(pAudio, h.nGrid);
    if (pGrid == 0)
        return 0;
    nSeen = func_0062A160(pSender, &h, nId);
    if (nSeen != 0xFFFF && nSeen != nId) {
        if (pAudio->x74 != 0)
            func_006125D0(pAudio, h.nGrid, nId, h.nSrc);
        return 0;
    }
    if (h.nGrid != 0) {
        if ((unsigned char)(h.nType - 32) < 4 || h.nType == 47)
            return func_006250D0(pGrid, &h, p, pBuf, nLen);
        if (h.nType == 48)
            return func_006253B8(pGrid, &h, p, pBuf, nLen);
    }
    return func_00625690(pGrid, &h, p, pBuf, nLen);
}

/* Sends a message to one peer: its second address below state 2, its first in 0 and 2. */
int func_00610FA0(AUDIO *pAudio, int nTo, GRID_HEADER *h, void *pData, int nLen)
{
    char buf[1280];
    PEER *p = func_00629EB8(pAudio->pPeers, nTo);
    int result = 0;
    int n;

    if (nLen + sizeof(GRID_HEADER) > sizeof(buf))
        return -1;
    if (p == 0)
        return -1;
    n = sizeof(GRID_HEADER);
    memcpy(buf, h, sizeof(GRID_HEADER));
    memcpy(buf + n, pData, nLen);
    n += nLen;
    if (p->info.nAddr2 != 0 && p->nState < 2) {
        if (p->info.nPort2 == 0)
            return -1;
        result = n == func_006110D8(pAudio, buf, n, p->info.nAddr2, p->info.nPort2) ? 0 : -1;
    }
    if (p->info.nAddr != 0 && (p->nState == 2 || p->nState == 0)) {
        if (p->info.nPort == 0)
            return -1;
        if (n != func_006110D8(pAudio, buf, n, p->info.nAddr, p->info.nPort))
            result = -1;
    }
    return result;
}

/* Sends to a peer's first address in states 0 and 2, its second below 2.
 * OPEN (13 words, all registers): the target keeps the state in s0 and the peer in s1, 2.96 here
 * the other way round -- the state a local (it lives across the first send), every spelling tried
 * (its declaration's place, int or unsigned, the tests' order and nesting, the info through a
 * pointer, the peer a typed copy of a void parameter, the results' variables) gives the same. */
int func_00611180(AUDIO *pAudio, char *pBuf, int nLen, PEER *p)
{
    unsigned int nState = p->nState;
    int result = nLen;
    int n;

    if (p->info.nAddr != 0 && (nState == 2 || nState == 0)) {
        if (p->info.nPort == 0)
            return -1;
        result = func_006110D8(pAudio, pBuf, nLen, p->info.nAddr, p->info.nPort);
    }
    if (p->info.nAddr2 != 0 && nState < 2) {
        if (p->info.nPort2 == 0)
            return -1;
        n = func_006110D8(pAudio, pBuf, nLen, p->info.nAddr2, p->info.nPort2);
        if (n != nLen)
            result = n;
    }
    return result;
}

/* One datagram, unless it is the host's (0). */
int func_00611250(AUDIO *pAudio, char *pBuf, int nSize, int *pnFrom, int *pnPort)
{
    char szAddr[16];
    unsigned short nPort;
    int nRead = 0;

    *pnFrom = 0;
    *pnPort = 0;
    func_0063F4C8(pAudio->pSocket);
    func_0063E4A0(pAudio->pSocket, -2, szAddr, &nPort);
    if (func_0063ECC8(pAudio->pSocket, -2, pBuf, nSize, &nRead) != 0)
        return -1;
    if (nRead != 0) {
        if (pAudio->nPort == (unsigned short)*pnPort && strcmp(szAddr, pAudio->szHost) == 0)
            return 0;
        data_00676694 += nRead;
        *pnFrom = func_0062BCB8(szAddr);
        *pnPort = nPort;
    }
    return nRead;
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

/* Decodes a peer's message with its decoder (made again when the type changes) for the host's
 * callback; an empty message ends a talk the peer has closed (0x1000). */
int func_00611438(AUDIO *pAudio, PEER *p, int nType, char *pData, int nLen)
{
    char buf[2048];
    int nResult = 0;
    int n;

    if (p->pOut != 0) {
        if (nLen != 0) {
            if (func_006137E8(p->pOut) != nType) {
                func_0062AE10(func_0060FFB0(pAudio), p->pOut);
                p->pOut = 0;
            }
        } else {
            nType = func_006137E8(p->pOut);
        }
    }
    if (p->pOut == 0) {
        p->pOut = func_0062ABE0(func_0060FFB0(pAudio), nType);
        if (p->pOut == 0)
            return 0;
    }
    n = func_00613B80(p->pOut, pData, nLen, buf, sizeof(buf));
    if (n != 0) {
        nResult = n;
        if (pAudio->pfnDecoded != 0)
            pAudio->pfnDecoded(pAudio, nType, p->info.nId, buf, nResult);
    } else if (nLen == 0 && (func_0062A130(p) & 0x1000)) {
        func_0062AE10(func_0060FFB0(pAudio), p->pOut);
        p->pOut = 0;
        func_0062A148(p, 0x1000);
        if (pAudio->pfnDecoded != 0)
            pAudio->pfnDecoded(pAudio, nType, p->info.nId, buf, 0);
    }
    return nResult;
}

/* 0x6115C0: not written */

/* Sends a type-0x30 message to the group spoken in; an empty one restarts its count. */
int func_006117F0(AUDIO *pAudio, void *pData, int nLen)
{
    GRID_HEADER h;
    GRID *p = func_00611900(pAudio);

    if (p == 0)
        return 0;
    h.nType = 0x30;
    h.nSrc = pAudio->nId;
    h.nGrid = func_00624828(p);
    h.nLen = nLen;
    h.nSeq = func_0060FFF8(pAudio);
    func_00624830(p, pAudio->nId, &h, pData, h.nLen);
    if (nLen == 0)
        func_0060FFC0(pAudio);
    return nLen;
}

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

/* Joins a group (leaving the one spoken in first, if joined). */
int func_00611938(AUDIO *pAudio, int nGrid)
{
    GRID_HEADER h;
    unsigned short data;
    int result = 0;
    GRID *p;
    PEER *pSelf;

    if (pAudio->nGrid > 0 && pAudio->nGrid != nGrid && (pAudio->nFlags & 4))
        func_00611A40(pAudio, pAudio->nGrid);
    if (nGrid > 0) {
        p = func_00611388(pAudio, nGrid);
        data = nGrid;
        func_00624BE0(pAudio->pGrids, &h, 9, sizeof(data));
        result = func_00624830(pAudio->pGrids, pAudio->nId, &h, &data, sizeof(data));
        if (result == 0) {
            pSelf = func_00629EB8(pAudio->pPeers, pAudio->nId);
            pAudio->nGrid = nGrid;
            func_0062A0A8(pSelf, nGrid);
            if (p != 0)
                func_00626658(p, pSelf);
        }
        func_00626848(p);
    }
    return result;
}

/* Leaves the group spoken in. */
int func_00611A40(AUDIO *pAudio, int nGrid)
{
    GRID_HEADER h;
    unsigned short data;
    int result;
    GRID *p;

    if (nGrid == 0 || nGrid != pAudio->nGrid)
        return -1;
    data = nGrid;
    func_00624BE0(pAudio->pGrids, &h, 10, sizeof(data));
    result = func_00624830(pAudio->pGrids, pAudio->nId, &h, &data, sizeof(data));
    p = func_00611388(pAudio, pAudio->nGrid);
    if (p != 0) {
        func_00626788(p, pAudio->nId);
        func_006265F0(p);
    }
    pAudio->nGrid = 0;
    func_0062A0A8(pAudio->pSelf, 0);
    func_00611420(pAudio, 4);
    return result;
}

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

/* Each peer's information reported to the callback (0x8000 set while it talks) until it returns 0. */
int func_00611BA0(AUDIO *pAudio, int (*pfn)(AUDIO *pAudio, void *pUser, PEER_INFO *pInfo), void *pUser)
{
    PEER_INFO info;
    PEER *p, *pFirst;

    if (pfn == 0 || pAudio->pGrids == 0)
        return -1;
    p = pAudio->pPeers;
    pFirst = p;
    while (p != 0) {
        memcpy(&info, &p->info, sizeof(info));
        if (func_0062A130(p) & 0x4000)
            info.nFlags |= 0x8000;
        else
            info.nFlags &= ~0x8000;
        if (pfn(pAudio, pUser, &info) == 0)
            break;
        p = p->pPrev;
        if (p == pFirst)
            p = 0;
    }
    return 0;
}


/* The same over a group's members. */
int func_00611CB0(AUDIO *pAudio, int (*pfn)(AUDIO *pAudio, void *pUser, PEER_INFO *pInfo), void *pUser, int nGrid)
{
    PEER_INFO info;
    GRID *g = func_00611388(pAudio, nGrid);
    GRID_NODE *n, *pFirst;
    PEER *p;

    if (pfn == 0 || g == 0)
        return -1;
    n = func_006247E8(g);
    pFirst = n;
    while (n != 0) {
        p = n->pPeer;
        memcpy(&info, &p->info, sizeof(info));
        if (func_0062A130(p) & 0x4000)
            info.nFlags |= 0x8000;
        else
            info.nFlags &= ~0x8000;
        if (pfn(pAudio, pUser, &info) == 0)
            break;
        n = n->pPrev;
        if (n == pFirst)
            n = 0;
    }
    return 0;
}

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

int func_00611F08(AUDIO *pAudio, int nId, unsigned int nFlags)
{
    GRID_HEADER h;
    unsigned int data;
    PEER *p;

    if (nId == 0xFFFF || nId == pAudio->nId)
        return -1;
    p = func_006113F8(pAudio, nId);
    if (p == 0)
        return -1;
    p->info.nFlags = nFlags;
    data = nFlags;
    func_00624BE0(pAudio->pGrids, &h, 0x11, sizeof(data));
    func_00610FA0(pAudio, nId, &h, &data, sizeof(data));
    return 0;
}

int func_00611FB0(AUDIO *pAudio, int x)
{
    if (pAudio->x74 != 0 && x == 0) {
        if (pAudio->nGrid > 0)
            func_00626A38(func_00611388(pAudio, pAudio->nGrid));
    }
    pAudio->x74 = x;
    return 0;
}

/* A peer told of: updated, or added (and its group created, and announced, when new). */
PEER *func_00612010(AUDIO *pAudio, PEER_INFO *pInfo)
{
    GRID_HEADER h;
    unsigned short data;
    PEER *p;
    GRID *g;

    p = func_00629EB8(pAudio->pPeers, pInfo->nId);
    if (p != 0) {
        func_0062A0A8(p, pInfo->nGrid);
    } else {
        p = func_00629F78(pAudio, pInfo);
        if (pAudio->pPeers == 0)
            pAudio->pPeers = p;
        p->pPrev = pAudio->pPeers;
        p->pNext = pAudio->pPeers->pNext;
        p->pPrev->pNext = p;
        p->pNext->pPrev = p;
    }
    func_00626658(pAudio->pGrids, p);
    if (pInfo->nGrid > 0) {
        g = func_00611388(pAudio, pInfo->nGrid);
        if (g == 0) {
            g = func_00611348(pAudio, pInfo->nGrid, 3);
            data = pInfo->nGrid;
            func_00624BE0(g, &h, 7, sizeof(data));
            func_00610FA0(pAudio, pInfo->nId, &h, &data, sizeof(data));
        }
        func_00626658(g, p);
    }
    return p;
}

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

/* The statistics: bytes a packet in and out (x1600), milliseconds an encode and a decode. */
void func_00612270(AUDIO *pAudio, float *pfIn, float *pfOut, float *pfEncode, float *pfDecode)
{
    if (pAudio->nIn != 0)
        *pfIn = (float)pAudio->nInBytes * 1600.0f / (float)pAudio->nIn;
    else
        *pfIn = 0;
    if (pAudio->nOut != 0)
        *pfOut = (float)pAudio->nOutBytes * 1600.0f / (float)pAudio->nOut;
    else
        *pfOut = 0;
    if (pAudio->nEncoded != 0)
        *pfEncode = (float)pAudio->tEncode / (float)pAudio->nEncoded;
    else
        *pfEncode = 0;
    if (pAudio->nDecoded != 0)
        *pfDecode = (float)pAudio->tDecode / (float)pAudio->nDecoded;
    else
        *pfDecode = 0;
}

/* The socket's connection finished: this client's address and port, or none. */
void func_00612460(AUDIO *pAudio, int nError)
{
    char szAddr[16];

    if (nError == 0) {
        func_0063E390(pAudio->pSocket, szAddr, &pAudio->nLocalPort);
        pAudio->nLocalAddr = func_0062BCB8(szAddr);
        pAudio->pSelf->info.nAddr = pAudio->nLocalAddr;
        pAudio->nConnect = AUDIO_CONNECT_DONE;
        pAudio->pSelf->info.nPort = pAudio->nLocalPort;
        if (pAudio->pfnConnect != 0)
            pAudio->pfnConnect(pAudio, pAudio->nLocalAddr, pAudio->nLocalPort);
    } else {
        pAudio->nLocalPort = 0;
        pAudio->nLocalAddr = 0;
        pAudio->pSelf->info.nAddr = pAudio->nLocalAddr;
        pAudio->nConnect = AUDIO_CONNECT_DONE;
        pAudio->pSelf->info.nPort = pAudio->nLocalPort;
        if (pAudio->pfnConnect != 0)
            pAudio->pfnConnect(pAudio, pAudio->nLocalAddr, pAudio->nLocalPort);
    }
}

/* The socket's event callback (the session its user data).
 * OPEN (14 words): every case a sibling call, the table from 0 (cases 0 and 1 return) as the
 * target's; but 2.96 copies the error code into a1 at the entry where the target copies it in
 * case 5 (its error unsigned, short, the null test nested: the same or worse). */
void func_00612518(void *pSocket, int n1, int nEvent, int nError, AUDIO *pAudio)
{
    if (pAudio == 0)
        return;
    switch (nEvent) {
    case 0:
    case 1:
        return;
    case 2:
        func_0063F270(pAudio->pSocket);
        return;
    case 3:
        func_0063E830(pAudio->pSocket);
        return;
    case 4:
        func_0063E830(pAudio->pSocket);
        return;
    case 5:
        func_00612460(pAudio, nError);
        return;
    case 6:
        func_0063E830(pAudio->pSocket);
        return;
    case 7:
        func_0063E830(pAudio->pSocket);
        return;
    case 8:
        func_0063E830(pAudio->pSocket);
        return;
    }
}

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

typedef struct AUDIO_INIT {
    unsigned short nPort;               /* 0x00 */
    char pad02[2];
    int a04;                            /* 0x04 */
    int a08;                            /* 0x08 */
    int a0C;                            /* 0x0C */
    int a10;                            /* 0x10 */
    int a14;                            /* 0x14 */
    int a18;                            /* 0x18 */
    int nWidth;                         /* 0x1C */
    int nHeight;                        /* 0x20 */
    int a24;                            /* 0x24 */
    int nChannels;                      /* 0x28 */
    int a2C;                            /* 0x2C */
    int (*pfnRead)(struct AUDIO *pAudio, int *pnType, char *pBuf, int nSize); /* 0x30 */
    void (*pfnDecoded)(struct AUDIO *pAudio, int nType, int nId, char *pBuf, int nLen); /* 0x34 */
    int (*pfnCapture)(struct AUDIO *pAudio, void *p1, void *p2, void *p3); /* 0x38 */
    void (*pfnPlay)(struct AUDIO *pAudio, int nId, void *pFrame); /* 0x3C */
} AUDIO_INIT;

typedef struct AUDIO_SOCKET {           /* the socket's options (rt_udp's 0x63DD18) */
    unsigned short nPort;               /* 0x00 */
    int nTry;                           /* 0x04: 1, then 2, 3, 4 while the port is taken */
    int a08;                            /* 0x08: 256 */
    int a0C;                            /* 0x0C: 1280 */
    int a10;                            /* 0x10: 4096 */
    int a14;                            /* 0x14: 4096 */
    int a18;
    int a1C;                            /* 0x1C */
    void *pfnMessage;                   /* 0x20 */
    void *pUser;                        /* 0x24 */
} AUDIO_SOCKET;                         /* 0x28 */

typedef struct AUDIO_SELF {             /* the record func_00629F78 makes from */
    int a00;
    int nAddr;                          /* 0x04 */
    int a08;
    int nPort;                          /* 0x0C */
    int a10;
    int nId;                            /* 0x14 */
    int a18;                            /* 0x18 */
    int a1C;
} AUDIO_SELF;

extern void *func_0062A870(AUDIO *pAudio, int nChannels);
extern void *func_006456D0(int nWidth, int nHeight);
extern int func_0063DD18(void **ppSocket, AUDIO_SOCKET *pOptions);
extern void func_0063E438(void *pSocket, char *szAddr, int *pnPort);
extern void func_00612518();

/* the voice library's start (0x610010): the parameters kept, the codec and the frame converters made, a socket on the first of the ports
 * that will bind, this client's own peer record the first of the ring. */
int func_00610010(AUDIO *pAudio, AUDIO_INIT *pInit)
{
    AUDIO_SOCKET options;
    AUDIO_SELF self;
    char szAddr[16];
    int r;

    if (pAudio->nFlags & 1)
        return 1;
    func_00611348(pAudio, 0, 3);
    pAudio->x0C = pInit->nChannels;
    pAudio->pfnRead = pInit->pfnRead;
    pAudio->x74 = pInit->a24;
    pAudio->pfnDecoded = pInit->pfnDecoded;
    pAudio->pfnCapture = pInit->pfnCapture;
    pAudio->x10 = pInit->a08;
    pAudio->x14 = pInit->a14;
    pAudio->pfnPlay = pInit->pfnPlay;
    pAudio->x20 = pInit->a04;
    pAudio->x24 = pInit->a10;
    pAudio->x18 = pInit->a0C;
    pAudio->x1C = pInit->a18;
    if (pAudio->x0C <= 0)
        pAudio->x0C = 1;
    pAudio->pCodec = func_0062A870(pAudio, pAudio->x0C);
    pAudio->nWidth = pInit->nWidth;
    pAudio->nHeight = pInit->nHeight;
    if (pAudio->pfnCapture != 0) {
        pAudio->pHandles[0] = func_006456D0(pAudio->nWidth, pAudio->nHeight);
        pAudio->pHandles[1] = func_006456D0(pAudio->nWidth / 2, pAudio->nHeight / 2);
        pAudio->pHandles[2] = func_006456D0(pAudio->nWidth / 2, pAudio->nHeight / 2);
    }
    if (pAudio->pfnPlay != 0) {
        pAudio->pHandles[3] = func_006456D0(pAudio->nWidth, pAudio->nHeight);
        pAudio->pHandles[4] = func_006456D0(pAudio->nWidth / 2, pAudio->nHeight / 2);
        pAudio->pHandles[5] = func_006456D0(pAudio->nWidth / 2, pAudio->nHeight / 2);
    }
    memset(&options, 0, sizeof(options));
    options.nPort = pInit->nPort;
    options.nTry = 1;
    options.a0C = 1280;
    options.a08 = 256;
    options.pfnMessage = func_00612518;
    options.a1C = pInit->a2C;
    options.pUser = pAudio;
    options.a14 = 4096;
    options.a10 = 4096;
    while (1) {
        r = func_0063DD18(&pAudio->pSocket, &options);
        if (r == 0)
            break;
        if (r == 17) {
            if (options.nTry == 1) {
                options.nTry = 2;
            } else if (options.nTry == 2) {
                options.nTry = 3;
            } else if (options.nTry == 3) {
                options.nTry = 4;
            } else
                break;
        } else {
            return -1;
        }
    }
    szAddr[0] = 0;
    func_0063E438(pAudio->pSocket, szAddr, &pAudio->x80);
    pAudio->x7C = func_0062BCB8(szAddr);
    memset(&self, 0, sizeof(self));
    self.nId = pAudio->nId;
    self.nAddr = pAudio->x7C;
    self.nPort = pAudio->x80;
    self.a18 = 0;
    pAudio->pSelf = func_00629F78(pAudio, (PEER_INFO *)&self);
    pAudio->pPeers = pAudio->pSelf;
    pAudio->pSelf->pPrev = pAudio->pSelf;
    pAudio->pPeers->pNext = pAudio->pPeers;
    func_00626658(pAudio->pGrids, pAudio->pSelf);
    pAudio->nFlags |= 1;
    return 0;
}

extern GRID *func_00611898(AUDIO *pAudio);
extern void *func_0062AB10(void *pCodec, int nType, int nMembers);
extern int func_00626830(GRID *p);
extern int func_006137F8(void *pFrame);
extern int func_00613808(void *pFrame, void *pData, int nFlag, void *pBuf, int nSize);

/* the frames of one type sent to the group spoken in (0x6115C0): the codec's frame made or changed as the type asks, each packet it gives
 * sent with its own sequence number; nFlag 0 sends the header alone and ends the talk. */
/* Byte-exact (round 77): the send's length is the header's own field read back (`h.nLen`), not `n & 0xFFFF` -- the
 * `andi` is then the field's zero-extension and the length store issues before the jal, the sequence's in its slot. */
int func_006115C0(AUDIO *pAudio, int nType, char *pData, int nFlag)
{
    GRID_HEADER h;
    char abBuf[480];
    int nTotal = 0;
    GRID *p = func_00611898(pAudio);
    void *pFrame;
    int nSize;
    int n;
    int nSeq;

    if (p == 0)
        return 0;
    pFrame = pAudio->pFrame;
    if (nFlag != 0) {
        if (pAudio->x90 != nType) {
            if (pFrame != 0) {
                func_0062AE10(pAudio->pCodec, pFrame);
                pAudio->pFrame = 0;
                pFrame = 0;
            }
            pAudio->x90 = 0;
        }
        if (pFrame == 0) {
            pAudio->pFrame = func_0062AB10(pAudio->pCodec, nType, func_00626830(p));
            if (pAudio->pFrame == 0) {
                pAudio->x90 = 0;
                return 0;
            }
            pAudio->x90 = nType;
            func_0060FFB8(pAudio);
            pFrame = pAudio->pFrame;
        }
    }
    nSize = 136;
    if (pFrame != 0)
        nSize = func_006137F8(pFrame) * 4;
    switch (nType) {
    case 33:
        h.nType = 32;
        nSize = 240;
        break;
    case 34:
        h.nType = 33;
        break;
    case 36:
        h.nType = 35;
        break;
    case 35:
        h.nType = 34;
        break;
    case 63:
    default:
        h.nType = 47;
        nSize = 480;
        break;
    }
    h.nSrc = pAudio->nId;
    h.nGrid = func_00624828(p);
    if (nFlag == 0) {
        h.nSeq = func_0060FFF8(pAudio);
        h.nLen = 0;
        func_00624830(p, pAudio->nId, &h, abBuf, 0);
        if (pAudio->pFrame != 0) {
            func_0062AE10(pAudio->pCodec, pAudio->pFrame);
            pAudio->pFrame = 0;
        }
        pAudio->x90 = 0;
        return 0;
    }
    if (nFlag == -1)
        nFlag = 0;
    for (;;) {
        n = func_00613808(pAudio->pFrame, pData, nFlag, abBuf, nSize);
        nFlag = 0;
        if (n <= 0)
            return nTotal;
        nSeq = func_0060FFF8(pAudio);
        nTotal += n;
        h.nLen = n;
        h.nSeq = nSeq;
        func_00624830(p, pAudio->nId, &h, abBuf, h.nLen);
    }
}
