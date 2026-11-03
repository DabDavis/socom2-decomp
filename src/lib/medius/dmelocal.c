/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* DmeLocal.c: DME's internal functions (OVL2 0x613FD0-0x61Dxxx; dme_client/src/DmeLocal.c 1.52, whose
 * $Header heads the .data its functions use -- DMEmalloc's message, NetIpBinaryToString's format), the
 * bodies SOCOM 1's Dme.c had, behind SOCOM II's locked wrappers (dme.c); SCEA's DME client, prebuilt
 * with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). Only the functions written so far are here,
 * in image order. Names are SOCOM 1's where the map has them (s1:exact, s1:calls, s1:order);
 * placeholders elsewhere. DME's error codes are the values returned: 23 a bad argument, 8 an index
 * out of range, 37 the transport refused, 38 not initialised. The client records are reached by
 * offset until their layout is written down.
 *
 * The vendor nests the success path -- `if (ok) { ...; return 0; } return ERR;` -- where the target
 * branches away to the error with its code in the delay slot (NetIpBinaryToString, func_00614EC8);
 * a guard clause where the error branch is the fall-through. OPEN: func_00614758 (6 words: the
 * transport call's failure branch -- guard and nested forms each lay one of its two returns out
 * the other way), func_006147E0 (6: the world index loaded into v0 and copied to a0 in the target,
 * into v1 here), func_0061C6C8 (4: the slot's address and the range test computed into each
 * other's registers), func_0061C8E8 (11s: 2.96 turns the last test into movn, which the target
 * does not -- guard, nested and single-exit forms tried). func_0061BEB8 returns the second call's
 * result through a variable: no sibling call then. */

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
extern int func_00614F58(int nIndex, void **ppObject);
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

extern int func_0061C720(int nWorld, int nClient, DmeClientInfo **ppClient);
extern int func_00614EC8(void **ppObject, unsigned int nIndex);
extern int func_00615DE0(int nClass, int nConnection, int nTarget, int nMsgType, int nLen, void *pMsg);
extern void (*data_0067A6E8[16])(int nClient, int nIndex);
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
    DmeWorld *pWorld = 0;
    if ((unsigned int)data_00651178 < DME_MAX_WORLDS) {
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

int FindFreePingSlot(void)
{
    int i;

    for (i = 0; i < DME_MAX_PING_SLOTS; i++)
        if (data_0067B7B8[i].bInUse == 0)
            return i;
    return DME_MAX_PING_SLOTS;
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

int func_0061C6C8(unsigned int nWorld)
{
    void **ppMemory = &data_0066CAF8[nWorld];

    if (nWorld >= DME_MAX_WORLDS)
        return DME_ERR_ARGUMENT;
    if (*ppMemory != 0)
        func_00614888(ppMemory);
    *ppMemory = 0;
    return DME_OK;
}

char *func_0061C8E8(unsigned int nWorld)
{
    DmeWorld *pWorld = 0;

    if (nWorld >= DME_MAX_WORLDS)
        return 0;
    if (func_0061C600(nWorld, &pWorld) != 0)
        return 0;
    if (pWorld == 0)
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
