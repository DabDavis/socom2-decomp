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
 * into v1 here). */

#define DME_OK              0
#define DME_ERR_RANGE       8
#define DME_ERR_ARGUMENT    23

#define DME_MAX_OBJECTS     4096
#define DME_MAX_WORLDS      4
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

extern int printf(const char *fmt, ...);
extern int sprintf(char *buf, const char *fmt, ...);
extern void *memset(void *p, int c, unsigned int n);
extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);

extern int func_0061C600(int nWorld, char **ppWorld);
extern int func_00639A90(int nConnection, int *pValue);
extern int func_00614F58(int nIndex, void **ppObject);
extern int NetFreeRemoteObject(int nIndex);
extern int func_006437E0(void *p);
extern int func_00618B38();

extern int data_00651178;               /* the world this client is in */
extern int data_006511D8;               /* its connection */
extern void *data_006766E0[DME_MAX_OBJECTS];
extern signed char NetObjectStatus[DME_MAX_OBJECTS];
typedef struct DmeAddress { int w[6]; } DmeAddress;    /* 24 bytes, copied whole */
extern DmeAddress data_0066CF08;
extern DmePingSlot data_0067B7B8[DME_MAX_PING_SLOTS];

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
    char *pWorld = 0;
    if ((unsigned int)data_00651178 < DME_MAX_WORLDS) {
        if (func_0061C600(data_00651178, &pWorld) == 0 && pWorld != 0)
            return *(int *)(pWorld + 1084);
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
    char *pWorld = 0;
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
        *pValue = *(int *)(pWorld + 1028);
    return result;
}

int func_006150F8(int *pValue, unsigned int nWorld)
{
    char *pWorld = 0;
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
        *pValue = *(int *)(pWorld + 1080);
    return result;
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
