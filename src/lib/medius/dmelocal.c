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
 * local (func_0061C6C8, the round 4b OPEN). func_00614758 (open since round 4b: CSE followed the
 * jump past `return 37` with the call's v0 known zero and dropped the final `move v0,zero`) matched
 * in round 14b as SCEA wrote it -- the tests positive, their errors in elses (`if (nValue != -2)
 * *pValue = nValue; else return 1;` inside `if (f(...) == 0) {...} else return 37;`). The same body
 * with the call's test a guard is byte-exact too but moves func_0061C510 (its element address's
 * high part then kept in s3 across DMEmalloc), and func_00615B48's loop nested one level deeper
 * moves func_0061C7A8 the same way: a later function's allocation follows an earlier one's
 * spelling, so each spelling here is the one that keeps both. OPEN: func_00617610 and
 * func_006189C8 (below). func_0061BEB8 returns the second call's result through a
 * variable: no sibling call then. */

#include "dmelocal.h"

#define DME_OK              0
#define DME_ERR_RANGE       8
#define DME_ERR_ARGUMENT    23

#define DME_MAX_PING_SLOTS  127

typedef struct DmePingSlot {           /* a ping in flight (round 20b: 0x6192A0 fills one) */
    unsigned char abIp[4];              /* 0x00: the address pinged, binary */
    unsigned short nPort;               /* 0x04 */
    short field_06;
    int bTimebase;                      /* 0x08: 1 a timebase ping, its callback told world and client */
    void (*pfnCallback)();              /* 0x0C: null when the slot is free */
    unsigned int nSent;                 /* 0x10: the local time it was sent */
    int nWorld;                         /* 0x14 */
    int nClient;                        /* 0x18 */
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
extern int func_0061BC68(unsigned char bBlock);
extern int func_0061CCA8(unsigned char bReport);
extern void func_00614888(void **ppMemory);
extern char *strncpy(char *dst, const char *src, unsigned int n);

extern int data_00651178;               /* the world this client is in */
extern int data_006511D8;               /* its connection */
extern void *data_006766E0[DME_MAX_OBJECTS];
extern signed char NetObjectStatus[DME_MAX_OBJECTS];
extern DmeAddress data_0066CF08;
extern DmePingSlot data_0067B7B8[DME_MAX_PING_SLOTS];
/* the last update's status (0x61BC68 fills it, 0x61C0F0 hands it out): its size, each world's code, the peer layer's */
typedef struct DmeStatus {
    int nSize;                          /* 0x00: 24 */
    int aStatus[DME_MAX_WORLDS];        /* 0x04 */
    int nP2p;                           /* 0x14 */
} DmeStatus;

extern DmeStatus data_0066CF60;
extern void *data_0066CAF8[DME_MAX_WORLDS];

/* a networked object's record (NetGetObject hands one out) */
typedef struct DmeObject {
    unsigned char nClass;               /* 0x00: the registered object type */
    unsigned char field_01;
    unsigned char nStatus;              /* 0x02: 3 and 4 are live */
    unsigned char field_03;
    int nOwner;                         /* 0x04: -1 when unowned */
    int nCreator;                       /* 0x08: the client that made it */
} DmeObject;                            /* a named object's name follows at 0x0C */

/* a client's own record (clients.c's): its block of networked objects */
typedef struct DmeClient {
    int field_00;
    unsigned short nMyObject;           /* 0x04 */
    unsigned short nFirstObject;        /* 0x06 */
    unsigned short nObjects;            /* 0x08 */
    unsigned short nFirstStream;        /* 0x0A */
    unsigned short nStreams;            /* 0x0C */
    char szName[14];                    /* 0x0E */
} DmeClient;

extern int func_0061C720(unsigned int nWorld, unsigned int nClient, DmeClientInfo **ppClient);
extern int func_00614EC8(void **ppObject, unsigned int nIndex);
extern int func_00615DE0(char nMsgClass, unsigned int nWorld, int nTarget, unsigned int nType, int nLength,
                         void *pData);
extern void (*data_0067A6E8[16])(int nClient, int nIndex);

/* a data stream's slot (0x44 bytes, 2048 of them, made on first use by func_00614F58) */
typedef struct DmeStreamSlot {
    char field_00[16];                  /* 0x00: the state (2 open), a flag at 0x04 */
    void *pBuffer;                      /* 0x10 */
    char field_14[24];
    char circ[24];                      /* 0x2C: its rt_circ_buf */
} DmeStreamSlot;

extern DmeStreamSlot *data_006511D0;

extern void (*data_0067A728[16])(int nClient, int nIndex, int nField);
extern void (*data_0067A768[16])(int nClient, int nIndex);
extern int data_00651180;               /* NetInitialize has run */
extern int data_00686A30;               /* the object-deletion message's type */

int func_00614168(unsigned short *pCount);
extern int func_0061D448(int nClient);
extern int data_00686A50;               /* this client's state report's message type */

/* this client's state report (16 bytes) */
typedef struct DmeClientReport {
    unsigned char nState;               /* 0x00 */
    unsigned char nClient;              /* 0x01 */
    unsigned char field_02;             /* 0x02 */
    unsigned char field_03;             /* 0x03 */
    unsigned short nObjects;            /* 0x04: the live objects it made (func_00614168) */
    int nValue;                         /* 0x08 */
    int nWorldValue;                    /* 0x0C */
} DmeClientReport;

/* new in SOCOM II (0x613FD0, DmeLocal.c's first function): this client's state report, sent to one
 * client or (nTarget -1) to every other connected one */
void func_00613FD0(unsigned int nWorld, int nTarget)
{
    DmeClientReport msg;
    DmeWorld *pWorld = 0;
    DmeClientInfo *pClient = 0;
    unsigned int nMyClient;
    unsigned int i;

    if (nWorld >= DME_MAX_WORLDS)
        return;
    if (func_0061C600(nWorld, &pWorld) != 0)
        return;
    if (pWorld == 0)
        return;
    nMyClient = pWorld->nMyClient;
    if (nMyClient >= DME_MAX_CLIENTS)
        return;
    if (func_0061C720(nWorld, nMyClient, &pClient) != 0)
        return;
    if (pClient == 0)
        return;
    msg.nClient = func_0061D448(nMyClient);
    msg.nState = pClient->nState;
    msg.field_02 = pClient->field_18;
    msg.field_03 = pClient->field_1C;
    msg.nValue = pClient->nValue;
    msg.nWorldValue = pWorld->field_448;
    if (func_00614168(&msg.nObjects) != 0)
        return;
    if (nTarget == -1) {
        for (i = 0; i < DME_MAX_CLIENTS; i++) {
            if (func_0061C720(nWorld, i, &pClient) != 0)
                return;
            if (pClient != 0 && pClient->bConnected == 1 && i != nMyClient)
                func_00615DE0(80, nWorld, i, data_00686A50, sizeof(msg), &msg);
        }
        return;
    }
    func_00615DE0(80, nWorld, nTarget, data_00686A50, sizeof(msg), &msg);
}

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

/* DME's connection indices against rt_msg_client's (SOCOM 1's two, s1:exact): DME counts the
 * server connections from 1, rt_msg_client from 0. Defined here, above every caller -- a call
 * compiled after its callee keeps a branch's slot unannulled (func_006189C8's beq; round 13c). */
int DmeToRtMsgConnectionIndex(int nConnection)
{
    if (nConnection < 5)
        return (nConnection - 1) & 0xFFFF;
    return nConnection & 0xFFFF;
}

int RtMsgToDmeConnectionIndex(unsigned short nConnection)
{
    return nConnection < 5 ? nConnection + 1 : nConnection;
}

extern int data_006511C4;
extern int data_006511C8;

/* the two words NetConnect's peer path passes on (func_006180B8's params 0x0C/0x10) */
int func_00614290(int nA, int nB)
{
    data_006511C4 = nA;
    data_006511C8 = nB;
    return DME_OK;
}

/* a client's delay samples (func_0061C510 hands the record out, 0x1A0 bytes made on first use) */
typedef struct DmeDelayRecord {
    unsigned int anDelay[100];          /* 0x000 */
    unsigned int nCount;                /* 0x190 */
    int nNext;                          /* 0x194: the next sample's slot */
    int tStamp[2];                      /* 0x198: the last sample's time */
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
    if (func_00639A90(data_006511D8, &nValue) == 0) {
        if (nValue != -2)
            *pValue = nValue;
        else
            return 1;
    } else
        return 37;
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

extern int CurrentNetTime;              /* SOCOM 1's name: the network clock's base */

/* SOCOM 1's NetResetTime (0x614838), byte-identical, before DMEmalloc as there */
void NetResetTime(int nTime)
{
    CurrentNetTime = nTime;
}

void *DMEmalloc(unsigned int nSize)
{
    void *pMemory = 0;

    if (rt_memory_allocate_init_zero(&pMemory, nSize) != 0)
        printf("rt_memory_allocate failed \n");
    return pMemory;
}

extern int rt_memory_free(void **ppMemory);

/* SOCOM 1's DmeFree (0x614888, round 5b's name by the handlers' calls): the block freed when there
 * is one -- a sibling call under the frame it keeps for the other path */
void func_00614888(void **ppMemory)
{
    if (*ppMemory != 0)
        rt_memory_free(ppMemory);
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

extern int rt_time_get_local(unsigned int *pTime);

/* NetGetLocalTime's body (0x615028, SOCOM 1's NetGetLocalTime with a null test) stands as its matched
 * draft: written here (`if (!p) return 23; rt_time_get_local(&t); *p = t; return 0;`, byte-exact) it
 * moves func_0061C7A8's registers after DMEmalloc (23s) -- round 14b's "an earlier function's
 * spelling moves a later one's", a third time */
extern int func_00615028(unsigned int *pnTime);

/* NetGetTime's body (0x615068): SOCOM 1's NetGetTime with a null test */
int func_00615068(int *pnTime)
{
    if (pnTime == 0)
        return DME_ERR_ARGUMENT;
    *pnTime = CurrentNetTime;
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
    char abTarget[33];                  /* 0x00: a bit per client */
    int nFirst;                         /* 0x24: the lowest client set */
    int nLast;                          /* 0x28: the highest, -1 for none */
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
    pMask->nFirst = DME_MAX_CLIENTS;
    pMask->nLast = -1;
    if (func_0061C600(nWorld, &pWorld) != 0)
        return;
    if (pWorld == 0)
        return;
    pMask->nFirst = pWorld->field_444;
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
extern int func_0061D478(char nMsgClass, unsigned char *pbReliable);
extern char *func_0061C8E8(unsigned int nWorld);
extern int func_0063A618(int nP2P, int nTarget, unsigned char bReliable, void *pData, unsigned short nLength);
extern unsigned char data_0066BEF8[512];
extern unsigned char data_0066C0F8[512];

/* SOCOM 1's NetSendMessageByUDP grown (0x615660): to every peer (0xFFFF, by the world's name) or to one
 * other client, through the peer layer */
int func_00615660(char nMsgClass, unsigned int nWorld, int nTarget, unsigned int nType, unsigned int nSubType,
                  int nLength, void *pData)
{
    DmeWorld *pWorld = 0;
    unsigned char bReliable = 0;
    int result;

    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
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
    result = func_0061D478(nMsgClass, &bReliable);
    if (result != DME_OK)
        return result;
    data_0066BEF8[0] = nType;
    data_0066BEF8[1] = nSubType;
    memcpy(&data_0066BEF8[2], pData, nLength);
    if (nTarget == 0xFFFF) {
        if (func_0063A838(data_006511D8, func_0061C8E8(nWorld), data_0067A6E0, data_0066BEF8, nLength + 2) != 0)
            return 37;
    } else {
        result = func_0061C600(nWorld, &pWorld);
        if (result != DME_OK || pWorld == 0)
            return result;
        if (nTarget != pWorld->nMyClient) {
            if (func_0063A618(data_006511D8, nTarget, bReliable, data_0066BEF8, nLength + 2) != 0)
                return 37;
        }
    }
    return result;
}

/* SOCOM 1's NetSendMessageByTCP's place (0x6157E0): to one other client through SendPacket */
int func_006157E0(char nMsgClass, unsigned int nWorld, int nTarget, unsigned int nType, unsigned int nSubType,
                  int nLength, void *pData)
{
    DmeWorld *pWorld = 0;
    int result;

    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
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
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK || pWorld == 0)
        return result;
    if (nTarget == pWorld->nMyClient)
        return result;
    data_0066C0F8[0] = nType;
    data_0066C0F8[1] = nSubType;
    memcpy(&data_0066C0F8[2], pData, nLength);
    return SendPacket(nMsgClass, nWorld, nTarget, nLength + 2, data_0066C0F8);
}
extern int func_006158F8(char nMsgClass, unsigned int nWorld, int nTarget, unsigned int nType,
                         unsigned int nSubType, int nLength, void *pData);

/* SOCOM 1's NetSendTargetClientBitmask grown (0x615B48): the message to each client the mask holds
 * (but this one), 13 if any send failed.
 * Round 14b: the loop sits inside `if (pMask->nLast >= 0)` -- a guard returning `result` there let
 * 2.96's CSE follow the jump knowing v0 still held the world call's result (round 13b's "known-zero
 * v0", held a toolchain difference, is the source's shape: func_00614758 too). */
int func_00615B48(char nMsgClass, unsigned int nWorld, unsigned int nType, unsigned int nSubType, int nLength,
                  void *pData, DmeTargetMask *pMask)
{
    DmeWorld *pWorld = 0;
    void *pClient = 0;
    int result;
    int i;

    if (nType >= 6)
        return DME_ERR_ARGUMENT;
    if (nSubType >= 256)
        return DME_ERR_ARGUMENT;
    if (nLength <= 0)
        return DME_ERR_ARGUMENT;
    if (pData == 0)
        return DME_ERR_ARGUMENT;
    if (pMask == 0)
        return DME_ERR_ARGUMENT;
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK || pWorld == 0)
        return result;
    if (pWorld->field_44C == 4)
        return 15;
    if (pMask->nLast >= 0) {
        for (i = pMask->nFirst; i <= pMask->nLast; i++) {
            if (i == pWorld->nMyClient)
                continue;
            func_006126E8(&pClient, nWorld, i);
            if (pClient != 0 && ((pMask->abTarget[i >> 3] >> (i & 7)) & 1)) {
                if (func_006158F8(nMsgClass, nWorld, i, nType, nSubType, nLength, pData) != 0)
                    result = 13;
            }
        }
    }
    return result;
}

/* SOCOM 1's NetSendApplicationMessage and NetSendDmeMessage grown (0x615D88, 0x615DE0): a message of one of
 * 256 types through NetSendMessage's body as kind 2 (the application's) or kind 0 (DME's own), now checked
 * first; Dme.c's locked wrappers carry the names (round 7b). Round 25b wrote them here for func_0061A118:
 * its first arm's owner test is the target's `bnel` only once the send it falls into is compiled above it
 * (round 13c's rule, met the other way round). */
int func_00615D88(char nMsgClass, unsigned int nWorld, int nTarget, unsigned int nType, int nLength, void *pData)
{
    if (nType < 256) {
        if (nLength > 0 && pData != 0)
            return func_006158F8(nMsgClass, nWorld, nTarget, 2, nType, nLength, pData);
        return DME_ERR_ARGUMENT;
    }
    return DME_ERR_ARGUMENT;
}

int func_00615DE0(char nMsgClass, unsigned int nWorld, int nTarget, unsigned int nType, int nLength, void *pData)
{
    if (nType < 256) {
        if (nLength > 0 && pData != 0)
            return func_006158F8(nMsgClass, nWorld, nTarget, 0, nType, nLength, pData);
        return DME_ERR_ARGUMENT;
    }
    return DME_ERR_ARGUMENT;
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

/* an object's allocated parts (0x1C-0x37), freed with it */
typedef struct DmeObjectParts {
    char field_00[0x1C];
    void *pPart1C;                      /* 0x1C */
    void *pPart20;                      /* 0x20 */
    char field_24[0x0C];
    void *pPart30;                      /* 0x30 */
    void *pPart34;                      /* 0x34 */
} DmeObjectParts;

extern int func_006126E8(void **ppClient, unsigned int nWorld, unsigned int nClient);

/* SOCOM 1's NetFreeRemoteObject grown (0x615FC8): the deletion callback, then the object looked up
 * again; its status 1, back to 0 unless the index lies in its creator's block; four parts freed where
 * SOCOM 1 freed two unconditionally, then the table's slot and the record */
int NetFreeRemoteObject(int nIndex)
{
    DmeObjectParts *pObject;
    DmeClient *pClient = 0;

    if (nIndex < 0 || nIndex >= DME_MAX_OBJECTS)
        return DME_ERR_ARGUMENT;
    if ((unsigned int)data_00651178 >= DME_MAX_WORLDS)
        return DME_ERR_RANGE;
    func_00614EC8((void **)&pObject, nIndex);
    if (pObject == 0)
        return DME_ERR_RANGE;
    RemoteObjectDelete(((DmeObject *)pObject)->nCreator, nIndex);
    func_00614EC8((void **)&pObject, nIndex);
    if (pObject != 0) {
        NetObjectStatus[nIndex] = 1;
        if (data_00651178 < DME_MAX_WORLDS) {
            func_006126E8((void **)&pClient, data_00651178, ((DmeObject *)pObject)->nCreator);
            if (pClient != 0) {
                if (nIndex < pClient->nFirstObject || nIndex >= pClient->nFirstObject + pClient->nObjects)
                    NetObjectStatus[nIndex] = 0;
            }
        }
        if (pObject->pPart20 != 0) {
            func_00614888(&pObject->pPart20);
            pObject->pPart20 = 0;
        }
        if (pObject->pPart1C != 0) {
            func_00614888(&pObject->pPart1C);
            pObject->pPart1C = 0;
        }
        if (pObject->pPart30 != 0) {
            func_00614888(&pObject->pPart30);
            pObject->pPart30 = 0;
        }
        if (pObject->pPart34 != 0) {
            func_00614888(&pObject->pPart34);
            pObject->pPart34 = 0;
        }
        data_006766E0[nIndex] = 0;
        func_00614888((void **)&pObject);
    }
    return DME_OK;
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
extern int strcmp(const char *a, const char *b);

/* SOCOM 1's LocalObjectName2Index grown (0x616290): a client's named object by its name, -1 for none.
 * OPEN (8 bytes short): the target tests the name's address for null as computed (`addiu v0,v0,12;
 * beqz v0`, the sum then strcmp's argument); 2.96 here folds `p + 12 != 0` into `p != -12` and keeps
 * -12 in a fifth saved register. */
int func_00616290(char *szName, unsigned int nWorld, unsigned int nClient)
{
    DmeClient *pClient;
    DmeObject *pObject;
    int i, nIndex;

    if (szName == 0)
        return -1;
    if (nWorld >= DME_MAX_WORLDS)
        return -1;
    if (nClient >= DME_MAX_CLIENTS)
        return -1;
    func_006126E8((void **)&pClient, nWorld, nClient);
    if (pClient == 0)
        return -1;
    for (i = 0; i < pClient->nObjects; i++) {
        nIndex = i + pClient->nFirstObject;
        if (NetObjectStatus[nIndex] == 2) {
            func_00614EC8((void **)&pObject, nIndex);
            if ((char *)pObject + 12 != 0 && strcmp((char *)pObject + 12, szName) == 0)
                return nIndex;
        }
    }
    return -1;
}

/* SOCOM 1's FindFreeLocalObject grown (0x6163A0): the first of this client's objects not in use */
int func_006163A0(void)
{
    DmeWorld *pWorld = 0;
    DmeClient *pClient = 0;
    int i, nIndex;
    int result;

    if ((unsigned int)data_00651178 >= DME_MAX_WORLDS)
        return -1;
    result = func_0061C600(data_00651178, &pWorld);
    if (result != DME_OK || pWorld == 0)
        return result;
    func_006126E8((void **)&pClient, data_00651178, pWorld->nMyClient);
    if (pClient == 0)
        return -1;
    for (i = 0; i < pClient->nObjects; i++) {
        nIndex = i + pClient->nFirstObject;
        if (NetObjectStatus[nIndex] != 2)
            return nIndex;
    }
    return -1;
}


extern unsigned char data_0067C5B0[4];
extern int data_0067B7AC;
extern char data_006766D0[16];
extern unsigned short data_0067B7A8;
extern void func_00616478(int nWorld, int nClient, unsigned short nConnection, int nValue,
                          unsigned short nPort);
extern int RtMsgToDmeConnectionIndex(unsigned short nConnection);

extern unsigned short data_0067B7A8;
extern unsigned short data_0067A6E0;
extern int RtMsgToDmeConnectionIndex(unsigned short nConnection);
extern void func_00614B18(unsigned int nWorld, unsigned int nClient, char *szIp, unsigned short nPort);
extern int func_006164E0(int nWorld, int nClient);

/* SOCOM 1's RemoteClientEvent grown (0x616748): a client of a world connected or left */
void func_00616748(unsigned short nConnection, unsigned short nClient, char *szIp, unsigned char bConnected)
{
    DmeWorld *pWorld = 0;
    unsigned int nWorld = RtMsgToDmeConnectionIndex(nConnection);
    unsigned short nPort;

    if (nWorld >= DME_MAX_WORLDS)
        return;
    if (szIp == 0)
        return;
    if (func_0061C600(nWorld, &pWorld) != 0)
        return;
    if (pWorld == 0)
        return;
    if (bConnected) {
        pWorld->field_440++;
        if (data_0067B7A8 == 0)
            nPort = data_0067A6E0;
        else
            nPort = data_0067B7A8;
        func_00614B18(nWorld, nClient, szIp, nPort);
    } else
        func_006164E0(nWorld, nClient);
}

/* the server's address for the peer layer (0x616C10): the transport brought up first */
int func_00616C10(char *szIp, int nPort)
{
    int result;

    if (szIp == 0)
        return DME_ERR_ARGUMENT;
    if (strlen(szIp) == 0)
        return DME_ERR_ARGUMENT;
    if (nPort == 0)
        return DME_ERR_ARGUMENT;
    if (data_006511D4 == 0) {
        result = func_00616C98();
        if (result != 0)
            return result;
    }
    func_006148E0(data_0067C5B0, szIp);
    data_0067B7AC = nPort;
    return DME_OK;
}

/* the peer layer's parameters: the port, its limits, and DME's callbacks (0x61D530 ...) */
typedef struct DmeP2PParams {
    unsigned short nPort;               /* 0x00 */
    int field_04;                       /* 8 */
    int field_08;                       /* 512 */
    int field_0C;
    int field_10;
    int field_14;                       /* 30 */
    int field_18;
    void *pNatServer;                   /* 0x1C */
    void *pfnLinkLost;                  /* 0x20 */
    void *pfn24;
    void *pfnConnection;                /* 0x28 */
    void *pfn2C;
    void *pfn30;
    void *pfn34;
} DmeP2PParams;                         /* 0x38 */

extern int data_006511C4;
extern int data_006511C8;
extern int data_00651188;
extern unsigned char data_006511DC;
extern char data_0066F378[];
extern int func_006395B8(DmeP2PParams *pParams, int *pnP2P);
extern void func_0061D530(int nReason);
extern void func_0061D5E8(int nValue);
extern void func_0061D7A8(int nConnection, unsigned int nEvent);
extern void func_0061D898(int nConnection, int nValue, unsigned short nPort);
extern void func_0061D948(char *szIp, unsigned short nPort, int nValue, unsigned short nPort2);
extern void func_0061D9B8(int nClient, int bValue);

/* the transport's startup (0x616C98): the peer layer opened over one UDP endpoint */
int func_00616C98(void)
{
    DmeP2PParams params;

    if (data_006511D4 == 0) {
        params.nPort = data_0067A6E0;
        params.field_04 = 8;
        params.field_08 = 512;
        params.field_0C = data_006511C4;
        params.field_10 = data_006511C8;
        params.field_14 = 30;
        params.field_18 = data_00651188;
        params.pfnLinkLost = func_0061D530;
        params.pfn24 = func_0061D5E8;
        params.pfnConnection = func_0061D7A8;
        params.pfn2C = func_0061D898;
        params.pfn30 = func_0061D948;
        params.pfn34 = func_0061D9B8;
        if (data_006511DC != 0)
            params.pNatServer = data_0066F378;
        else
            params.pNatServer = 0;
        if (func_006395B8(&params, &data_006511D8) != 0)
            return 37;
        data_006511D4 = 1;
    }
    return DME_OK;
}

extern int func_00633108(char *szIp);
extern int data_0067B7B0;
extern int data_0067C5B4;
extern unsigned int data_00686A4C;
extern int strncmp(const char *a, const char *b, unsigned int n);

/* (0x616DA0) this client's own address told to the server, by the peer layer */
int func_00616DA0(int nValue)
{
    char szIp[16];
    unsigned char abIp[4];
    int nMsg;
    int result;

    if (data_006511D4 == 0) {
        result = func_00616C98();
        if (result != 0)
            return result;
    }
    result = NetMapMsgClientError(func_00633108(szIp), 24);
    if (result != 0)
        return result;
    data_0067C5B4 = nValue;
    nMsg = data_0067B7B0;
    if (strncmp(szIp, "0.0.0.0", 16) == 0)
        return 35;
    func_006148E0(abIp, szIp);
    return func_00615558(0, abIp, 0, data_00686A4C, 4, &nMsg);
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
/* (0x616ED0) */
void func_00616ED0(unsigned short nConnection, unsigned short nPort, int nValue, unsigned short nPort2)
{
    unsigned int nWorld = RtMsgToDmeConnectionIndex(nConnection);

    if (nWorld >= DME_MAX_WORLDS)
        return;
    if (nValue == 0)
        return;
    if (nPort2 == 0)
        return;
    data_006766D0[0] = 0;
    data_0067B7A8 = 0;
    func_00616478(func_006147E0(), nWorld, nPort, nValue, nPort2);
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


extern void (*data_00651198)(int *pInfo);
extern int data_0065119C;

/* a world's connection lost (0x617560): the host told, as func_0061D530 tells it.
 * OPEN (7 words, the right size): with the world's state an enum (gcc r21b, as 0x61D530) the loads
 * come ahead of the stores, in sched1's order the target's (pWorld, the data word's %hi and load, the
 * state; stores 8, 0, 4, 16, 12) -- but sched1 issues the report's 1 beside the memset call, so it
 * lives across the loads in a1, where the target's `li v0,1` follows the state's load (v0 reused);
 * the 120 orders of the five statements and the 1 through a variable give 7-9 words. */
void func_00617560(unsigned short nConnection)
{
    int info[5];
    DmeWorld *pWorld = 0;
    unsigned int nWorld = RtMsgToDmeConnectionIndex(nConnection);

    if (nWorld >= DME_MAX_WORLDS)
        return;
    if (func_0061C600(nWorld, &pWorld) != 0)
        return;
    if (pWorld == 0)
        return;
    if (data_00651198 != 0) {
        memset(info, 0, sizeof(info));
        info[2] = 1;
        info[0] = nWorld;
        info[1] = pWorld->field_438;
        info[4] = data_0065119C;
        info[3] = 0;
        data_00651198(info);
    }
}

/* gcc r21b: the port's store is the third (after pParams[2]): in source order sched1 issues the
 * port's load after the zeroing stores before it, and nine placements of that one statement give the
 * target's at the third alone. */
int func_00617610(int *pParams)
{
    int result;

    if (pParams == 0)
        return DME_ERR_ARGUMENT;
    memset(pParams, 0, 0x128);
    pParams[1] = 256;
    pParams[2] = 0;
    pParams[73] = data_0067A6E0 + 1;
    pParams[41] = 0;
    pParams[43] = 0;
    pParams[45] = 0;
    pParams[47] = 0;
    pParams[49] = 0;
    pParams[50] = 0;
    result = func_006437E0(pParams + 51);
    if (result == DME_OK)
        pParams[0] = 1;
    return result;
}
typedef struct DmeConnectKind {
    DmeConnectType nType;               /* 0x00: 1, 3 and 4 client-server, 2 peer to peer */
    int field_04;
    char szIp[16];                      /* 0x08: the server */
    int nPort;                          /* 0x18 */
    DmeAddress nat;                     /* 0x1C: its first word 3, a NAT server's address */
    int nLocalPort;                     /* 0x34 */
    char abKey[64];                     /* 0x38 */
    char szUser[17];                    /* 0x78 */
    char szPassword[17];                /* 0x89 */
} DmeConnectKind;                       /* 0x9C */

typedef struct DmeConnectParams {
    int nVersion;                       /* 0x00: 1 */
    int field_04;
    DmeConnectKind kind;                /* 0x08 */
} DmeConnectParams;

typedef struct DmeConnectStatus {
    int field_00;
    int nResult;                        /* 0x04: 3 while connecting */
    int nWorld;                         /* 0x08 */
} DmeConnectStatus;

extern int func_0061D3C8(void);
extern int func_00617788();
extern int func_00617CD8(DmeConnectParams *pParams, DmeConnectKind *pKind, DmeConnectStatus *pStatus);

/* SOCOM 1's NetConnect grown (0x617690): the worlds' links checked, then the connection by its kind */
int func_00617690(DmeConnectParams *pParams, DmeConnectStatus *pStatus)
{
    DmeConnectKind *pKind;
    int result;

    if (pStatus == 0)
        return DME_ERR_ARGUMENT;
    pStatus->nResult = 3;
    pStatus->field_00 = 1;
    result = func_0061D3C8();
    if (result != DME_OK)
        return result;
    if (pParams == 0) {
        pStatus->nResult = DME_ERR_ARGUMENT;
        return DME_ERR_ARGUMENT;
    }
    if (pParams->nVersion != 1) {
        pStatus->nResult = 38;
        return 38;
    }
    pKind = &pParams->kind;
    if (pKind->nType == 1 || pKind->nType == 3 || pKind->nType == 4)
        pStatus->nResult = func_00617788(pParams, pKind, pStatus);
    else if (pKind->nType == 2)
        pStatus->nResult = func_00617CD8(pParams, pKind, pStatus);
    else
        pStatus->nResult = DME_ERR_ARGUMENT;
    return pStatus->nResult;
}

/* SOCOM 1's NetConnect_ClientServerTCP grown (0x617788): the connection by rt_msg_client, its world filled in, and the wait
 * for the callback when no host callback will take it. */
typedef struct DmeConnectOptions {
    int nKind;                          /* 0x00: 1 for the kind-3 connection */
    char szIp[16];                      /* 0x04 */
    unsigned short nPort;               /* 0x14 */
    unsigned short nLocalPort;          /* 0x16 */
    unsigned short nPort2;              /* 0x18 */
    void *pKey;                         /* 0x1C */
    int nTimeout;                       /* 0x20: 15000 */
    int field_24;
    int nA;                             /* 0x28 */
    int nB;                             /* 0x2C */
    char *pData;                        /* 0x30 */
    unsigned short nData;               /* 0x34 */
    void *pfnConnect;                   /* 0x38 */
    void *pfnClient;                    /* 0x3C */
    void *pfnAddress;                   /* 0x40 */
    void *pfnLost;                      /* 0x44 */
    void *pfnMessage;                   /* 0x48 */
} DmeConnectOptions;

typedef struct DmeConnectReport {
    int nWorld;                         /* 0x00 */
    int nState;                         /* 0x04 */
    int nResult;                        /* 0x08 */
    int nError;                         /* 0x0C */
    int nUser;                          /* 0x10 */
} DmeConnectReport;

extern int rt_msg_client_set_default_connect_options(DmeConnectOptions *pOptions);
extern int rt_msg_client_connect(unsigned short *pnConnection, DmeConnectOptions *pOptions);

extern char *strcpy(char *d, const char *s);
extern int func_00643900(int nWorld, int nMaxClients, void *pKey);
extern void (*data_00651190)(void *pReport);    /* the waiting caller's callback */
extern int data_00651194;               /* and its user value */
extern int rt_msg_client_disconnect(int nConnection);
extern int rt_msg_client_update(int nConnection, unsigned char bBlock);
extern int func_0061C638(unsigned int nWorld, DmeWorld **ppWorld);
extern int func_0061C998(unsigned int nWorld, int nValue);
extern int func_0061C930(unsigned int nWorld, const char *szName);
extern int data_006511C4;
extern int data_006511C8;
extern void func_00617250(unsigned short nConnection, unsigned char nStatus, unsigned short nClient, unsigned short nMaxClients, int nReason);
extern void func_0061D9F8(unsigned short nConnection, unsigned char nType, int a2, int a3, unsigned char a4,                    unsigned short a5, int a6);

/* the host's connect callbacks (0xA4), and the world's (0xB4: pfnClientCallback's, a client's leaving) */
typedef struct DmeConnectCallbacks {
    void (*pfnConnect)(void *);         /* 0x00 */
    int nConnectUser;                   /* 0x04 */
    void (*pfnLost)(int *);             /* 0x08 */
    int nLostUser;                      /* 0x0C */
} DmeConnectCallbacks;

typedef struct DmeClientCallbacks {
    DmeClientCallback pfnClient;        /* 0x00 */
    void *nClientUser;                  /* 0x04 */
    DmeClientCallback pfnLeave;         /* 0x08 */
    void *nLeaveUser;                   /* 0x0C */
} DmeClientCallbacks;

typedef struct DmeConnectExtra {
    int stream;                         /* 0x00: func_00643900's (0xCC) */
    int nStream;                        /* 0x04: 1, media streams wanted */
    char pad08[0x4C];
    int field_54;                       /* 0x54 (0x120) */
    int nPort2;                         /* 0x58 (0x124) */
} DmeConnectExtra;

typedef struct DmeConnectFull {
    int nVersion;                       /* 0x00: 1 */
    int nClients;                       /* 0x04 */
    DmeConnectKind kind;                /* 0x08 */
    DmeConnectCallbacks host;           /* 0xA4 */
    DmeClientCallbacks world;           /* 0xB4 */
    int field_C4[2];
    DmeConnectExtra extra;              /* 0xCC */
} DmeConnectFull;

int func_00617788(DmeConnectFull *pParams, DmeConnectKind *pKind, DmeConnectStatus *pStatus)
{
    DmeConnectOptions options;
    unsigned int tStart[4];
    char abData[256];
    unsigned short nConnection;
    unsigned int nElapsed = 0;
    DmeWorld *pWorld = 0;
    unsigned short nData = 0;
    char *pData = 0;
    char *nOne = (char *)1;
    int nWorld;
    int result;
    int nMapped;
    int nUpdate;

    if (pStatus == 0)
        return DME_ERR_ARGUMENT;
    pStatus->nResult = 3;
    pStatus->nWorld = -1;
    pStatus->field_00 = (int)nOne;
    if (pParams == 0 || pKind == 0) {
        pStatus->nResult = DME_ERR_ARGUMENT;
        return DME_ERR_ARGUMENT;
    }
    if (pParams->nVersion != (int)nOne) {
        pStatus->nResult = 38;
        return 38;
    }
    if (pParams->nClients <= 0 || (pKind->nType != (int)nOne && pKind->nType != 3 && pKind->nType != 4)) {
        pStatus->nResult = DME_ERR_ARGUMENT;
        return DME_ERR_ARGUMENT;
    }
    if (pKind->nat.w[0] == 3)
        func_00618458(&pKind->nat);
    data_00651190 = pParams->host.pfnConnect;
    data_00651194 = pParams->host.nConnectUser;
    data_00651198 = pParams->host.pfnLost;
    data_0065119C = pParams->host.nLostUser;
    if (strlen(pKind->szUser) != 0 && strlen(nOne = pKind->szPassword) != 0) {
        memset(abData, 0, sizeof(abData));
        strncpy(abData, pKind->szUser, 17);
        strncpy(abData + 17, nOne, 17);
        pData = abData + 34;
        nData = pData - abData;
        pData = abData;
    }
    nMapped = NetMapMsgClientError(rt_msg_client_set_default_connect_options(&options), 24);
    if (nMapped != 0)
        return nMapped;
    if (pKind->nType == 1 || pKind->nType == 4) {
        options.nKind = 0;
    } else if (pKind->nType == 3) {
        options.nKind = 1;
        options.nPort2 = pParams->extra.nPort2;
    } else {
        pStatus->nResult = DME_ERR_ARGUMENT;
        return DME_ERR_ARGUMENT;
    }
    strcpy(options.szIp, pKind->szIp);
    options.nPort = pKind->nPort;
    options.nLocalPort = pKind->nLocalPort;
    options.pKey = pKind->abKey;
    options.nTimeout = 15000;
    options.nA = data_006511C4;
    options.nB = data_006511C8;
    options.pData = pData;
    options.nData = nData;
    options.pfnConnect = func_00617250;
    options.pfnLost = func_00617560;
    options.pfnClient = func_00616748;
    options.pfnAddress = func_00616ED0;
    options.pfnMessage = func_0061D9F8;
    nOne = (char *)rt_msg_client_connect(&nConnection, &options);
    if (nOne != 0) {
        rt_msg_client_disconnect(nConnection);
        if (data_00651190 != 0) {
            DmeConnectReport report;

            memset(&report, 0, sizeof(report));
            report.nWorld = -1;
            report.nState = 3;
            report.nResult = 1;
            report.nError = 3;
            report.nUser = data_00651194;
            data_00651190(&report);
        }
        nMapped = NetMapMsgClientError((int)nOne, 3);
        pStatus->nResult = nMapped;
        if (nMapped != 0)
            return nMapped;
    }
    nWorld = RtMsgToDmeConnectionIndex(nConnection);
    pStatus->nWorld = nWorld;
    pStatus->nResult = func_0061C638(nWorld, &pWorld);
    if (pStatus->nResult == 0) {
        if (pParams->extra.nStream == 1)
            pStatus->nResult = func_00643900(nWorld, pParams->nClients, &pParams->extra.stream);
        if (pStatus->nResult == 0 && pWorld != 0)
            goto ok;
    }
    rt_msg_client_disconnect(nConnection);
    if (data_00651190 != 0) {
        DmeConnectReport report;

        memset(&report, 0, sizeof(report));
        report.nWorld = -1;
        report.nState = 3;
        report.nResult = 1;
        report.nError = 3;
        report.nUser = data_00651194;
        data_00651190(&report);
    }
    return pStatus->nResult;
ok:
    func_0061C998(nWorld, 256);
    pWorld->pfnClientCallback = pParams->world.pfnClient;
    pWorld->field_42C = pParams->world.nClientUser;
    pWorld->field_430 = pParams->world.pfnLeave;
    pWorld->field_434 = pParams->world.nLeaveUser;
    nOne = (char *)2;
    pWorld->field_438 = (int)nOne;
    pWorld->field_43C = pKind->nLocalPort;
    pWorld->field_444 = pParams->nClients;
    pWorld->field_448 = pParams->extra.field_54;
    pWorld->field_44C = pKind->nType;
    nMapped = func_0061C930(nWorld, pKind->szIp);
    pStatus->nResult = nMapped;
    if (nMapped != 0) {
        pWorld->field_438 = 3;
        return nMapped;
    }
    if (data_00651190 == 0) {
        rt_time_start(tStart);
        while (pWorld->field_438 == 2) {
            nUpdate = rt_msg_client_update(nConnection, 1);
            if (nUpdate != 0) {
                pWorld->field_438 = 3;
                nMapped = NetMapMsgClientError(nUpdate, 3);
                pStatus->nResult = nMapped;
                if (nMapped != 0)
                    return nMapped;
            }
            rt_time_elapsed(tStart, &nElapsed);
            if (nElapsed > 30000) {
                pStatus->nResult = 3;
                pWorld->field_438 = 3;
                return 3;
            }
        }
        if (pWorld->field_438 != 1) {
            pWorld->field_438 = 3;
            pStatus->nResult = 3;
            return 3;
        }
    }
    pStatus->nResult = 0;
    return 0;
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
/* the peer layer's connect parameters: two addresses by kind, the 64-byte session key */
typedef struct DmeHostAddr {
    int nKind;                          /* 0x00: 2 the first address, 1 the second */
    char szIp[16];                      /* 0x04 */
    unsigned short nPort;               /* 0x14 */
} DmeHostAddr;                          /* 0x18 */

typedef struct DmeHostKey { char b[64]; } DmeHostKey;

typedef struct DmeHostParams {
    int nVersion;                       /* 0x00: 1 */
    DmeHostAddr aAddr[2];               /* 0x04 */
    DmeHostKey key;                     /* 0x34 */
} DmeHostParams;

typedef struct DmeHostInfo {
    DmeHostKey key;                     /* 0x00 */
    char szIp[16];                      /* 0x40: kind 2's */
    char szIp2[16];                     /* 0x50: kind 1's */
    unsigned short nPort;               /* 0x60 */
    unsigned short nPort2;              /* 0x62 */
} DmeHostInfo;                          /* 0x64 */

extern int func_006187A0(DmeHostParams *pParams, DmeConnectStatus *pStatus);
extern int func_0061CB18(unsigned char *pbValue);

/* the join's checks (0x6186B0), as func_00617690's for the connection: then func_006187A0 */
int func_006186B0(int *pParams, DmeConnectStatus *pStatus)
{
    unsigned char bValue = 0;

    if (pStatus == 0)
        return DME_ERR_ARGUMENT;
    pStatus->nResult = 30;
    pStatus->field_00 = 1;
    if (pParams == 0) {
        pStatus->nResult = DME_ERR_ARGUMENT;
        return DME_ERR_ARGUMENT;
    }
    if (data_00651180 != 1) {
        pStatus->nResult = 17;
        return 17;
    }
    if (pParams[0] != 1) {
        pStatus->nResult = 38;
        return 38;
    }
    if (data_006511D4 != 1) {
        pStatus->nResult = 26;
        return 26;
    }
    pStatus->nResult = func_0061CB18(&bValue);
    if (pStatus->nResult == 0 && bValue == 1)
        func_006187A0((DmeHostParams *)pParams, pStatus);
    return pStatus->nResult;
}
/* a peer the address resolution waits on (0x66CF78: 256 of 36 bytes) */
typedef struct DmePendingPeer {
    unsigned char nState;               /* 0x00: 2 waiting, 3 answered */
    char szIp[17];                      /* 0x01 */
    unsigned short nPort;               /* 0x12 */
    unsigned int tStart[2];             /* 0x14 */
    unsigned int nElapsed;              /* 0x1C */
    void (*pfnCallback)(int nEvent, char *szIp, int nValue);   /* 0x20 */
} DmePendingPeer;                       /* 0x24 */

extern DmePendingPeer data_0066CF78[DME_MAX_CLIENTS];
extern int rt_time_elapsed(unsigned int *pStart, unsigned int *pnMsecs);

/* the pending peer answered (0x61CBB8): its time to answer kept */
void func_0061CBB8(char *szIp, unsigned short nPort)
{
    unsigned int i;

    if (szIp == 0)
        return;
    if (szIp[0] == 0)
        return;
    for (i = 0; i < DME_MAX_CLIENTS; i++) {
        if (strncmp(szIp, data_0066CF78[i].szIp, 16) == 0 && data_0066CF78[i].nState == 2
            && nPort == data_0066CF78[i].nPort) {
            rt_time_elapsed(data_0066CF78[i].tStart, &data_0066CF78[i].nElapsed);
            data_0066CF78[i].nState = 3;
            return;
        }
    }
}
/* a free pending-peer slot and the next port past the address's (0x61CEF0) */
int func_0061CEF0(char *szIp, unsigned int *pnSlot, unsigned short *pnPort)
{
    unsigned int i;
    int bFound = 0;

    if (szIp == 0)
        return DME_ERR_ARGUMENT;
    if (pnSlot == 0)
        return DME_ERR_ARGUMENT;
    if (pnPort == 0)
        return DME_ERR_ARGUMENT;
    if (*(unsigned char *)szIp == 0)
        return DME_ERR_ARGUMENT;
    *pnSlot = 0;
    *pnPort = 0;
    for (i = 0; i < DME_MAX_CLIENTS; i++) {
        if (data_0066CF78[i].nState == 0) {
            *pnSlot = i;
            bFound = 1;
        }
        if (strncmp(szIp, data_0066CF78[i].szIp, 16) == 0 && data_0066CF78[i].nPort >= *pnPort)
            *pnPort = data_0066CF78[i].nPort + 1;
    }
    return bFound ? DME_OK : DME_ERR_RANGE;
}

extern unsigned char data_006511CC;
extern int func_006328E0(void);

/* the pending peers dropped (0x61CFF0): each waiting one's callback told 14, the UDP layer shut */
int func_0061CFF0(void)
{
    unsigned int i;
    int result;

    for (i = 0; i < DME_MAX_CLIENTS; i++) {
        if ((unsigned char)(data_0066CF78[i].nState - 1) < 2)
            data_0066CF78[i].nState = 4;
        if (data_0066CF78[i].nState == 4) {
            if (data_0066CF78[i].pfnCallback != 0) {
                data_0066CF78[i].pfnCallback(14, data_0066CF78[i].szIp, 0);
                data_0066CF78[i].pfnCallback = 0;
            }
            data_0066CF78[i].nState = 0;
        }
    }
    if (data_006511CC == 1) {
        result = NetMapMsgClientError(func_006328E0(), 24);
        if (result != 0)
            return result;
        data_006511CC = 0;
    }
    memset(data_0066CF78, 0, sizeof(data_0066CF78));
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

/* the broadcast receive switch: the server's connection through the peer layer, the others through
 * rt_msg_client. The `beq` on 0xFFFF (round 13b's one-word open) keeps its slot because the
 * fall-through's DmeToRtMsgConnectionIndex is defined above (round 13c's rule, round 17b). */
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
        if (data_0067B7B8[i].pfnCallback == 0)
            return i;
    return DME_MAX_PING_SLOTS;
}



extern int rt_time_get_time_stamp(void *t);

/* the timebase ping's callback (0x619138, after FindFreePingSlot where SOCOM 1's TimeBasePingCallback
 * is, rewritten): half the round trip recorded as the client's next delay sample; the session master's
 * answer, with its time, re-bases the clock unless the median delay is shorter */
void func_00619138(int nStatus, int nUnused, unsigned int nRoundTrip, int nRemoteTime, unsigned int nWorld,
                   int nClient)
{
    DmeWorld *pWorld = 0;
    DmeDelayRecord *pRecord;
    int nHost;
    int nBase;
    int nMedian;
    int nDelay;
    int n;

    if (func_0061C600(nWorld, &pWorld) != 0)
        return;
    if (pWorld == 0)
        return;
    if (nWorld != data_00651178)
        return;
    if (nClient >= pWorld->field_444)
        return;
    if (nStatus != 0)
        return;
    nHost = 256;
    pRecord = 0;
    func_0061C510(nClient, (void **)&pRecord);
    if (pRecord == 0)
        return;
    nDelay = nRoundTrip / 2;
    n = pRecord->nNext;
    pRecord->anDelay[n++] = nDelay;
    if (pRecord->nCount < 100)
        pRecord->nCount++;
    func_00615090(&nHost, nWorld);
    if (nWorld == data_00651178 && nClient == nHost && nRemoteTime != 0) {
        func_00615068(&nBase);
        func_00614380((unsigned int *)&nMedian, nClient);
        if (nMedian >= nDelay)
            NetResetTime(nRemoteTime + nDelay);
    }
    if (n >= 100)
        n = 0;
    pRecord->nNext = n;
    rt_time_get_time_stamp(pRecord->tStamp);
}

extern int NetSendMessage(char msgClass, int connIndex, int toClient, int reliable, int msgType, int len,
                          void *msg);
extern int data_00686A0C;               /* the timebase ping's message type */

/* SOCOM 1's SendTimebaseCorrection grown (0x6192A0): the clock and a free ping slot sent to the client,
 * the slot then filled -- the client's address and port, its world and index, the time sent and the
 * callback 0x619138 (bTimebase 1: told world and client) */
void func_006192A0(unsigned int nClient)
{
    struct {
        int nTime;
        char nSlot;
        unsigned char bTimebase;
    } msg;
    DmeClientInfo *pClient = 0;

    if (nClient >= DME_MAX_CLIENTS)
        return;
    msg.bTimebase = 1;
    msg.nSlot = FindFreePingSlot();
    if (msg.nSlot == DME_MAX_PING_SLOTS)
        return;
    msg.nTime = CurrentNetTime;
    NetSendMessage(-128, data_00651178, nClient, 0, data_00686A0C, sizeof(msg), &msg);
    if (func_0061C720(data_00651178, nClient, &pClient) != 0)
        return;
    if (pClient == 0)
        return;
    func_00615028(&data_0067B7B8[(unsigned char)msg.nSlot].nSent);
    func_006148E0(data_0067B7B8[(unsigned char)msg.nSlot].abIp, pClient->szIp);
    data_0067B7B8[(unsigned char)msg.nSlot].nPort = pClient->nPort;     /* tools/ccperm.py: 1 of 120 */
    data_0067B7B8[(unsigned char)msg.nSlot].pfnCallback = func_00619138;
    data_0067B7B8[(unsigned char)msg.nSlot].bTimebase = 1;
    data_0067B7B8[(unsigned char)msg.nSlot].nWorld = data_00651178;
    data_0067B7B8[(unsigned char)msg.nSlot].nClient = nClient;
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

extern unsigned int data_00686A34;
extern unsigned int data_00686A10;
extern int func_0061D448(int nClient);

/* an object's ownership asked for (0x61A118, SOCOM 1's NetRequestObjectPrivateOwnership's place): taken
 * at once when this client made it, else asked of its creator.
 * Byte-exact since round 25b (open since it was first written). Round 24b came to registers: the ask a one-member
 * struct (an unsigned short is a promoted mode, forced to slot 16 before the pointers' ADDRESSOFs), pObject
 * declared before pWorld, the arms an if/else before one `return DME_OK` (a return in each let jump2
 * cross-jump the two sends), the creator read once into a local (passed on to RemoteObjectUpdate in a0).
 * Round 25b: the owner tests POSITIVE with their `return 11` in an else (round 14b's rule) -- as guards
 * the first if-conversion pass hoisted `li v0,11` above each branch, so v0 was live across the owner's
 * load and the -1 took v1 and the creator a2; positive, the -1 takes v0 and the creator a0 as the target's,
 * and func_00618F70 after it keeps its match (round 24b's guarded body moved it by 8 bytes). The last
 * word, arm 1's `bnel` (`bne` here), came with func_00615DE0 -- the send arm 1 falls into -- compiled above
 * (round 13c's callee rule; arm 2's RemoteObjectUpdate was already above, its `bne` the target's). */
int func_0061A118(unsigned int nIndex)
{
    struct { unsigned char nOwner; unsigned char pad; unsigned short nIndex; } msg;
    DmeObject *pObject = 0;
    DmeWorld *pWorld = 0;
    int nCreator;
    int result;

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
    nCreator = pObject->nCreator;
    if (nCreator != pWorld->nMyClient) {
        struct { unsigned short nIndex; } ask;

        if (pObject->nOwner == -1) {
            ask.nIndex = nIndex;
            func_00615DE0(80, data_00651178, nCreator, data_00686A34, sizeof(ask), &ask);
        } else
            return 11;
    } else {
        if (pObject->nOwner == -1) {
            pObject->nOwner = nCreator;
            RemoteObjectUpdate(nCreator, nIndex, -2);
            msg.nIndex = nIndex;
            msg.nOwner = func_0061D448(pObject->nOwner);
            func_00615DE0(80, data_00651178, -1, data_00686A10, 4, &msg);
        } else
            return 11;
    }
    return DME_OK;
}

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

extern int rt_circ_buf_get_avail(void *pCircBuf, unsigned int *pnAvail);

/* a data stream's free space (0x61B650): 9 when the stream is not open. Its slot keeps its
 * `char` array: typed as bytes, the unit's alias analysis drops func_0061C7A8's reload */
int func_0061B650(unsigned int *pnAvail, unsigned int nStream)
{
    DmeStreamSlot *pSlot = 0;
    unsigned int nAvail = 0;
    int result;

    if (pnAvail == 0)
        return DME_ERR_ARGUMENT;
    *pnAvail = 0;
    if (nStream >= DME_MAX_STREAMS)
        return DME_ERR_ARGUMENT;
    result = func_00614F58(nStream, (void **)&pSlot);
    if (result != DME_OK || pSlot == 0)
        return result;
    if ((unsigned char)pSlot->field_00[0] != 2)
        return 9;
    if ((unsigned char)pSlot->field_00[4] != 0)
        return 9;
    if (pSlot->circ == 0)
        return 9;
    if (rt_circ_buf_get_avail(pSlot->circ, &nAvail) == 0) {
        *pnAvail = nAvail;
        return DME_OK;
    }
    return 9;
}

typedef struct DmeStreamHead {          /* a stream slot's words (the slot itself stays a char array) */
    int field_00;
    int field_04;
    int field_08;
    int nOwner;                         /* 0x0C */
    void *pBuffer;                      /* 0x10 */
    int field_14[6];
    int nCircStart;                     /* 0x2C: the slot's rt_circ_buf */
    int nCircEnd;
    int nCircSize;                      /* 0x34 */
    int nCircUsed;
    void *pCircBuf;                     /* 0x3C */
} DmeStreamHead;

extern int rt_circ_buf_store(void *pCircBuf, void *pData, int nBytes);
extern int rt_circ_buf_normalize(void *pCircBuf, unsigned char bUseBuffer);

/* SOCOM 1's NetAddToDataStream grown (0x61ABD8): the data stored in the stream's ring, the ring
 * normalised and copied out to the stream's buffer */
int func_0061ABD8(unsigned int nStream, void *pData, int nLength)
{
    DmeWorld *pWorld = 0;
    DmeStreamSlot *pSlot = 0;
    unsigned int nAvail = 0;
    int result;

    if (nStream >= DME_MAX_STREAMS)
        return DME_ERR_ARGUMENT;
    if (pData == 0)
        return DME_ERR_ARGUMENT;
    if (nLength <= 0)
        return DME_ERR_ARGUMENT;
    result = func_0061B650(&nAvail, nStream);
    if (result != DME_OK)
        return result;
    if ((int)nAvail < nLength)
        return 31;
    result = func_00614F58(nStream, (void **)&pSlot);
    if (result != DME_OK || pSlot == 0)
        return result;
    result = func_0061C600(data_00651178, &pWorld);
    if (result != DME_OK || pWorld == 0)
        return result;
    if (pWorld->nMyClient != ((DmeStreamHead *)pSlot)->nOwner)
        return 9;
    if (rt_circ_buf_store(pSlot->circ, pData, nLength) != 0)
        return 31;
    rt_circ_buf_normalize(pSlot->circ, 1);
    memcpy(((DmeStreamHead *)pSlot)->pBuffer, ((DmeStreamHead *)pSlot)->pCircBuf, ((DmeStreamHead *)pSlot)->nCircSize);
    return DME_OK;
}

extern int func_00615090(int *pValue, unsigned int nWorld);
extern int data_0067E948[];
extern unsigned int data_00686A28;

/* SOCOM 1's NetReleaseObjectPrivateOwnership's place (0x61B538): the release sent to the given client,
 * or for an object whose class wants it, to the session's host when none is given.
 * Closed in round 24b (open since 13b on the frame: the byte message took slot 0): the message is a one-byte
 * struct -- an unsigned char local is a promoted mode, so taking its address forces it to the stack at once,
 * where the pointers' ADDRESSOFs get theirs only when purged, in insn order (0x618F70's finding); a struct
 * of one byte is not promoted and takes slot 12 after them. The zeroed pointers in declaration order. */
void func_0061B538(int nTarget, unsigned int nIndex)
{
    DmeObject *pObject = 0;
    DmeWorld *pWorld = 0;
    int nHost;

    if (nIndex >= DME_MAX_OBJECTS)
        return;
    if ((unsigned int)data_00651178 >= DME_MAX_WORLDS)
        return;
    if (func_0061C600(data_00651178, &pWorld) != DME_OK || pWorld == 0)
        return;
    if (func_00614F10((void **)&pObject, nIndex) != DME_OK || pWorld == 0)
        return;
    if (func_00615090(&nHost, data_00651178) != DME_OK)
        return;
    if (nHost < 0)
        return;
    if (nHost >= DME_MAX_CLIENTS)
        return;
    if (data_0067E948[pObject->field_03] != 0 && pWorld->nMyClient != nHost && nTarget == -1)
        nTarget = nHost;
    {
        struct { unsigned char nIndex; } msg;

        msg.nIndex = nIndex;
        func_00615DE0(80, data_00651178, nTarget, data_00686A28, sizeof(msg), &msg);
    }
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
    if (pAddress[0] != sizeof(DmeStatus))
        return 39;
    *(DmeStatus *)pAddress = data_0066CF60;
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
extern int rt_time_get_local(unsigned int *pTime);

/* the pings unanswered for 30 seconds (0x61C9E8): each slot's callback told 14 and freed (six
 * arguments for a timebase ping, four otherwise). Byte-exact since round 20b typed the slot as
 * 0x6192A0 fills it -- the port a halfword (`lhu`), the in-use word the callback itself (round 13b's
 * `int` port and `int` flag called through a cast were 16 bytes long). */
int func_0061C9E8(void)
{
    char szIp[16];
    unsigned int nNow;
    unsigned int i;

    rt_time_get_local(&nNow);
    for (i = 0; i < DME_MAX_PING_SLOTS; i++) {
        if (data_0067B7B8[i].pfnCallback != 0 && (int)(nNow - data_0067B7B8[i].nSent) - 30000 > 0) {
            NetIpBinaryToString(szIp, (unsigned char *)&data_0067B7B8[i]);
            NetIpBinaryToString(data_006766D0, (unsigned char *)&data_0067B7B8[i]);
            data_0067B7A8 = data_0067B7B8[i].nPort;
            if (data_0067B7B8[i].bTimebase == 1)
                data_0067B7B8[i].pfnCallback(14, szIp, 0, 0, data_0067B7B8[i].nWorld,
                                         data_0067B7B8[i].nClient);
            else
                data_0067B7B8[i].pfnCallback(14, szIp, 0, 0);
            data_0067B7B8[i].pfnCallback = 0;
        }
    }
    return DME_OK;
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

/* gcc r21b: guards to the end -- `if (pClient == 0) return DME_OK;` leaves the call's zero known in v0
 * (the target's beqzl to the epilogue), and the 30 is the last statement's. */
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
    if (pClient == 0)
        return DME_OK;
    *pbConnected = pClient->bConnected != 0;
    return 30;
}

extern int rt_circ_buf_destroy(void *pCircBuf);

/* gcc r21b: the table's null test is a guard (`return DME_OK` hoisted into v0 by if-conversion, so the
 * table's address takes a0 and the branch's slot the zero, as the target's) -- the loop inside a
 * positive test was 8 bytes short. */
int func_0061D320(void)
{
    unsigned int i;

    if (data_006511D0 == 0)
        return DME_OK;
    for (i = 0; i < DME_MAX_STREAMS; i++) {
        if (data_006511D0[i].pBuffer != 0)
            func_00614888(&data_006511D0[i].pBuffer);
        if (data_006511D0[i].circ != 0)
            rt_circ_buf_destroy(data_006511D0[i].circ);
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

extern void func_00617060(unsigned int nWorld, int nClient, unsigned char bConnected, unsigned short nValue, int nArg);

void func_0061D4D8(int nClient, unsigned short nPort, int nError)
{
    if (nError == 0 && nClient != -2) {
        func_00617060(0, nClient, 1, nPort, 0);
        return;
    }
    func_00617060(0, DME_MAX_CLIENTS, 0, 0, 1);
}


/* the connection to the server lost (0x61D530): the world's link closed, the host told why.
 * gcc r21b: byte-exact once the world's state (0x438) is an enum (dmelocal.h's DmeWorldState): an
 * int field's load is ordered after the report's int stores before it (the report's address is
 * taken and the world pointer has no known base), an enum's is not -- every order of the five
 * statements had come within 5 words. */
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
    int field_18;                       /* 0x18: 2 */
    char szLocalIp[16];                 /* 0x1C: rt_msg_client's */
    int nLocalPort;                     /* 0x2C */
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
extern void func_00614B18(unsigned int nWorld, unsigned int nClient, char *szIp, unsigned short nPort);
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

extern void NetFreeClients(unsigned int nWorld);
extern int func_0061D320(void);
extern int func_0061C998(unsigned int nWorld, int nValue);
extern int data_00686A48;               /* the leave message's type */
extern unsigned char data_0065117C;
extern unsigned char MyClientIsJoined;
extern int ActiveNetObjectCount;
extern void *data_0067C5B8;
extern void *data_0067C5BC;             /* clients.c's join/leave callback */
extern void *data_006511A0;
extern void *data_006511A4;             /* ... and its user data */
extern void *data_006511B0;             /* clients.c's session-master callback */
extern void *data_006511B4;             /* ... and its user data */

/* the leave message: one byte, a struct -- not promoted to a word as an unsigned char is, so its slot
 * comes after the world pointer's (ADDRESSOF purged in insn order), as the target's frame has it */
typedef struct DmeLeaveMsg {
    unsigned char nClient;
} DmeLeaveMsg;

/* SOCOM 1's NetLeave grown (0x618F70, after the join 0x618B38 as SOCOM 1's follows NetJoin): broadcasts
 * off, the leave message, the objects and clients freed, this client's record back to connecting and
 * reported, every world's value reset (SOCOM 1 stored 256 into the four directly), the callbacks cleared */
int func_00618F70(unsigned int nWorld)
{
    DmeWorld *pWorld = 0;
    DmeLeaveMsg msg;
    DmeClientInfo *pClient = 0;
    int result;
    int i;

    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK)
        return result;
    if (pWorld == 0)
        return result;
    func_006189C8(nWorld, 0);
    msg.nClient = func_0061D448(pWorld->nMyClient);
    func_006158F8(80, nWorld, -1, 0, data_00686A48, sizeof(msg), &msg);
    func_0061D320();
    NetFreeAllObjects();
    NetFreeClients(nWorld);
    data_00651178 = 4;
    data_0065117C = 0;
    MyClientIsJoined = 0;
    result = func_0061C720(nWorld, pWorld->nMyClient, &pClient);
    if (result != DME_OK)
        return result;
    if (pClient == 0)
        return result;
    pClient->nState = DME_CLIENT_CONNECTING;
    pClient->field_18 = 0;
    pClient->field_1C = 0;
    func_00613FD0(nWorld, -1);
    for (i = 0; i < DME_MAX_WORLDS; i++) {
        if (func_0061C600(i, &pWorld) == DME_OK && pWorld != 0)
            func_0061C998(i, 256);
    }
    ActiveNetObjectCount = 0;
    data_0067C5B8 = 0;
    data_006511A0 = 0;
    data_0067C5BC = 0;
    data_006511A4 = 0;
    data_006511B0 = 0;
    data_006511B4 = 0;
    return DME_OK;
}

/* an object update's head (4 bytes), then its fields */
typedef struct DmeUpdateHeader {
    unsigned char nClass;               /* 0x00: the object's class */
    unsigned char nFields;              /* 0x01: the fields that follow (func_0061A488's count) */
    unsigned short nIndex;              /* 0x02: the object */
} DmeUpdateHeader;

typedef struct DmeUpdateMsg {
    DmeUpdateHeader hdr;
    unsigned char abData[508];
} __attribute__((aligned(4))) DmeUpdateMsg;

extern DmeUpdateMsg data_0066C4F8;      /* the update being sent */
extern int data_00686A20;               /* the object update's message type */
extern void func_0061A328(DmeTargetMask *pMask, unsigned int nIndex);
struct DmeObjectData;
extern unsigned char func_0061A488(struct DmeObjectData *pObject, int nIndex, char *pnMsgClass,
                                   unsigned char *pData, unsigned int *pnLength);

/* SOCOM 1's NetSendObjectFullUpdate's head (0x61A6D8; its body is 0x61A488 now): the object's changed
 * fields to the clients its filter picks, or to everyone. Measured: the header goes in by memcpy through a
 * pointer to a word-aligned buffer (lw/sw -- a struct assignment of the 2-aligned header is lwl/lwr/swl/swr;
 * the attribute stands for whatever made SCEA's buffer type word-aligned); the buffer's address a local
 * (one register for the fill's +4 and both sends); the sends an if/else before one `return DME_OK` -- the
 * join is an eleventh block at sched1, past haifa's 10-block region limit, so the call's arguments are not
 * moved above the range test as they are with a `return` in each arm (-fno-sched-interblock does the same);
 * the zeroed locals in declaration order, the length set before the class byte (ccperm-style sweep). */
int func_0061A6D8(unsigned int nIndex)
{
    DmeUpdateHeader hdr;
    DmeTargetMask mask;
    DmeObject *pObject = 0;
    int nLength = 0;
    char nMsgClass = 0;
    DmeUpdateMsg *pMsg;
    int result;

    if (nIndex >= DME_MAX_OBJECTS)
        return DME_ERR_ARGUMENT;
    result = func_00614EC8((void **)&pObject, nIndex);
    if (result != DME_OK)
        return result;
    if (pObject == 0)
        return result;
    if (pObject->field_01 != 0)
        func_0061A328(&mask, nIndex);
    memset(&hdr, 0, sizeof(hdr));
    pMsg = &data_0066C4F8;
    hdr.nIndex = nIndex;
    nLength = sizeof(hdr);
    hdr.nClass = pObject->nClass;
    hdr.nFields = func_0061A488((struct DmeObjectData *)pObject, nIndex, &nMsgClass, pMsg->abData,
                                (unsigned int *)&nLength);
    memcpy(pMsg, &hdr, sizeof(hdr));
    if (hdr.nFields == 0)
        return DME_OK;
    if (pObject->field_01 != 0)
        func_00615B48(nMsgClass, data_00651178, 0, data_00686A20, nLength, pMsg, &mask);
    else
        func_00615DE0(nMsgClass, data_00651178, -1, data_00686A20, nLength, pMsg);
    return DME_OK;
}

extern int (*data_0067E848[])(int nClient, unsigned int nIndex);   /* each object filter (dme.c registers them) */

/* SOCOM 1's NetGetFilteredObjectTargetClients grown (0x61A328): the clients of this client's world the object's
 * filter accepts, as a target mask; guarded, and the world's client count bounds the walk. The clear loop has its
 * own counter (round 13b's rule: a2 in the target, the walk's s0 shared otherwise); the filter is read before the
 * mask's two stores (one order of six). */
void func_0061A328(DmeTargetMask *pMask, unsigned int nIndex)
{
    DmeObject *pObject = 0;
    DmeWorld *pWorld = 0;
    void *pClient;
    int (*pfnFilter)(int nClient, unsigned int nIndex);
    int i;
    int j;

    if (pMask == 0)
        return;
    if (nIndex >= DME_MAX_OBJECTS)
        return;
    if ((unsigned int)data_00651178 >= DME_MAX_WORLDS)
        return;
    if (func_0061C600(data_00651178, &pWorld) != DME_OK)
        return;
    if (pWorld == 0)
        return;
    func_00614EC8((void **)&pObject, nIndex);
    if (pObject == 0)
        return;
    pfnFilter = data_0067E848[pObject->field_01];
    pMask->nFirst = pWorld->field_444;
    pMask->nLast = -1;
    for (j = 0; j < 33; j++)
        pMask->abTarget[j] = 0;
    for (i = 0; i < pWorld->field_444; i++) {
        pClient = 0;
        func_006126E8(&pClient, data_00651178, i);
        if (pClient != 0 && i != pWorld->nMyClient && pfnFilter(i, nIndex))
            func_00615330(pMask, i);
    }
}

/* what a field's own comparison is handed (0x61B2A0 builds it on its stack) */
typedef struct DmeFieldCompare {
    int nIndex;                         /* 0x00: the object */
    int nField;                         /* 0x04 */
    int nSize;                          /* 0x08 */
    int nCount;                         /* 0x0C */
    void *pNew;                         /* 0x10 */
    void *pOld;                         /* 0x14 */
} DmeFieldCompare;

/* an object structure's field, as the update path reads it (dme.c's NetField; its bytes from 0x10 are the
 * name there, a structure field's kind, comparison and flag here) */
typedef struct DmeField {
    int nOffset;                        /* 0x00 */
    int nSize;                          /* 0x04 */
    int nCount;                         /* 0x08 */
    int nType;                          /* 0x0C */
    int nInterval;                      /* 0x10: added to the time it was last sent */
    int nKind;                          /* 0x14: 5 -- compared by its own function */
    int pfnCompare;                     /* 0x18: its comparison, an int cast at the call */
    unsigned char bFlags;               /* 0x1C: or'ed into the message's flags */
} DmeField;

/* an object structure (dme.c's NetStructure, 16 of them) */
typedef struct DmeStructure {
    int nSize;                          /* 0x000 */
    int nFields;                        /* 0x004 */
    char szName[32];                    /* 0x008 */
    DmeField *apFields[64];             /* 0x028 */
    int anOffsets[64];                  /* 0x128 */
} DmeStructure;                         /* 0x228 */

extern DmeStructure NetStructureList[16];

/* a networked object's record as the update path reads it (DmeObject's head, its data after the name) */
typedef struct DmeObjectData {
    unsigned char nClass;               /* 0x00 */
    unsigned char field_01;
    unsigned char nStatus;              /* 0x02 */
    unsigned char field_03;
    int nOwner;                         /* 0x04 */
    int nCreator;                       /* 0x08 */
    char szName[16];                    /* 0x0C */
    char *pNew;                         /* 0x1C: the object's data */
    char *pOld;                         /* 0x20: as last sent */
    int field_24;
    int field_28;
    int field_2C;
    int field_30;
    unsigned int *pnSent;               /* 0x34: each field's time last sent */
} DmeObjectData;

extern unsigned char func_0061ACF0(DmeField *pField, char *pNew, char *pOld);

/* SOCOM 1's GlobalFieldErrorExceedsThreshold grown (0x61B2A0): whether a field has changed enough to send --
 * each element by func_0061ACF0 (SOCOM 1's ElementErrorExceedsThreshold), or a kind-5 field by its own
 * comparison. Measured: the comparison is an int field cast at the call -- its load then shares the int
 * stores' alias set and sched1 keeps it after the arguments are filled, so the null test's `return 0` is
 * the target's annulled slot (a function-pointer or void * field is hoisted above the stores: 8 bytes
 * short); the test positive (as a guard, 7 words). */
unsigned char func_0061B2A0(DmeObjectData *pObject, int nIndex, unsigned char nField)
{
    DmeFieldCompare args;
    DmeStructure *pStruct;
    DmeField *pField;
    char *pNew;
    char *pOld;
    int i;

    if (pObject == 0)
        return 0;
    pStruct = &NetStructureList[pObject->nClass];
    if (pStruct == 0)
        return 0;
    pField = pStruct->apFields[nField];
    if (pField == 0)
        return 0;
    pNew = pObject->pNew + pStruct->anOffsets[nField];
    if (pNew == 0)
        return 0;
    pOld = pObject->pOld + pStruct->anOffsets[nField];
    if (pOld == 0)
        return 0;
    if (pField->nKind == 5) {
        args.nIndex = nIndex;
        args.nField = nField;
        args.nSize = pField->nSize;
        args.nCount = pField->nCount;
        args.pNew = pNew;
        args.pOld = pOld;
        if (pField->pfnCompare != 0)
            return ((int (*)(DmeFieldCompare *))pField->pfnCompare)(&args);
        return 0;
    }
    for (i = 0; i < pField->nCount; i++) {
        if (func_0061ACF0(pField, pNew, pOld))
            return 1;
        pNew += pField->nSize;
        pOld += pField->nSize;
    }
    return 0;
}

/* SOCOM 1's NetSendObjectFullUpdate's body (0x61A488, its head is 0x61A6D8): each field due -- its interval
 * passed since it was last sent -- that has changed (func_0061B2A0) and fits, as its index and its bytes;
 * the bytes kept as last sent, the field's flags or'ed into the message class; the count returned */
unsigned char func_0061A488(DmeObjectData *pObject, int nIndex, char *pnMsgClass, unsigned char *pData,
                            unsigned int *pnLength)
{
    unsigned char nFields = 0;
    DmeStructure *pStruct;
    DmeField *pField;
    char *pNew;
    char *pOld;
    unsigned char i;

    if (pData == 0)
        return 0;
    if (pnLength == 0)
        return 0;
    if (pObject == 0)
        return 0;
    if (pObject->nClass >= 16)
        return 0;
    pStruct = &NetStructureList[pObject->nClass];
    if (pStruct == 0)
        return 0;
    for (i = 0; i < pStruct->nFields; i++) {
        if (*pnLength >= 511)
            continue;
        pField = pStruct->apFields[i];
        if (pField == 0)
            continue;
        if (pField->nKind == 0)
            continue;
        if (pObject->pnSent[i] + pField->nInterval >= CurrentNetTime)
            continue;
        pNew = pObject->pNew + pStruct->anOffsets[i];
        pOld = pObject->pOld + pStruct->anOffsets[i];
        if (func_0061B2A0(pObject, nIndex, i) == 0)
            continue;
        if (510 - *pnLength < pField->nSize * pField->nCount + 1)
            continue;
        *pData = i;
        pData++;
        (*pnLength)++;
        memcpy(pData, pNew, pField->nSize * pField->nCount);
        pData += pField->nSize * pField->nCount;
        nFields++;
        *pnLength += pField->nSize * pField->nCount;
        *pnMsgClass |= pField->bFlags;
        memcpy(pOld, pNew, pField->nSize * pField->nCount);
        pObject->pnSent[i] = CurrentNetTime;
    }
    return nFields;
}

/* the 12 bytes ahead of a data stream's packet */
typedef struct DmeStreamPacket {
    unsigned char nStream;              /* 0x00 */
    unsigned char field_01;             /* 0x01: the slot's byte 5 */
    unsigned char field_02;             /* 0x02: its byte 3 */
    unsigned char field_03;             /* 0x03: its byte 1 */
    unsigned char field_04;             /* 0x04: its byte 2 */
    unsigned char field_05;
    unsigned short nSize;               /* 0x06: the stream's buffer, end less start */
    unsigned short nOffset;             /* 0x08: its read position less its start */
    unsigned short nLength;             /* 0x0A: the bytes that follow */
} DmeStreamPacket;

extern int rt_circ_buf_is_wrapped(void *pCircBuf, unsigned char *pbWrapped);
extern int rt_circ_buf_get_start_used_ptr(void *pCircBuf, void **ppStart);

/* SOCOM 1's NetGetDataStreamPacket rewritten over the stream's ring (0x61B3E0): the packet's header, then
 * nLength bytes from the ring's start (normalised first when it wraps); the packet's size, or 0. The getter's
 * result and its pointer tested together (round 5b's shape): that `return 0` block has two predecessors, so
 * it is not if-converted and jump2 cross-jumps the last guard's into it -- the target's in-line block. */
int func_0061B3E0(unsigned char *pPacket, unsigned int nStream, int nLength)
{
    DmeStreamPacket hdr;
    DmeStreamSlot *pStream = 0;
    void *pStart = 0;
    unsigned char bWrapped = 0;

    if (pPacket == 0)
        return 0;
    if (nLength <= 0)
        return 0;
    if (nStream >= DME_MAX_STREAMS)
        return 0;
    if (func_00614F58(nStream, (void **)&pStream) != DME_OK || pStream == 0)
        return 0;
    hdr.nStream = nStream;
    hdr.field_01 = pStream->field_00[5];
    hdr.field_02 = pStream->field_00[3];
    hdr.field_03 = pStream->field_00[1];
    hdr.field_04 = pStream->field_00[2];
    hdr.nSize = ((DmeStreamHead *)pStream)->field_14[0] - (int)((DmeStreamHead *)pStream)->pBuffer;
    hdr.nOffset = ((DmeStreamHead *)pStream)->field_14[1] - (int)((DmeStreamHead *)pStream)->pBuffer;
    hdr.nLength = nLength;
    *(DmeStreamPacket *)pPacket = hdr;
    if (rt_circ_buf_is_wrapped(pStream->circ, &bWrapped) != 0)
        return 0;
    if (bWrapped) {
        if (rt_circ_buf_normalize(pStream->circ, 1) != 0)
            return 0;
        memcpy(((DmeStreamHead *)pStream)->pBuffer, ((DmeStreamHead *)pStream)->pCircBuf,
               ((DmeStreamHead *)pStream)->nCircSize);
    }
    if (rt_circ_buf_get_start_used_ptr(pStream->circ, &pStart) != 0)
        return 0;
    memcpy(pPacket + sizeof(hdr), pStart, nLength);
    return nLength + sizeof(hdr);
}

extern int func_0061C5A0(void);
extern void NetFreeAllClients(void);
extern void NetFreeClientPointers(void);
extern int func_0063C810(void);
extern void func_00639808(int nConnection);
extern void func_00643790(void);
extern int rt_msg_client_shutdown(void);

/* SOCOM 1's NetClose grown (0x61C2F0; the name is Dme.c's locked wrapper's, 0x620810): the pending peers
 * dropped, each world in state 1 closed, the objects, clients, streams and the client tables freed, the
 * transport and the message client shut. Round 25b, unwritten before. */
int func_0061C2F0(void)
{
    DmeWorld *pWorld = 0;
    int i;
    int j;
    int result;

    if (data_00651180 != 1)
        return 17;
    func_0061CFF0();
    for (i = 0; i < DME_MAX_WORLDS; i++) {
        result = func_0061C600(i, &pWorld);
        if (result != DME_OK)
            return result;
        if (pWorld != 0 && pWorld->field_438 == DME_WORLD_STATE_1)
            func_0061C150(i);
    }
    NetFreeAllObjects();
    NetFreeAllClients();
    NetFreeClientPointers();
    func_0063C810();
    if (data_006511D0 != 0) {
        func_0061D320();
        func_00614888((void **)&data_006511D0);
        data_006511D0 = 0;
    }
    for (j = 0; j < DME_MAX_WORLDS; j++) {
        for (i = 0; i < DME_MAX_CLIENTS; i++)
            func_0061C868(j, i);
        func_0061C6C8(j);
    }
    func_0061C5A0();
    if (data_006511D4 != 0) {
        func_00639808(data_006511D8);
        data_006511D4 = 0;
    }
    rt_msg_client_shutdown();
    func_00643790();
    data_00651180 = 0;
    return DME_OK;
}

extern int func_00644060(int nWorld);
extern int func_00643E48(int nWorld);
extern int func_0063A4A0(int nConnection);
extern int func_0063A590(int nConnection);
extern int rt_time_start(unsigned int *pStart);
extern int rt_msg_client_disconnect(int nConnection);

/* SOCOM 1's NetDisconnect grown (0x61C150; NetDisconnect 0x623900, Dme.c's wrapper, calls it): StreamMedia's 0x643E48 run where its
 * 0x644060 says 1, then in states 1-3 the peer transport (world 0) or the message client's connection dropped, and
 * the world's records cleared. World 0 polls the transport for half a second, as SOCOM 1's polled
 * rt_msg_client_update_all. Round 25b, unwritten before. */
int func_0061C150(unsigned int nWorld)
{
    unsigned int tStart[2];
    DmeWorld *pWorld = 0;
    DmeClientInfo *pClient = 0;
    DmeWorld *p;
    unsigned int nElapsed;
    unsigned int i;
    int result;

    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK || pWorld == 0)
        return result;
    if (func_00644060(nWorld) == 1)
        func_00643E48(nWorld);
    if ((unsigned int)(pWorld->field_438 - 1) < 3) {
        if (data_006511D4 != 0 && nWorld == 0) {
            func_0063A4A0(data_006511D8);
            rt_time_start(tStart);
            do {
                result = NetMapMsgClientError(func_0063A590(data_006511D8), 24);
                if (result != DME_OK)
                    return result;
                rt_time_elapsed(tStart, &nElapsed);
            } while (nElapsed < 500);
        }
        if (nWorld != 0)
            rt_msg_client_disconnect(DmeToRtMsgConnectionIndex(nWorld));
    }
    for (i = 0; i < DME_MAX_CLIENTS; i++) {
        func_0061C720(nWorld, i, &pClient);
        if (pClient != 0)
            memset(pClient, 0, sizeof(DmeClientInfo));
    }
    p = pWorld;
    p->field_43C = -1;
    p->nMyClient = 256;
    p->nSessionMaster = 256;
    p->pfnClientCallback = 0;
    p->field_42C = 0;
    p->field_430 = 0;
    p->field_434 = 0;
    p->field_440 = 0;
    p->field_438 = DME_WORLD_STATE_0;
    memset(data_0067C5B0, 0, sizeof(data_0067C5B0));
    data_0067B7AC = 0;
    return DME_OK;
}

/* a world's record (0x61C600, written here above its caller 0x61D1B0, round 25b: that function's first
 * `bnez` keeps its plain slot only with this call's nothrow note; the object update path at the unit's end,
 * above this, is compiled without it as the image has it -- those functions lie before 0x61C600 there) */
int func_0061C600(int nWorld, DmeWorld **ppWorld)
{
    if (ppWorld == 0)
        return DME_ERR_ARGUMENT;
    *ppWorld = 0;
    if ((unsigned int)nWorld >= DME_MAX_WORLDS)
        return 15;
    *ppWorld = data_0066CAF8[nWorld];
    return DME_OK;
}

typedef struct DmeJoinReport {          /* what the join callback is handed (0x61D1B0) */
    int nResult;                        /* 0x00: 0 */
    void *pUser;                        /* 0x04: its user data */
} DmeJoinReport;

/* the join finished once every other connected client has a state (0x61D1B0, SOCOM 1 has no counterpart):
 * MyClientIsJoined set and the join callback told, once. Round 25b. */
int func_0061D1B0(void)
{
    DmeWorld *pWorld = 0;
    DmeClientInfo *pClient = 0;
    int i = 0;
    int result;
    DmeJoinReport report;

    if (data_00651178 < 0)
        return DME_ERR_RANGE;
    if (data_00651178 < DME_MAX_WORLDS && MyClientIsJoined == 0) {
        result = func_0061C600(data_00651178, &pWorld);
        if (result != DME_OK || pWorld == 0)
            return result;
        if (pWorld->nMyClient >= DME_MAX_CLIENTS)
            return DME_ERR_RANGE;
        result = func_0061C720(data_00651178, pWorld->nMyClient, &pClient);
        if (result != DME_OK || pClient == 0)
            return result;
        if ((unsigned int)(pClient->nState - DME_CLIENT_JOINED) >= 2)
            return result;
        for (; i < DME_MAX_CLIENTS; i++) {
            if (i == pWorld->nMyClient)
                continue;
            result = func_0061C720(data_00651178, i, &pClient);
            if (result != DME_OK)
                return result;
            if (pClient != 0 && pClient->bConnected == 1 && pClient->nState == 0)
                return result;
        }
        MyClientIsJoined = 1;
        if (data_0067C5B8 != 0) {
            report.nResult = 0;
            report.pUser = data_006511A0;
            ((void (*)(DmeJoinReport *))data_0067C5B8)(&report);
            data_0067C5B8 = 0;
            data_006511A0 = 0;
        }
    }
    return DME_OK;
}

typedef struct DmeResolveParams {       /* SOCOM 1's NetResolveAddr's */
    int nVersion;                       /* 0x00: 1 */
    void (*pfnCallback)(DmeAddressReport *pReport); /* 0x04 */
    DmeAddress address;                 /* 0x08: its first word 3 when set */
} DmeResolveParams;

extern int func_00639870(int nConnection, void *pAddress, void *pfnCallback);

/* SOCOM 1's NetResolveAddr grown (0x6184E8, after NetSetDefaultResolveAddrParams as SOCOM 1's follows it;
 * Dme.c's locked wrapper 0x621B28 calls it): answered at once from the last report when the transport is up and has one, else
 * the transport brought up and the resolve server asked. Each `status 23` a copy of its own, cross-jumped into
 * the last. Round 25b, unwritten before. */
int func_006184E8(DmeResolveParams *pParams, DmeConnectStatus *pStatus)
{
    struct { char szIp[16]; unsigned short nPort; } addr;
    int result;

    if (pStatus == 0)
        return DME_ERR_ARGUMENT;
    pStatus->nResult = 30;
    pStatus->field_00 = 1;
    if (pParams == 0) {
        pStatus->nResult = DME_ERR_ARGUMENT;
        return DME_ERR_ARGUMENT;
    }
    if (data_00651180 != 1) {
        pStatus->nResult = 17;
        return 17;
    }
    if (pParams->nVersion != 1) {
        pStatus->nResult = 38;
        return 38;
    }
    if (pParams->pfnCallback == 0) {
        pStatus->nResult = DME_ERR_ARGUMENT;
        return DME_ERR_ARGUMENT;
    }
    if (data_006511D4 == 1 && data_0066CF20.field_00 == 1 && data_0066CF20.nResult == 0)
        pParams->pfnCallback(&data_0066CF20);
    else {
        if (data_006511D4 == 0) {
            result = func_00616C98();
            pStatus->nResult = result;
            if (result != DME_OK)
                return result;
        }
        if (pParams->address.w[0] == 3)
            func_00618458(&pParams->address);
        if (data_0066CF08.w[0] != 3) {
            pStatus->nResult = DME_ERR_ARGUMENT;
            return DME_ERR_ARGUMENT;
        }
        data_0066CF58 = pParams->pfnCallback;
        strncpy(addr.szIp, (char *)&data_0066CF08.w[1], 16);
        addr.nPort = *(unsigned short *)&data_0066CF08.w[5];
        if (func_00639870(data_006511D8, &addr, func_0061D650) != 0) {
            pStatus->nResult = 37;
            return 37;
        }
    }
    pStatus->nResult = DME_OK;
    return DME_OK;
}

extern unsigned int strlen(const char *s);
extern int rt_msg_client_udp_set_default_options(void *pParams);
extern int rt_msg_client_enable_udp(void *pParams);
extern int rt_msg_client_send_udp(unsigned char *pbSent, char *szIp, unsigned short nPort);

typedef struct DmeUdpParams {           /* rt_msg_client's UDP options (4 bytes read here) */
    void (*pfnCallback)(char *szIp, unsigned short nPort);
} DmeUdpParams;

/* an address asked for its reachability (0x6195E0): the message client's UDP brought up once, a pending-peer slot
 * taken and the probe sent; a null or empty address ends every pending one instead. Round 25b, unwritten before. */
int func_006195E0(char *szIp, void (*pfnCallback)(int nEvent, char *szIp, int nValue))
{
    unsigned short nPort;
    unsigned char bSent;
    DmeUdpParams params;
    unsigned int nSlot;
    int result;

    if (szIp == 0 || strlen(szIp) == 0)
        return func_0061CFF0();
    if (strlen(szIp) == 0)
        return DME_ERR_ARGUMENT;
    if (pfnCallback == 0)
        return DME_ERR_ARGUMENT;
    if (data_006511CC != 1) {
        result = NetMapMsgClientError(rt_msg_client_udp_set_default_options(&params), 24);
        if (result != DME_OK)
            return result;
        params.pfnCallback = func_0061CBB8;
        result = NetMapMsgClientError(rt_msg_client_enable_udp(&params), 24);
        if (result != DME_OK)
            return result;
        memset(data_0066CF78, 0, sizeof(data_0066CF78));
        data_006511CC = 1;
    }
    result = func_0061CEF0(szIp, &nSlot, &nPort);
    if (result != DME_OK)
        return result;
    data_0066CF78[nSlot].nState = 1;
    data_0066CF78[nSlot].nPort = nPort;
    data_0066CF78[nSlot].pfnCallback = pfnCallback;
    strncpy(data_0066CF78[nSlot].szIp, szIp, 16);
    result = NetMapMsgClientError(rt_msg_client_send_udp(&bSent, data_0066CF78[nSlot].szIp,
                                                         data_0066CF78[nSlot].nPort), 24);
    if (result != DME_OK)
        return result;
    if (bSent == 1) {
        data_0066CF78[nSlot].nState = 2;
        rt_time_get_time_stamp(data_0066CF78[nSlot].tStart);
    }
    return DME_OK;
}

/* a message's fragment header (20 bytes): a message past 510 bytes goes out in pieces of 490 under DME's own
 * fragment type (data_00686A1C), each with its type, the piece's length, the count, the index, one random id
 * for the set, the whole length and the piece's offset */
typedef struct DmeFragment {
    unsigned char nType;                /* 0x00 */
    unsigned char nSubType;             /* 0x01 */
    unsigned short nLength;             /* 0x02 */
    unsigned short nCount;              /* 0x04 */
    unsigned short nIndex;              /* 0x06 */
    unsigned char nId;                  /* 0x08 */
    int nTotal;                         /* 0x0C */
    int nOffset;                        /* 0x10 */
} DmeFragment;

extern unsigned char randomMT(void);
extern unsigned int data_00686A1C;

/* SOCOM 1's NetSendMessage grown (0x6158F8): the checks, the fragments, and the choice of transport: world 0xFFFF
 * and world 0 by UDP (func_00615660), the others by TCP (func_006157E0) */
int func_006158F8(char nMsgClass, unsigned int nWorld, int nTarget, unsigned int nType, unsigned int nSubType,
                  int nLength, void *pData)
{
    DmeFragment frag;
    struct {
        DmeFragment hdr;
        unsigned char abData[492];
    } msg;
    DmeWorld *pWorld = 0;
    int nOffset = 0;
    int nSend = 0;
    int nCount;
    unsigned int nSendType;
    unsigned int nSendSub;
    char nClass;
    int i;
    int result;

    if (nType >= 6)
        return DME_ERR_ARGUMENT;
    if (nSubType >= 256)
        return DME_ERR_ARGUMENT;
    if (nLength <= 0)
        return DME_ERR_ARGUMENT;
    if (pData == 0)
        return DME_ERR_ARGUMENT;
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK || pWorld == 0)
        return result;
    if (pWorld->field_44C == 4)
        return 15;
    if (nLength >= 511) {
        if (nTarget == 0xFFFF)
            return 31;
        nCount = (nLength + 489) / 490u;
        nSendSub = data_00686A1C;
        nSendType = 0;
        nClass = nMsgClass | 0x50;
        frag.nId = randomMT();
    } else {
        nSend = nLength;
        nClass = nMsgClass;
        nSendType = nType;
        nOffset = nSend;
        nSendSub = nSubType;
        nCount = 1;
        memcpy(&msg, pData, nSend);
    }
    for (i = 0; i < nCount; i++) {
        if (nSend != nLength) {
            int nPiece = nLength - nOffset;

            if (nPiece > 490)
                nPiece = 490;
            frag.nIndex = i;
            frag.nType = nType;
            frag.nSubType = nSubType;
            frag.nTotal = nLength;
            nSend = nPiece + 20;
            frag.nCount = nCount;
            frag.nLength = nPiece;
            frag.nOffset = nOffset;
            memcpy(&msg, &frag, sizeof(frag));
            memcpy(msg.abData, (char *)pData + nOffset, nPiece);
            nOffset += nPiece;
        }
        if (nWorld == 0xFFFF || nWorld == 0) {
            result = func_00615660(nClass, nWorld, nTarget, nSendType, nSendSub, nSend, &msg);
            if (result != 0)
                return result;
        } else {
            result = func_006157E0(nClass, nWorld, nTarget, nSendType, nSendSub, nSend, &msg);
            if (result != 0)
                return result;
        }
    }
    return DME_OK;
}

/* SOCOM 1's NetPing grown (0x6193F8): every client's slot takes the callback; 0xFFFF pings the world's server, whose
 * slot holds world 4 and client 256 */
/* SOCOM 1's NetPing grown (0x6193F8): every client's slot takes the callback; 0xFFFF pings the world's server, whose
 * slot holds world 4 and client 256 */
int func_006193F8(unsigned int nWorld, unsigned int nTarget, void (*pfnCallback)())
{
    struct {
        int nTime;
        char nSlot;
        unsigned char bTimebase;
    } msg;
    DmeClientInfo *pClient = 0;
    int nSlot;
    int result;

    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    if (nTarget >= DME_MAX_CLIENTS && nTarget != 0xFFFF)
        return DME_ERR_ARGUMENT;
    if (pfnCallback == 0)
        return DME_ERR_ARGUMENT;
    if (nTarget == -1)
        return DME_ERR_RANGE;
    nSlot = FindFreePingSlot();
    if (nSlot == DME_MAX_PING_SLOTS)
        return 25;
    msg.bTimebase = 1;
    msg.nTime = 0;
    msg.nSlot = nSlot;
    NetSendMessage(-128, nWorld, nTarget, 0, data_00686A0C, sizeof(msg), &msg);
    if (nTarget == 0xFFFF) {
        result = func_006148E0(data_0067B7B8[nSlot].abIp, func_0061C8E8(nWorld));
        if (result != 0)
            return result;
        data_0067B7B8[nSlot].nPort = data_0067A6E0;
        data_0067B7B8[nSlot].nWorld = 4;
        data_0067B7B8[nSlot].nClient = 256;
    } else {
        result = func_0061C720(nWorld, nTarget, &pClient);
        if (result != 0)
            return result;
        if (pClient == 0)
            return result;
        result = func_006148E0(data_0067B7B8[nSlot].abIp, pClient->szIp);
        if (result != 0)
            return result;
        data_0067B7B8[nSlot].nWorld = nWorld;
        data_0067B7B8[nSlot].nPort = pClient->nPort;
        data_0067B7B8[nSlot].nClient = nTarget;
    }
    data_0067B7B8[nSlot].pfnCallback = pfnCallback;
    data_0067B7B8[nSlot].bTimebase = 0;
    func_00615028(&data_0067B7B8[nSlot].nSent);
    return DME_OK;
}

extern int data_006511C0;               /* the clock's last local reading */
extern unsigned int data_00651184;      /* the last ping round's time */
extern int func_006203C8(void);
extern void func_0061BA10(void);
extern void func_0061B6E8(void);
extern int KM_SetLocalClientID(int nKMID, int nClient);
extern int func_00643EF0();
extern int func_00639A30(int nP2p, int nWhich, char *szAddress, unsigned short *pnPort);
extern void (*data_00651190)(void *pReport);    /* the waiting caller's callback */
extern int data_00651194;               /* and its user value */

/* SOCOM 1's NetUpdate grown (0x61BEE8): the clock, the pings every three seconds, then the rest of the tick */
int func_0061BEE8(void)
{
    int tStart;
    int tNow;
    DmeWorld *pWorld;
    DmeClientInfo *pClient;
    int i;
    int result;

    if (data_00651180 == 0)
        return 17;
    rt_time_get_local(&tStart);
    result = func_0061D3C8();
    if (result != DME_OK)
        return result;
    result = func_0061BC68(1);
    if (result != DME_OK)
        return result;
    rt_time_get_local(&tNow);
    CurrentNetTime += tNow - data_006511C0;
    data_006511C0 = tNow;
    if (data_00651178 < DME_MAX_WORLDS) {
        pWorld = 0;
        pClient = 0;
        result = func_0061C600(data_00651178, &pWorld);
        if (result != DME_OK || pWorld == 0)
            return result;
        if (data_00651184 + 3000 < CurrentNetTime && pWorld->field_438 == 1
            && (int)pWorld->nMyClient < pWorld->field_444) {
            i = 0;
            if (pWorld->field_444 > 0) {
                do {
                    result = func_0061C720(data_00651178, i, &pClient);
                    if (result != DME_OK)
                        return result;
                    if (pClient != 0 && i != pWorld->nMyClient && pClient->bConnected == 1)
                        func_006192A0(i);
                    i++;
                } while (i < pWorld->field_444);
            }
            data_00651184 = CurrentNetTime;
        }
        if (pWorld->field_438 == 1 && (int)pWorld->nMyClient < pWorld->field_444 && data_0065117C == 1) {
            func_0061BA10();
            func_0061B6E8();
        }
    }
    result = func_0061C9E8();
    if (result != DME_OK)
        return result;
    result = func_0061CCA8(1);
    if (result != DME_OK)
        return result;
    result = func_0061D1B0();
    if (result != DME_OK)
        return result;
    result = func_006203C8();
    return result;
}

/* the connection's result (0x617060, in SOCOM 1's MsgClientConnectCallback's place): the world marked connected and
 * this client's record made, or the world closed; the waiting caller told through its callback, once */
void func_00617060(unsigned int nWorld, int nClient, unsigned char bConnected, unsigned short nValue, int nArg)
{
    struct {
        int nWorld;
        int nState;
        int nArg;
        int nResult;
        int nUser;
    } report;
    DmeWorld *pWorld = 0;
    DmeClientInfo *pClient = 0;
    int result;

    if (nWorld >= DME_MAX_WORLDS)
        return;
    if (func_0061C600(nWorld, &pWorld) != DME_OK)
        return;
    if (pWorld == 0)
        return;
    if (pWorld->field_438 == 1)
        return;
    if (bConnected) {
        if (func_0061D440(nClient) != 1)
            return;
        result = func_0061C7A8(nWorld, nClient, &pClient);
        if (result != DME_OK)
            return;
        if (pClient == 0)
            return;
        pWorld->field_438 = 1;
        pWorld->nMyClient = nClient;
        pWorld->field_440 = nValue - 1;
        pClient->nState = 2;
        pClient->field_18 = 0;
        pClient->field_1C = 0;
        pClient->bConnected = 1;
        KM_SetLocalClientID(-1, nClient);
        if (func_00644060(nWorld) == 1) {
            result = func_00643EF0(nWorld, nClient, pClient);
            if (result != DME_OK)
                return;
        }
        if (func_00639A30(data_006511D8, 1, pClient->szIp, &pClient->nPort) != 0)
            return;
    } else {
        pWorld->nMyClient = 256;
        result = 3;
        pWorld->field_438 = 3;
        if (func_00644060(nWorld) == 1)
            func_00643E48(nWorld);
    }
    if (data_00651190 != 0) {
        memset(&report, 0, sizeof(report));
        report.nWorld = nWorld;
        report.nState = pWorld->field_438;
        report.nArg = nArg;
        report.nResult = result;
        report.nUser = data_00651194;
        data_00651190(&report);
        data_00651190 = 0;
        data_00651194 = 0;
    }
}

extern int data_0066F3F8[DME_MAX_WORLDS];   /* a world's receive failed */
extern int rt_msg_client_update(int nConnection, unsigned char bBlock);
extern int func_00643878(unsigned int nWorld, unsigned char bBlock);
extern int func_0063A5C0(int nP2p);

/* SOCOM 1's NetTick grown (0x61BC68): each world's message client updated in turn, each one's error kept in the
 * status record (data_0066CF60: its size, four worlds' codes, the peer layer's), then the peer layer's own update */
/* SOCOM 1's NetTick grown (0x61BC68): each world's message client updated in turn, each one's error kept in the
 * status record (its size, four worlds' codes, the peer layer's), then the peer layer's own update */
/* SOCOM 1's NetTick grown (0x61BC68): each world's message client updated in turn, each one's error kept in the
 * status record (its size, four worlds' codes, the peer layer's), then the peer layer's own update. The three failure
 * arms are one block in the target by cross-jumping: written as one (a goto), the loop is short enough for loop.c to
 * hoist the constants 1 and 12 (scan_loop's threshold x savings x lifetime against the insn count). */
/* OPEN (round 26b): one word short: the target has a no-op `movz v0,s0,s0` after `move v0,s0` before the pWorld test;
 * the source of it is not found. The rest is exact with the three fail arms written apart (loop.c's hoist threshold
 * counts the loop's length, jump.c cross-jumps them later) and the `aStatus[0] = nP2p = 12` chain. */
int func_0061BC68(unsigned char bBlock)
{
    DmeWorld *pWorld = 0;
    int nStatus = 0;
    int result;
    unsigned char i;

    memset(data_0066F3F8, 0, sizeof(data_0066F3F8));
    memset(&data_0066CF60, 0, sizeof(data_0066CF60));
    data_0066CF60.nSize = sizeof(data_0066CF60);
    for (i = 0; i < DME_MAX_WORLDS; i++) {
        result = func_0061C600(i, &pWorld);
        if (result != DME_OK) {
            data_0066CF60.aStatus[i] = result;
            nStatus = result;
            continue;
        }
        if (pWorld == 0) {
            data_0066CF60.aStatus[i] = 0;
            continue;
        }
        if (i != 0 && (unsigned int)(pWorld->field_438 - 1) < 2) {
            result = NetMapMsgClientError(rt_msg_client_update(DmeToRtMsgConnectionIndex(i), bBlock), 24);
            if (result != DME_OK) {
                data_0066CF60.aStatus[i] = result;
                nStatus = result;
                continue;
            }
            if (data_0066F3F8[i] != 0) {
                data_0066CF60.aStatus[i] = 12;
                nStatus = 12;
                continue;
            }
        }
        if (func_00644060(i) == 1) {
            result = func_00643878(i, bBlock);
            if (result != DME_OK) {
                data_0066CF60.aStatus[i] = result;
                nStatus = result;
                continue;
            }
        }
    }
    if (data_006511D4 != 1)
        return nStatus;
    if (bBlock)
        result = func_0063A5C0(data_006511D8) != 0 ? 37 : 0;
    else
        result = func_0063A590(data_006511D8) != 0 ? 37 : 0;
    if (result != 0) {
        data_0066CF60.nP2p = result;
        return result;
    }
    if (data_0066F3F8[0] != 0) {
        data_0066CF60.aStatus[0] = data_0066CF60.nP2p = 12;
        return 12;
    }
    result = func_0061C600(0, &pWorld);
    if (pWorld == 0) {
        if (nStatus == 0)
            return 0;
        return nStatus;
    }
    if (result == 0 && (unsigned int)(pWorld->field_438 - 1) < 2)
        data_0066CF60.aStatus[0] = data_0066CF60.nP2p;
    return nStatus;
}

extern char *strcpy(char *d, const char *s);
extern int func_0063A038(int nP2p, void *pInfo);
extern unsigned char data_0066C2F8[512];
extern unsigned int data_00686A24;      /* the named object's request's message type */

/* in SOCOM 1's NetHostPeerToPeer's place (0x6187A0): hands the peer layer its two addresses and the key */
int func_006187A0(DmeHostParams *pParams, DmeConnectStatus *pStatus)
{
    DmeHostInfo info;
    unsigned int i;

    if (pStatus == 0)
        return DME_ERR_ARGUMENT;
    pStatus->nResult = 30;
    pStatus->field_00 = 1;
    if (pParams == 0) {
        pStatus->nResult = DME_ERR_ARGUMENT;
        return DME_ERR_ARGUMENT;
    }
    if (data_00651180 != 1) {
        pStatus->nResult = 17;
        return 17;
    }
    if (pParams->nVersion != 1) {
        pStatus->nResult = 38;
        return 38;
    }
    if (data_006511D4 != 1) {
        pStatus->nResult = 26;
        return 26;
    }
    memset(&info, 0, sizeof(info));
    info.key = pParams->key;
    for (i = 0; i < 2; i++) {
        if (pParams->aAddr[i].nKind == 2) {
            strncpy(info.szIp, pParams->aAddr[i].szIp, 16);
            info.nPort = pParams->aAddr[i].nPort;
        } else if (pParams->aAddr[i].nKind == 1) {
            strncpy(info.szIp2, pParams->aAddr[i].szIp, 16);
            info.nPort2 = pParams->aAddr[i].nPort;
        }
    }
    if (func_0063A038(data_006511D8, &info) != 0)
        pStatus->nResult = 37;
    else
        pStatus->nResult = DME_OK;
    return pStatus->nResult;
}

/* a named object's creation message: its head, the name, then the structure's fields */
typedef struct DmeCreateMsg {
    unsigned char nClient;              /* 0x00 */
    unsigned char nClass;               /* 0x01 */
    unsigned char nField02;             /* 0x02 */
    unsigned char nField03;             /* 0x03 */
    unsigned char nField04;             /* 0x04 */
    short nIndex;                       /* 0x06 */
    short nField08;                     /* 0x08 */
    int nTime;                          /* 0x0C */
    int nField10;                       /* 0x10 */
    char szName[16];                    /* 0x14 */
} DmeCreateMsg;                         /* 0x24 */

/* SOCOM 1's NetRequestCreateRemoteNamedObject grown (0x6197A0): asks a client to make a named object, unless this one
 * already knows it; the time is offset from the clock now */
int func_006197A0(int *pnIndex, int nUnused, int a2, int nTimeOffset, int t0, int t1, int t2, void *pData, char *szName,
                  unsigned int nClass, unsigned int nClient)
{
    DmeCreateMsg msg;
    unsigned char abDefault[512];
    DmeStructure *pStruct;
    int nIndex;
    int nOffset;
    int i;

    if (pnIndex == 0)
        return DME_ERR_ARGUMENT;
    if (pData == 0)
        return DME_ERR_ARGUMENT;
    if (szName == 0)
        return DME_ERR_ARGUMENT;
    if (nClass >= 16)
        return DME_ERR_ARGUMENT;
    if (nClient >= DME_MAX_CLIENTS)
        return DME_ERR_ARGUMENT;
    if ((unsigned int)data_00651178 >= DME_MAX_WORLDS)
        return DME_ERR_RANGE;
    nIndex = func_00616290(szName, data_00651178, nClient);
    *pnIndex = nIndex;
    if (nIndex != -1)
        return DME_OK;
    nOffset = sizeof(msg);
    pStruct = &NetStructureList[nClass];
    i = 0;
    strcpy(msg.szName, szName);
    msg.nIndex = nIndex;
    msg.nClient = func_0061D448(nClient);
    msg.nClass = nClass;
    msg.nField02 = t0;
    msg.nField04 = a2;
    msg.nField03 = t2;
    msg.nTime = CurrentNetTime + nTimeOffset;
    msg.nField10 = t1;
    msg.nField08 = 0;
    memcpy(data_0066C2F8, &msg, sizeof(msg));
    if (pStruct->nFields > 0) {
        do {
            DmeField *pField = pStruct->apFields[i];
            int nBytes = pField->nSize * pField->nCount;

            memcpy(data_0066C2F8 + nOffset, (char *)pData + pStruct->anOffsets[i], nBytes);
            nOffset += nBytes;
            i++;
        } while (i < pStruct->nFields);
    }
    func_00615DE0(80, data_00651178, nClient, data_00686A24, nOffset, data_0066C2F8);
    return DME_OK;
}
extern int rt_msg_client_udp_disable_heartbeat(void);

/* the pending peers' tick (0x61CCA8): the probe resent until it goes out, ten seconds to answer, and -- when bReport
 * says so -- each one's callback told, once */
int func_0061CCA8(unsigned char bReport)
{
    unsigned char bSent;
    unsigned int nElapsed;
    unsigned int i;
    int result;

    if (data_006511CC != 1)
        return DME_OK;
    result = NetMapMsgClientError(rt_msg_client_udp_disable_heartbeat(), 24);
    if (result != DME_OK)
        return result;
    for (i = 0; i < DME_MAX_CLIENTS; i++) {
        switch (data_0066CF78[i].nState) {
        case 0:
            break;
        case 1:
            bSent = 0;
            if (rt_msg_client_send_udp(&bSent, data_0066CF78[i].szIp, data_0066CF78[i].nPort) == 0 && bSent == 1) {
                rt_time_get_time_stamp(data_0066CF78[i].tStart);
                data_0066CF78[i].nState = 2;
            }
            break;
        case 2:
            rt_time_elapsed(data_0066CF78[i].tStart, &nElapsed);
            if (nElapsed > 10000) {
                data_0066CF78[i].nState = 4;
                if (bReport == 1) {
                    if (data_0066CF78[i].pfnCallback != 0) {
                        data_0066CF78[i].pfnCallback(14, data_0066CF78[i].szIp, 0);
                        data_0066CF78[i].pfnCallback = 0;
                    }
                    data_0066CF78[i].nState = 0;
                }
            }
            break;
        case 4:
            if (bReport == 1) {
                if (data_0066CF78[i].pfnCallback != 0) {
                    data_0066CF78[i].pfnCallback(14, data_0066CF78[i].szIp, 0);
                    data_0066CF78[i].pfnCallback = 0;
                }
                data_0066CF78[i].nState = 0;
            }
            break;
        case 3:
            if (bReport == 1) {
                if (data_0066CF78[i].pfnCallback != 0) {
                    data_0066CF78[i].pfnCallback(0, data_0066CF78[i].szIp, data_0066CF78[i].nElapsed);
                    data_0066CF78[i].pfnCallback = 0;
                }
                data_0066CF78[i].nState = 0;
            }
            break;
        default:
            data_0066CF78[i].nState = 0;
            break;
        }
    }
    return DME_OK;
}

/* SOCOM 1's SendClientNetObjectUpdates grown (0x61BA10): every live object this client owns (or made, unowned) sent
 * whole, or field by field to each client asking for updates, then the requests cleared */
void func_0061BA10(void)
{
    DmeObjectData *pObject = 0;
    DmeWorld *pWorld = 0;
    DmeClientInfo *pClient = 0;
    void *pEntry;
    unsigned int nMyClient;
    int i;
    int j;

    if ((unsigned int)data_00651178 >= DME_MAX_WORLDS)
        return;
    if (func_0061C600(data_00651178, &pWorld) != DME_OK)
        return;
    if (pWorld == 0)
        return;
    nMyClient = pWorld->nMyClient;
    if (nMyClient >= DME_MAX_CLIENTS)
        return;
    for (i = 0; i < ActiveNetObjectCount; i++) {
        if (NetObjectStatus[i] != 2)
            continue;
        if (func_00614EC8((void **)&pObject, i) != DME_OK)
            continue;
        if (pObject == 0)
            continue;
        if (nMyClient != pObject->nOwner && (pObject->nOwner != -1 || nMyClient != pObject->nCreator))
            continue;
        if (pObject->field_2C == 0 || (pObject->field_28 != 0 && (unsigned int)(pObject->field_2C + pObject->field_28) < (unsigned int)CurrentNetTime)) {
            func_0061A800(-1, i);
            continue;
        }
        for (j = 0; j < pWorld->field_444; j++) {
            pEntry = 0;
            func_006126E8(&pEntry, data_00651178, j);
            if (pEntry != 0) {
                func_0061C720(data_00651178, j, &pClient);
                if (pClient != 0 && j != nMyClient && pClient->field_29 == 1)
                    func_0061A800(j, i);
            }
        }
        func_0061A6D8(i);
    }
    for (j = 0; j < pWorld->field_444; j++) {
        func_0061C720(data_00651178, j, &pClient);
        if (pClient != 0 && j != nMyClient && pClient->field_29 == 1)
            pClient->field_29 = 0;
    }
}

extern int func_0061F920(int nWorld, int a1, int a2, int a3);
extern int func_006447C0(int nWorld, int nClient);
extern void NetClientLeaves(int nWorld, int nClient);
extern int rt_circ_buf_create(void *pCirc, int nSize, void *pBuffer);
extern int data_0067C5A8;               /* the message classes registered */
extern int func_0063A898(int nP2p, int nMaxClients);
extern int func_00639FB8(int nP2p, unsigned int *pnClient);
extern int func_00643900(int nWorld, int nMaxClients, void *pKey);

/* a data stream's slot, whole (DmeStream's head; 0x44 bytes) */
typedef struct DmeStreamRec {
    unsigned char nState;               /* 0x00: 2 open */
    unsigned char field_01;             /* 0x01 */
    unsigned char field_02;             /* 0x02 */
    unsigned char nType;                /* 0x03 */
    unsigned char field_04;             /* 0x04 */
    unsigned char field_05;             /* 0x05: 1 to everyone */
    int nTarget;                        /* 0x08 */
    int nOwner;                         /* 0x0C */
    char *pBuffer;                      /* 0x10 */
    char *pEnd;                         /* 0x14 */
    char *pRead;                        /* 0x18 */
    int field_1C;                       /* 0x1C */
    int nRate;                          /* 0x20 */
    unsigned short field_24;            /* 0x24 */
    unsigned short field_26;            /* 0x26 */
    int field_28;                       /* 0x28 */
    char circ[24];                      /* 0x2C */
} DmeStreamRec;

/* the host's parameters */
typedef struct DmeHostPeerKey {
    int field_00;
    int nFlag;                          /* 0x04: 1 opens the media streams */
} DmeHostPeerKey;

typedef struct DmeHostPeerParams {
    int nVersion;                       /* 0x00: 1 */
    void (*pfnCallback)(void *pReport); /* 0x04 */
    int nUser;                          /* 0x08 */
    void *pfnClient;                    /* 0x0C */
    void *nClientUser;                  /* 0x10 */
    void (*pfnLeave)(void *pReport);    /* 0x14 */
    void *nLeaveUser;                   /* 0x18 */
    int nMaxClients;                    /* 0x1C */
    DmeHostPeerKey key;                 /* 0x20 */
    char field_28[76];
    int nValue;                         /* 0x74 */
} DmeHostPeerParams;

/* SOCOM 1's RemoteClientDisconnectEvent grown (0x6164E0): a client gone from a world -- its objects freed, the world's
 * client callback told, and when this client was the one closing the world, the world closed */
/* OPEN (round 26b): d=10: the first arm's `beqzl v1,..; b tail; lw v1,20(v1)` where this gives bnezl, and the report's
 * store order (pClient last; not permuted). */
int func_006164E0(int nWorld, int nClient)
{
    struct {
        int nWorld;
        int nClient;
        DmeClientInfo *pClient;
        int nState;
        int field_10;
        int nUser;
    } report;
    DmeWorld *pWorld = 0;
    DmeClientInfo *pClient = 0;
    void *pEntry = 0;
    int nMyClient;
    int result;

    if (nClient < 0 && nWorld >= DME_MAX_CLIENTS)
        return DME_ERR_ARGUMENT;
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK || pWorld == 0)
        return result;
    result = func_0061C720(nWorld, nClient, &pClient);
    if (result != DME_OK)
        return result;
    nMyClient = pWorld->nMyClient;
    if (pClient == 0) {
        if (pWorld->field_440 != 0)
            pWorld->field_440--;
        result = func_0061C720(nWorld, nMyClient, &pClient);
        if (result != DME_OK || pClient == 0)
            return result;
        goto tail;
    } else if (pClient->bConnected != 0) {
        func_006126E8(&pEntry, nWorld, nClient);
        if (pEntry != 0)
            NetClientLeaves(nWorld, nClient);
        if (nMyClient != DME_MAX_CLIENTS) {
            unsigned char bConnected;

            result = func_0061C720(nWorld, nMyClient, &pClient);
            if (result != DME_OK || pWorld == 0)
                return result;
            bConnected = pClient->bConnected;
            if (bConnected == 1) {
                if (func_00644060(nWorld) == bConnected)
                    func_006447C0(nWorld, nClient);
                if (pWorld->field_430 != 0) {
                    result = func_0061C720(nWorld, nClient, &pClient);
                    if (result != DME_OK || pWorld == 0)
                        return result;
                    memset(&report, 0, sizeof(report));
                    report.nState = pClient->nState;
                    report.nUser = pWorld->field_434;
                    report.nWorld = nWorld;
                    report.nClient = nClient;
                    report.pClient = pClient;
                    pWorld->field_430(&report);
                }
            }
        }
        RemoteClientDisconnects(nWorld, nClient);
        result = func_0061C720(nWorld, nMyClient, &pClient);
        if (result != DME_OK || pClient == 0)
            return result;
        if (pClient->nState == 3)
            func_0061F920(nWorld, 0, 0, 0);
    } else {
        if (pWorld->field_440 != 0)
            pWorld->field_440--;
tail:
        if (pClient->nState == 3)
            func_0061F920(nWorld, 0, 0, 0);
    }
    return DME_OK;
}

/* SOCOM 1's NetOpenDataStream grown (0x619DD8): a free stream slot of this client's block made open, its ring buffer
 * allocated; 21 for a client without a block, 20 for no free slot */
/* OPEN (round 26b): d=14 with the stream in a one-member struct (a real alias set; a plain pointer's alias word did not
 * flip under pert.py): the constant -1 takes a1 in the target and a2 here, and pStream loads before pWorld. */
int func_00619DD8(int *pnStream, int nSize, int nRate, int nType, int a4, int nTarget, int a6, unsigned short a7,
                  unsigned short a8)
{
    DmeClient *pClient = 0;
    DmeWorld *pWorld = 0;
    struct {                            /* an aggregate: its reload survives int stores (its MEM keeps an alias set, a
                                         * scalar's ADDRESSOF does not) -- only the byte stores reload it in the target */
        DmeStreamRec *p;
    } stream;
    int nMyClient;
    int nIndex;
    int i;
    int result;

    stream.p = 0;
    if (pnStream == 0)
        return DME_ERR_ARGUMENT;
    *pnStream = -1;
    if (nSize < 0)
        return DME_ERR_ARGUMENT;
    if (nRate < 0)
        return DME_ERR_ARGUMENT;
    if ((unsigned int)data_00651178 >= DME_MAX_WORLDS)
        return 15;
    if (nType < 0 || nType >= data_0067C5A8)
        return DME_ERR_ARGUMENT;
    result = func_0061C600(data_00651178, &pWorld);
    if (result != DME_OK || pWorld == 0)
        return result;
    nMyClient = pWorld->nMyClient;
    if (nMyClient < 0 || nMyClient >= pWorld->field_444)
        return 21;
    if (func_006126E8((void **)&pClient, data_00651178, nMyClient) != 0 || pClient == 0)
        return 21;
    if (pClient->nStreams == 0)
        return 20;
    for (i = 0; i < pClient->nStreams;) {
        nIndex = i + pClient->nFirstStream;
        i++;
        if (func_00614F58(nIndex, (void **)&stream.p) != DME_OK)
            return 20;
        if (stream.p == 0)
            return 20;
        if (stream.p->nState != 2) {
            stream.p->pBuffer = DMEmalloc(nSize);
            if (stream.p->pBuffer == 0) {
                stream.p->nState = 0;
                return 22;
            }
            if (rt_circ_buf_create(stream.p->circ, nSize, stream.p->pBuffer) != 0) {
                func_00614888((void **)&stream.p->pBuffer);
                stream.p->pBuffer = 0;
                return 20;
            }
            stream.p->nState = 2;
            stream.p->nOwner = pWorld->nMyClient;
            stream.p->nRate = nRate;
            stream.p->nType = nType;
            stream.p->field_02 = a4;
            stream.p->nTarget = nTarget;
            stream.p->field_04 = 0;
            stream.p->field_24 = a7;
            stream.p->field_26 = a8;
            stream.p->pRead = stream.p->pBuffer;
            stream.p->pEnd = stream.p->pBuffer + nSize;
            stream.p->field_28 = 0;
            stream.p->field_1C = 0;
            stream.p->field_01 = a6;
            if (nTarget == -1)
                stream.p->field_05 = 1;
            else
                stream.p->field_05 = 0;
            *pnStream = nIndex;
            return DME_OK;
        }
    }
    return 20;
}

/* SOCOM 1's NetHostPeerToPeer grown (0x6181B8): world 0 made this client's, its first client, the transport and the
 * peer layer's host started, the join reported at once through 0x617060 */
/* OPEN (round 26b): d=9 when the alias state above it is stale, d=119 as this file stands (pStatus's field_08 written
 * as `((int *)pStatus)[2]` because declaring it flips 0x617060): the stores' order and a1/a2 for the constant 2. */
int func_006181B8(DmeHostPeerParams *pParams, DmeConnectStatus *pStatus)
{
    struct {
        int nWorld;
        int nState;
        int nFlag;
        int nResult;
        int nUser;
    } report;
    DmeWorld *pWorld = 0;
    DmeClientInfo *pClient = 0;
    int result = 0;
    int r;

    if (pStatus == 0)
        return DME_ERR_ARGUMENT;
    pStatus->nResult = 29;
    pStatus->field_00 = 1;
    r = func_0061D3C8();
    if (r != DME_OK)
        return r;
    if (pParams == 0) {
        pStatus->nResult = DME_ERR_ARGUMENT;
        return DME_ERR_ARGUMENT;
    }
    if (pParams->nVersion != 1) {
        pStatus->nResult = 38;
        return 38;
    }
    if (pParams->nMaxClients == 0) {
        pStatus->nResult = DME_ERR_ARGUMENT;
        return DME_ERR_ARGUMENT;
    }
    r = func_0061C638(0, &pWorld);
    pStatus->nResult = r;
    if (r != DME_OK || pWorld == 0)
        return r;
    r = func_0061C7A8(0, 0, &pClient);
    pStatus->nResult = r;
    if (r != DME_OK || pClient == 0)
        return r;
    func_0061C998(0, 256);
    pWorld->field_43C = 1;
    pWorld->field_444 = pParams->nMaxClients;
    pWorld->field_448 = pParams->nValue;
    data_00651190 = pParams->pfnCallback;
    data_00651194 = pParams->nUser;
    ((int *)pStatus)[2] = 0;
    pWorld->pfnClientCallback = pParams->pfnClient;
    pWorld->field_42C = pParams->nClientUser;
    pWorld->field_438 = 2;
    pWorld->field_430 = pParams->pfnLeave;
    pWorld->field_434 = pParams->nLeaveUser;
    pWorld->nMyClient = 256;
    if (data_006511D4 == 0)
        result = func_00616C98();
    if (func_0063A898(data_006511D8, pWorld->field_444) != 0)
        return 37;
    if (result == 0 && pParams->key.nFlag == 1)
        result = func_00643900(0, pParams->nMaxClients, &pParams->key);
    if (result != 0) {
        func_00643E48(0);
        pWorld->field_438 = 3;
        if (data_00651190 != 0) {
            memset(&report, 0, sizeof(report));
            report.nWorld = 0;
            report.nState = 3;
            report.nFlag = 1;
            report.nResult = result;
            report.nUser = data_00651194;
            data_00651190(&report);
        }
        pStatus->nResult = result;
    }
    if (result != 0)
        return result;
    if (func_00639FB8(data_006511D8, &pWorld->nMyClient) == 0) {
        func_00617060(0, pWorld->nMyClient, 1, 1, 0);
        pStatus->nResult = 0;
    } else {
        func_00617060(0, 256, 0, 0, 1);
        pStatus->nResult = 3;
    }
    return pStatus->nResult;
}

extern int func_00644698(unsigned int nWorld, unsigned int nClient);
extern int data_00686A44;               /* a new client's greeting's message type */
extern int data_00686A3C;               /* a stream's packet's message type */
extern int rt_circ_buf_get_used(void *pCircBuf, unsigned int *pnUsed);
extern int rt_circ_buf_free(void *pCircBuf, unsigned int nLength);
extern unsigned char data_0066C8F8[512];

/* SOCOM 1's RemoteClientConnects grown (0x614B18): the client's record filled, its streams, the client callback for a
 * client-server world; a peer's world answers with this client's address and public key and its state report */
/* OPEN (round 26b): d=137 here, 97 at its image place, flipped at K>=14 dummy declarations: the target reloads pClient
 * after each store, here the stale alias set (purge_addressof's half-written MEM) stops that. */
void func_00614B18(unsigned int nWorld, unsigned int nClient, char *szIp, unsigned short nPort)
{
    struct {
        unsigned char nWorld;
        unsigned char nMaster;
        unsigned char abIp[6];
        char abKey[64];
    } msg;
    struct {
        int nWorld;
        int nClient;
        DmeClientInfo *pClient;
        int nState;
        int field_10;
        int nUser;
    } report;
    DmeWorld *pWorld = 0;
    DmeClientInfo *pClient = 0;
    int nHost = 256;
    char *pRecord;

    if (nClient >= DME_MAX_CLIENTS)
        return;
    if (nWorld >= DME_MAX_WORLDS)
        return;
    if (func_0061C600(nWorld, &pWorld) != DME_OK)
        return;
    if (pWorld == 0)
        return;
    if (func_0061C7A8(nWorld, nClient, &pClient) != DME_OK)
        return;
    if (pClient == 0)
        return;
    if (pClient->bConnected == 1) {
        if (pWorld->field_44C != 4)
            func_00613FD0(nWorld, nClient);
        return;
    }
    if (pWorld->field_440 != 0)
        pWorld->field_440--;
    strncpy(pClient->szIp, szIp, 16);
    pClient->nPort = nPort;
    pClient->bConnected = 1;
    if (func_0061C720(nWorld, pWorld->nMyClient, &pClient) != DME_OK)
        return;
    if (pClient == 0)
        return;
    if (pClient->bConnected != 0) {
        if (func_00644060(nWorld) == 1) {
            if (func_00644698(nWorld, nClient) != 0)
                return;
        }
        if (pWorld->field_44C == 4) {
            if (func_0061C720(nWorld, nClient, &pClient) != DME_OK)
                return;
            if (pClient == 0)
                return;
            pClient->nState = 2;
            pClient->field_18 = 0;
            pClient->field_1C = 0;
            pClient->nValue = 0;
            memset(&report, 0, sizeof(report));
            report.nState = pClient->nState;
            report.nUser = pWorld->field_42C;
            report.nWorld = nWorld;
            report.nClient = nClient;
            report.pClient = pClient;
            report.field_10 = 0;
            pWorld->pfnClientCallback(&report, 0, 0);
            pClient->field_2A = 1;
        }
    }
    if (pWorld->field_438 != 1)
        return;
    if (pWorld->field_44C == 4)
        return;
    msg.nMaster = func_0061D448(pWorld->nMyClient);
    if (func_0061C720(nWorld, msg.nMaster, &pClient) != DME_OK)
        return;
    if (pClient == 0)
        return;
    func_006148E0(msg.abIp, pClient->szIp);
    msg.nWorld = nWorld;
    func_0062F818(0, msg.abKey);
    func_00615DE0(80, nWorld, nClient, data_00686A44, sizeof(msg), &msg);
    func_00613FD0(nWorld, nClient);
    func_00615090(&nHost, nWorld);
    if (nWorld != 0)
        return;
    if (pWorld->nMyClient != nHost)
        return;
    pRecord = 0;
    func_0061C510(nClient, (void **)&pRecord);
    if (pRecord != 0)
        rt_time_get_time_stamp(pRecord + 408);
}

typedef struct DmeStreamView {          /* a data stream's slot as the send reads it */
    unsigned char nState;               /* 0x00: 2 open */
    unsigned char field_01;
    unsigned char field_02;
    unsigned char nType;                /* 0x03 */
    unsigned char bClosing;             /* 0x04 */
    unsigned char bToHost;              /* 0x05 */
    int nTarget;                        /* 0x08 */
    int nOwner;                         /* 0x0C */
    char *pBuffer;                      /* 0x10 */
    char *pEnd;                         /* 0x14 */
    char *pRead;                        /* 0x18 */
    int field_1C;                       /* 0x1C */
    int nRate;                          /* 0x20 */
    unsigned short field_24;            /* 0x24 */
    unsigned short nMaxSend;            /* 0x26 */
    unsigned int tLast;                 /* 0x28 */
    struct {
        int x00;
        int x04;
        int nLength;                    /* 0x34 */
        int x0C;
        char *pData;                    /* 0x3C */
        int x14;
    } circ;                             /* 0x2C */
} DmeStreamView;

/* SOCOM 1's SendClientDataStreams grown (0x61B6E8): each of this client's open streams sends what it holds as far as its
 * rate allows; one that is closing and empty is freed and its close sent */
/* OPEN (round 26b): d=112, one word long: the `move v0,s3` copy before the *1000 shifts is missing, and the registers
 * shift from there. tools/retsweep.py: rt_circ_buf_destroy declared void brings it nearer (difflib 70 to 64), not home.
 * */
void func_0061B6E8(void)
{
    int nHost = 256;
    DmeStreamView *pStream = 0;
    DmeClient *pClient = 0;
    DmeWorld *pWorld = 0;
    unsigned int nUsed;
    int nIndex;
    int nTarget;
    unsigned int i;

    if ((unsigned int)data_00651178 >= DME_MAX_WORLDS)
        return;
    if (func_0061C600(data_00651178, &pWorld) != DME_OK)
        return;
    if (pWorld == 0)
        return;
    if (func_006126E8((void **)&pClient, data_00651178, pWorld->nMyClient) != 0)
        return;
    if (pClient == 0)
        return;
    if (pClient->nFirstStream >= DME_MAX_STREAMS)
        return;
    if (func_00615090(&nHost, data_00651178) != 0)
        return;
    if (nHost < 0)
        return;
    if (nHost >= DME_MAX_CLIENTS)
        return;
    for (i = 0; i < pClient->nStreams; i++) {
        nIndex = i + pClient->nFirstStream;
        if (func_00614F58(nIndex, (void **)&pStream) != DME_OK)
            continue;
        if (pStream == 0)
            continue;
        if (pStream->nState != 2)
            continue;
        nTarget = pStream->nTarget;
        if (data_0067E948[pStream->nType] != 0 && pWorld->nMyClient != nHost && pStream->bToHost != 0)
            nTarget = nHost;
        if (&pStream->circ == 0)
            continue;
        nUsed = 0;
        if (rt_circ_buf_get_used(&pStream->circ, &nUsed) != 0)
            continue;
        if (nUsed != 0) {
            DmeStreamView *p = pStream;
            int nElapsed;
            unsigned int nSend;
            unsigned int nMax;

            nMax = p->nMaxSend;
            if (nMax >= nUsed)
                nMax = nUsed;
            nElapsed = CurrentNetTime - p->tLast;
            nSend = 255;
            if (nMax < 256)
                nSend = nMax;
            nUsed = nSend;
            if ((double)(nUsed * 1000) < (double)(p->nRate * nElapsed) * 1.02) {
                if (func_00615DE0(80, data_00651178, nTarget, data_00686A3C,
                                  func_0061B3E0(data_0066C8F8, nIndex, nSend), data_0066C8F8) != 0)
                    continue;
                pStream->tLast = CurrentNetTime;
                rt_circ_buf_free(&pStream->circ, nUsed);
                rt_circ_buf_normalize(&pStream->circ, 1);
                memcpy(pStream->pBuffer, pStream->circ.pData, pStream->circ.nLength);
            }
        } else if (pStream->bClosing != 0) {
            func_00614888((void **)&pStream->pBuffer);
            rt_circ_buf_destroy(&pStream->circ);
            pStream->nState = 1;
            pStream->pBuffer = 0;
            func_0061B538(nTarget, nIndex);
        }
    }
}

/* ---- round 27b: SOCOM 1's NetInitialize grown (0x616820, the body under dme.c's NetInitialize). ---- */

typedef struct DmeInitParams {
    int nVersion;                       /* 0x00: 1, else 38 */
    int nSoftwareID;                    /* 0x04 */
    void *pKey;                         /* 0x08: 128 bytes copied, or none */
    int nPort;                          /* 0x0C */
    int nStreamBufferSize;              /* 0x10: 0 for 0x80000 */
    void (*pfnHostMessage)(DmeHostMessage *pMessage);   /* 0x14 */
    int field_18;                       /* 0x18 */
} DmeInitParams;

typedef struct DmeInitResult {
    int field_00;
    int nError;                         /* 0x04 */
    int aVersion[1];                    /* 0x08: func_006204E8 fills it */
} DmeInitResult;

extern void setNewSeed(unsigned int nSeed);
extern int func_006204E8(int *pVersion);
extern int func_0063C700(unsigned int nBufSize, unsigned int nCols, unsigned int nRows);
extern void func_0061FBC8(void);
extern void NetInitializeClients(void);
extern int func_006204B8(void);
extern int rt_msg_client_startup(void *pKey);
extern int func_00633078(char *szIp);
extern int func_00643730(void);
extern char *strncpy(char *dst, const char *src, unsigned int n);

extern int data_0067B7AC;
extern int data_0067B7B0;
extern int data_0067C5A4;
extern int data_0067C5A8;
extern int NetFieldCount;
extern int NetStructureCount;
extern int ActiveNetObjectCount;
extern unsigned char data_006511DC;
extern unsigned char data_006511CC;
extern unsigned short data_0067A6E0;
int func_00616820(DmeInitParams *pParams, DmeInitResult *pResult)
{
    unsigned int tNow;
    int i, j;
    int result;
    int nStartup, nLocalIp, nSize;

    if (pResult == 0)
        return DME_ERR_ARGUMENT;
    rt_time_start(&tNow);
    setNewSeed(tNow);
    pResult->nError = 18;
    pResult->field_00 = 1;
    result = func_006204E8(pResult->aVersion);
    if (result != 0) {
        pResult->nError = result;
        return result;
    }
    if (pParams == 0) {
        pResult->nError = DME_ERR_ARGUMENT;
        return DME_ERR_ARGUMENT;
    }
    if (pParams->nVersion != 1) {
        pResult->nError = 38;
        return DME_ERR_ARGUMENT;
    }
    if (data_00651180 != 0) {
        pResult->nError = 0;
        return 0;
    }
    data_00651180 = 1;
    memset(data_0066CAF8, 0, sizeof(data_0066CAF8));
    memset(data_0066CB08, 0, sizeof(data_0066CB08));
    data_0067B7AC = 0;
    data_0067B7B0 = pParams->nSoftwareID;
    NetResetTime(0);
    if (func_0063C700(1536, 256, 6) != 0)
        return 30;
    for (i = DME_MAX_PING_SLOTS - 1; i >= 0; i--)
        data_0067B7B8[i].pfnCallback = 0;
    NetFieldCount = 0;
    NetStructureCount = 0;
    ActiveNetObjectCount = 0;
    data_0067C5A4 = 1;
    data_0067C5A8 = 0;
    func_0061FBC8();
    for (j = 0; j < DME_MAX_OBJECTS; j++) {
        NetObjectStatus[j] = 0;
        data_006766E0[j] = 0;
    }
    for (i = 0; i < 16; i++) {
        data_0067A6E8[i] = 0;
        data_0067A728[i] = 0;
        data_0067A768[i] = 0;
    }
    NetInitializeClients();
    result = func_006204B8();
    if (result != 0)
        return result;
    if (pParams->pKey != 0) {
        memcpy(data_0066F378, pParams->pKey, 128);
        data_006511DC = 1;
    } else {
        memset(data_0066F378, 0, 128);
        data_006511DC = 0;
    }
    nStartup = rt_msg_client_startup(pParams->pKey);
    if (nStartup != 0) {
        printf("msg client startup failed, msg error code = %d\n", nStartup);
        pResult->nError = 18;
        return 18;
    }
    nLocalIp = func_00633078(data_006766A0.szLocalIp);
    if (nLocalIp != 0)
        printf("get local IP failed, msg error code = %d\n", nLocalIp);
    strncpy(data_006766A0.szIp, data_006766A0.szLocalIp, 16);
    data_0067A6E0 = pParams->nPort;
    data_006766A0.nPort = data_0067A6E0;
    data_006766A0.field_00 = 1;
    data_006766A0.field_18 = 2;
    data_006766A0.nLocalPort = data_0067A6E0;
    pResult->nError = 0;
    memset(&data_0066CF20, 0, sizeof(data_0066CF20));
    memset(data_0066CF78, 0, sizeof(data_0066CF78));
    data_006511CC = 0;
    result = func_00643730();
    if (result != 0)
        return result;
    data_006511B8 = pParams->pfnHostMessage;
    data_006511BC = pParams->field_18;
    nSize = 0x80000;
    if (pParams->nStreamBufferSize != 0)
        nSize = pParams->nStreamBufferSize;
    data_00651188 = nSize;
    return result;
}

/* SOCOM 1's MsgClientConnectCallback (0x617250, by its body and its calls: RtMsgToDmeConnectionIndex first, the disconnect
 * and MapMsgClientFailureReason arms), grown: the world and client records, the host's callback given the report. */
extern int rt_msg_client_get_connection_ip(unsigned short nConnection, char *szIp);
extern void (*data_00651190)(void *pReport);    /* the waiting caller's callback */
extern int data_00651194;               /* and its user value */
extern int rt_msg_client_get_connection_port(unsigned short nConnection);
extern int rt_msg_client_get_connection_status(unsigned short nConnection);
extern int func_0061D440(int nClient);
extern int func_0061C600(int nWorld, DmeWorld **ppWorld);
extern int func_0061C7A8(unsigned int nWorld, unsigned int nClient, DmeClientInfo **ppClient);
extern int func_00644060(int nWorld);
extern int func_00643E48(int nWorld);
extern int func_00643EF0();
extern int strncmp(const char *a, const char *b, unsigned int n);
extern int func_006189C8(unsigned int nConnection, int bEnable);
extern DmeLocalAddress data_006766A0;

void func_00617250(unsigned short nConnection, unsigned char nStatus, unsigned short nClient, unsigned short nMaxClients,
                   int nReason)
{
    int info[5];
    DmeWorld *pWorld = 0;
    DmeClientInfo *pClient = 0;
    int nResult = 0;
    int nWorld;
    int nValid;
    int nError;

    nWorld = RtMsgToDmeConnectionIndex(nConnection);
    if ((unsigned int)nWorld >= DME_MAX_WORLDS)
        return;
    nValid = func_0061D440(nClient);
    if (nValid != 1)
        return;
    if (func_0061C600(nWorld, &pWorld) != 0)
        return;
    if (pWorld == 0)
        return;
    if (func_0061C7A8(nWorld, nClient, &pClient) != 0)
        return;
    if (pClient == 0)
        return;
    if (nStatus == 1) {
        if (nWorld < DME_MAX_WORLDS && nClient < pWorld->field_444) {
            if (func_00644060(nWorld) == 1 && func_00643EF0(nWorld, nClient) != 0)
                return;
            if (pWorld == 0)
                return;
            pWorld->field_438 = 1;
            pWorld->field_440 = nMaxClients - 1;
            nResult = 0;
            nError = 0;
            pWorld->nMyClient = nClient;
            pClient->nState = 2;
            pClient->field_18 = 0;
            pClient->field_1C = 0;
            if (strncmp(data_006766A0.szIp, data_006766A0.szLocalIp, 16) == 0)
                rt_msg_client_get_connection_ip(nConnection, data_006766A0.szIp);
            strncpy(pClient->szIp, data_006766A0.szIp, 16);
            pClient->nPort = data_0067A6E0;
            pClient->bConnected = 1;
            if (pWorld->field_44C == 4)
                func_006189C8(nWorld, 1);
        } else {
            if (func_00644060(nWorld) == 1)
                func_00643E48(nWorld);
            if (nWorld < DME_MAX_WORLDS && pWorld != 0) {
                pWorld->field_438 = 3;
                pWorld->nMyClient = 256;
            }
            nResult = 1;
            nError = nClient >= pWorld->field_444 ? 28 : 3;
        }
    } else {
        if (func_00644060(nWorld) == 1)
            func_00643E48(nWorld);
        if (nWorld < DME_MAX_WORLDS && pWorld != 0)
            pWorld->field_438 = 3;
        nError = 3;
        if (func_00616F80(&nResult, nReason) != 0)
            nResult = 1;
    }
    if (data_00651190 != 0) {
        memset(info, 0, sizeof(info));
        info[0] = nWorld;
        if (nWorld < DME_MAX_WORLDS && pWorld != 0)
            info[1] = pWorld->field_438;
        else
            info[1] = 3;
        info[2] = nResult;
        info[3] = nError;
        info[4] = data_00651194;
        data_00651190(info);
        data_00651190 = 0;
        data_00651194 = 0;
    }
}
