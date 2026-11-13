/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* StreamMedia.c: DME's media streams over the voice library (OVL2 0x643168-0x6451E8), SCEA's
 * dme_client/src/StreamMedia.c (its $Header and StreamMedia.h's in .rodata after SHA1_platform.c's,
 * the two switch tables after them), prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC
 * front"). The object follows SHA1.c's (0x643140 reads SHA1.c's .rodata) and precedes the one at
 * 0x6451F0; its functions alone read the two .data statics at 0x6560C8 and the .bss table at
 * 0x676480. SOCOM 1 has no such object: every name here is a placeholder or ours.
 *
 * A session per DME world (four, allocated zeroed, 0x228 bytes) holds the voice library's handle
 * (rt_audio.c's API, 0x62BD30-0x62C060, each call's error mapped to 42 by the last function here),
 * its members (one 8-byte record per client), the stream ids, three callbacks the application
 * registers and two replies the voice library's callbacks fill in. Every entry point tests that
 * the library is started (17), its arguments (23), and a session's validity (41).
 *
 * SCEA's shapes, measured: a boolean query as one `&&` after its guards (0x644060, 0x644390: a
 * guard per test is if-converted to `xor; sltiu`); the callbacks' user data a pointer, its own
 * alias class, so its load is scheduled above the int stores into the callback's record; the
 * handle compared as `hAudio == pSession->hAudio`; a call's error returned from inside the
 * positive test (`if (connected) { r = f(); if (r) return r; } return 0;`). 33 of 39 byte-exact;
 * the six OPEN are diagnosed in place. */

extern void *memset(void *p, int c, unsigned int n);
extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);

/* DmeLocal.c's range tests: a world below 4, a client below 256 */
extern int func_0061D440(int nClient);
extern int func_0061D450(int nWorld);
extern int func_0061C720(int nWorld, int nClient, void **ppClient);
extern int func_00615DE0(int nType, int nWorld, int nTarget, int nFlags, int nSize, void *pData);
extern int data_00686A58;

/* rt_audio.c's API (the voice library's public functions, each forwarding to audio.c) */
extern void func_0062BD30(int nAddr, char *szAddr);
extern int func_0062BEA0(void *hAudio, int *pnId);
extern int func_0062BEC0(void *hAudio, int nId, void *pInfo);
extern int func_0062BED8(void *hAudio, void *pPeer);
extern void func_0062BF00(void *hAudio, int nClient);
extern void func_0062BE70(void *hAudio);
extern void func_0062BFA8(void *hAudio);
extern int func_0062BF18(void *hAudio, int nClient, int nMode);
extern int func_0062BF30(void *hAudio, int nStream);
extern int func_0062BF48(void *hAudio, int nStream);
extern int func_0062BF60(void *hAudio);
extern int func_0062BF78(void *hAudio);
extern int func_0062BF90(void *hAudio, int nType, int a2, int a3);
extern int func_0062BFD0(void *hAudio, void *pfnCallback, int a2);
extern int func_0062BFE8(void *hAudio, void *pfnCallback, int a2);

typedef struct STREAM_MEMBER {          /* a client's record in a session (8 bytes) */
    int nId;                            /* 0x00 */
    unsigned char bJoined;              /* 0x04 */
    unsigned char bTalking;             /* 0x05: added to the voice library */
    unsigned char bAnnounced;           /* 0x06: told the others */
    unsigned char b07;
} STREAM_MEMBER;

typedef struct STREAM_REPLY {           /* the stream reply the voice library's callback fills (12) */
    int nWorld;                         /* 0x00 */
    unsigned int nStream;               /* 0x04 */
    int nValue;                         /* 0x08 */
} STREAM_REPLY;

typedef struct STREAM_QUERY {           /* a client query, in and out (16) */
    int nWorld;                         /* 0x00 */
    int nClient;                        /* 0x04 */
    int nFound;                         /* 0x08 */
    int nIndex;                         /* 0x0C */
} STREAM_QUERY;

typedef struct STREAM_SESSION {
    int nWorld;                         /* 0x000 */
    int nMembers;                       /* 0x004: up to 256 */
    unsigned char bValid;               /* 0x008 */
    unsigned char bEnabled;             /* 0x009 */
    unsigned char bReady;               /* 0x00A */
    char x0B;
    int nMaxMembers;                    /* 0x00C: 256 */
    int x10;                            /* 0x010: the parameters' copy from here, 84 bytes */
    int x14;
    unsigned int nStreams;              /* 0x018 */
    int x1C;                            /* 0x01C */
    int x20;                            /* 0x020 */
    int x24;
    int x28;
    int x2C;
    int x30;
    int x34;
    int x38;
    int (*pfnType)(void *pInfo);        /* 0x03C */
    void *pTypeData;                    /* 0x040 */
    int (*pfnData)(void *pInfo);        /* 0x044 */
    void *pDataData;                    /* 0x048 */
    int x4C;
    int x50;
    int (*pfnSend)(void *pInfo);        /* 0x054 */
    void *pSendData;                    /* 0x058 */
    int (*pfnRecv)(void *pInfo);        /* 0x05C */
    void *pRecvData;                    /* 0x060 */
    int options[16];                    /* 0x064: the voice library's options */
    void *hAudio;                       /* 0x0A4: the voice library's session */
    unsigned char bA8;                  /* 0x0A8 */
    unsigned char bConnected;           /* 0x0A9 */
    char xAA[2];
    int anInfo[5];                      /* 0x0AC: the voice library's own description */
    int nC0;                            /* 0x0C0 */
    int nC4;
    int nC8;
    int nCC;                            /* 0x0CC: func_00644610 fills it from the description */
    char xD0[0x18];
    char szAddr[16];                    /* 0x0E8 */
    int nPort;                          /* 0x0F8 */
    int anStreams[64];                  /* 0x0FC */
    unsigned char bStreaming;           /* 0x1FC */
    char x1FD[3];
    unsigned int nStream;               /* 0x200 */
    STREAM_MEMBER *pMembers;            /* 0x204 */
    unsigned char bWaiting;             /* 0x208: a reply awaited */
    char x209[3];
    STREAM_REPLY reply;                 /* 0x20C */
    STREAM_QUERY query;                 /* 0x218 */
} STREAM_SESSION;                       /* 0x228 */

/* an address's kind in STREAM_INFO (gcc r21b): an enum -- its own alias class, so func_00644610 loads
 * each port before it stores the kind word, as the target does (an int kind orders the load after) */
typedef enum STREAM_ADDR_KIND {
    STREAM_ADDR_0,
    STREAM_ADDR_1,                      /* 1: the peer's own address */
    STREAM_ADDR_2                       /* 2: its second address */
} STREAM_ADDR_KIND;

typedef struct STREAM_INFO {            /* a client as the voice library is told of it (48) */
    STREAM_ADDR_KIND nType;             /* 0x00: 2 */
    char szAddr[16];                    /* 0x04 */
    int nPort;                          /* 0x14 */
    STREAM_ADDR_KIND nType2;            /* 0x18: 1 */
    char szAddr2[16];                   /* 0x1C */
    int nPort2;                         /* 0x2C */
} STREAM_INFO;

typedef struct STREAM_PEER {            /* audio.c's peer description (32) */
    int nAddr;                          /* 0x00 */
    int nAddr2;                         /* 0x04 */
    int nPort;                          /* 0x08 */
    int nPort2;                         /* 0x0C */
    int nGrid;                          /* 0x10 */
    int nId;                            /* 0x14 */
    unsigned int nFlags;                /* 0x18 */
    int x1C;
} STREAM_PEER;

typedef struct STREAM_CLIENT {          /* func_006448A0's argument */
    int nWorld;                         /* 0x00 */
    int nClient;                        /* 0x04 */
    int nAddr2;                         /* 0x08 */
    int nPort2;                         /* 0x0C */
    int nAddr;                          /* 0x10 */
    int nPort;                          /* 0x14 */
    int nGrid;                          /* 0x18 */
} STREAM_CLIENT;

typedef struct STREAM_TALK {            /* func_00643650's argument */
    int x00;
    int nWorld;                         /* 0x04 */
    int nClient;                        /* 0x08 */
    int bListen;                        /* 0x0C */
} STREAM_TALK;

typedef struct STREAM_PARAMS {          /* the default parameters (84 bytes) */
    int an[21];
} STREAM_PARAMS;

typedef struct STREAM_ANNOUNCE {        /* the message func_00644BD0 sends (28) */
    int nWorld;
    int nMaxMembers;
    int nInfo1;
    int nInfo3;
    int nInfo0;
    int nInfo2;
    int nInfo4;
} STREAM_ANNOUNCE;

static unsigned char gbInitialized = 0;
static short gnInitCount = 0;
static STREAM_SESSION *gapStreamSessions[4];

int func_006443D8(int nWorld, STREAM_SESSION **ppSession);
int func_00644258(int nWorld);
int func_00644308(int nWorld, STREAM_SESSION **ppSession);
int func_00644390(STREAM_SESSION *pSession);
int func_00644480(void *hAudio, STREAM_SESSION **ppSession);
int func_00644588(STREAM_SESSION *pSession, int nMember, STREAM_MEMBER **ppMember);
int func_00644610(STREAM_PEER *pPeer, STREAM_INFO *pInfo);
int func_006449C0(int nType, int *pnType);
int func_00644A58(int nType, int *pnType);
int func_00644B20(STREAM_SESSION *pSession, int nId, int *pnIndex);
int func_00644BD0(STREAM_SESSION *pSession, STREAM_MEMBER *pMember);
int func_00644C90(void *hAudio, int a1, unsigned short *pMsg);
int func_00644D20(void *hAudio, int a1, int *pMsg);
int func_006451E0(int nError, int nCode);

int func_00643168(int nWorld, unsigned int nStream)
{
    STREAM_SESSION *pSession;
    int result;

    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    if (nStream >= 64)
        return 23;
    result = func_006443D8(nWorld, &pSession);
    if (result != 0)
        return result;
    if (nStream >= pSession->nStreams)
        return 23;
    pSession->nStream = nStream;
    result = func_006451E0(func_0062BF30(pSession->hAudio, pSession->anStreams[pSession->nStream]), 42);
    if (result != 0)
        return result;
    pSession->bStreaming = 1;
    return 0;
}

int func_00643220(int nWorld, unsigned int nStream)
{
    STREAM_SESSION *pSession;
    int result;

    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    if (nStream >= 64)
        return 23;
    result = func_006443D8(nWorld, &pSession);
    if (result != 0)
        return result;
    if (nStream >= pSession->nStreams)
        return 23;
    result = func_006451E0(func_0062BF48(pSession->hAudio, pSession->anStreams[nStream]), 42);
    if (result != 0)
        return result;
    pSession->bStreaming = 0;
    return 0;
}

int func_006432C8(int nWorld, unsigned int nStream, STREAM_REPLY *pReply)
{
    STREAM_SESSION *pSession;
    int result;
    int err;

    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    if (pReply == 0)
        return 23;
    if (nStream >= 64)
        return 23;
    result = func_006443D8(nWorld, &pSession);
    if (result != 0)
        return result;
    if (nStream >= pSession->nStreams)
        return 23;
    pSession->bWaiting = 1;
    pSession->reply.nWorld = nWorld;
    pSession->reply.nStream = nStream;
    err = func_0062BFD0(pSession->hAudio, func_00644C90, 0);
    if (pSession->bWaiting == 1)
        return 41;
    pSession->bWaiting = 0;
    result = func_006451E0(err, 42);
    if (result != 0)
        return result;
    *pReply = pSession->reply;
    return 0;
}

int func_006433D8(int nWorld, int nClient, STREAM_QUERY *pQuery)
{
    STREAM_SESSION *pSession;
    void *pClient = 0;
    int bValid;
    int result;

    if (pQuery == 0)
        return 23;
    pQuery->nWorld = nWorld;
    pQuery->nClient = nClient;
    pQuery->nFound = 0;
    pQuery->nIndex = 0;
    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    bValid = func_0061D440(nClient);
    if (bValid != 1)
        return 23;
    result = func_006443D8(nWorld, &pSession);
    if (result != 0)
        return result;
    if (func_0061C720(nWorld, nClient, &pClient) != 0)
        return 0;
    if (pClient != 0) {
        pSession->bWaiting = bValid;
        pSession->query = *pQuery;
        if (func_0062BFE8(pSession->hAudio, func_00644D20, 0) == 0 && pSession->bWaiting == 0)
            *pQuery = pSession->query;
        pSession->bWaiting = 0;
    }
    return 0;
}

int func_00643530(int nWorld, int nType)
{
    STREAM_SESSION *pSession;
    int nAudioType;
    int result;

    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    result = func_006443D8(nWorld, &pSession);
    if (result != 0)
        return result;
    result = func_006449C0(nType, &nAudioType);
    if (result != 0)
        return result;
    return func_006451E0(func_0062BF90(pSession->hAudio, nAudioType, 0, 0), 42);
}

int func_006435B8(int nWorld, int *pbConnected)
{
    STREAM_SESSION *pSession;
    int result;

    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    if (pbConnected == 0)
        return 23;
    result = func_006443D8(nWorld, &pSession);
    if (result != 0)
        return result;
    if ((func_0062BF60(pSession->hAudio) & 8) && pSession->bConnected == 1)
        *pbConnected = 1;
    else
        *pbConnected = 0;
    return 0;
}

int func_00643650(STREAM_TALK *pTalk)
{
    STREAM_SESSION *pSession;
    int result;

    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    if (pTalk == 0)
        return 23;
    if (func_0061D440(pTalk->nClient) != 1)
        return 23;
    result = func_006443D8(pTalk->nWorld, &pSession);
    if (result != 0)
        return result;
    if (pTalk->bListen != 0) {
        result = func_006451E0(func_0062BF18(pSession->hAudio, pTalk->nClient, 3), 42);
        if (result != 0)
            return result;
    } else {
        result = func_006451E0(func_0062BF18(pSession->hAudio, pTalk->nClient, 0), 42);
        if (result != 0)
            return result;
    }
    return 0;
}

int func_00643730(void)
{
    if (gnInitCount == 0) {
        memset(gapStreamSessions, 0, sizeof(gapStreamSessions));
        gbInitialized = 1;
    }
    gnInitCount++;
    return 0;
}

int func_00643790(void)
{
    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    if (gnInitCount == 1)
        gbInitialized = 0;
    gnInitCount--;
    return 0;
}

/* OPEN (20 words, the right size): the 21 default stores schedule in another order than the
 * target's (1, 0, 9, 10, 2..8, 11..20 there); 17 source orders tried -- offset order, grouped by
 * value, each constant's last use first -- past ccperm's reach (21 statements). */
int func_006437E0(STREAM_PARAMS *pParams)
{
    if (pParams == 0)
        return 23;
    memset(pParams, 0, sizeof(*pParams));
    pParams->an[1] = 0;
    pParams->an[0] = 1;
    pParams->an[9] = 16;
    pParams->an[10] = 8000;
    pParams->an[2] = 1;
    pParams->an[3] = 1;
    pParams->an[4] = 0;
    pParams->an[5] = 1;
    pParams->an[6] = 16;
    pParams->an[7] = 8000;
    pParams->an[8] = 1;
    pParams->an[11] = 0;
    pParams->an[12] = 0;
    pParams->an[13] = 0;
    pParams->an[14] = 0;
    pParams->an[15] = 0;
    pParams->an[16] = 0;
    pParams->an[17] = 0;
    pParams->an[18] = 0;
    pParams->an[19] = 0;
    pParams->an[20] = 0;
    return 0;
}

int func_00643878(int nWorld, unsigned char bEnabled)
{
    STREAM_SESSION *pSession;
    int result;
    int err;

    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    result = func_006443D8(nWorld, &pSession);
    if (result != 0)
        return result;
    pSession->bEnabled = bEnabled;
    err = func_0062BF78(pSession->hAudio);
    pSession->bEnabled = 1;
    return func_006451E0(err, 42);
}

extern int func_00644100(int nWorld, unsigned int nMembers);
extern int func_00644AE8(int nMode, int *pnMode);
extern int func_0062BDF0(void **phAudio, void *pOptions);
extern int func_0062BE58(void *hAudio, int nType, int *pnStream, char *szName);
extern int func_0061D458(void **ppAddress);
extern int func_0062BE40(void *hAudio, void *pAddress, int nPort, void *pfn);
extern int func_00644DE0(void *hAudio, int *pnType, int a2, int a3);
extern int func_00644EA0(void *hAudio, int nType, int a2, int a3, int t0);
extern int func_00644F68(void *hAudio, int a1, int a2, int a3);
extern int func_00645018(void *hAudio, int a1, int a2);
extern int func_006450B0(void *hAudio, int nAddr, int nPort);
extern int sprintf(char *buf, const char *fmt, ...);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern unsigned short data_0067A6E0;
extern int data_00651188;

typedef struct STREAM_ADDRESS {         /* the NAT server's address, func_0061D458's */
    int nKind;                          /* 0x00: 3 */
    char szIp[16];                      /* 0x04 */
    int nPort;                          /* 0x14 */
} STREAM_ADDRESS;

/* a world's media session brought up (0x643900): the parameters copied in, the voice library's options filled from them, a session made
 * with its streams, the NAT server told when there is one. */
int func_00643900(int nWorld, unsigned int nMembers, STREAM_PARAMS *pParams)
{
    char szName[64];
    STREAM_SESSION *pSession;
    int nType;
    STREAM_ADDRESS *pAddress = 0;
    int nStream = 0;
    int nId;
    unsigned int i;
    int j;
    int result;
    int r2;

    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    if (pParams == 0)
        return 23;
    if ((unsigned int)pParams->an[2] > 64)
        return 23;
    result = func_00644100(nWorld, nMembers);
    if (result != 0)
        return result;
    result = func_00644308(nWorld, &pSession);
    if (result != 0)
        return result;
    if (pSession == 0)
        return 41;
    if (pSession->bValid != 0)
        return 41;
    pSession->hAudio = 0;
    pSession->nWorld = nWorld;
    pSession->bReady = 0;
    pSession->nMaxMembers = 256;
    memcpy(&pSession->x10, pParams, sizeof(*pParams));
    pSession->bA8 = 0;
    pSession->bConnected = 0;
    pSession->bStreaming = 0;
    pSession->bEnabled = 0;
    memset(pSession->options, 0, sizeof(pSession->options));
    pSession->options[10] = pSession->x1C;
    pSession->options[0] = data_0067A6E0;
    pSession->options[1] = pSession->x24;
    pSession->options[4] = pSession->x30;
    pSession->options[2] = pSession->x28;
    pSession->options[5] = pSession->x34;
    pSession->options[3] = pSession->x2C;
    pSession->options[6] = pSession->x38;
    pSession->options[7] = pSession->x4C;
    pSession->options[8] = pSession->x50;
    pSession->options[11] = data_00651188;
    if (*(void **)&pParams->an[13] != 0 || pParams->an[11] != 0) {
        pSession->options[12] = (int)func_00644DE0;
        pSession->options[13] = (int)func_00644EA0;
    } else {
        pSession->options[12] = 0;
        pSession->options[13] = 0;
    }
    if (pParams->an[17] != 0)
        pSession->options[14] = (int)func_00644F68;
    else
        pSession->options[14] = 0;
    if (pParams->an[19] != 0)
        pSession->options[15] = (int)func_00645018;
    else
        pSession->options[15] = 0;
    if (pSession->pMembers == 0)
        return 41;
    for (j = 0; j < pSession->nMembers; j++) {
        pSession->pMembers[j].nId = j;
        pSession->pMembers[j].bJoined = 0;
        pSession->pMembers[j].bTalking = 0;
        pSession->pMembers[j].bAnnounced = 0;
    }
    result = func_006451E0(func_0062BDF0(&pSession->hAudio, pSession->options), 42);
    if (result != 0)
        return result;
    result = func_00644AE8(pSession->x20, &nType);
    if (result != 0)
        return result;
    i = 0;
    if (pSession->nStreams != 0) {
        do {
            sprintf(szName, "%d", i);
            result = func_006451E0(func_0062BE58(pSession->hAudio, nType, &nStream, szName), 42);
            if (result != 0)
                return result;
            pSession->anStreams[i++] = nStream;
        } while (i < pSession->nStreams);
    }
    if (i != 0) {
        result = func_006451E0(func_0062BF48(pSession->hAudio, nStream), 42);
        if (result != 0)
            return result;
    }
    pSession->bValid = 1;
    result = func_0061D458((void **)&pAddress);
    if (result != 0)
        return result;
    if (pAddress == 0)
        return 10;
    if (pAddress->nKind == 3) {
        pSession->bA8 = 0;
        result = func_006451E0(func_0062BE40(pSession->hAudio, pAddress->szIp, pAddress->nPort, func_006450B0), 42);
        if (result != 0)
            return result;
    } else {
        pSession->bConnected = 1;
    }
    result = func_006451E0(func_0062BEA0(pSession->hAudio, &nId), 42);
    if (result != 0)
        return result;
    result = func_006451E0(func_0062BEC0(pSession->hAudio, nId, pSession->anInfo), 42);
    if (result != 0)
        return result;
    result = func_00644610((STREAM_PEER *)pSession->anInfo, (STREAM_INFO *)&pSession->nCC);
    if (result != 0)
        return result;
    pSession->bEnabled = 1;
    return result;
}

int func_00643E48(int nWorld)
{
    STREAM_SESSION *pSession;
    int result;

    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    result = func_006443D8(nWorld, &pSession);
    if (result != 0)
        return result;
    func_0062BE70(pSession->hAudio);
    func_0062BFA8(pSession->hAudio);
    pSession->bValid = 0;
    pSession->bReady = 0;
    pSession->nMaxMembers = 256;
    pSession->hAudio = 0;
    pSession->bEnabled = 0;
    func_00644258(nWorld);
    return 0;
}

int func_00643EF0(int nWorld, int nClient)
{
    STREAM_SESSION *pSession;
    STREAM_MEMBER *pMember;
    int nId;
    int bValid;
    int result;

    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    bValid = func_0061D440(nClient);
    if (bValid != 1)
        return 23;
    result = func_006443D8(nWorld, &pSession);
    if (result != 0)
        return result;
    result = func_006451E0(func_0062BE88(pSession->hAudio, nClient), 42);
    if (result != 0)
        return result;
    result = func_006451E0(func_0062BEC0(pSession->hAudio, nClient, pSession->anInfo), 42);
    if (result != 0)
        return result;
    result = func_006451E0(func_0062BEA0(pSession->hAudio, &nId), 42);
    if (result != 0)
        return result;
    result = func_006451E0(func_0062BEC0(pSession->hAudio, nId, pSession->anInfo), 42);
    if (result != 0)
        return result;
    if (pSession->nC0 != nClient)
        return 41;
    pSession->bReady = bValid;
    pSession->nMaxMembers = nClient;
    result = func_00644588(pSession, nClient, &pMember);
    if (result != 0)
        return result;
    pMember->bJoined = bValid;
    pMember->bTalking = bValid;
    pMember->bAnnounced = bValid;
    return 0;
}

int func_00644060(int nWorld)
{
    STREAM_SESSION *pSession;
    int bValid;

    if (gnInitCount <= 0)
        return 0;
    if (gbInitialized != 1)
        return 0;
    bValid = func_0061D450(nWorld);
    if (bValid != 1)
        return 0;
    if (func_00644308(nWorld, &pSession) == 0 && pSession != 0 && pSession->bValid == 1)
        return 1;
    return 0;
}

int func_00644100(int nWorld, unsigned int nMembers)
{
    STREAM_SESSION **ppSession;
    STREAM_SESSION *pSession;

    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    if (func_0061D450(nWorld) != 1)
        return 23;
    if (nWorld >= 4)
        return 23;
    if (nMembers > 256)
        return 23;
    if (nMembers == 0)
        return 23;
    if (gapStreamSessions[nWorld] == 0) {
        if (rt_memory_allocate_init_zero((void **)&gapStreamSessions[nWorld], sizeof(STREAM_SESSION)) != 0)
            return 41;
    }
    ppSession = &gapStreamSessions[nWorld];
    pSession = *ppSession;
    if (pSession == 0)
        return 41;
    if (pSession->pMembers != 0)
        rt_memory_free((void **)&pSession->pMembers);
    if (rt_memory_allocate_init_zero((void **)&pSession->pMembers, nMembers * sizeof(STREAM_MEMBER)) != 0) {
        rt_memory_free((void **)ppSession);
        *ppSession = 0;
        return 41;
    }
    if (pSession->pMembers == 0)
        return 41;
    pSession->nMembers = nMembers;
    pSession->nWorld = nWorld;
    pSession->nMaxMembers = 256;
    pSession->bReady = 0;
    pSession->bValid = 0;
    return 0;
}

int func_00644258(int nWorld)
{
    STREAM_SESSION **ppSession;

    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    if (func_0061D450(nWorld) != 1)
        return 23;
    if (nWorld >= 4)
        return 23;
    ppSession = &gapStreamSessions[nWorld];
    if (*ppSession == 0)
        return 23;
    if ((*ppSession)->pMembers != 0)
        rt_memory_free((void **)&(*ppSession)->pMembers);
    rt_memory_free((void **)ppSession);
    *ppSession = 0;
    return 0;
}

int func_00644308(int nWorld, STREAM_SESSION **ppSession)
{
    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    if (func_0061D450(nWorld) != 1)
        return 23;
    if (ppSession == 0)
        return 23;
    *ppSession = gapStreamSessions[nWorld];
    return 0;
}

int func_00644390(STREAM_SESSION *pSession)
{
    if (pSession != 0 && pSession->bValid == 1 && func_0061D450(pSession->nWorld) == 1)
        return 1;
    return 0;
}

int func_006443D8(int nWorld, STREAM_SESSION **ppSession)
{
    int result;

    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    if (func_0061D450(nWorld) != 1)
        return 23;
    result = func_00644308(nWorld, ppSession);
    if (result != 0)
        return result;
    if (func_00644390(*ppSession) != 1)
        return 41;
    return 0;
}

int func_00644480(void *hAudio, STREAM_SESSION **ppSession)
{
    STREAM_SESSION *pSession;
    int result;
    int i;

    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    if (ppSession == 0)
        return 23;
    *ppSession = 0;
    for (i = 0; i < 4; i++) {
        result = func_00644308(i, &pSession);
        if (result != 0)
            return result;
        if (pSession != 0 && hAudio == pSession->hAudio) {
            *ppSession = pSession;
            break;
        }
    }
    if (func_00644390(*ppSession) != 1)
        return 41;
    return 0;
}

int func_00644540(int nWorld, void **phAudio)
{
    STREAM_SESSION *pSession;
    int result;

    if (phAudio == 0)
        return 23;
    result = func_006443D8(nWorld, &pSession);
    if (result != 0)
        return result;
    *phAudio = pSession->hAudio;
    return 0;
}

int func_00644588(STREAM_SESSION *pSession, int nMember, STREAM_MEMBER **ppMember)
{
    if (ppMember == 0)
        return 23;
    if (func_00644390(pSession) != 1)
        return 23;
    if (pSession->pMembers == 0)
        return 23;
    if (nMember >= pSession->nMembers)
        return 23;
    *ppMember = &pSession->pMembers[nMember];
    if (*ppMember == 0)
        return 41;
    return 0;
}

/* the peer's two addresses in the voice library's form (STREAM_INFO's kinds an enum: above) */
int func_00644610(STREAM_PEER *pPeer, STREAM_INFO *pInfo)
{
    if (pPeer == 0)
        return 23;
    if (pInfo == 0)
        return 23;
    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->nType = 2;
    pInfo->nPort = pPeer->nPort2;
    func_0062BD30(pPeer->nAddr2, pInfo->szAddr);
    pInfo->nType2 = 1;
    pInfo->nPort2 = pPeer->nPort;
    func_0062BD30(pPeer->nAddr, pInfo->szAddr2);
    return 0;
}

int func_00644698(int nWorld, int nClient)
{
    STREAM_SESSION *pSession;
    int nId;
    STREAM_MEMBER *pMember;
    int bValid;
    int result;

    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    bValid = func_0061D440(nClient);
    if (bValid != 1)
        return 23;
    result = func_006443D8(nWorld, &pSession);
    if (result != 0)
        return result;
    if (pSession->bReady != 1)
        return 41;
    result = func_006451E0(func_0062BEA0(pSession->hAudio, &nId), 42);
    if (result != 0)
        return result;
    result = func_006451E0(func_0062BEC0(pSession->hAudio, nId, pSession->anInfo), 42);
    if (result != 0)
        return result;
    result = func_00644588(pSession, nClient, &pMember);
    if (result != 0)
        return result;
    pMember->bJoined = 1;
    if (pSession->bConnected == 1) {
        result = func_00644BD0(pSession, pMember);
        if (result != 0)
            return result;
    }
    return 0;
}

int func_006447C0(int nWorld, int nClient)
{
    STREAM_SESSION *pSession;
    STREAM_MEMBER *pMember;
    int bValid;
    int result;

    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    bValid = func_0061D440(nClient);
    if (bValid != 1)
        return 23;
    result = func_006443D8(nWorld, &pSession);
    if (result != 0)
        return result;
    result = func_00644588(pSession, nClient, &pMember);
    if (result != 0)
        return result;
    if (pMember->bTalking == 1) {
        func_0062BF00(pSession->hAudio, nClient);
        pMember->bTalking = 0;
    }
    pMember->bJoined = 0;
    pMember->bAnnounced = 0;
    return 0;
}

/* OPEN (wrong size, 0x120 against 0x11C): the peer description's seven fields and the frame;
 * not diagnosed further. */
int func_006448A0(STREAM_CLIENT *pClient)
{
    STREAM_PEER peer;
    STREAM_INFO info;
    STREAM_SESSION *pSession;
    STREAM_MEMBER *pMember;
    int bValid;
    int result;

    if (gnInitCount <= 0)
        return 17;
    if (gbInitialized != 1)
        return 17;
    if (pClient == 0)
        return 23;
    result = func_006443D8(pClient->nWorld, &pSession);
    if (result != 0)
        return result;
    bValid = func_0061D440(pClient->nClient);
    if (bValid != 1)
        return 41;
    result = func_00644588(pSession, pClient->nClient, &pMember);
    if (result != 0)
        return result;
    memset(&peer, 0, sizeof(peer));
    peer.nAddr2 = pClient->nAddr2;
    peer.nPort2 = pClient->nPort2;
    peer.nAddr = pClient->nAddr;
    peer.nPort = pClient->nPort;
    peer.nGrid = pClient->nGrid;
    peer.nId = pClient->nClient;
    peer.nFlags = 0;
    result = func_00644610(&peer, &info);
    if (result != 0)
        return result;
    result = func_006451E0(func_0062BED8(pSession->hAudio, &info), 42);
    if (result != 0)
        return result;
    pMember->bTalking = bValid;
    return 0;
}

/* the application's stream types to the voice library's, and back */
int func_006449C0(int nType, int *pnType)
{
    if (pnType == 0)
        return 23;
    switch (nType) {
    case 0:
        *pnType = 33;
        break;
    case 1:
        *pnType = 63;
        break;
    case 2:
        *pnType = 34;
        break;
    case 3:
        *pnType = 36;
        break;
    case 4:
        *pnType = 35;
        break;
    default:
        return 23;
    }
    return 0;
}

int func_00644A58(int nType, int *pnType)
{
    if (pnType == 0)
        return 23;
    switch (nType) {
    case 33:
        *pnType = 0;
        break;
    case 63:
        *pnType = 1;
        break;
    case 34:
        *pnType = 2;
        break;
    case 36:
        *pnType = 3;
        break;
    case 35:
        *pnType = 4;
        break;
    default:
        return 23;
    }
    return 0;
}

int func_00644AE8(int nMode, int *pnMode)
{
    if (pnMode == 0)
        return 23;
    switch (nMode) {
    case 0:
        *pnMode = 3;
        break;
    case 1:
        *pnMode = 0;
        break;
    default:
        return 23;
    }
    return 0;
}

int func_00644B20(STREAM_SESSION *pSession, int nId, int *pnIndex)
{
    unsigned int i;

    if (func_00644390(pSession) != 1)
        return 23;
    if (pnIndex == 0)
        return 23;
    for (i = 0; i < pSession->nStreams; i++) {
        if (nId == pSession->anStreams[i]) {
            *pnIndex = i;
            return 0;
        }
    }
    return 41;
}

int func_00644BD0(STREAM_SESSION *pSession, STREAM_MEMBER *pMember)
{
    STREAM_ANNOUNCE msg;
    int result;

    if (func_00644390(pSession) != 1)
        return 23;
    if (pMember == 0)
        return 23;
    if (pSession->bConnected != 1)
        return 23;
    msg.nWorld = pSession->nWorld;
    msg.nMaxMembers = pSession->nMaxMembers;
    msg.nInfo1 = pSession->anInfo[1];
    msg.nInfo3 = pSession->anInfo[3];
    msg.nInfo0 = pSession->anInfo[0];
    msg.nInfo2 = pSession->anInfo[2];
    msg.nInfo4 = pSession->anInfo[4];
    result = func_00615DE0(80, pSession->nWorld, pMember->nId, data_00686A58, sizeof(msg), &msg);
    if (result == 0)
        pMember->bAnnounced = 1;
    return result;
}

/* the voice library's callbacks: a stream's reply, a client's */
int func_00644C90(void *hAudio, int a1, unsigned short *pMsg)
{
    STREAM_SESSION *pSession = 0;

    if (pMsg == 0)
        return 0;
    if (func_00644480(hAudio, &pSession) != 0)
        return 0;
    if (pSession->reply.nStream >= 64)
        return 0;
    if (pSession->bWaiting != 1)
        return 1;
    if (pSession->anStreams[pSession->reply.nStream] == pMsg[0]) {
        pSession->reply.nValue = pMsg[1];
        pSession->bWaiting = 0;
        return 0;
    }
    return 1;
}

/* the client query's reply. GCC round 74a (open since written; nine guard shapes tried): the 1
 * materialised in v0 before the waiting test is a `return 1` hoisted from below a NESTED if -- the
 * waiting and client tests each wrap the rest (`if (x == 1) { if (y == z) {...} } return 1;`), and the
 * found stores go through a block's own copy of pSession: the int store kills purge_addressof's bare
 * slot, so pSession itself is loaded again for the second store. */
int func_00644D20(void *hAudio, int a1, int *pMsg)
{
    STREAM_SESSION *pSession = 0;

    if (pMsg == 0)
        return 0;
    if (func_00644480(hAudio, &pSession) != 0)
        return 0;
    if (!func_0061D450(pSession->query.nWorld))
        return 0;
    if (!func_0061D440(pSession->query.nClient))
        return 0;
    if (pSession->bWaiting == 1) {
        if (pMsg[5] == pSession->query.nClient) {
            if (pMsg[4] == 0)
                return 1;
            if (func_00644B20(pSession, pMsg[4], &pSession->query.nIndex) != 0)
                return 0;
            {
            STREAM_SESSION *pFound = pSession;

            pFound->query.nFound = 1;
            pFound->bWaiting = 0;
            }
            return 0;
        }
    }
    return 1;
}

/* the voice library's data callbacks, passed on to the application's */
typedef struct STREAM_CB_TYPE {
    int nType;
    int a2;
    int a3;
    int nResult;
    void *pData;
} STREAM_CB_TYPE;

int func_00644DE0(void *hAudio, int *pnType, int a2, int a3)
{
    STREAM_CB_TYPE cb;
    STREAM_SESSION *pSession = 0;

    if (a2 == 0)
        return 0;
    if (pnType == 0)
        return 0;
    if (func_00644480(hAudio, &pSession) != 0)
        return 0;
    cb.nType = 0;
    cb.nResult = 0;
    if (pSession->pfnType != 0 && pSession->bEnabled == 1 && pSession->bConnected == 1) {
        cb.a2 = a2;
        cb.a3 = a3;
        cb.pData = pSession->pTypeData;
        pSession->pfnType(&cb);
    }
    if (func_006449C0(cb.nType, pnType) != 0)
        return 0;
    return cb.nResult;
}

typedef struct STREAM_CB_DATA {
    int nType;
    int a2;
    int a3;
    int t0;
    int nResult;
    void *pData;
} STREAM_CB_DATA;

/* OPEN (5 words): registers -- the type and a3 take s1/s2 the other way round, and the user
 * data is loaded between the stores in the target. */
int func_00644EA0(void *hAudio, int nType, int a2, int a3, int t0)
{
    STREAM_CB_DATA cb;
    STREAM_SESSION *pSession = 0;

    if (a3 == 0)
        return t0;
    if (func_00644480(hAudio, &pSession) != 0)
        return t0;
    cb.nResult = t0;
    if (pSession->pfnData != 0 && pSession->bEnabled == 1 && pSession->bConnected == 1) {
        if (func_00644A58(nType, &cb.nType) != 0)
            return t0;
        cb.a2 = a2;
        cb.a3 = a3;
        cb.t0 = t0;
        cb.pData = pSession->pDataData;
        pSession->pfnData(&cb);
    }
    return cb.nResult;
}

typedef struct STREAM_CB_SEND {
    int a1;
    int a2;
    int a3;
    int nResult;
    void *pData;
} STREAM_CB_SEND;

int func_00644F68(void *hAudio, int a1, int a2, int a3)
{
    STREAM_CB_SEND cb;
    STREAM_SESSION *pSession = 0;

    if (a1 == 0)
        return -1;
    if (a2 == 0)
        return -1;
    if (a3 == 0)
        return -1;
    if (func_00644480(hAudio, &pSession) != 0)
        return -1;
    cb.nResult = 0;
    if (pSession->pfnSend != 0 && pSession->bEnabled == 1 && pSession->bConnected == 1) {
        cb.a1 = a1;
        cb.pData = pSession->pSendData;
        cb.a2 = a2;
        cb.a3 = a3;
        pSession->pfnSend(&cb);
    }
    return cb.nResult;
}

typedef struct STREAM_CB_RECV {
    int a1;
    int a2;
    int nResult;
    void *pData;
} STREAM_CB_RECV;

int func_00645018(void *hAudio, int a1, int a2)
{
    STREAM_CB_RECV cb;
    STREAM_SESSION *pSession = 0;

    if (a2 == 0)
        return -1;
    if (func_00644480(hAudio, &pSession) != 0)
        return -1;
    cb.nResult = 0;
    if (pSession->pfnRecv != 0 && pSession->bEnabled == 1 && pSession->bConnected == 1) {
        cb.a1 = a1;
        cb.a2 = a2;
        cb.pData = pSession->pRecvData;
        pSession->pfnRecv(&cb);
    }
    return cb.nResult;
}

/* the voice library's connect callback: the local address known, every joined member announced */
/* (GCC round 74a, open since written at 37 words: the port stored through a block's own copy of
 * pSession -- the int store kills the bare slot purge_addressof leaves, and the target does not
 * load pSession again for the address argument; the rest followed. The member loop's `bJoined == 1
 * && bAnnounced == 0` folds into one word load and a 0x00FF00FF mask, fold_truthop's.) */
int func_006450B0(void *hAudio, int nAddr, int nPort)
{
    STREAM_SESSION *pSession = 0;
    int nId;
    STREAM_MEMBER *pMember;
    int result;
    int i;

    if (func_00644480(hAudio, &pSession) != 0)
        return -1;
    {
        STREAM_SESSION *p = pSession;

        p->nPort = nPort;
        func_0062BD30(nAddr, p->szAddr);
    }
    result = func_006451E0(func_0062BEA0(pSession->hAudio, &nId), 42);
    if (result != 0)
        return result;
    result = func_006451E0(func_0062BEC0(pSession->hAudio, nId, pSession->anInfo), 42);
    if (result != 0)
        return result;
    pSession->bA8 = 1;
    pSession->bConnected = 1;
    for (i = 0; i < pSession->nMembers; i++) {
        result = func_00644588(pSession, i, &pMember);
        if (result != 0)
            return result;
        if (pMember->bJoined == 1 && pMember->bAnnounced == 0) {
            result = func_00644BD0(pSession, pMember);
            if (result != 0)
                return result;
        }
    }
    return 0;
}

int func_006451E0(int nError, int nCode)
{
    return nError ? nCode : 0;
}
