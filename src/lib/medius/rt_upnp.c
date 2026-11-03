/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_upnp.c: the RT library's public-address resolver (OVL2 0x642330-0x642968), SCEA's
 * rt_upnp/src/rt_upnp.c ("rt_upnp version: 1.01.0007", the string its first function returns,
 * just before the object's $Header in .rodata; '%u.%u.%u.%u' after it), prebuilt with ee-gcc 2.96
 * at -O2 -G0 (README "The GCC front"), sibling calls on (0x6425F0 ends in `j`). SOCOM 1 has no
 * such object (its rt_rel_udp resolves the public address itself): the names are placeholders.
 *
 * A resolver (0x60 bytes, allocated zeroed) sends a four-byte request (0xE4 in its third byte)
 * over an rt_comm channel to a server, which answers with the address and port it saw; the
 * update resends every 800 ms up to five times, then calls back with an error, and keeps the
 * mapping alive (0xD4) every 5 s once resolved. The UPnP calls proper (0x642418-0x642458) are
 * stubs answering 1. */

extern void *memset(void *p, int c, unsigned int n);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern char *strncpy(char *dst, const char *src, unsigned int n);
extern int strncmp(const char *a, const char *b, unsigned int n);
extern int sprintf(char *buf, const char *fmt, ...);
extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);
extern int rt_comm_get_local_ip(void *p);
extern int rt_comm_send_to(int nChannelId, char *szAddress, unsigned short nPort, void *pData,
                           unsigned short nLength, unsigned short *pnSent);
extern int rt_comm_receive_from(int nChannelId, char *szAddress, unsigned short *pnPort, void *pData,
                                unsigned short nLength, unsigned short *pnReceived);

typedef struct RT_TIME {
    unsigned int secs;
    unsigned int usecs;
} RT_TIME;

extern int rt_time_get_time_stamp(RT_TIME *t);
extern int rt_time_elapsed(RT_TIME *start, unsigned int *msecs);

typedef struct RT_UPNP {
    int nChannel;                       /* 0x00: the rt_comm channel */
    char szLocalIp[16];                 /* 0x04 */
    char szPublic[16];                  /* 0x14: the address the server saw */
    unsigned short nPort;               /* 0x24 */
    unsigned short nPublicPort;         /* 0x26 */
    char szServer[16];                  /* 0x28 */
    unsigned short nServerPort;         /* 0x38 */
    char x3A[2];
    int nError;                         /* 0x3C */
    RT_TIME tSent;                      /* 0x40 */
    unsigned int nTries;                /* 0x48 */
    unsigned char bKeepAlive;           /* 0x4C */
    char x4D[3];
    int nState;                         /* 0x50: 1 while resolving */
    void (*pfnCallback)(struct RT_UPNP *p, int nError, char *szAddr, unsigned short nPort); /* 0x54 */
    void (*pfnUser)(struct RT_UPNP *p, int nError, int a2, char *szAddr, unsigned short nPort,
                    void *pUser);       /* 0x58 */
    void *pUser;                        /* 0x5C */
} RT_UPNP;                              /* 0x60 */

typedef struct RT_UPNP_REPLY {          /* the server's answer (6 bytes) */
    unsigned int nAddr;
    unsigned short nPort;
} RT_UPNP_REPLY;

static char *gpszVersion = "rt_upnp version: 1.01.0007";

char *func_00642330(void)
{
    return gpszVersion;
}

int func_00642340(RT_UPNP **ppUpnp, int nChannel, unsigned short nPort)
{
    RT_UPNP *pUpnp = 0;

    if (ppUpnp == 0)
        return 2;
    *ppUpnp = 0;
    if (nChannel == 0)
        return 2;
    if (rt_memory_allocate_init_zero((void **)&pUpnp, sizeof(RT_UPNP)) != 0)
        return 3;
    if (rt_comm_get_local_ip(pUpnp->szLocalIp) != 0) {
        rt_memory_free((void **)&pUpnp);
        return 5;
    }
    pUpnp->nPort = nPort;
    pUpnp->nChannel = nChannel;
    *ppUpnp = pUpnp;
    return 0;
}

int func_006423E0(RT_UPNP *pUpnp)
{
    RT_UPNP *pFree = 0;

    if (pUpnp == 0)
        return 2;
    pFree = pUpnp;
    rt_memory_free((void **)&pFree);
    return 0;
}

int func_00642418(void)
{
    return 1;
}

int func_00642420(void)
{
    return 1;
}

int func_00642428(void)
{
    return 1;
}

int func_00642430(void)
{
    return 1;
}

int func_00642438(void)
{
    return 1;
}

int func_00642440(void)
{
    return 1;
}

int func_00642448(void)
{
    return 1;
}

int func_00642450(void)
{
    return 1;
}

int func_00642458(void)
{
    return 1;
}

int func_00642460(RT_UPNP *pUpnp, char *szServer, unsigned short nPort,
                  void (*pfnCallback)(RT_UPNP *p, int nError, char *szAddr, unsigned short nPort))
{
    unsigned char abRequest[4];
    unsigned short nSent = 0;

    if (pUpnp == 0)
        return 2;
    if (pUpnp->nState != 0)
        return 7;
    if (szServer == 0)
        return 2;
    if (pfnCallback == 0)
        return 2;
    memset(pUpnp->szServer, 0, 40);
    strncpy(pUpnp->szServer, szServer, 16);
    pUpnp->nServerPort = nPort;
    pUpnp->pfnCallback = pfnCallback;
    pUpnp->nError = 0;
    abRequest[2] = 0xE4;
    if (rt_comm_send_to(pUpnp->nChannel, pUpnp->szServer, pUpnp->nServerPort, abRequest, 4, &nSent) != 0
        || nSent != 4)
        return 8;
    rt_time_get_time_stamp(&pUpnp->tSent);
    pUpnp->nTries++;
    pUpnp->nState = 1;
    return 0;
}

int func_00642560(RT_UPNP *pUpnp, char *szAddr, unsigned short *pnPort)
{
    if (pUpnp == 0)
        return 2;
    if (pUpnp->nState != 0)
        return 7;
    if (szAddr == 0)
        return 2;
    if (pnPort == 0)
        return 2;
    strncpy(szAddr, pUpnp->szPublic, 16);
    *pnPort = pUpnp->nPublicPort;
    return 0;
}

int func_006425C8(RT_UPNP *pUpnp)
{
    if (pUpnp == 0)
        return 2;
    if (pUpnp->nState != 0)
        return 7;
    pUpnp->bKeepAlive = 1;
    return 0;
}

void func_006426B8(RT_UPNP *pUpnp, int nError, char *szAddr, unsigned short nPort);

int func_006425F0(RT_UPNP *pUpnp, char *szServer, unsigned short nPort,
                  void (*pfnUser)(RT_UPNP *p, int nError, int a2, char *szAddr, unsigned short nPort, void *pUser),
                  void *pUser)
{
    if (pUpnp == 0)
        return 2;
    if (pUpnp->nState != 0)
        return 7;
    pUpnp->pfnUser = pfnUser;
    pUpnp->pUser = pUser;
    return func_00642460(pUpnp, szServer, nPort, func_006426B8);
}

int func_006426E8(RT_UPNP *pUpnp);
int func_00642768(RT_UPNP *pUpnp);

int func_00642648(RT_UPNP *pUpnp)
{
    int result = 0;

    if (pUpnp == 0)
        return 2;
    switch (pUpnp->nState) {
    case 0:
        result = func_006426E8(pUpnp);
        break;
    case 1:
        result = func_00642768(pUpnp);
        break;
    }
    return result;
}

int func_006426B0(void)
{
    return 0;
}

void func_006426B8(RT_UPNP *pUpnp, int nError, char *szAddr, unsigned short nPort)
{
    pUpnp->pfnUser(pUpnp, nError, 0, szAddr, nPort, pUpnp->pUser);
}

int func_006426E8(RT_UPNP *pUpnp)
{
    unsigned char abRequest[4];
    unsigned short nSent = 0;
    unsigned int nElapsed;

    if (pUpnp->bKeepAlive != 0) {
        rt_time_elapsed(&pUpnp->tSent, &nElapsed);
        if (nElapsed > 5000) {
            abRequest[3] = 0xD4;
            rt_comm_send_to(pUpnp->nChannel, pUpnp->szServer, pUpnp->nServerPort, abRequest, 4, &nSent);
            rt_time_get_time_stamp(&pUpnp->tSent);
        }
    }
    return 0;
}

int func_00642768(RT_UPNP *pUpnp)
{
    unsigned char abReply[8];
    unsigned char abAddr[4];
    char szAddr[16];
    unsigned short nPort;
    unsigned short nReceived = 0;
    unsigned short nPublicPort = 0;
    int result = 0;
    unsigned int nElapsed;

    if (rt_comm_receive_from(pUpnp->nChannel, szAddr, &nPort, abReply, 6, &nReceived) == 0 && nReceived == 6) {
        if (strncmp(szAddr, pUpnp->szServer, 16) == 0 && nPort == pUpnp->nServerPort) {
            memcpy(abAddr, abReply, 4);
            memcpy(&nPublicPort, abReply + 4, 2);
            sprintf(pUpnp->szPublic, "%u.%u.%u.%u", abAddr[0], abAddr[1], abAddr[2], abAddr[3]);
            pUpnp->bKeepAlive = 1;
            pUpnp->nPublicPort = (unsigned short)((nPublicPort << 8) | (nPublicPort >> 8));
            pUpnp->nError = 0;
            rt_time_get_time_stamp(&pUpnp->tSent);
            pUpnp->nState = 0;
            pUpnp->pfnCallback(pUpnp, pUpnp->nError, pUpnp->szPublic, pUpnp->nPublicPort);
        } else {
            result = 9;
        }
    }
    if (pUpnp->nState != 0) {
        rt_time_elapsed(&pUpnp->tSent, &nElapsed);
        if (nElapsed > 800) {
            if (pUpnp->nTries < 5) {
                unsigned char abRequest[4];
                unsigned short nSent = 0;

                abRequest[2] = 0xE4;
                if (rt_comm_send_to(pUpnp->nChannel, pUpnp->szServer, pUpnp->nServerPort, abRequest, 4, &nSent) != 0
                    || nSent != 4) {
                    pUpnp->nError = 8;
                    pUpnp->nState = 0;
                    pUpnp->pfnCallback(pUpnp, 8, pUpnp->szPublic, pUpnp->nPublicPort);
                } else {
                    rt_time_get_time_stamp(&pUpnp->tSent);
                    pUpnp->nTries++;
                }
            } else {
                pUpnp->nError = 6;
                pUpnp->nState = 0;
                pUpnp->pfnCallback(pUpnp, 6, pUpnp->szPublic, pUpnp->nPublicPort);
            }
        }
    }
    return result;
}
