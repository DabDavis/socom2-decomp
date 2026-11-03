/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_udp.c: the RT library's UDP transport (OVL2 0x63DC90-0x642330), SCEA's rt_udp/src/rt_udp.c
 * ("rt_udp version: 01.02.0048", the .data string 0x63DD08 returns), prebuilt with ee-gcc 2.96 at
 * -O2 -G0 (README "The GCC front"), sibling calls on (0x63DF90 ends in `j`). SOCOM 1's nearest
 * object is rt_rel_udp (0x411750), whose bodies this one does not keep: the names are placeholders.
 *
 * A UDP endpoint (0x54 bytes, allocated zeroed) owns an rt_comm channel -- its own, or one of eight
 * shared ones (gapUdp, by the slot at 0x48 when it is below 8) -- a table of connections (0x90
 * each: a state, the peer's address and port, a send and a receive message ring) and a message
 * ring of its own; the rings are the message-buffer object's at 0x6313D8 (0x18 bytes). Error codes
 * are the values returned: 1 a bad argument, 2 no endpoint, 3 a connection index out of range, 9
 * out of memory, 10 a ring failed, 15 the public address, 16 the send.
 *
 * SCEA's shapes, each measured: one guard per argument, each with its own return; a two-way choice
 * written `if (n == 0) {...; return 0;} if (n == 1) {...; return 0;} return 1;` (a shared tail
 * cross-jumps the two bodies, a switch moves every block); an index zeroed at its declaration (the
 * prologue's `move s1,zero`); a result kept in a saved register to one `return result` after a
 * time stamp call (0x641230); the RTO as `n = srtt + MAX(rttvar, 1); if (n > 500) n = 500;`; the
 * header stores in the orders tools/ccperm.py found (0x63F948, 0x641A70, 0x63F3A8).
 * OPEN: 0x63DD18 (the create: the target lays the allocation out after the option copy, jumping
 * back to it, and keeps the new record in one register through the copy -- 95 words with a local
 * copy, 111 without), 0x63EBA0 (the raw send: the length test's `return 1` is a fall-through `b`,
 * 33s), 0x63E230 (8: the shared path keeps its 1 in v0 and the delay slots differ), 0x63E4A0 (11:
 * the index lives in v1 and the -2 in v0), 0x63E5E8 (10, the connect), 0x63F168 and 0x6419E8 (2
 * each: the call's arguments scheduled the other way -- the -1 key-manager id in the delay slot),
 * 0x641E30 (3: a bnez/bnezl choice and a register after the shared channel's port query),
 * 0x63ECC8 (round 12b, 93s: the receive -- the reliable test reads the header's first word, `lw;
 * andi 6`, as a word-wide `& 6` gives; the loop's message pointer lives in a0 where the target's is
 * v0, and the endpoint path's free does not cross-jump with the connection's). Round 12b: 0x63E928
 * (the connection send) is byte-exact with RT_UDP_BARRIER below, a stand-in. Not written: the
 * datagram handlers 0x63EF38, 0x63F4C8, 0x63FA28, 0x63FC08, 0x6400A8, 0x6404D8, 0x640940, 0x640B70,
 * 0x640D38, 0x640EA0, 0x641308, 0x641490, 0x641748, 0x6418A0, 0x641BA0. */

extern void *memset(void *p, int c, unsigned int n);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern char *strncpy(char *dst, const char *src, unsigned int n);
extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);
extern int rt_comm_get_local_ip(char *szAddress);
extern int rt_comm_send_to(int nChannelId, char *szAddress, unsigned short nPort, void *pData,
                           unsigned short nLength, unsigned short *pnSent);
extern unsigned int randomMT(void);

typedef struct RT_TIME {
    unsigned int secs;
    unsigned int usecs;
} RT_TIME;

extern int rt_time_get_time_stamp(RT_TIME *t);
extern int rt_time_elapsed(RT_TIME *start, unsigned int *msecs);
extern int memcmp(const void *a, const void *b, unsigned int n);
extern int rt_comm_receive_from(int nChannelId, char *szAddress, unsigned short *pnPort, void *pData,
                                unsigned short nLength, unsigned short *pnReceived);

typedef struct RT_UDP_BUF {             /* the message ring (0x6313D8's object) */
    unsigned int nStart;                /* 0x00 */
    unsigned int nEnd;                  /* 0x04 */
    unsigned int nSize;                 /* 0x08 */
    unsigned int nUsed;                 /* 0x0C */
    char *pBuf;                         /* 0x10 */
    void *pFirst;                       /* 0x14: the first message */
} RT_UDP_BUF;                           /* 0x18 */

extern int func_006313D8(RT_UDP_BUF *pBuf, unsigned int nSize);
extern int func_00631460(RT_UDP_BUF *pBuf);
extern int func_00631B10(RT_UDP_BUF *pBuf, void **ppMsg);
extern int func_00631D18(RT_UDP_BUF *pBuf, void *pArg);
extern int func_00631748(RT_UDP_BUF *pBuf, void *pMsg);
extern void *func_00631E58(void *pMsg);
extern void *func_00631E88(void *pMsg);

typedef struct RT_UDP_CONN {
    int nState;                         /* 0x00: 12 connected */
    int x04;
    char szAddress[16];                 /* 0x08 */
    unsigned short nPort;               /* 0x18 */
    char x1A[2];
    RT_UDP_BUF send;                    /* 0x1C */
    RT_UDP_BUF recv;                    /* 0x34 */
    RT_TIME tLastRecv;                  /* 0x4C */
    RT_TIME tLastSend;                  /* 0x54 */
    RT_TIME tSent;                      /* 0x5C: when the awaited datagram went */
    int nAwaitSeq;                      /* 0x64: the reliable sequence awaiting its ack */
    unsigned int nRttVar;               /* 0x68 */
    unsigned int nSrtt;                 /* 0x6C */
    unsigned int nRto;                  /* 0x70: srtt + rttvar, at most 500 ms */
    int nId;                            /* 0x74: the session's id, in every header */
    int nSeqUnrel;                      /* 0x78 */
    int nSeqRel;                        /* 0x7C */
    int x80;                            /* 0x80 */
    int nRecvSeq;                       /* 0x84: the last reliable sequence handed up */
    int nReliable;                      /* 0x88: reliable datagrams queued */
    int nKMID;                          /* 0x8C: the peer's key-manager id */
} RT_UDP_CONN;                          /* 0x90 */

typedef struct RT_UDP_OPTIONS {
    unsigned short nPort;               /* 0x00 */
    unsigned int nShared;               /* 0x04 */
    unsigned int nConns;                /* 0x08 */
    unsigned int nMaxPacket;            /* 0x0C */
    int x10;                            /* 0x10 */
    int nBufSize;                       /* 0x14 */
    int x18;                            /* 0x18 */
    int x1C;                            /* 0x1C */
    void *pfnCallback;                  /* 0x20 */
    void *pUser;                        /* 0x24 */
} RT_UDP_OPTIONS;                       /* 0x28 */

typedef struct RT_UDP {
    int nChannel;                       /* 0x00: the rt_comm channel */
    RT_UDP_CONN *pConns;                /* 0x04 */
    void *pUpnp;                        /* 0x08: the public-address resolver (rt_upnp.c's) */
    RT_UDP_BUF buf;                     /* 0x0C */
    char *pSendBuf;                     /* 0x24 */
    char *pRecvBuf;                     /* 0x28 */
    unsigned int nConns;                /* 0x2C */
    int x30;                            /* 0x30: 4096 by default */
    int nBufSize;                       /* 0x34: 4096 by default */
    unsigned int nMaxPacket;            /* 0x38: 1438 by default */
    int x3C;                            /* 0x3C: 30 by default */
    int x40;                            /* 0x40 */
    unsigned char x44;                  /* 0x44 */
    unsigned char bResolving;           /* 0x45 */
    unsigned short nPort;               /* 0x46 */
    unsigned int nShared;               /* 0x48: below 8 one of the shared channels */
    void (*pfnCallback)(struct RT_UDP *pUdp, int nConn, int nEvent, int nError, void *pUser); /* 0x4C */
    void *pUser;                        /* 0x50 */
} RT_UDP;                               /* 0x54 */

typedef struct RT_UDP_ADDR {            /* a message's sender, as the ring keeps it */
    char szAddress[16];
    unsigned short nPort;
} RT_UDP_ADDR;

typedef struct RT_UDP_TARGET {          /* a peer to connect to, a server to resolve through */
    char szAddress[16];                 /* 0x00 */
    unsigned short nPort;               /* 0x10 */
    int nKMID;                          /* 0x14: its key-manager id */
} RT_UDP_TARGET;

typedef struct RSAKey {
    unsigned int n[16];
} RSAKey;                               /* 0x40 */

extern int strcmp(const char *a, const char *b);
extern int rt_time_start(RT_TIME *t);
extern int func_0062FCD8(int nKMID, int nID, RSAKey *pKey);

typedef struct RT_UDP_HEADER {          /* a datagram's header (12 bytes) */
    unsigned char nType;                /* 0x00: flags -- 2 sequenced, 4 reliable, 8 a keep-alive, 0x10 close, 0x20 an ack */
    unsigned char nShared;              /* 0x01 */
    unsigned short nLength;             /* 0x02 */
    int nId;                            /* 0x04 */
    int nSeq;                           /* 0x08 */
} RT_UDP_HEADER;

extern int func_0062F818(int nKMID, void *pKey);
extern int func_00641E30(RT_UDP *pUdp);
extern int func_006420E8(RT_UDP *pUdp);
extern int func_00642340(void **ppUpnp, int nChannel, unsigned short nPort);
extern int func_006426B0(void *pUpnp, int nTimeout);
extern int func_006425F0(void *pUpnp, char *szServer, unsigned short nPort, void *pfnCallback, void *pUser);
extern int func_00642560(void *pUpnp, char *szAddress, unsigned short *pnPort);
extern int func_006425C8(void *pUpnp);
void func_00642200(void *pUpnp, int nError, int a2, char *szAddress, unsigned short nPort, RT_UDP *pUdp);
extern int func_006423E0(void *pUpnp);
extern int rt_comm_startup(void);
extern int rt_comm_create(int *pnChannelId, char *szAddress, unsigned short nPort, void *pOptions);
extern int rt_comm_destroy(int nChannelId);
extern int func_0062D4D8(int nChannelId, unsigned short *pnPort);
int func_006400A8(RT_UDP *pUdp, RT_UDP_CONN *pConn);
int func_00640408(RT_UDP *pUdp);
int func_00641BA0(RT_UDP *pUdp, RT_UDP_CONN *pConn, RT_UDP_HEADER *pRecv, unsigned char *pbDup);
int func_00641B08(RT_UDP *pUdp, RT_UDP_CONN *pConn, RT_UDP_HEADER *pRecv);

int func_0063DEA0(RT_UDP *pUdp);
int func_0063F948(RT_UDP *pUdp, RT_UDP_CONN *pConn, unsigned char nType, RT_UDP_HEADER *pHeader);
int func_0063FA28(RT_UDP_CONN *pConn, RT_UDP_HEADER *pHeader, void *pData, unsigned int nLength);
int func_0063FC08(RT_UDP *pUdp, RT_UDP_CONN *pConn, unsigned char nType, void *pMsg, void *pData, unsigned int nLength);
int func_0063FD68(void *pMsg, RT_UDP_HEADER *pHeader);
int func_006419E8(RT_UDP *pUdp, RT_UDP_CONN *pConn);
int func_006404D8(RT_UDP *pUdp, char *szAddress, unsigned short nPort, unsigned short nLength);
int func_0063E5E8(RT_UDP *pUdp, RT_UDP_TARGET *pTarget, unsigned int *pnConn);
int func_00640B70(RT_UDP *pUdp, RT_UDP_CONN *pConn, RSAKey *pKey);
extern int func_006314C0(RT_UDP_BUF *pBuf);
extern unsigned int func_00631E70(void *pMsg);
int func_0063E438(RT_UDP *pUdp, char *szAddress, unsigned short *pnPort);
int func_0063E390(RT_UDP *pUdp, char *szAddress, unsigned short *pnPort);
int func_0063F8A0(RT_UDP *pUdp, unsigned int *pnConn);
unsigned int func_0063DCE8(void);
int func_0063DC90(void);
int func_0063FE48(void *pMsg, RT_UDP_HEADER *pHeader);
int func_0063FEA8(void *pMsg, RT_UDP_HEADER *pHeader);

static int gnSharedChannel = 0;         /* 0x656028: the eight shared endpoints' rt_comm channel */
static unsigned short gnSharedPort = 0; /* 0x65602C */
static char *gpSharedSendBuf = 0;       /* 0x656030 */
static char *gpSharedRecvBuf = 0;       /* 0x656034 */
static void *gpUpnp = 0;                /* 0x656038: the shared channels' resolver */
static unsigned char gbResolving = 0;   /* 0x65603C */
static char gszVersion[] = "rt_udp version: 01.02.0048";
static int gnKeyState = 0;              /* 0x65605C */
RT_UDP *gapUdp[8];                      /* 0x676460: the endpoints on the shared channel, by slot */

int func_0063DC90(void)
{
    if (gnKeyState == 0) {
        if (func_0062F818(-1, 0) == 0)
            gnKeyState = 1;
        else
            gnKeyState = 2;
    }
    return gnKeyState;
}

unsigned int func_0063DCE8(void)
{
    unsigned int n;

    do
        n = randomMT();
    while (n == 0);
    return n;
}

char *func_0063DD08(void)
{
    return gszVersion;
}

int func_0063DD18(RT_UDP **ppUdp, RT_UDP_OPTIONS *pOptions)
{
    RT_UDP *pUdp = 0;
    int result;

    if (ppUdp == 0)
        return 1;
    *ppUdp = 0;
    if (pOptions == 0)
        return 1;
    if (pOptions->pfnCallback == 0)
        return 1;
    if (rt_memory_allocate_init_zero((void **)&pUdp, sizeof(RT_UDP)) != 0) {
        result = 9;
        goto fail;
    }
    pUdp->nConns = pOptions->nConns;
    pUdp->nShared = pOptions->nShared;
    pUdp->x30 = pOptions->x10;
    pUdp->nBufSize = pOptions->nBufSize;
    pUdp->nPort = pOptions->nPort;
    pUdp->pfnCallback = pOptions->pfnCallback;
    pUdp->nMaxPacket = pOptions->nMaxPacket;
    pUdp->pUser = pOptions->pUser;
    pUdp->x3C = pOptions->x18;
    pUdp->x40 = pOptions->x1C;
    if (pUdp->nConns == 0)
        pUdp->nConns = 1;
    if (pUdp->x30 == 0)
        pUdp->x30 = 4096;
    if (pUdp->nBufSize == 0)
        pUdp->nBufSize = 4096;
    if (pUdp->nMaxPacket == 0)
        pUdp->nMaxPacket = 1438;
    if (pUdp->x3C == 0)
        pUdp->x3C = 30;
    if (rt_memory_allocate_init_zero((void **)&pUdp->pConns, pUdp->nConns * sizeof(RT_UDP_CONN)) != 0) {
        result = 9;
        goto fail;
    }
    if (func_006313D8(&pUdp->buf, pUdp->nBufSize) != 0) {
        result = 10;
        goto fail;
    }
    result = func_00641E30(pUdp);
    if (result != 0)
        goto fail;
    *ppUdp = pUdp;
    return 0;
fail:
    func_0063DEA0(pUdp);
    return result;
}

int func_0063DEA0(RT_UDP *pUdp)
{
    unsigned int i = 0;
    RT_UDP *pFree = 0;

    if (pUdp == 0)
        return 2;
    pFree = pUdp;
    if (pFree->nChannel != 0)
        func_006420E8(pFree);
    if (pFree->pConns != 0) {
        for (; i < pFree->nConns; i++) {
            if (pFree->pConns[i].nState != 0) {
                func_00631460(&pFree->pConns[i].send);
                func_00631460(&pFree->pConns[i].recv);
                pFree->pConns[i].nState = 0;
            }
        }
        rt_memory_free((void **)&pFree->pConns);
    }
    func_00631460(&pFree->buf);
    rt_memory_free((void **)&pFree);
    return 0;
}

int func_0063DF90(RT_UDP *pUdp, unsigned int nConn, int nWhich, void *pArg)
{
    RT_UDP_CONN *pConn;

    if (pUdp == 0)
        return 2;
    if (nConn >= pUdp->nConns)
        return 3;
    if (pUdp->pConns == 0)
        return 3;
    pConn = &pUdp->pConns[nConn];
    if (pConn->nState != 12)
        return 3;
    if (pArg == 0)
        return 1;
    if (nWhich == 0)
        return func_00631D18(&pConn->send, pArg);
    if (nWhich == 1)
        return func_00631D18(&pConn->recv, pArg);
    return 1;
}

int func_0063E030(RT_UDP *pUdp, int n)
{
    if (pUdp == 0)
        return 2;
    pUdp->x3C = n;
    return 0;
}

int func_0063E048(RT_UDP *pUdp, unsigned int nConn, int nWhich, unsigned int *pnSize)
{
    RT_UDP_CONN *pConn;

    if (pnSize == 0)
        return 1;
    *pnSize = 0;
    if (pUdp == 0)
        return 2;
    if (nConn >= pUdp->nConns)
        return 3;
    if (pUdp->pConns == 0)
        return 3;
    pConn = &pUdp->pConns[nConn];
    if (pConn->nState != 12)
        return 3;
    if (nWhich == 0) {
        *pnSize = pConn->send.nSize;
        return 0;
    }
    if (nWhich == 1) {
        *pnSize = pConn->recv.nSize;
        return 0;
    }
    return 1;
}

int func_0063E0C8(RT_UDP *pUdp, int *pn)
{
    if (pn == 0)
        return 1;
    *pn = 0;
    if (pUdp == 0)
        return 2;
    *pn = pUdp->x3C;
    return 0;
}

int func_0063E0F0(RT_UDP *pUdp, unsigned int nConn, int nWhich, unsigned int *pnSize,
                  unsigned int *pnUsed, unsigned int *pnPercent)
{
    RT_UDP_CONN *pConn;

    if (pUdp == 0)
        return 2;
    if (nConn >= pUdp->nConns)
        return 3;
    if (pUdp->pConns == 0)
        return 3;
    pConn = &pUdp->pConns[nConn];
    if (pConn->nState != 12)
        return 3;
    if (nWhich == 0) {
        if (pnSize != 0)
            *pnSize = pConn->send.nSize;
        if (pnUsed != 0)
            *pnUsed = pConn->send.nUsed;
        if (pnPercent != 0) {
            *pnPercent = pConn->send.nUsed * 100 / pConn->send.nSize;
            if (*pnPercent > 100)
                *pnPercent = 100;
        }
        return 0;
    }
    if (nWhich == 1) {
        if (pnSize != 0)
            *pnSize = pConn->recv.nSize;
        if (pnUsed != 0)
            *pnUsed = pConn->recv.nUsed;
        if (pnPercent != 0) {
            *pnPercent = pConn->recv.nUsed * 100 / pConn->recv.nSize;
            if (*pnPercent > 100)
                *pnPercent = 100;
        }
        return 0;
    }
    return 1;
}

int func_0063E230(RT_UDP *pUdp, RT_UDP_TARGET *pServer)
{
    if (pUdp == 0)
        return 2;
    if (pServer == 0)
        return 1;
    if (pUdp->bResolving != 0)
        return 15;
    if (pUdp->nShared < 8) {
        if (gpUpnp == 0) {
            if (func_00642340(&gpUpnp, pUdp->nChannel, pUdp->nPort) != 0)
                return 15;
            if (func_006426B0(gpUpnp, pUdp->x40) != 0)
                return 15;
        }
        pUdp->pUpnp = gpUpnp;
        if (gbResolving == 0) {
            if (func_006425F0(gpUpnp, pServer->szAddress, pServer->nPort, func_00642200, pUdp) != 0)
                return 15;
            pUdp->bResolving = 1;
            gbResolving = 1;
            return 0;
        }
    } else {
        if (pUdp->pUpnp == 0) {
            if (func_00642340(&pUdp->pUpnp, pUdp->nChannel, pUdp->nPort) != 0)
                return 15;
            if (func_006426B0(pUdp->pUpnp, pUdp->x40) != 0)
                return 15;
        }
        if (func_006425F0(pUdp->pUpnp, pServer->szAddress, pServer->nPort, func_00642200, pUdp) != 0)
            return 15;
    }
    pUdp->bResolving = 1;
    return 0;
}

int func_0063E390(RT_UDP *pUdp, char *szAddress, unsigned short *pnPort)
{
    if (pUdp == 0)
        return 2;
    if (szAddress == 0)
        return 1;
    if (pnPort == 0)
        return 1;
    if (pUdp->pUpnp != 0) {
        if (func_00642560(pUdp->pUpnp, szAddress, pnPort) == 0)
            return 0;
        return func_0063E438(pUdp, szAddress, pnPort);
    }
    return func_0063E438(pUdp, szAddress, pnPort);
}

int func_0063E438(RT_UDP *pUdp, char *szAddress, unsigned short *pnPort)
{
    if (pUdp == 0)
        return 2;
    if (szAddress == 0 || pnPort == 0)
        return 1;
    if (rt_comm_get_local_ip(szAddress) != 0)
        return 16;
    *pnPort = pUdp->nPort;
    return 0;
}

int func_0063E4A0(RT_UDP *pUdp, unsigned int nConn, char *szAddress, unsigned short *pnPort)
{
    void *pMsg = 0;
    RT_UDP_ADDR addr;

    if (pUdp == 0)
        return 2;
    if (szAddress == 0)
        return 1;
    if (pnPort == 0)
        return 1;
    if (nConn < pUdp->nConns) {
        strncpy(szAddress, pUdp->pConns[nConn].szAddress, 16);
        *pnPort = pUdp->pConns[nConn].nPort;
        return 0;
    }
    if (nConn != (unsigned int)-2)
        return 3;
    if (func_00631B10(&pUdp->buf, &pMsg) != 0)
        return 10;
    memcpy(&addr, func_00631E58(pMsg), sizeof(RT_UDP_ADDR));
    strncpy(szAddress, addr.szAddress, 16);
    *pnPort = addr.nPort;
    return 0;
}

int func_0063E5C0(RT_UDP *pUdp)
{
    if (pUdp == 0)
        return 2;
    func_006425C8(pUdp->pUpnp);
    return 0;
}

int func_0063E5E8(RT_UDP *pUdp, RT_UDP_TARGET *pTarget, unsigned int *pnConn)
{
    char szLocal[16];
    unsigned short nLocalPort;
    unsigned int nConn = -1;
    RT_UDP_CONN *pConn;
    int result;

    if (pnConn == 0)
        return 1;
    *pnConn = -1;
    if (pTarget == 0)
        return 1;
    if (pUdp == 0)
        return 2;
    szLocal[0] = 0;
    nLocalPort = 0;
    func_0063E390(pUdp, szLocal, &nLocalPort);
    if (pTarget->nPort == nLocalPort && strcmp(pTarget->szAddress, szLocal) == 0)
        return 21;
    szLocal[0] = 0;
    nLocalPort = 0;
    func_0063E438(pUdp, szLocal, &nLocalPort);
    if (pTarget->nPort == nLocalPort && strcmp(pTarget->szAddress, szLocal) == 0)
        return 21;
    {
        RSAKey key;
        RT_UDP_HEADER header;

        result = func_0063F8A0(pUdp, &nConn);
        if (result != 0)
            return result;
        *pnConn = nConn;
        pConn = &pUdp->pConns[nConn];
        if (func_006313D8(&pConn->send, pUdp->x30) != 0) {
            pUdp->pConns[nConn].nState = 6;
            return 7;
        }
        if (func_006313D8(&pConn->recv, pUdp->nBufSize) != 0)
            goto fail;
        pConn->nState = 9;
        pConn->nId = func_0063DCE8();
        pConn->nSeqUnrel = func_0063DCE8();
        pConn->nSeqRel = func_0063DCE8();
        pConn->nPort = pTarget->nPort;
        strncpy(pConn->szAddress, pTarget->szAddress, 16);
        rt_time_start(&pConn->tLastRecv);
        rt_time_start(&pConn->tLastSend);
        pConn->nRto = 1000;
        pConn->nSrtt = 0;
        pConn->nKMID = pTarget->nKMID;
        if (func_0063DC90() == 1) {
            func_0062FCD8(-1, pConn->nKMID, &key);
            result = func_00640B70(pUdp, pConn, &key);
        } else {
            if (func_0063F948(pUdp, pConn, 4, &header) != 0)
                goto fail;
            header.nType |= 0x40;
            result = func_0063FA28(pConn, &header, 0, 0);
        }
        if (result == 0)
            return 0;
    fail:
        pConn->nState = 6;
        return 7;
    }
}

int func_0063E830(RT_UDP *pUdp, unsigned int nConn)
{
    RT_UDP_CONN *pConn;
    RT_UDP_HEADER header;
    int result;

    if (pUdp == 0)
        return 2;
    if (nConn >= pUdp->nConns)
        return 3;
    pConn = &pUdp->pConns[nConn];
    if (pConn->nState == 12 || pConn->nState == 3) {
        pConn->nState = 5;
        result = func_0063F948(pUdp, pConn, 4, &header);
        if (result != 0)
            return result;
        header.nType |= 0x10;
        result = func_0063FA28(pConn, &header, 0, 0);
        if (result != 0)
            return result;
    } else
        pConn->nState = 4;
    return 0;
}

int func_0063E8E8(RT_UDP *pUdp, unsigned int nConn, int n)
{
    if (pUdp == 0)
        return 2;
    if (nConn >= pUdp->nConns)
        return 3;
    pUdp->pConns[nConn].nKMID = n;
    return 0;
}

/* A stand-in, not SCEA's text: an empty asm statement with a memory clobber, where the vendor's
 * source has some construct that compiles to nothing yet ends GCC's view of memory -- with it
 * func_0063E928 is byte-exact: the length test's `return 1` stays a block of its own (a
 * fall-through `b`, as the target lays it out) and the walk reloads the message pointer it just
 * stored before the loop's test. No flag does either (-fno-gcse, -fno-cse-follow-jumps,
 * -fno-rerun-cse-after-loop, -fno-expensive-optimizations, -fno-force-mem, -fno-thread-jumps);
 * a volatile pointer reloads everywhere (127s). */
#define RT_UDP_BARRIER() __asm__ volatile ("" ::: "memory")

/* queues a datagram on a connection: an unreliable one (bit 0 clear) is appended to a queued
 * datagram of the same kind with room for it, else a new message is stored; then the queue is sent */
int func_0063E928(RT_UDP *pUdp, unsigned int nConn, unsigned char nType, void *pData, unsigned int nLength)
{
    RT_UDP_HEADER header;
    RT_UDP_HEADER sent;
    void *pMsg = 0;
    void *pFound = 0;
    RT_UDP_CONN *pConn;
    int result;

    if (pUdp == 0)
        return 2;
    if (nConn >= pUdp->nConns)
        return 3;
    pConn = &pUdp->pConns[nConn];
    if (pConn->nState != 12)
        return 3;
    if (pData == 0)
        return 1;
    if (nLength == 0)
        return 1;
    if (nLength > pUdp->nMaxPacket) {
        RT_UDP_BARRIER();
        return 1;
    }
    if (func_00631B10(&pConn->send, &pMsg) != 0)
        return 10;
    while (pMsg != 0 && (unsigned char)(nType & 1) == 0) {
        if (func_0063FD68(pMsg, &sent) == 0 && func_0063FE48(pMsg, &header) == 0 && sent.nType == 0) {
            if (pFound != 0 && (nType & 2) && (header.nType & 2))
                pFound = 0;
            if ((header.nType & 7) == (nType & 7) && pFound == 0
                && header.nLength + nLength <= pUdp->nMaxPacket && !(header.nType & 0x80))
                pFound = pMsg;
        }
        pMsg = func_00631E88(pMsg);
        RT_UDP_BARRIER();
    }
    if (pFound != 0) {
        result = func_0063FC08(pUdp, pConn, nType, pFound, pData, nLength);
        if (result != 0)
            return result;
    } else {
        result = func_0063F948(pUdp, pConn, nType, &header);
        if (result != 0)
            return result;
        header.nType |= 0x40;
        header.nLength = nLength;
        result = func_0063FA28(pConn, &header, pData, nLength);
        if (result != 0)
            return result;
        if ((unsigned char)(nType & 1))
            pConn->nReliable++;
        if (pConn->nAwaitSeq == 0 && (nType & 4))
            pConn->nAwaitSeq = header.nSeq;
    }
    func_006400A8(pUdp, pConn);
    return 0;
}

int func_0063EBA0(RT_UDP *pUdp, char *szAddress, unsigned short nPort, void *pData, unsigned int nLength)
{
    unsigned short nSent = 0;
    RT_UDP_HEADER header;
    unsigned int nTotal;

    if (pUdp == 0)
        return 2;
    if (szAddress == 0 || nPort == 0 || pData == 0 || nLength == 0)
        return 1;
    if (nLength > pUdp->nMaxPacket)
        return 1;
    nTotal = nLength + 12;
    memset(&header, 0, sizeof header);
    header.nType = 0;
    header.nShared = pUdp->nShared;
    header.nLength = nLength;
    memcpy(pUdp->pSendBuf, &header, sizeof header);
    memcpy(pUdp->pSendBuf + 12, pData, nLength);
    if (rt_comm_send_to(pUdp->nChannel, szAddress, nPort, pUdp->pSendBuf, nTotal, &nSent) != 0)
        return 16;
    if (nSent != nTotal)
        return 16;
    return 0;
}

/* receives one datagram: from the endpoint's own list (-2: a sender's address before the header)
 * or a connection's -- a reliable one only in sequence -- and frees its message */
int func_0063ECC8(RT_UDP *pUdp, unsigned int nConn, void *pData, unsigned int nLength,
                  unsigned int *pnReceived)
{
    RT_UDP_HEADER header;
    void *pMsg = 0;
    RT_UDP_CONN *pConn;

    if (pUdp == 0)
        return 1;
    if (pData == 0)
        return 1;
    if (nLength == 0)
        return 1;
    if (pnReceived == 0)
        return 1;
    *pnReceived = 0;
    if (nConn == (unsigned int)-2) {
        if (func_00631B10(&pUdp->buf, &pMsg) != 0)
            return 10;
        if (pMsg != 0 && func_00631E70(pMsg) != 0) {
            memcpy(&header, (char *)func_00631E58(pMsg) + sizeof(RT_UDP_ADDR), sizeof header);
            *pnReceived = header.nLength < nLength ? header.nLength : nLength;
            memcpy(pData, (char *)func_00631E58(pMsg) + sizeof(RT_UDP_ADDR) + sizeof header, *pnReceived);
            func_00631748(&pUdp->buf, pMsg);
        } else
            *pnReceived = 0;
        return 0;
    }
    if (nConn >= pUdp->nConns)
        return 1;
    pConn = &pUdp->pConns[nConn];
    if (pConn->nState != 12)
        return 4;
    if (func_00631B10(&pConn->recv, &pMsg) != 0)
        return 10;
    if (pMsg == 0) {
        *pnReceived = 0;
        return 0;
    }
    for (;;) {
        if (func_00631E70(pMsg) != 0) {
            memcpy(&header, func_00631E58(pMsg), sizeof header);
            if ((*(unsigned int *)&header & 6) == 6 || (header.nType & 4)) {
                if (header.nSeq == pConn->nRecvSeq + 1 || pConn->nRecvSeq == 0) {
                    *pnReceived = header.nLength < nLength ? header.nLength : nLength;
                    memcpy(pData, (char *)func_00631E58(pMsg) + sizeof header, *pnReceived);
                    func_00631748(&pConn->recv, pMsg);
                    pConn->nRecvSeq = header.nSeq;
                    if (pConn->nRecvSeq == -1)
                        pConn->nRecvSeq = 0;
                    return 0;
                }
            } else {
                *pnReceived = header.nLength < nLength ? header.nLength : nLength;
                memcpy(pData, (char *)func_00631E58(pMsg) + sizeof header, *pnReceived);
                func_00631748(&pConn->recv, pMsg);
                return 0;
            }
        }
        pMsg = func_00631E88(pMsg);
        if (pMsg == 0) {
            *pnReceived = 0;
            return 0;
        }
    }
}

int func_0063F148(RT_UDP *pUdp, unsigned char b)
{
    if (pUdp == 0)
        return 1;
    pUdp->x44 = b;
    return 0;
}

int func_0063F168(RT_UDP *pUdp, unsigned int nConn)
{
    RSAKey key;
    RT_UDP_HEADER header;
    RT_UDP_CONN *pConn;

    if (pUdp == 0)
        return 1;
    if (nConn >= pUdp->nConns)
        return 1;
    pConn = &pUdp->pConns[nConn];
    if (pConn->nState != 7)
        return 4;
    pConn->nState = 8;
    if (func_0063DC90() == 1) {
        func_0062FCD8(-1, pConn->nKMID, &key);
        if (func_00640B70(pUdp, pConn, &key) != 0) {
            pConn->nState = 6;
            return 7;
        }
    } else {
        if (func_0063F948(pUdp, pConn, 4, &header) != 0) {
            pConn->nState = 6;
            return 7;
        }
        header.nType |= 0x40;
        if (func_0063FA28(pConn, &header, 0, 0) != 0) {
            pConn->nState = 6;
            return 7;
        }
    }
    return 0;
}

int func_0063F270(RT_UDP *pUdp, unsigned int nConn)
{
    RT_UDP_CONN *pConn;

    if (pUdp == 0)
        return 1;
    if (nConn >= pUdp->nConns)
        return 1;
    pConn = &pUdp->pConns[nConn];
    if (pConn->nState != 7)
        return 4;
    pConn->nState = 4;
    func_006419E8(pUdp, pConn);
    return 0;
}

int func_0063F2D8(RT_UDP *pUdp, RT_UDP_TARGET *pTarget, unsigned int *pnConn)
{
    void *pMsg = 0;
    RT_UDP_HEADER header;
    RT_UDP_CONN *pConn;
    int result;

    if (pnConn == 0)
        return 1;
    *pnConn = -1;
    if (pUdp == 0)
        return 2;
    if (pTarget == 0)
        return 1;
    result = func_0063E5E8(pUdp, pTarget, pnConn);
    if (result != 0)
        return result;
    pConn = &pUdp->pConns[*pnConn];
    pConn->nId = 0;
    if (func_00631B10(&pConn->send, &pMsg) != 0) {
        pConn->nState = 6;
        return 10;
    }
    if (func_0063FE48(pMsg, &header) != 0) {
        pConn->nState = 6;
        return 11;
    }
    header.nId = 0;
    if (func_0063FEA8(pMsg, &header) != 0) {
        pConn->nState = 6;
        return 11;
    }
    return 0;
}

int func_0063F3A8(RT_UDP *pUdp)
{
    unsigned int i = 0;
    unsigned short nSent = 0;
    unsigned int nElapsed;
    RT_UDP_HEADER header;
    RT_UDP_CONN *pConn;
    int result;

    if (pUdp == 0)
        return 2;
    for (; i < pUdp->nConns; i++) {
        pConn = &pUdp->pConns[i];
        result = func_006400A8(pUdp, pConn);
        if (result != 0)
            return result;
        if (pConn->nState == 12) {
            rt_time_elapsed(&pConn->tLastSend, &nElapsed);
            if (nElapsed > 5000) {
                memset(&header, 0, sizeof header);
                header.nId = pConn->nId;
                header.nShared = pUdp->nShared;
                rt_comm_send_to(pUdp->nChannel, pConn->szAddress, pConn->nPort, &header, sizeof header, &nSent);
                rt_time_get_time_stamp(&pConn->tLastSend);
            }
        }
    }
    return func_00640408(pUdp);
}

int func_0063F8A0(RT_UDP *pUdp, unsigned int *pnConn)
{
    unsigned int i = 0;

    if (pnConn == 0)
        return 1;
    *pnConn = -1;
    for (; i < pUdp->nConns; i++) {
        if (pUdp->pConns[i].nState == 0) {
            memset(&pUdp->pConns[i], 0, sizeof(RT_UDP_CONN));
            *pnConn = i;
            return 0;
        }
    }
    return 8;
}

int func_0063F948(RT_UDP *pUdp, RT_UDP_CONN *pConn, unsigned char nType, RT_UDP_HEADER *pHeader)
{
    if (pHeader == 0)
        return 1;
    if (pConn == 0)
        return 1;
    memset(pHeader, 0, sizeof(RT_UDP_HEADER));
    pHeader->nShared = pUdp->nShared;
    pHeader->nId = pConn->nId;
    pHeader->nType = nType;
    if ((nType & 6) == 6 || (nType & 4)) {
        pHeader->nSeq = pConn->nSeqRel;
        pConn->nSeqRel++;
        if (pConn->nSeqRel == 0)
            pConn->nSeqRel++;
    } else if (nType & 2) {
        pHeader->nSeq = pConn->nSeqUnrel;
        pConn->nSeqUnrel++;
        if (pConn->nSeqUnrel == 0)
            pConn->nSeqUnrel++;
    }
    return 0;
}

int func_0063FD68(void *pMsg, RT_UDP_HEADER *pHeader)
{
    if (pMsg == 0)
        return 1;
    if (pHeader == 0)
        return 1;
    if (func_00631E70(pMsg) != 0) {
        memcpy(pHeader, func_00631E58(pMsg), sizeof(RT_UDP_HEADER));
        return 0;
    }
    return 10;
}

int func_0063FDE8(void *pMsg, RT_UDP_HEADER *pHeader)
{
    if (pMsg == 0)
        return 1;
    if (pHeader == 0)
        return 1;
    memcpy(func_00631E58(pMsg), pHeader, sizeof(RT_UDP_HEADER));
    return 0;
}

int func_0063FE48(void *pMsg, RT_UDP_HEADER *pHeader)
{
    if (pMsg == 0)
        return 1;
    if (pHeader == 0)
        return 1;
    memcpy(pHeader, (char *)func_00631E58(pMsg) + 12, sizeof(RT_UDP_HEADER));
    return 0;
}

int func_0063FEA8(void *pMsg, RT_UDP_HEADER *pHeader)
{
    if (pMsg == 0)
        return 1;
    if (pHeader == 0)
        return 1;
    memcpy((char *)func_00631E58(pMsg) + 12, pHeader, sizeof(RT_UDP_HEADER));
    return 0;
}

int func_0063FF08(void *pMsg, unsigned int *pnTrailer)
{
    if (pMsg == 0)
        return 1;
    if (pnTrailer == 0)
        return 1;
    if (func_00631E70(pMsg) > 24)
        memcpy(pnTrailer, (char *)func_00631E58(pMsg) + func_00631E70(pMsg) - 4, 4);
    else
        *pnTrailer = 0;
    return 0;
}

int func_0063FF98(void *pMsg, unsigned int nTrailer)
{
    if (pMsg == 0)
        return 1;
    if (func_00631E70(pMsg) > 24)
        memcpy((char *)func_00631E58(pMsg) + func_00631E70(pMsg) - 4, &nTrailer, 4);
    return 0;
}

int func_00640010(void *pMsg, char **ppData, unsigned int *pnLength)
{
    if (ppData == 0)
        return 1;
    *ppData = 0;
    if (pnLength == 0)
        return 1;
    *pnLength = 0;
    if (pMsg == 0)
        return 1;
    if (func_00631E70(pMsg) > 24) {
        *ppData = (char *)func_00631E58(pMsg) + 24;
        *pnLength = func_00631E70(pMsg) - 28;
    } else {
        *ppData = 0;
        *pnLength = 0;
    }
    return 0;
}

int func_00640408(RT_UDP *pUdp)
{
    char szAddress[16];
    unsigned short nPort;
    unsigned short nReceived = 0;
    int result;

    do {
        nReceived = 0;
        if (pUdp->nShared < 8)
            result = rt_comm_receive_from(pUdp->nChannel, szAddress, &nPort, pUdp->pRecvBuf, 1450, &nReceived);
        else
            result = rt_comm_receive_from(pUdp->nChannel, szAddress, &nPort, pUdp->pRecvBuf,
                                          pUdp->nMaxPacket + 12, &nReceived);
        if (result == 0) {
            if (nReceived == 0)
                break;
            if (nReceived >= 12)
                func_006404D8(pUdp, szAddress, nPort, nReceived);
        }
    } while (nReceived != 0);
    return 0;
}

int func_006410E0(RT_UDP *pUdp, RT_UDP_HEADER *pRecv, unsigned int nConn)
{
    unsigned char bDup = 0;
    RT_UDP_CONN *pConn;

    if (nConn == (unsigned int)-1)
        return 7;
    pConn = &pUdp->pConns[nConn];
    func_00641BA0(pUdp, pConn, pRecv, &bDup);
    if (bDup == 0) {
        if (pConn->nState == 12) {
            pConn->nState = 3;
            return func_00641B08(pUdp, pConn, pRecv);
        }
        if (pConn->nState == 4 || pConn->nState == 5)
            return func_00641B08(pUdp, pConn, pRecv);
        pConn->nState = 1;
        return func_006419E8(pUdp, pConn);
    }
    return func_00641B08(pUdp, pConn, pRecv);
}

int func_006411C8(RT_UDP *pUdp, RT_UDP_HEADER *pHeader, unsigned int nConn)
{
    RT_UDP_CONN *pConn;

    if (nConn == (unsigned int)-1)
        return 7;
    pConn = &pUdp->pConns[nConn];
    if (pConn->nState != 4) {
        pConn->nState = 1;
        func_006314C0(&pConn->send);
        func_006314C0(&pConn->recv);
    }
    return 0;
}

int func_00641230(RT_UDP *pUdp, RT_UDP_HEADER *pRecv, unsigned int nConn)
{
    int result = 0;
    RT_UDP_HEADER header;
    RT_UDP_CONN *pConn;

    if (nConn == (unsigned int)-1)
        return 7;
    pConn = &pUdp->pConns[nConn];
    memset(&header, 0, sizeof header);
    header.nId = pConn->nId;
    header.nShared = pUdp->nShared;
    if (memcmp(&header, pRecv, sizeof header) == 0 && pConn->nState == 12)
        rt_time_get_time_stamp(&pConn->tLastRecv);
    else if (pConn->nState == 12)
        result = 4;
    else
        result = 5;
    return result;
}

int func_006419E8(RT_UDP *pUdp, RT_UDP_CONN *pConn)
{
    unsigned short nSent = 0;
    RT_UDP_HEADER header;

    if (pUdp == 0)
        return 2;
    if (pConn == 0)
        return 1;
    memset(&header, 0, sizeof header);
    header.nShared = pUdp->nShared;
    header.nType = 8;
    header.nId = pConn->nId;
    rt_comm_send_to(pUdp->nChannel, pConn->szAddress, pConn->nPort, &header, sizeof header, &nSent);
    return 0;
}

int func_00641A70(RT_UDP *pUdp, char *szAddress, unsigned short nPort, int nId)
{
    unsigned short nSent = 0;
    RT_UDP_HEADER header;

    if (pUdp == 0)
        return 2;
    memset(&header, 0, sizeof header);
    header.nShared = pUdp->nShared;
    header.nType = 8;
    header.nId = nId;
    rt_comm_send_to(pUdp->nChannel, szAddress, nPort, &header, sizeof header, &nSent);
    return 0;
}

int func_00641B08(RT_UDP *pUdp, RT_UDP_CONN *pConn, RT_UDP_HEADER *pRecv)
{
    RT_UDP_HEADER header;
    unsigned char ack[5];

    memset(&header, 0, sizeof header);
    header.nType = 0x20;
    header.nShared = pUdp->nShared;
    header.nId = pConn->nId;
    header.nLength = 5;
    ack[0] = pRecv->nType;
    memcpy(&ack[1], &pRecv->nSeq, 4);
    return func_0063FA28(pConn, &header, ack, 5);
}

extern int abs(int);
#define MAX(a, b) ((a) > (b) ? (a) : (b))
int func_00641D50(RT_UDP *pUdp, RT_UDP_CONN *pConn, unsigned char nType, int nSeq)
{
    unsigned int nElapsed;
    unsigned int nSrtt;
    unsigned int nRttVar;
    unsigned int nRto;

    if (pUdp == 0)
        return 1;
    if (pConn == 0)
        return 1;
    if ((nType & 6) == 6 || (nType & 4)) {
        if (pConn->nAwaitSeq != 0 && nSeq == pConn->nAwaitSeq) {
            rt_time_elapsed(&pConn->tSent, &nElapsed);
            nSrtt = pConn->nSrtt;
            nRttVar = pConn->nRttVar;
            if (nSrtt == 0) {
                nSrtt = nElapsed;
                nRttVar = nElapsed >> 1;
            } else {
                nRttVar = (nRttVar * 3 + abs(nSrtt - nElapsed)) >> 2;
                nSrtt = (nSrtt * 7 + nElapsed) >> 3;
            }
            pConn->nSrtt = nSrtt;
            pConn->nRttVar = nRttVar;
            pConn->nAwaitSeq = 0;
            nRto = nSrtt + MAX(nRttVar, 1);
            if (nRto > 500)
                nRto = 500;
            pConn->nRto = nRto;
        }
    }
    return 0;
}

int func_00641E30(RT_UDP *pUdp)
{
    int options[3];
    int result;

    if (pUdp->nShared < 8) {
        if (pUdp->nMaxPacket > 1438)
            return 1;
        if (gnSharedChannel == 0) {
            gnSharedPort = pUdp->nPort;
            if (rt_memory_allocate_init_zero((void **)&gpSharedSendBuf, 1450) != 0)
                return 9;
            if (rt_memory_allocate_init_zero((void **)&gpSharedRecvBuf, 1450) != 0)
                return 9;
            memset(gapUdp, 0, sizeof gapUdp);
            memset(options, 0, sizeof options);
            options[1] = 1450;
            options[0] = 1;
            result = rt_comm_create(&gnSharedChannel, 0, gnSharedPort, options);
            if (result == 5) {
                if (rt_comm_startup() != 0)
                    return 16;
                if (rt_comm_create(&gnSharedChannel, 0, gnSharedPort, options) != 0)
                    return 16;
            } else if (result != 0)
                return 16;
            if (func_0062D4D8(gnSharedChannel, &gnSharedPort) != 0)
                return 16;
            pUdp->nPort = gnSharedPort;
        }
        if (gapUdp[pUdp->nShared] != 0)
            return 17;
        if (pUdp->nPort != gnSharedPort)
            return 16;
        gapUdp[pUdp->nShared] = pUdp;
        pUdp->nChannel = gnSharedChannel;
        pUdp->pSendBuf = gpSharedSendBuf;
        pUdp->pRecvBuf = gpSharedRecvBuf;
    } else {
        if (pUdp->nMaxPacket > 1438)
            return 1;
        if (rt_memory_allocate_init_zero((void **)&pUdp->pSendBuf, pUdp->nMaxPacket + 12) != 0)
            return 9;
        if (rt_memory_allocate_init_zero((void **)&pUdp->pRecvBuf, pUdp->nMaxPacket + 12) != 0)
            return 9;
        memset(options, 0, sizeof options);
        options[1] = pUdp->nMaxPacket + 12;
        options[0] = 1;
        result = rt_comm_create(&pUdp->nChannel, 0, pUdp->nPort, options);
        if (result == 5) {
            if (rt_comm_startup() != 0)
                return 16;
            if (rt_comm_create(&pUdp->nChannel, 0, pUdp->nPort, options) != 0)
                return 16;
        } else if (result != 0)
            return 16;
        if (func_0062D4D8(pUdp->nChannel, &pUdp->nPort) != 0)
            return 16;
    }
    return 0;
}

int func_006420E8(RT_UDP *pUdp)
{
    int bLast = 1;
    unsigned int i;

    if (pUdp->nShared < 8) {
        gapUdp[pUdp->nShared] = 0;
        for (i = 0; i < 8; i++)
            if (gapUdp[i] != 0)
                bLast = 0;
        if (bLast) {
            rt_comm_destroy(gnSharedChannel);
            gnSharedChannel = 0;
            gnSharedPort = 0;
            rt_memory_free((void **)&gpSharedSendBuf);
            gpSharedSendBuf = 0;
            rt_memory_free((void **)&gpSharedRecvBuf);
            gpSharedRecvBuf = 0;
            if (gpUpnp != 0) {
                func_006423E0(gpUpnp);
                gpUpnp = 0;
                gbResolving = 0;
            }
        }
    } else {
        rt_comm_destroy(pUdp->nChannel);
        rt_memory_free((void **)&pUdp->pSendBuf);
        rt_memory_free((void **)&pUdp->pRecvBuf);
        if (pUdp->pUpnp != 0) {
            func_006423E0(pUdp->pUpnp);
            pUdp->pUpnp = 0;
        }
    }
    return 0;
}

void func_00642200(void *pUpnp, int nError, int a2, char *szAddress, unsigned short nPort, RT_UDP *pUdp)
{
    unsigned int i;

    if (pUdp == 0)
        return;
    if (pUdp->nShared < 8) {
        gbResolving = 0;
        for (i = 0; i < 8; i++) {
            if (gapUdp[i] != 0 && gapUdp[i]->bResolving != 0) {
                gapUdp[i]->bResolving = 0;
                if (nError == 0)
                    gapUdp[i]->pfnCallback(pUdp, -2, 5, 0, gapUdp[i]->pUser);
                else
                    gapUdp[i]->pfnCallback(pUdp, -2, 5, 15, gapUdp[i]->pUser);
            }
        }
        return;
    }
    pUdp->bResolving = 0;
    if (nError == 0)
        pUdp->pfnCallback(pUdp, -2, 5, 0, pUdp->pUser);
    else
        pUdp->pfnCallback(pUdp, -2, 5, 15, pUdp->pUser);
}
