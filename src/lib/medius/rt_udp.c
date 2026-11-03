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
 * out of memory, 10 a ring failed, 15 the public address, 16 the send. */

extern void *memset(void *p, int c, unsigned int n);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern char *strncpy(char *dst, const char *src, unsigned int n);
extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);
extern int rt_comm_get_local_ip(char *szAddress);
extern int rt_comm_send_to(int nChannelId, char *szAddress, unsigned short nPort, void *pData,
                           unsigned short nLength, unsigned short *pnSent);
extern unsigned int randomMT(void);

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
extern void *func_00631E58(void *pMsg);

typedef struct RT_UDP_CONN {
    int nState;                         /* 0x00: 12 connected */
    int x04;
    char szAddress[16];                 /* 0x08 */
    unsigned short nPort;               /* 0x18 */
    char x1A[2];
    RT_UDP_BUF send;                    /* 0x1C */
    RT_UDP_BUF recv;                    /* 0x34 */
    char x4C[0x40];
    int x8C;                            /* 0x8C */
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
    int x28;
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

typedef struct RT_UDP_HEADER {          /* a datagram's header (12 bytes) */
    unsigned char nType;                /* 0x00 */
    unsigned char nShared;              /* 0x01 */
    unsigned short nLength;             /* 0x02 */
    char x04[8];
} RT_UDP_HEADER;

extern int func_0062F818(int nKMID, void *pKey);
extern int func_00641E30(RT_UDP *pUdp);
extern int func_006420E8(RT_UDP *pUdp);
extern int func_00642340(void **ppUpnp, int nChannel, unsigned short nPort);
extern int func_006426B0(void *pUpnp, int nTimeout);
extern int func_006425F0(void *pUpnp, char *szServer, unsigned short nPort, void *pfnCallback, void *pUser);
extern int func_00642560(void *pUpnp);
extern int func_006425C8(void *pUpnp);
extern void func_00642200();
extern int func_0063F948(RT_UDP *pUdp, RT_UDP_CONN *pConn, unsigned char nType, void *pHeader);
extern int func_0063FA28(RT_UDP_CONN *pConn, void *pHeader, void *pData, int nLength);

int func_0063DEA0(RT_UDP *pUdp);
int func_0063E438(RT_UDP *pUdp, char *szAddress, unsigned short *pnPort);

static void *gpUpnp = 0;                /* 0x656038: the shared channels' resolver */
static unsigned char gbResolving = 0;   /* 0x65603C */
static char gszVersion[] = "rt_udp version: 01.02.0048";
static int gnKeyState = 0;              /* 0x65605C */

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

int func_0063E230(RT_UDP *pUdp, char *szServer)
{
    if (pUdp == 0)
        return 2;
    if (szServer == 0)
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
            if (func_006425F0(gpUpnp, szServer, *(unsigned short *)(szServer + 16), func_00642200, pUdp) != 0)
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
        if (func_006425F0(pUdp->pUpnp, szServer, *(unsigned short *)(szServer + 16), func_00642200, pUdp) != 0)
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
        if (func_00642560(pUdp->pUpnp) == 0)
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

int func_0063E830(RT_UDP *pUdp, unsigned int nConn)
{
    RT_UDP_CONN *pConn;
    unsigned char header[12];
    int result;

    if (pUdp == 0)
        return 2;
    if (nConn >= pUdp->nConns)
        return 3;
    pConn = &pUdp->pConns[nConn];
    if (pConn->nState == 12 || pConn->nState == 3) {
        pConn->nState = 5;
        result = func_0063F948(pUdp, pConn, 4, header);
        if (result != 0)
            return result;
        header[0] |= 0x10;
        result = func_0063FA28(pConn, header, 0, 0);
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
    pUdp->pConns[nConn].x8C = n;
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

int func_0063F148(RT_UDP *pUdp, unsigned char b)
{
    if (pUdp == 0)
        return 1;
    pUdp->x44 = b;
    return 0;
}
