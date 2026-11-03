/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* DmeLocal.c: DME's internal functions (OVL2 0x613FD0-0x61Dxxx; dme_client/src/DmeLocal.c 1.52, whose
 * $Header heads the .data its functions use -- DMEmalloc's message, NetIpBinaryToString's format), the
 * bodies SOCOM 1's Dme.c had, behind SOCOM II's locked wrappers (dme.c); SCEA's DME client, prebuilt
 * with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). Only the functions written so far are here,
 * in image order. Names are SOCOM 1's where the map has them (s1:exact, s1:calls, s1:order);
 * placeholders elsewhere. DME's error codes are the values returned: 23 a bad argument, 8 an index
 * out of range, 37 the transport refused, 38 not initialised. The world, client, object and stream
 * records are dmelocal.h's (and below), typed: func_0061F410 in DmeMessage.c matches only so.
 *
 * SCEA's shapes, each measured: a getter's result and its pointer tested together, `if (result != 0
 * || p == 0) return result;` (func_006152B0, func_0061C8E8 -- the round 4b OPEN -- NetGetValidClientCount);
 * separate guards where the null pointer has its own code (NetGetClientIpAddress's 15,
 * func_0061C720's 10, and its two range tests); the success path nested -- `if (ok) { ...; return
 * 0; } return ERR;` -- where the target branches away to the error with its code in the delay slot
 * (NetIpBinaryToString, func_00614EC8, func_0061A240); a local initialised inside the block that
 * needs it (func_006147E0, the round 4b OPEN); a table indexed in place, not through a pointer
 * local (func_0061C6C8, the round 4b OPEN). OPEN: func_00614758 (6 words: the transport call's
 * failure branch -- guard, nested and `||` forms each lay one of its two returns out the other way;
 * round 13b: byte-exact under -fno-cse-follow-jumps alone -- CSE follows the jump past `return 37`
 * with the call's v0 known zero and drops the final `move v0,zero`, so reorg cannot put the 37 in
 * the bnez's slot as the target does -- but the flag costs six of the unit's matches, and four more
 * shapes (the success path nested, a result variable) miss), func_00617610 and func_006189C8 (below). func_0061BEB8 returns the second call's result through a
 * variable: no sibling call then. */

#include "dmelocal.h"

#define DME_OK              0
#define DME_ERR_RANGE       8
#define DME_ERR_ARGUMENT    23

#define DME_MAX_PING_SLOTS  127

typedef struct DmePingSlot {
    int field_00;
    int field_04;
    int field_08;
    int bInUse;                         /* 0x0C */
    int field_10;
    int field_14;
    int field_18;
} DmePingSlot;                          /* 0x1C */

typedef struct DmeAddress { int w[6]; } DmeAddress;    /* 24 bytes, copied whole */

extern int printf(const char *fmt, ...);
extern int sprintf(char *buf, const char *fmt, ...);
extern void *memset(void *p, int c, unsigned int n);
extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);

extern int func_00639A90(int nConnection, int *pValue);
extern int func_00614F58(unsigned int nStream, void **ppStream);
extern int NetFreeRemoteObject(int nIndex);
extern int func_006437E0(void *p);
extern int func_00618B38();
extern int func_0061BC68(int);
extern int func_0061CCA8(int);
extern int func_00614888(void **ppMemory);
extern char *strncpy(char *dst, const char *src, unsigned int n);

extern int data_00651178;               /* the world this client is in */
extern int data_006511D8;               /* its connection */
extern void *data_006766E0[DME_MAX_OBJECTS];
extern signed char NetObjectStatus[DME_MAX_OBJECTS];
extern DmeAddress data_0066CF08;
extern DmePingSlot data_0067B7B8[DME_MAX_PING_SLOTS];
extern DmeAddress data_0066CF60;        /* sized: its first word is its size, 24 */
extern void *data_0066CAF8[DME_MAX_WORLDS];

/* a networked object's record (NetGetObject hands one out) */
typedef struct DmeObject {
    unsigned char nClass;               /* 0x00: the registered object type */
    unsigned char field_01;
    unsigned char nStatus;              /* 0x02: 3 and 4 are live */
    unsigned char field_03;
    int nOwner;                         /* 0x04: -1 when unowned */
    int nCreator;                       /* 0x08: the client that made it */
} DmeObject;

extern int func_0061C720(unsigned int nWorld, unsigned int nClient, DmeClientInfo **ppClient);
extern int func_00614EC8(void **ppObject, unsigned int nIndex);
extern int func_00615DE0(int nClass, int nConnection, int nTarget, int nMsgType, int nLen, void *pMsg);
extern void (*data_0067A6E8[16])(int nClient, int nIndex);

/* a data stream's slot (0x44 bytes, 2048 of them, made on first use by func_00614F58) */
typedef struct DmeStreamSlot {
    char field_00[16];
    void *pBuffer;                      /* 0x10 */
    char field_14[24];
    char circ[24];                      /* 0x2C: its rt_circ_buf */
} DmeStreamSlot;

extern DmeStreamSlot *data_006511D0;

extern void (*data_0067A728[16])(int nClient, int nIndex, int nField);
extern void (*data_0067A768[16])(int nClient, int nIndex);
extern int data_00651180;               /* NetInitialize has run */
extern int data_00686A30;               /* the object-deletion message's type */

int func_00614168(unsigned short *pCount)
{
    unsigned int i = 0;
    DmeWorld *pWorld = 0;
    DmeObject *pObject = 0;
    int result;

    if (pCount == 0)
        return DME_ERR_ARGUMENT;
    *pCount = 0;
    if (data_00651178 == DME_MAX_WORLDS)
        return DME_OK;
    if ((unsigned int)data_00651178 >= DME_MAX_WORLDS)
        return DME_ERR_RANGE;
    result = func_0061C600(data_00651178, &pWorld);
    if (result != DME_OK || pWorld == 0)
        return result;
    if (pWorld->nMyClient >= DME_MAX_CLIENTS)
        return DME_ERR_RANGE;
    for (i = 0; i < DME_MAX_OBJECTS; i++) {
        result = func_00614EC8((void **)&pObject, i);
        if (result != DME_OK)
            return result;
        if (pObject != 0 && (unsigned char)(pObject->nStatus - 3) < 2 && pWorld->nMyClient == pObject->nCreator)
            (*pCount)++;
    }
    return DME_OK;
}

/* a client's delay samples (func_0061C510 hands the record out, 0x1A0 bytes made on first use) */
typedef struct DmeDelayRecord {
    unsigned int anDelay[100];          /* 0x000 */
    unsigned int nCount;                /* 0x190 */
    int field_194[3];
} DmeDelayRecord;

extern int func_0061C510(unsigned int nClient, void **ppRecord);

/* the median of a client's last hundred delays (0x6142A8: SOCOM 1's NetGetAverageDelayToClient
 * grown -- the samples bubble-sorted in a copy) */
int func_006142A8(unsigned int *pnDelay, unsigned int nClient)
{
    unsigned int anDelay[100];
    DmeDelayRecord *pRecord = 0;
    unsigned int t;
    int i, j;
    int result;

    if (nClient >= DME_MAX_CLIENTS)
        return DME_ERR_ARGUMENT;
    if (pnDelay == 0)
        return DME_ERR_ARGUMENT;
    result = func_0061C510(nClient, (void **)&pRecord);
    if (result != DME_OK)
        return result;
    if (pRecord == 0)
        return 22;
    for (i = 0; i < 100; i++)
        anDelay[i] = pRecord->anDelay[i];
    for (i = 99; i > 0; i--)
        for (j = 0; j < i; j++)
            if (anDelay[j] > anDelay[j + 1]) {
                t = anDelay[j];
                anDelay[j] = anDelay[j + 1];
                anDelay[j + 1] = t;
            }
    *pnDelay = anDelay[50];
    return DME_OK;
}

/* the delay a twentieth of the way up the samples the record holds (0x614380) */
int func_00614380(unsigned int *pnDelay, unsigned int nClient)
{
    unsigned int anDelay[100];
    DmeDelayRecord *pRecord = 0;
    unsigned int t;
    int i, j, n;
    int result;

    if (nClient >= DME_MAX_CLIENTS)
        return DME_ERR_ARGUMENT;
    if (pnDelay == 0)
        return DME_ERR_ARGUMENT;
    result = func_0061C510(nClient, (void **)&pRecord);
    if (result != DME_OK)
        return result;
    if (pRecord == 0)
        return 22;
    for (i = 0; i < 100; i++)
        anDelay[i] = pRecord->anDelay[i];
    for (i = pRecord->nCount - 1; i > 0; i--)
        for (j = 0; j < i; j++)
            if (anDelay[j] > anDelay[j + 1]) {
                t = anDelay[j];
                anDelay[j] = anDelay[j + 1];
                anDelay[j + 1] = t;
            }
    n = pRecord->nCount / 20;
    if (n < 1)
        n = 1;
    *pnDelay = anDelay[n];
    return DME_OK;
}

extern int DmeToRtMsgConnectionIndex(int nConnection);
extern int NetMapMsgClientError(int nError, int nDefault);
extern int rt_msg_client_get_buffer_status(int nConnection, unsigned int *a, unsigned int *b,
                                           unsigned int *c, unsigned int *d);
extern int func_00639B08(int nP2P, unsigned int *a, unsigned int *b, unsigned int *c, unsigned int *d);

/* SOCOM 1's NetGetBufferStatus (0x6144A0): the peer layer's buffers for the server's connection
 * where SOCOM 1 asked rt_msg_client's UDP; the last pointer tested is the second, as SOCOM 1's */
int func_006144A0(int nConnection, unsigned int *pnA, unsigned int *pnB, unsigned int *pnC,
                  unsigned int *pnD)
{
    unsigned int a, b, c, d;
    int result;

    if (nConnection == 0xFFFF || nConnection == 0) {
        if (func_00639B08(data_006511D8, &a, &b, &c, &d) != 0)
            return 37;
    } else {
        result = NetMapMsgClientError(rt_msg_client_get_buffer_status(DmeToRtMsgConnectionIndex(nConnection),
                                                                      &a, &b, &c, &d), 24);
        if (result != 0)
            return result;
    }
    if (pnA != 0)
        *pnA = a;
    if (pnB != 0)
        *pnB = b;
    if (pnC != 0)
        *pnC = c;
    if (pnB != 0)
        *pnD = d;
    return DME_OK;
}

extern int func_00639F88(int nP2P);
extern int func_00633418(int nConnection, int nTime);
extern int func_006334B8(int nConnection, int nTime);

/* SOCOM 1's NetSetSendAggregationInterval (0x614590): the peer layer's for the server's connection */
int func_00614590(int nConnection, int nTime, int nServerTime)
{
    int result;

    if (nConnection == 0xFFFF || nConnection == 0) {
        if (func_00639F88(data_006511D8) != 0)
            return 37;
    } else {
        result = NetMapMsgClientError(func_00633418(DmeToRtMsgConnectionIndex(nConnection), nTime), 24);
        if (result != 0)
            return result;
        result = NetMapMsgClientError(func_006334B8(DmeToRtMsgConnectionIndex(nConnection), nServerTime), 24);
        if (result != 0)
            return result;
    }
    return DME_OK;
}

typedef struct DmeMsgMetric {
    unsigned int nCount;
    float fRate;
    float fMaxRate;
} DmeMsgMetric;

typedef struct DmeMsgMetrics {          /* rt_msg_client's */
    unsigned int tStart[2];
    DmeMsgMetric sends;                 /* 0x08 */
    DmeMsgMetric bytesSent;             /* 0x14 */
    DmeMsgMetric recvs;                 /* 0x20 */
    DmeMsgMetric bytesReceived;         /* 0x2C */
} DmeMsgMetrics;

typedef struct NetBandwidthInfo {
    float fSendRate;
    float fSendMaxRate;
    float fBytesSentRate;
    float fBytesSentMaxRate;
    float fRecvRate;
    float fRecvMaxRate;
    float fBytesReceivedRate;
    float fBytesReceivedMaxRate;
} NetBandwidthInfo;

extern int rt_msg_client_get_metrics(int nConnection, DmeMsgMetrics *pMetrics);

/* SOCOM 1's, as it was */
int NetGetBandwidthInfo(NetBandwidthInfo *pInfo)
{
    DmeMsgMetrics metrics;
    int result;

    if (pInfo == 0)
        return DME_ERR_ARGUMENT;
    result = NetMapMsgClientError(rt_msg_client_get_metrics(0, &metrics), 24);
    if (result != 0)
        return result;
    pInfo->fBytesSentRate = metrics.bytesSent.fRate;
    pInfo->fBytesSentMaxRate = metrics.bytesSent.fMaxRate;
    pInfo->fSendRate = metrics.sends.fRate;
    pInfo->fSendMaxRate = metrics.sends.fMaxRate;
    pInfo->fBytesReceivedRate = metrics.bytesReceived.fRate;
    pInfo->fBytesReceivedMaxRate = metrics.bytesReceived.fMaxRate;
    pInfo->fRecvRate = metrics.recvs.fRate;
    pInfo->fRecvMaxRate = metrics.recvs.fMaxRate;
    return result;
}

int func_006146D8(int *pValue, unsigned int nWorld)
{
    DmeWorld *pWorld = 0;
    int result;

    if (pValue == 0)
        return DME_ERR_ARGUMENT;
    *pValue = DME_MAX_CLIENTS;
    if (data_00651180 == 0)
        return 17;
    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK)
        return result;
    if (pWorld != 0)
        *pValue = pWorld->nMyClient;
    return result;
}

int func_00614758(int nReserved, int *pValue)
{
    int nValue = -2;

    if (nReserved != 0)
        return DME_ERR_ARGUMENT;
    if (pValue == 0)
        return DME_ERR_ARGUMENT;
    *pValue = 256;
    if (func_00639A90(data_006511D8, &nValue) != 0)
        return 37;
    if (nValue == -2)
        return 1;
    *pValue = nValue;
    return DME_OK;
}

int func_006147E0(void)
{
    DmeWorld *pWorld;

    if ((unsigned int)data_00651178 < DME_MAX_WORLDS) {
        pWorld = 0;
        if (func_0061C600(data_00651178, &pWorld) == 0 && pWorld != 0)
            return pWorld->field_43C;
    }
    return -1;
}

void *DMEmalloc(unsigned int nSize)
{
    void *pMemory = 0;

    if (rt_memory_allocate_init_zero(&pMemory, nSize) != 0)
        printf("rt_memory_allocate failed \n");
    return pMemory;
}

extern unsigned int strlen(const char *s);
extern char *strstr(const char *s, const char *t);
extern int atoi(const char *s);

/* SOCOM 1's NetIpStringToBinary grown (0x6148E0): the string copied with its length bounded, an
 * empty one refused, a missing dot refused */
int func_006148E0(unsigned char *pBinary, char *szIp)
{
    char buf[16];
    char *p = buf;
    char *q;
    int i;

    if (szIp != 0 && pBinary != 0) {
        strncpy(buf, szIp, 16);
        i = 0;
        if (strlen(buf) == 0)
            return DME_ERR_ARGUMENT;
        for (; i < 3; i++) {
            q = strstr(p, ".");
            if (q == 0)
                return 10;
            *q = 0;
            pBinary[i] = atoi(p);
            p = q + 1;
        }
        pBinary[3] = atoi(p);
        return DME_OK;
    }
    return 10;
}

int NetIpBinaryToString(char *szIp, unsigned char *pBinary)
{
    if (szIp != 0 && pBinary != 0) {
        sprintf(szIp, "%u.%u.%u.%u", pBinary[0], pBinary[1], pBinary[2], pBinary[3]);
        return 0;
    }
    return 10;
}

int NetGetClientIpAddress(char *szIp, unsigned int nWorld, unsigned int nClient)
{
    DmeWorld *pWorld = 0;
    DmeClientInfo *pClient = 0;
    int result;

    if (szIp == 0)
        return DME_ERR_ARGUMENT;
    memset(szIp, 0, 16);
    if (nClient >= DME_MAX_CLIENTS)
        return DME_ERR_ARGUMENT;
    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK)
        return result;
    if (pWorld == 0)
        return 15;
    result = func_0061C720(nWorld, nClient, &pClient);
    if (result != DME_OK)
        return result;
    if (pClient == 0)
        return DME_ERR_RANGE;
    if (pClient->bConnected == 0)
        return DME_ERR_RANGE;
    strncpy(szIp, pClient->szIp, 16);
    return DME_OK;
}

void RemoteClientDisconnects(unsigned int nWorld, unsigned int nClient)
{
    DmeWorld *pWorld = 0;
    DmeClientInfo *pClient = 0;

    if (nWorld >= DME_MAX_WORLDS || nClient >= DME_MAX_CLIENTS)
        return;
    if (func_0061C600(nWorld, &pWorld) != 0 || pWorld == 0)
        return;
    if (func_0061C720(nWorld, nClient, &pClient) != 0 || pClient == 0)
        return;
    memset(pClient, 0, sizeof(DmeClientInfo));
}

int func_00614EC8(void **ppObject, unsigned int nIndex)
{
    if (ppObject == 0)
        return DME_ERR_ARGUMENT;
    if (nIndex < DME_MAX_OBJECTS) {
        *ppObject = data_006766E0[nIndex];
        return DME_OK;
    }
    *ppObject = 0;
    return DME_ERR_RANGE;
}

int func_00614F10(void **ppObject, int nIndex)
{
    void *pObject = 0;
    int result;
    result = func_00614F58(nIndex, &pObject);
    if (result != DME_OK)
        return result;
    if (pObject != 0)
        *ppObject = pObject;
    return result;
}
/* SOCOM 1's NetGetDataStream grown (0x614F58): the slot table made on first use, each slot's first
 * byte cleared */
int func_00614F58(unsigned int nStream, void **ppStream)
{
    unsigned int i;

    if (ppStream == 0)
        return DME_ERR_ARGUMENT;
    *ppStream = 0;
    if (nStream >= DME_MAX_STREAMS)
        return DME_ERR_RANGE;
    if (data_006511D0 == 0) {
        data_006511D0 = DMEmalloc(DME_MAX_STREAMS * sizeof(DmeStreamSlot));
        if (data_006511D0 == 0)
            return 22;
        for (i = 0; i < DME_MAX_STREAMS; i++)
            data_006511D0[i].field_00[0] = 0;
    }
    *ppStream = &data_006511D0[nStream];
    return DME_OK;
}


int func_00615090(int *pValue, unsigned int nWorld)
{
    DmeWorld *pWorld = 0;
    int result;

    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    if (pValue == 0)
        return DME_ERR_ARGUMENT;
    *pValue = 256;
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK)
        return result;
    if (pWorld != 0)
        *pValue = pWorld->nSessionMaster;
    return result;
}

int func_006150F8(int *pValue, unsigned int nWorld)
{
    DmeWorld *pWorld = 0;
    int result;

    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    if (pValue == 0)
        return DME_ERR_ARGUMENT;
    *pValue = 0xFFFFFF;
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK)
        return result;
    if (pWorld != 0)
        *pValue = pWorld->field_438;
    return result;
}

int NetGetValidClientCount(int *pCount, unsigned int nWorld)
{
    DmeWorld *pWorld = 0;
    unsigned int i = 0;
    int result;

    if (pCount == 0)
        return DME_ERR_ARGUMENT;
    *pCount = 0;
    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK || pWorld == 0)
        return result;
    for (i = 0; i < DME_MAX_CLIENTS; i++)
        if (pWorld->apClients[i] != 0 && pWorld->apClients[i]->bConnected == 1)
            (*pCount)++;
    return DME_OK;
}

extern int func_00615160(int *pCount, unsigned int nWorld);

int func_00615200(int *pStatus, unsigned int nWorld)
{
    DmeWorld *pWorld = 0;
    int result;

    if (pStatus == 0)
        return DME_ERR_ARGUMENT;
    memset(pStatus, 0, 12);
    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK)
        return result;
    if (pWorld == 0)
        return 15;
    result = func_006150F8(pStatus, nWorld);
    if (result != DME_OK)
        return result;
    result = func_00615160(pStatus + 1, nWorld);
    if (result != DME_OK)
        return result;
    pStatus[2] = pStatus[1] + pWorld->field_440;
    return result;
}

int func_006152B0(char *szVersion, unsigned int nWorld)
{
    DmeWorld *pWorld = 0;
    int result;

    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    if (szVersion == 0)
        return DME_ERR_ARGUMENT;
    memset(szVersion, 0, 16);
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK || pWorld == 0)
        return result;
    strncpy(szVersion, pWorld->szServerVersion, 16);
    return DME_OK;
}

/* the clients a message goes to, as a bit per client (SOCOM 1's NetSetClientTargetActive sets them) */
typedef struct DmeTargetMask {
    unsigned char abTarget[33];         /* 0x00 */
    int nClients;                       /* 0x24 */
    int field_28;                       /* 0x28: -1 */
} DmeTargetMask;

extern int func_006126E8(void **ppClient, unsigned int nWorld, unsigned int nClient);
extern int func_00615330(DmeTargetMask *pMask, int nClient);

/* SOCOM 1's FilterTargetClientBitmask grown (0x615390): the world's clients but this one, each the
 * filter accepts */
void func_00615390(DmeTargetMask *pMask, unsigned int nWorld, int (*pfnFilter)(int nClient, int nMyClient))
{
    DmeWorld *pWorld = 0;
    void *pClient;
    int i, j;

    if (nWorld >= DME_MAX_WORLDS)
        return;
    if (pfnFilter == 0)
        return;
    if (pMask == 0)
        return;
    pMask->nClients = DME_MAX_CLIENTS;
    pMask->field_28 = -1;
    if (func_0061C600(nWorld, &pWorld) != 0)
        return;
    if (pWorld == 0)
        return;
    pMask->nClients = pWorld->field_444;
    for (j = 0; j <= 32; j++)
        pMask->abTarget[j] = 0;
    for (i = 0; i < pWorld->field_444; i++) {
        pClient = 0;
        func_006126E8(&pClient, nWorld, i);
        if (pClient != 0 && i != pWorld->nMyClient && pfnFilter(i, pWorld->nMyClient))
            func_00615330(pMask, i);
    }
}

extern int rt_msg_client_send_target(int nConnection, unsigned short nTarget, char nType, void *pMsg,
                                     unsigned short nLength);

/* SOCOM 1's, as it was: DME's client index to rt_msg_client's target (-1 all, 0xFFFF the server).
 * OPEN (8 words, the right size; SOCOM 1's copy is the target's byte for byte): the target keeps
 * 0xFFFF in t1 for the test and the value (`andi s0,a2,0xffff; movz s0,t1,v0`) and tests -1 with
 * `li v1,-1; xor` -- 2.96 here if-converts the 0xFFFF test the other way (`movn`) and folds the
 * `xor -1` into `nor`; nine shapes (if/else, the chain, a ternary, a switch, unsigned, long and
 * cast constants) give 8 words or worse. */
int SendPacket(char nType, int nConnection, int nTarget, int nLength, void *pMsg)
{
    unsigned short nRtTarget = nTarget;
    int result;

    if (nTarget == 0xFFFF)
        nRtTarget = 0xFFFF;
    if (nTarget == -1)
        nRtTarget = 0xFFFE;
    result = rt_msg_client_send_target(DmeToRtMsgConnectionIndex(nConnection), nRtTarget, nType, pMsg, nLength);
    if (result != 0) {
        printf("SendPacket error sending message, msg error code = %d\n", result);
        return 13;
    }
    return result;
}
extern unsigned char data_006511D4;     /* the transport is up */
extern int func_00616C98(void);
extern int func_0063A838(int nP2P, char *szIp, unsigned short nPort, void *pData, unsigned short nLength);
extern unsigned char data_0066BCF8[512];
extern unsigned short data_0067A6E0;    /* the port */
extern void *memcpy(void *d, const void *s, unsigned int n);

/* SOCOM 1's NetSendMessageByUDPToIP grown (0x615558): the transport brought up first, every argument
 * checked, the datagram sent through the peer layer */
int func_00615558(char nUnused, unsigned char *pIp, unsigned int nType, unsigned int nSubType, int nLength,
                  void *pData)
{
    char szIp[16];
    int result;

    if (data_006511D4 == 0) {
        result = func_00616C98();
        if (result != 0)
            return result;
    }
    if (nType >= 6)
        return DME_ERR_ARGUMENT;
    if (nSubType >= 256)
        return DME_ERR_ARGUMENT;
    if (nLength <= 0)
        return DME_ERR_ARGUMENT;
    if (nLength >= 511)
        return 31;
    if (pData == 0)
        return DME_ERR_ARGUMENT;
    if (pIp == 0)
        return DME_ERR_ARGUMENT;
    data_0066BCF8[0] = nType;
    data_0066BCF8[1] = nSubType;
    memcpy(&data_0066BCF8[2], pData, nLength);
    NetIpBinaryToString(szIp, pIp);
    if (func_0063A838(data_006511D8, szIp, data_0067A6E0, data_0066BCF8, nLength + 2) != 0)
        return 37;
    return DME_OK;
}


void RemoteObjectCreation(unsigned int nClient, unsigned int nIndex)
{
    DmeObject *pObject;

    if (nClient >= DME_MAX_CLIENTS || nIndex >= DME_MAX_OBJECTS)
        return;
    func_00614EC8((void **)&pObject, nIndex);
    if (pObject == 0)
        return;
    if (data_0067A6E8[pObject->nClass] != 0)
        data_0067A6E8[pObject->nClass](nClient, nIndex);
}

void RemoteObjectUpdate(int nClient, unsigned int nIndex, unsigned int nField)
{
    DmeObject *pObject;

    if (nIndex >= DME_MAX_OBJECTS)
        return;
    if (nField >= 64 && nField != -2 && nField != -1)
        return;
    func_00614EC8((void **)&pObject, nIndex);
    if (pObject == 0)
        return;
    if (data_0067A728[pObject->nClass] != 0)
        data_0067A728[pObject->nClass](nClient, nIndex, nField);
}

void RemoteObjectDelete(unsigned int nClient, unsigned int nIndex)
{
    DmeObject *pObject;

    if (nClient >= DME_MAX_CLIENTS || nIndex >= DME_MAX_OBJECTS)
        return;
    func_00614EC8((void **)&pObject, nIndex);
    if (pObject == 0)
        return;
    if (data_0067A768[pObject->nClass] != 0)
        data_0067A768[pObject->nClass](nClient, nIndex);
}

int NetFreeLocalObject(unsigned int nIndex)
{
    unsigned short msg;

    if (nIndex >= DME_MAX_OBJECTS)
        return DME_ERR_ARGUMENT;
    if ((unsigned int)data_00651178 >= DME_MAX_WORLDS)
        return DME_ERR_RANGE;
    NetFreeRemoteObject(nIndex);
    msg = nIndex;
    func_00615DE0(80, data_00651178, -1, data_00686A30, 2, &msg);
    return DME_OK;
}

int func_006161D0(unsigned int nIndex)
{
    DmeObject *pObject;
    DmeWorld *pWorld;
    int result;

    pWorld = 0;
    pObject = 0;
    if (nIndex >= DME_MAX_OBJECTS)
        return DME_ERR_ARGUMENT;
    if ((unsigned int)data_00651178 >= DME_MAX_WORLDS)
        return DME_ERR_RANGE;
    func_00614EC8((void **)&pObject, nIndex);
    if (pObject == 0)
        return DME_ERR_RANGE;
    result = func_0061C600(data_00651178, &pWorld);
    if (result != DME_OK || pWorld == 0)
        return result;
    if (pObject->nCreator == pWorld->nMyClient)
        NetFreeLocalObject(nIndex);
    else
        NetFreeRemoteObject(nIndex);
    return DME_OK;
}

int NetFreeAllObjects(void)
{
    int i;

    for (i = 0; i < DME_MAX_OBJECTS; i++) {
        if (NetObjectStatus[i] == 2)
            NetFreeRemoteObject(i);
        NetObjectStatus[i] = 0;
    }
    return DME_OK;
}

int func_00616F80(int *pMode, int nMode)
{
    if (pMode == 0)
        return DME_ERR_ARGUMENT;
    switch (nMode) {
    case 0:
        *pMode = 0;
        break;
    case 2:
        *pMode = 2;
        break;
    case 3:
        *pMode = 3;
        break;
    case 4:
        *pMode = 4;
        break;
    case 5:
        *pMode = 5;
        break;
    case 6:
        *pMode = 6;
        break;
    case 7:
        *pMode = 7;
        break;
    case 8:
        *pMode = 8;
        break;
    case 9:
        *pMode = 9;
        break;
    case 1:
        *pMode = 1;
        break;
    default:
        return DME_ERR_ARGUMENT;
    }
    return DME_OK;
}

extern unsigned short data_0067A6E0;    /* the port */

/* OPEN (7 words): the scheduler interleaves the port load with the zeroing stores differently
 * (the target loads it right after the store at 8); m2c's store order kept -- nine stores are past
 * tools/ccperm.py's reach. */
int func_00617610(int *pParams)
{
    int result;

    if (pParams == 0)
        return DME_ERR_ARGUMENT;
    memset(pParams, 0, 0x128);
    pParams[1] = 256;
    pParams[2] = 0;
    pParams[41] = 0;
    pParams[43] = 0;
    pParams[73] = data_0067A6E0 + 1;
    pParams[45] = 0;
    pParams[47] = 0;
    pParams[49] = 0;
    pParams[50] = 0;
    result = func_006437E0(pParams + 51);
    if (result == DME_OK)
        pParams[0] = 1;
    return result;
}

int func_00618158(int *pParams)
{
    int result;

    if (pParams == 0)
        return DME_ERR_ARGUMENT;
    memset(pParams, 0, 120);
    pParams[1] = 0;
    pParams[3] = 0;
    pParams[7] = 8;
    pParams[5] = 0;
    result = func_006437E0(pParams + 8);
    if (result == DME_OK)
        pParams[0] = 1;
    return result;
}

int func_00618458(void *pAddress)
{
    if (pAddress == 0)
        return DME_ERR_ARGUMENT;
    data_0066CF08 = *(DmeAddress *)pAddress;
    return DME_OK;
}

int func_00618AD0(int *pParams, int *pStatus)
{
    if (pStatus == 0)
        return DME_ERR_ARGUMENT;
    pStatus[0] = 1;
    pStatus[1] = 30;
    if (pParams == 0) {
        pStatus[1] = DME_ERR_ARGUMENT;
        return DME_ERR_ARGUMENT;
    }
    if (pParams[0] != 1)
        return 38;
    return pStatus[1] = func_00618B38(pParams);
}

extern int DmeToRtMsgConnectionIndex(int nConnection);
extern int rt_msg_client_receive_broadcasts(int nConnection, unsigned char bEnable);
extern int NetMapMsgClientError(int nError, int nDefault);
extern int func_0063A760(int nConnection, unsigned char bEnable, int nIndex);

/* OPEN (1 word): the target branches with beq on 0xFFFF where 2.96 here takes beql (the lui of
 * data_006511D8 in the slot either way) -- the RT path first (`!= 0 && != 0xFFFF`), an if/else,
 * else-if and a result variable tried; the broadcast flag is an int narrowed at each call. */
int func_006189C8(unsigned int nConnection, int bEnable)
{
    int result;

    if (nConnection >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    if (nConnection != 0 && nConnection != 0xFFFF) {
        result = NetMapMsgClientError(rt_msg_client_receive_broadcasts(DmeToRtMsgConnectionIndex(nConnection), bEnable), 24);
        if (result != DME_OK)
            return result;
    } else if (func_0063A760(data_006511D8, bEnable, nConnection) != 0)
        return 37;
    return DME_OK;
}

int FindFreePingSlot(void)
{
    int i;

    for (i = 0; i < DME_MAX_PING_SLOTS; i++)
        if (data_0067B7B8[i].bInUse == 0)
            return i;
    return DME_MAX_PING_SLOTS;
}

/* a data stream's record (NetGetDataStream hands one out) */
typedef struct DmeStream {
    unsigned char bOpen;                /* 0x00 */
    unsigned char field_01[3];
    unsigned char bClosing;             /* 0x04 */
    unsigned char field_05[7];
    int nOwner;                         /* 0x0C */
} DmeStream;

int func_0061A078(unsigned int nStream)
{
    DmeWorld *pWorld;
    DmeStream *pStream;
    int result;

    pStream = 0;
    pWorld = 0;
    if (nStream >= DME_MAX_STREAMS)
        return DME_ERR_ARGUMENT;
    if ((unsigned int)data_00651178 >= DME_MAX_WORLDS)
        return 9;
    result = func_0061C600(data_00651178, &pWorld);
    if (result != DME_OK || pWorld == 0)
        return result;
    result = func_00614F10((void **)&pStream, nStream);
    if (result != DME_OK)
        return 9;
    if (pStream == 0 || pStream->nOwner != pWorld->nMyClient)
        return 9;
    pStream->bClosing = 1;
    return result;
}

extern int func_0061A800(int nOwner, unsigned int nIndex);
extern int data_00686A38;               /* the ownership release's message type */

int func_0061A240(unsigned int nIndex)
{
    DmeWorld *pWorld;
    DmeObject *pObject;
    unsigned short msg[1];
    int result;

    pObject = 0;
    pWorld = 0;
    if (nIndex >= DME_MAX_OBJECTS)
        return DME_ERR_ARGUMENT;
    if ((unsigned int)data_00651178 >= DME_MAX_WORLDS)
        return DME_ERR_RANGE;
    result = func_0061C600(data_00651178, &pWorld);
    if (result != DME_OK || pWorld == 0)
        return result;
    func_00614EC8((void **)&pObject, nIndex);
    if (pObject == 0)
        return DME_ERR_RANGE;
    if (pObject->nOwner == pWorld->nMyClient) {
        pObject->nOwner = -1;
        RemoteObjectUpdate(-1, nIndex, -2);
        msg[0] = nIndex;
        func_0061A800(-1, nIndex);
        func_00615DE0(80, data_00651178, -1, data_00686A38, 2, msg);
        return DME_OK;
    }
    return DME_ERR_RANGE;
}

int func_0061BEB8(void)
{
    int result = func_0061BC68(0);

    if (result == DME_OK)
        result = func_0061CCA8(0);
    return result;
}

int func_0061C0F0(int *pAddress)
{
    if (pAddress == 0)
        return 10;
    if (pAddress[0] != sizeof(DmeAddress))
        return 39;
    *(DmeAddress *)pAddress = data_0066CF60;
    return DME_OK;
}

extern void *data_0066CB08[DME_MAX_CLIENTS];   /* each client's delay record, made on first use */

int func_0061C510(unsigned int nClient, void **ppRecord)
{
    if (ppRecord == 0)
        return DME_ERR_ARGUMENT;
    *ppRecord = 0;
    if (nClient >= DME_MAX_CLIENTS)
        return DME_ERR_ARGUMENT;
    if (data_0066CB08[nClient] == 0) {
        data_0066CB08[nClient] = DMEmalloc(0x1A0);
        if (data_0066CB08[nClient] == 0)
            return 22;
    }
    *ppRecord = data_0066CB08[nClient];
    return DME_OK;
}

int func_0061C638(unsigned int nWorld, DmeWorld **ppWorld)
{
    if (ppWorld == 0)
        return DME_ERR_ARGUMENT;
    *ppWorld = 0;
    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    if (data_0066CAF8[nWorld] == 0) {
        data_0066CAF8[nWorld] = DMEmalloc(sizeof(DmeWorld));
        if (data_0066CAF8[nWorld] == 0)
            return 22;
    }
    *ppWorld = data_0066CAF8[nWorld];
    return DME_OK;
}

int func_0061C6C8(unsigned int nWorld)
{
    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    if (data_0066CAF8[nWorld] != 0)
        func_00614888(&data_0066CAF8[nWorld]);
    data_0066CAF8[nWorld] = 0;
    return DME_OK;
}

int func_0061C720(unsigned int nWorld, unsigned int nClient, DmeClientInfo **ppClient)
{
    DmeWorld *pWorld = 0;
    int result;

    if (ppClient == 0)
        return DME_ERR_ARGUMENT;
    *ppClient = 0;
    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    if (nClient >= DME_MAX_CLIENTS)
        return DME_ERR_ARGUMENT;
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK)
        return result;
    if (pWorld == 0)
        return 10;
    *ppClient = pWorld->apClients[nClient];
    return result;
}

int func_0061C7A8(unsigned int nWorld, unsigned int nClient, DmeClientInfo **ppClient)
{
    DmeWorld *pWorld = 0;
    int result;

    if (ppClient == 0)
        return DME_ERR_ARGUMENT;
    *ppClient = 0;
    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    if (nClient >= DME_MAX_CLIENTS)
        return DME_ERR_ARGUMENT;
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK)
        return result;
    if (pWorld == 0)
        return 10;
    if (pWorld->apClients[nClient] == 0) {
        pWorld->apClients[nClient] = DMEmalloc(sizeof(DmeClientInfo));
        if (pWorld->apClients[nClient] == 0)
            return 22;
    }
    *ppClient = pWorld->apClients[nClient];
    return DME_OK;
}

int func_0061C868(unsigned int nWorld, unsigned int nClient)
{
    DmeWorld *pWorld = 0;
    int result;

    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    if (nClient >= DME_MAX_CLIENTS)
        return DME_ERR_ARGUMENT;
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK)
        return result;
    if (pWorld != 0) {
        if (pWorld->apClients[nClient] != 0)
            func_00614888((void **)&pWorld->apClients[nClient]);
        pWorld->apClients[nClient] = 0;
    }
    return DME_OK;
}

char *func_0061C8E8(unsigned int nWorld)
{
    DmeWorld *pWorld = 0;

    if (nWorld >= DME_MAX_WORLDS)
        return 0;
    if (func_0061C600(nWorld, &pWorld) != 0 || pWorld == 0)
        return 0;
    return pWorld->szName;
}

int func_0061C930(unsigned int nWorld, const char *szName)
{
    DmeWorld *pWorld = 0;
    int result;

    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_RANGE;
    if (szName == 0)
        return DME_ERR_ARGUMENT;
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK)
        return result;
    if (pWorld != 0) {
        strncpy(pWorld->szName, szName, 16);
        return DME_OK;
    }
    return result;
}

int func_0061C998(unsigned int nWorld, int nValue)
{
    DmeWorld *pWorld = 0;
    int result;

    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_RANGE;
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK)
        return result;
    if (pWorld != 0)
        pWorld->nSessionMaster = nValue;
    return result;
}

int func_0061CB18(unsigned char *pbValue)
{
    int nValue = -2;
    DmeWorld *pWorld = 0;
    int result;

    if (pbValue == 0)
        return DME_ERR_ARGUMENT;
    *pbValue = 0;
    result = func_0061C600(0, &pWorld);
    if (result != DME_OK || pWorld == 0)
        return result;
    if (func_00639A90(data_006511D8, &nValue) != 0)
        return 37;
    if (pWorld->nMyClient != DME_MAX_CLIENTS && nValue == pWorld->nMyClient)
        *pbValue = 1;
    return DME_OK;
}

extern int func_0061D440(int nClient);

/* OPEN (20s): returns 30 when the record is there (the target's `li v0,30` beside the sltu); the
 * range guard's and the two calls' layout differ -- separate and `||` guards tried. */
int func_0061D108(unsigned int nWorld, int nClient, int *pbConnected)
{
    DmeClientInfo *pClient = 0;
    int result;

    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    if (func_0061D440(nClient) != 1)
        return DME_ERR_ARGUMENT;
    if (pbConnected == 0)
        return DME_ERR_ARGUMENT;
    *pbConnected = 0xFFFFFF;
    result = func_0061C720(nWorld, nClient, &pClient);
    if (result != DME_OK)
        return result;
    if (pClient != 0) {
        *pbConnected = pClient->bConnected != 0;
        result = 30;
    }
    return result;
}

extern int rt_circ_buf_destroy(void *pCircBuf);

/* OPEN (35s): the target tests each slot's circular buffer by its address (`&slot->circ != 0`, an
 * addu and bnez the target keeps) and reloads the table pointer per slot; first shape only. */
int func_0061D320(void)
{
    unsigned int i;

    if (data_006511D0 != 0) {
        for (i = 0; i < DME_MAX_STREAMS; i++) {
            if (data_006511D0[i].pBuffer != 0)
                func_00614888(&data_006511D0[i].pBuffer);
            if (data_006511D0[i].circ != 0)
                rt_circ_buf_destroy(data_006511D0[i].circ);
        }
    }
    return DME_OK;
}

extern int func_0061C150(unsigned int nWorld);

int func_0061D3C8(void)
{
    DmeWorld *pWorld = 0;
    unsigned int i = 0;

    for (i = 0; i < DME_MAX_WORLDS; i++)
        if (func_0061C600(i, &pWorld) == 0 && pWorld != 0 && pWorld->field_438 == 3)
            func_0061C150(i);
    return DME_OK;
}

extern void func_00617060(int nWorld, int nClient, int bJoined, unsigned short nPort, int bFailed);

void func_0061D4D8(int nClient, unsigned short nPort, int nError)
{
    if (nError == 0 && nClient != -2) {
        func_00617060(0, nClient, 1, nPort, 0);
        return;
    }
    func_00617060(0, DME_MAX_CLIENTS, 0, 0, 1);
}

extern void (*data_00651198)(int *pInfo);
extern int data_0065119C;

/* the connection to the server lost (0x61D530): the world's link closed, the host told why.
 * OPEN (9 words, the right size): the report's three stores before the reason's test -- the target
 * loads pWorld into v1 ahead of them and keeps 1 in a1, 11 in a2; every order of the five
 * statements (tools/ccperm.py, an int array and a five-int struct alike) comes within 5 words at
 * best (the world's field stored first), a ternary for the reason is if-converted (the wrong size). */
void func_0061D530(int nReason)
{
    int info[5];
    DmeWorld *pWorld = 0;

    if (func_0061C600(0, &pWorld) != 0)
        return;
    if (pWorld == 0)
        return;
    func_0061C150(0);
    if (data_00651198 != 0) {
        memset(info, 0, sizeof(info));
        info[2] = 1;
        info[0] = 0;
        info[1] = pWorld->field_438;
        if (nReason == 11)
            info[3] = 14;
        else
            info[3] = 37;
        info[4] = data_0065119C;
        data_00651198(info);
    }
}

extern void (*data_006511A8)(int *pInfo);
extern int data_006511AC;

void func_0061D5E8(int nValue)
{
    int info[3];

    if (data_006511A8 != 0) {
        memset(info, 0, sizeof(info));
        info[0] = 0;
        info[1] = nValue;
        info[2] = data_006511AC;
        data_006511A8(info);
    }
}

/* this client's address as the peer layer resolved it (0x6766A0: 48 bytes, copied whole into the
 * report the host's callback gets) */
typedef struct DmeLocalAddress {
    int field_00;
    char szIp[16];                      /* 0x04 */
    int nPort;                          /* 0x14 */
    int field_18[6];
} DmeLocalAddress;                      /* 0x30 */

typedef struct DmeAddressReport {
    int field_00;                       /* 1 */
    int nResult;                        /* 0x04: 0, or 37 when the resolution failed */
    DmeLocalAddress address;            /* 0x08 */
} DmeAddressReport;                     /* 0x38 */

extern DmeLocalAddress data_006766A0;
extern DmeAddressReport data_0066CF20;
extern void (*data_0066CF58)(DmeAddressReport *pReport);

/* the address resolution's callback (0x61D650): the address kept, and this client's own record */
void func_0061D650(char *szIp, unsigned short nPort, int nError)
{
    DmeWorld *pWorld = 0;
    DmeClientInfo *pClient = 0;

    if (nError == 0) {
        data_006766A0.nPort = nPort;
        strncpy(data_006766A0.szIp, szIp, 16);
        if (func_0061C600(0, &pWorld) == 0 && pWorld != 0
            && func_0061C720(0, pWorld->nMyClient, &pClient) == 0 && pClient != 0) {
            strncpy(pClient->szIp, szIp, 16);
            pClient->nPort = nPort;
        }
    }
    data_0066CF20.field_00 = 1;
    data_0066CF20.nResult = nError == 0 ? 0 : 37;
    data_0066CF20.address = data_006766A0;
    data_0066CF58(&data_0066CF20);
}

extern int func_006398C0(int nP2P, int nConnection, int *pnWhich);
extern int func_00639940(int nP2P, int nConnection, int nWhich, char *szIp, unsigned short *pnPort);
extern int func_00614B18(int nWorld, int nClient, char *szIp, unsigned short nPort);
extern int func_006164E0(int nWorld, int nClient);
extern void func_00616478(int nWorld, int nClient, unsigned short nConnection, int nValue,
                          unsigned short nPort);
extern char data_006766D0[16];
extern unsigned short data_0067B7A8;

/* the peer layer's connection event (0x61D7A8): 0 a peer connected, 1 and 2 it left */
void func_0061D7A8(int nConnection, unsigned int nEvent)
{
    int nWhich;
    DmeWorld *pWorld;

    switch (nEvent) {
    case 0:
        nWhich = -1;
        pWorld = 0;
        if (func_006398C0(data_006511D8, nConnection, &nWhich) != 0)
            return;
        if (func_00639940(data_006511D8, nConnection, nWhich, data_006766D0, &data_0067B7A8) != 0)
            return;
        if (func_0061C600(0, &pWorld) == 0 && pWorld != 0)
            pWorld->field_440++;
        func_00614B18(0, nConnection, data_006766D0, data_0067B7A8);
        break;
    case 1:
    case 2:
        func_006164E0(0, nConnection);
        break;
    }
}

/* (0x61D898) */
void func_0061D898(int nConnection, int nValue, unsigned short nPort)
{
    int nWhich = -1;

    if (func_006398C0(data_006511D8, nConnection, &nWhich) == 0
        && func_00639940(data_006511D8, nConnection, nWhich, data_006766D0, &data_0067B7A8) == 0)
        func_00616478(func_006147E0(), 0, nConnection, nValue, nPort);
}

/* (0x61D948) */
void func_0061D948(char *szIp, unsigned short nPort, int nValue, unsigned short nPort2)
{
    strncpy(data_006766D0, szIp, 16);
    data_0067B7A8 = nPort;
    func_00616478(func_006147E0(), 0xFFFF, 256, nValue, nPort2);
}

/* a message for the host (0x61D9F8), its fields in a record its callback gets */
typedef struct DmeHostMessage {
    int nClient;                        /* 0x00 */
    unsigned char nType;                /* 0x04 */
    int field_08;
    int field_0C;
    unsigned char field_10;
    unsigned short field_12;
    int field_14;
    int field_18;                       /* the host's user data */
} DmeHostMessage;

extern void (*data_006511B8)(DmeHostMessage *pMessage);
extern int data_006511BC;
extern int RtMsgToDmeConnectionIndex(unsigned short nConnection);

void func_0061D9F8(unsigned short nConnection, unsigned char nType, int a2, int a3, unsigned char a4,
                   unsigned short a5, int a6)
{
    DmeHostMessage msg;
    void (*pfn)(DmeHostMessage *) = data_006511B8;

    if (pfn != 0) {
        msg.nClient = RtMsgToDmeConnectionIndex(nConnection);
        msg.field_08 = a2;
        msg.nType = nType;
        msg.field_0C = a3;
        msg.field_10 = a4;
        msg.field_12 = a5;
        msg.field_14 = a6;
        msg.field_18 = data_006511BC;
        pfn(&msg);
    }
}
