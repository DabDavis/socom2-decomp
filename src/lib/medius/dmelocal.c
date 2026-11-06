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
extern int func_0061BC68(int);
extern int func_0061CCA8(int);
extern void func_00614888(void **ppMemory);
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
extern int func_00615DE0(int nClass, int nConnection, int nTarget, int nMsgType, int nLen, void *pMsg);
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
extern int func_00614B18(int nWorld, int nClient, char *szIp, unsigned short nPort);
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
    int nType;                          /* 1, 3 and 4 client-server, 2 peer to peer */
} DmeConnectKind;

typedef struct DmeConnectParams {
    int nVersion;                       /* 0x00: 1 */
    int field_04;
    DmeConnectKind kind;                /* 0x08 */
} DmeConnectParams;

typedef struct DmeConnectStatus {
    int field_00;
    int nResult;                        /* 0x04: 3 while connecting */
} DmeConnectStatus;

extern int func_0061D3C8(void);
extern int func_00617788(DmeConnectParams *pParams, DmeConnectKind *pKind, DmeConnectStatus *pStatus);
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
extern int func_006187A0(int *pParams, DmeConnectStatus *pStatus);
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
        func_006187A0(pParams, pStatus);
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
 * OPEN (8 bytes short): the two sends cross-jump into one call here where the target keeps both (each
 * with its own `move v0,zero`), and the creator's message takes its slot before pWorld's; four shapes
 * (the message at the top, an else, a result kept from either call) tried */
int func_0061A118(unsigned int nIndex)
{
    struct { unsigned char nOwner; unsigned char pad; unsigned short nIndex; } msg;
    DmeWorld *pWorld = 0;
    DmeObject *pObject = 0;
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
    if (pObject->nCreator != pWorld->nMyClient) {
        unsigned short nMsg;

        if (pObject->nOwner != -1)
            return 11;
        nMsg = nIndex;
        func_00615DE0(80, data_00651178, pObject->nCreator, data_00686A34, 2, &nMsg);
        return DME_OK;
    }
    if (pObject->nOwner != -1)
        return 11;
    pObject->nOwner = pObject->nCreator;
    RemoteObjectUpdate(pObject->nCreator, nIndex, -2);
    msg.nIndex = nIndex;
    msg.nOwner = func_0061D448(pObject->nOwner);
    func_00615DE0(80, data_00651178, -1, data_00686A10, 4, &msg);
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
 * OPEN (the right size, the frame): the target's slots run pWorld 0, pObject 4, nHost 8, the byte
 * message 12; 2.96 here gives the message slot 0 wherever it is declared (top, inner block) and moves
 * the others up -- zeroing by statement and the `||` range guard (folded to one unsigned test) tried */
void func_0061B538(int nTarget, unsigned int nIndex)
{
    DmeWorld *pWorld = 0;
    DmeObject *pObject = 0;
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
        unsigned char nMsg = nIndex;

        func_00615DE0(80, data_00651178, nTarget, data_00686A28, 1, &nMsg);
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
