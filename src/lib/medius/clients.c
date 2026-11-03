/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* clients.c: DME's remote-client table (OVL2 0x6126A0-0x6132B8), SOCOM 1's first DME object grown --
 * the demo's object 0x4181C0-0x418B58, NetInitializeClients .. NetClientLeaves, in the same order --
 * with one function new at its end (the next session master's pick). SCEA's DME client, prebuilt
 * with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). The object has no `$Header` in .data, so its
 * file name is not known: it links between audio.c and codec.c, and this name keeps that order.
 * Names are SOCOM 1's by position (s1:order), each body checked against the demo's: SOCOM II keeps
 * one world's clients (256, NetClientPointerList indexed by the client alone) where SOCOM 1 kept a
 * row per world, tests the world against the one this client is in, and reports joins and leaves
 * through one callback with an event block where SOCOM 1 had two. The records are dmelocal.h's.
 * Four of SOCOM 1's functions are SOCOM II's API, so Dme.c's locked wrappers carry their names (the
 * game calls the wrappers: zNetGetGameClient calls NetGetClient's) and their bodies here keep
 * placeholders: 0x6126E8 NetGetClient's, 0x612750 NetGetClientStatus's, 0x612AB0
 * NetSendMyClientUpdate's, 0x612D80 NetSetMyClientObject's.
 *
 * Shapes, each measured (ccsweep): the success path nested where the target branches away to an
 * error with its code in the delay slot (0x6126E8's list test, 0x612750's range tests, 0x612AB0's
 * state test after a plain guard on the client); the free in NetFreeClient inside a loop
 * statement (below); a client's state an enum (dmelocal.h), so 0x613158 loads the new master's
 * index across the state's store; `i + base` where the sum's registers come that way round. */

#include "dmelocal.h"

#define DME_OK              0
#define DME_ERR_RANGE       8
#define DME_ERR_NO_CLIENTS  21
#define DME_ERR_ARGUMENT    23
#define DME_ERR_NO_OBJECT   30

#define DME_NO_OBJECT       0x1000
#define DME_MAX_STREAMS     2048

/* a client's own record (DMEmalloc'd, 0x1C bytes as in SOCOM 1): the object it controls, and the
 * blocks of networked objects and data streams reserved for it */
typedef struct DmeClient {
    int field_00;                       /* 0x00: the local time it joined */
    unsigned short nMyObject;           /* 0x04: DME_NO_OBJECT before func_00612D80 */
    unsigned short nFirstObject;        /* 0x06 */
    unsigned short nObjects;            /* 0x08 */
    unsigned short nFirstStream;        /* 0x0A */
    unsigned short nStreams;            /* 0x0C */
    char szName[14];                    /* 0x0E */
} DmeClient;                            /* 0x1C */

/* the client update func_00612AB0 sends (36 bytes, as SOCOM 1's) */
typedef struct DmeClientUpdate {
    unsigned char nClient;              /* 0x00 */
    char pad01[7];
    unsigned short nMyObject;           /* 0x08 */
    unsigned short nFirstObject;        /* 0x0A */
    unsigned short nObjects;            /* 0x0C */
    unsigned short nFirstStream;        /* 0x0E */
    unsigned short nStreams;            /* 0x10 */
    char pad12[4];
    char szName[14];                    /* 0x16 */
} DmeClientUpdate;                      /* 0x24 */

/* what the join/leave callback is handed (SOCOM 1 called NetClientJoinsCallback and
 * NetClientLeavesCallback with the world and the client) */
typedef struct DmeClientEvent {
    int nEvent;                         /* 0x00: 0 joins, 1 leaves */
    unsigned int nWorld;                /* 0x04 */
    unsigned int nClient;               /* 0x08 */
    void *pUserData;                    /* 0x0C */
} DmeClientEvent;

/* what the session-master callback is handed */
typedef struct DmeMasterEvent {
    unsigned int nWorld;                /* 0x00 */
    int nMaster;                        /* 0x04 */
    void *pUserData;                    /* 0x08 */
} DmeMasterEvent;

/* a networked object's record (dmelocal.c's) */
typedef struct DmeObject {
    unsigned char nClass;               /* 0x00 */
    unsigned char field_01;
    unsigned char nStatus;              /* 0x02: 3 and 4 are live */
    unsigned char field_03;
    int nOwner;                         /* 0x04: -1 when unowned */
    int nCreator;                       /* 0x08 */
} DmeObject;

/* a data stream's record */
typedef struct DmeStream {
    unsigned char nState;               /* 0x00: 1 reserved, 2 open */
    unsigned char field_01;
    unsigned char bAllocated;           /* 0x02: the buffer at 0x10 is DME's */
    unsigned char field_03;
    int field_04;
    int field_08;
    int field_0C;
    void *pBuffer;                      /* 0x10 */
} DmeStream;

extern void *memset(void *p, int c, unsigned int n);
extern char *strcpy(char *dst, const char *src);

extern void *DMEmalloc(unsigned int nSize);
extern int func_00614888(void **ppMemory);
extern int func_00614EC8(DmeObject **ppObject, unsigned int nIndex);
extern int func_00614F10(DmeStream **ppStream, int nIndex);
extern int func_00615090(int *pValue, unsigned int nWorld);
extern int func_00615DE0(int nClass, int nConnection, int nTarget, int nMsgType, int nLen, void *pMsg);
extern void RemoteObjectUpdate(int nClient, unsigned int nIndex, unsigned int nField);
extern int NetFreeRemoteObject(int nIndex);
extern int func_0061C720(unsigned int nWorld, unsigned int nClient, DmeClientInfo **ppClient);
extern int func_0061C998(unsigned int nWorld, int nValue);
extern int func_0061D440(unsigned int nClient);
extern int func_0061D448(unsigned int nClient);

extern unsigned int data_00651178;      /* the world this client is in */
extern unsigned char MyClientIsJoined;  /* SOCOM 1's MyClientIsJoined, a byte now */
extern int ActiveNetObjectCount;        /* SOCOM 1's: the object table's high water */
extern void *data_006511A4;             /* the join/leave callback's user data */
extern void (*data_006511B0)(DmeMasterEvent *pEvent);  /* the session master moved */
extern void *data_006511B4;             /* ... and its user data */
extern int data_00676698;               /* the local time */
extern void (*data_0067C5BC)(DmeClientEvent *pEvent);  /* a client joined or left */
extern int data_00686A40;               /* the client update's message type */
extern signed char NetObjectStatus[DME_MAX_OBJECTS];
extern DmeClient **NetClientPointerList;

int func_00612AB0(unsigned int nWorld, int nTarget);
int func_00613158(unsigned int nWorld, unsigned int nClient, int *pnMaster);

void NetInitializeClients(void)
{
    NetClientPointerList = DMEmalloc(DME_MAX_CLIENTS * sizeof(DmeClient *));
    MyClientIsJoined = 0;
    memset(NetClientPointerList, 0, DME_MAX_CLIENTS * sizeof(DmeClient *));
}

int func_006126E8(DmeClient **ppClient, unsigned int nWorld, unsigned int nClient)
{
    if (ppClient == 0)
        return DME_ERR_ARGUMENT;
    *ppClient = 0;
    if (nClient >= DME_MAX_CLIENTS)
        return DME_ERR_ARGUMENT;
    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    if (nWorld != data_00651178)
        return DME_OK;
    if (NetClientPointerList != 0) {
        *ppClient = NetClientPointerList[nClient];
        return DME_OK;
    }
    return DME_ERR_NO_CLIENTS;
}

int func_00612750(int *pnStatus, unsigned int nWorld, unsigned int nClient)
{
    DmeClientInfo *pInfo;
    int result;

    if (pnStatus == 0)
        return DME_ERR_ARGUMENT;
    *pnStatus = 0;
    if (nClient < DME_MAX_CLIENTS && nWorld < DME_MAX_WORLDS) {
        pInfo = 0;
        result = func_0061C720(nWorld, nClient, &pInfo);
        if (pInfo != 0 && result == DME_OK)
            *pnStatus = pInfo->nState;
        return DME_OK;
    }
    return DME_ERR_RANGE;
}

DmeClient *NetCreateClient(unsigned int nWorld, unsigned int nClient)
{
    DmeClient *pClient = 0;

    if (func_0061D440(nClient) != 1)
        return 0;
    if (nWorld >= DME_MAX_WORLDS)
        return 0;
    if (nClient < DME_MAX_CLIENTS) {
        pClient = DMEmalloc(sizeof(DmeClient));
        NetClientPointerList[nClient] = pClient;
        pClient->field_00 = 0;
        pClient->nMyObject = 0;
        pClient->nFirstObject = 0;
        pClient->nObjects = 0;
        pClient->nFirstStream = 0;
        pClient->nStreams = 0;
        pClient->szName[0] = 0;
    }
    return pClient;
}

/* The free is a statement inside a loop statement (`do ... while (0)`, as a statement macro
 * expands): 2.96 never makes a sibling call from inside a loop nest (calls.c, stmt_loop_nest_empty),
 * and the target calls DmeFree with jal here where SOCOM 1's NetFreeClient tail-called it and
 * NetFreeClientPointers still does (ccsweep: the void, int, nested, guard and pointer-local
 * spellings all give `j`; the loop around the whole body moves the prologue's saves). */
void NetFreeClient(unsigned int nWorld, unsigned int nClient)
{
    if (func_0061D440(nClient) != 1)
        return;
    if (nWorld >= DME_MAX_WORLDS)
        return;
    if (nClient >= DME_MAX_CLIENTS)
        return;
    do {
        func_00614888((void **)&NetClientPointerList[nClient]);
    } while (0);
}

void NetFreeClients(unsigned int nWorld)
{
    int i;

    if (nWorld >= DME_MAX_WORLDS)
        return;
    if (NetClientPointerList != 0) {
        for (i = 0; i < DME_MAX_CLIENTS; i++)
            NetFreeClient(nWorld, i);
    }
    MyClientIsJoined = 0;
}

void NetFreeAllClients(void)
{
    int i;

    if (NetClientPointerList != 0) {
        for (i = 0; i < DME_MAX_WORLDS; i++)
            NetFreeClients(i);
    }
    MyClientIsJoined = 0;
}

void NetFreeClientPointers(void)
{
    if (NetClientPointerList != 0)
        func_00614888((void **)&NetClientPointerList);
}

void ReserveNetObjectsAndDataStreamsForClient(DmeClient *pClient)
{
    DmeStream *pStream;
    int i;

    if (pClient == 0)
        return;
    if (pClient->nFirstObject < DME_MAX_OBJECTS) {
        for (i = 0; i < pClient->nObjects; i++)
            if (NetObjectStatus[i + pClient->nFirstObject] == 0)
                NetObjectStatus[i + pClient->nFirstObject] = 1;
    }
    if (pClient->nFirstStream < DME_MAX_STREAMS) {
        for (i = 0; i < pClient->nStreams; i++) {
            func_00614F10(&pStream, i + pClient->nFirstStream);
            if (pStream != 0)
                pStream->nState = 1;
        }
    }
}

int func_00612AB0(unsigned int nWorld, int nTarget)
{
    DmeClientUpdate msg;
    DmeWorld *pWorld = 0;
    DmeClientInfo *pInfo = 0;
    DmeClient *pClient;
    int result;

    if (nTarget < -1 || nTarget >= DME_MAX_CLIENTS)
        return DME_ERR_ARGUMENT;
    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    result = func_0061C600(nWorld, &pWorld);
    if (result != DME_OK)
        return result;
    if (pWorld == 0)
        return DME_ERR_RANGE;
    result = func_0061C720(nWorld, pWorld->nMyClient, &pInfo);
    if (result != DME_OK)
        return result;
    if (pInfo == 0)
        return DME_ERR_RANGE;
    func_006126E8(&pClient, nWorld, pWorld->nMyClient);
    if (pClient == 0)
        return DME_ERR_NO_CLIENTS;
    if (pInfo->nState == 4 || pInfo->nState == 5) {
        msg.nClient = func_0061D448(pWorld->nMyClient);
        msg.nMyObject = pClient->nMyObject;
        msg.nFirstObject = pClient->nFirstObject;
        msg.nObjects = pClient->nObjects;
        msg.nFirstStream = pClient->nFirstStream;
        msg.nStreams = pClient->nStreams;
        strcpy(msg.szName, pClient->szName);
        func_00615DE0(80, data_00651178, nTarget, data_00686A40, sizeof(msg), &msg);
        return DME_OK;
    }
    return DME_ERR_NO_CLIENTS;
}

void MyClientJoins(unsigned int nWorld, unsigned int nClient)
{
    DmeClient *pClient;

    if (func_0061D440(nClient) != 1)
        return;
    if (nWorld >= DME_MAX_WORLDS)
        return;
    func_006126E8(&pClient, nWorld, nClient);
    if (pClient == 0) {
        pClient = NetCreateClient(nWorld, nClient);
        if (pClient == 0)
            return;
    }
    pClient->nMyObject = DME_NO_OBJECT;
    func_00612AB0(nWorld, -1);
    ReserveNetObjectsAndDataStreamsForClient(pClient);
}

void NetClientJoins(unsigned int nWorld, unsigned int nClient)
{
    DmeClientEvent event;
    DmeClientInfo *pInfo = 0;
    DmeClient *pClient = 0;

    if (func_0061D440(nClient) != 1)
        return;
    if (nWorld >= DME_MAX_WORLDS)
        return;
    func_006126E8(&pClient, nWorld, nClient);
    if (pClient == 0)
        pClient = NetCreateClient(nWorld, nClient);
    if (func_0061C720(nWorld, nClient, &pInfo) != DME_OK)
        return;
    if (pInfo == 0)
        return;
    pClient->field_00 = data_00676698;
    pInfo->field_29 = 1;
    ReserveNetObjectsAndDataStreamsForClient(pClient);
    if (data_0067C5BC != 0) {
        event.nEvent = 0;
        event.nWorld = nWorld;
        event.nClient = nClient;
        event.pUserData = data_006511A4;
        data_0067C5BC(&event);
    }
    func_00612AB0(nWorld, nClient);
}

int func_00612D80(unsigned int nObject)
{
    DmeWorld *pWorld = 0;
    DmeClient *pClient;
    DmeObject *pObject;
    int result;

    if (data_00651178 >= DME_MAX_WORLDS)
        return DME_ERR_RANGE;
    if (nObject >= DME_MAX_OBJECTS)
        return DME_ERR_ARGUMENT;
    result = func_0061C600(data_00651178, &pWorld);
    if (result != DME_OK)
        return result;
    func_006126E8(&pClient, data_00651178, pWorld->nMyClient);
    if (pClient == 0)
        return DME_ERR_NO_CLIENTS;
    func_00614EC8(&pObject, nObject);
    if (pObject == 0)
        return DME_ERR_NO_OBJECT;
    if (pObject->nOwner != pWorld->nMyClient)
        return DME_ERR_RANGE;
    pClient->nMyObject = nObject;
    func_00612AB0(data_00651178, -1);
    return DME_OK;
}

void NetClientLeaves(unsigned int nWorld, unsigned int nClient)
{
    DmeClientEvent event;
    int nNewMaster = DME_MAX_CLIENTS;
    DmeClientInfo *pInfo = 0;
    DmeClient *pClient;
    int nMaster;
    DmeObject *pObject;
    DmeStream *pStream;
    int i;

    if (nWorld >= DME_MAX_WORLDS)
        return;
    if (nClient >= DME_MAX_CLIENTS)
        return;
    if (nWorld != data_00651178)
        return;
    if (func_0061C720(nWorld, nClient, &pInfo) != DME_OK)
        return;
    if (pInfo == 0)
        return;
    pInfo->nState = 2;
    pInfo->field_18 = 0;
    pInfo->field_1C = 0;
    func_006126E8(&pClient, nWorld, nClient);
    if (pClient == 0)
        return;
    func_00615090(&nMaster, nWorld);
    nNewMaster = nMaster;
    if (nClient == nMaster) {
        if (func_00613158(nWorld, nClient, &nNewMaster) != DME_OK)
            return;
        if (nNewMaster == DME_MAX_CLIENTS)
            return;
    }
    if (data_0067C5BC != 0) {
        event.nEvent = 1;
        event.nWorld = nWorld;
        event.nClient = nClient;
        event.pUserData = data_006511A4;
        data_0067C5BC(&event);
    }
    for (i = 0; i < ActiveNetObjectCount; i++) {
        if (NetObjectStatus[i] != 2)
            continue;
        func_00614EC8(&pObject, i);
        if (pObject == 0)
            continue;
        if (pObject->nCreator == nClient) {
            if ((unsigned char)(pObject->nStatus - 3) < 2) {
                func_00615090(&pObject->nCreator, nWorld);
                if (pObject->nOwner == nClient) {
                    pObject->nOwner = -1;
                    RemoteObjectUpdate(pObject->nOwner, i, -2);
                }
            } else {
                NetFreeRemoteObject(i);
                NetObjectStatus[i] = 0;
            }
        } else if (pObject->nOwner == nClient) {
            pObject->nOwner = -1;
            RemoteObjectUpdate(pObject->nOwner, i, -2);
        }
    }
    for (i = pClient->nFirstObject; i < pClient->nFirstObject + pClient->nObjects; i++)
        if (NetObjectStatus[i] == 1)
            NetObjectStatus[i] = 0;
    if (pClient->nFirstStream < DME_MAX_STREAMS) {
        for (i = 0; i < pClient->nStreams; i++) {
            func_00614F10(&pStream, i + pClient->nFirstStream);
            if (pStream == 0)
                continue;
            if (pStream->nState == 2 && pStream->bAllocated != 0)
                func_00614888(&pStream->pBuffer);
            pStream->nState = 0;
        }
    }
    NetFreeClient(nWorld, nClient);
}

/* new in SOCOM II: the session master left, so the joined client with the lowest positive count
 * at 0x24 becomes the master (state 5), and the session-master callback is told */
int func_00613158(unsigned int nWorld, unsigned int nClient, int *pnMaster)
{
    DmeMasterEvent event;
    DmeClientInfo *pInfo = 0;
    int nBest = 0x0FFFFFFF;
    int result;
    int i;

    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    if (func_0061D440(nClient) != 1)
        return DME_ERR_ARGUMENT;
    if (pnMaster == 0)
        return DME_ERR_ARGUMENT;
    *pnMaster = DME_MAX_CLIENTS;
    for (i = 0; i < DME_MAX_CLIENTS; i++) {
        if (nClient == i)
            continue;
        result = func_0061C720(nWorld, i, &pInfo);
        if (result != DME_OK)
            return result;
        if (pInfo != 0 && pInfo->nState == 4 && pInfo->nValue > 0 && pInfo->nValue < nBest) {
            nBest = pInfo->nValue;
            *pnMaster = i;
        }
    }
    if (*pnMaster != DME_MAX_CLIENTS) {
        result = func_0061C720(nWorld, *pnMaster, &pInfo);
        if (result != DME_OK || pInfo == 0)
            return result;
        pInfo->nState = 5;
        func_0061C998(nWorld, *pnMaster);
        if (data_006511B0 != 0) {
            event.nWorld = nWorld;
            event.nMaster = *pnMaster;
            event.pUserData = data_006511B4;
            data_006511B0(&event);
        }
    }
    return DME_OK;
}
