/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing */
/* libezcnf: Sony's ezNetCnf sample library ("libezcnf.c~ (1.0 beta)"), the client of the eznetcnf
 * RPC server ("**** eznetcnf RPC server not currently running") (OVL1 0x1E8530-0x1E89E8),
 * prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing like the microphone input beside it
 * (README "The GCC front"). The two list accessors and the three RPC calls name themselves in their
 * own messages; the bind (func_001E8530) and unload (func_001E85F8, a draft) have none, and the demo
 * does not link libezcnf (its sister libezctl calls them initEzNetCtl/exitEzNetCtl). Each call takes
 * the RPC buffer's address into a local first (the target keeps it in s0 from the entry). */

extern int scePrintf(const char *fmt, ...);

typedef struct ezNetCnfCombination {
    char data[104];
} ezNetCnfCombination_t;

typedef struct ezNetCnfCombinationList {
    int listLength;
    unsigned int defaultNumber;         /* +4, 1-based */
    char pad8[40];
    ezNetCnfCombination_t list[1];      /* +48, listLength of them */
} ezNetCnfCombinationList_t;

extern int sceSifSearchModuleByName(const char *name);
extern int sceSifBindRpc(void *cd, unsigned int num, unsigned int mode);
extern int sceSifCallRpc(void *cd, unsigned int fno, unsigned int mode, void *send, int ssize, void *recv,
                         int rsize, void (*end)(void *), void *end_param);
extern char *strcpy(char *d, const char *s);

typedef struct {
    int rpcd[4];
    unsigned int command;
    void *buff;
    void *cbuff;
    void (*func)(void *);
    void *para;
    void *serve;
} sceSifClientData;

typedef struct {
    int data[4];                        /* +0: the argument words; the result comes back in [0] */
    char fname[256];                    /* +16 */
    char usrname[32];                   /* +272 */
} EZCNF_RPC;                            /* 304 bytes each way */

extern int data_003DC100;               /* 1 once bound to the eznetcnf server */
extern sceSifClientData data_00408780;
extern EZCNF_RPC data_004087C0;

int func_001E8530(void)
{
    int ret;
    int i;

    ret = sceSifSearchModuleByName("eznetcnf");
    if (ret < 0)
        scePrintf("\"%s\" module not found (%d)\n", "eznetcnf", ret);
    while (1) {
        i = 0x10000;
        if (sceSifBindRpc(&data_00408780, 0x75499128, 0) < 0)
            scePrintf("could not bind to eznetcnf service\n");
        if (data_00408780.serve != 0)
            break;
        while (i--)
            ;
    }
    return data_003DC100 = 1;
}

int ezNetCnfGetCount(char *fname, int type)
{
    EZCNF_RPC *rpc = &data_004087C0;

    if (data_003DC100 == 0) {
        scePrintf("**** eznetcnf RPC server not currently running\n");
        return -1;
    }
    strcpy(rpc->fname, fname);
    rpc->data[0] = type;
    sceSifCallRpc(&data_00408780, 0, 0, rpc, 304, rpc, 304, 0, 0);
    scePrintf("%s> ", "libezcnf.c");
    scePrintf("ezNetCnfGetCount returned %d\n", rpc->data[0]);
    return rpc->data[0];
}

int ezNetCnfGetCombinationList(char *fname, int type, void *list)
{
    EZCNF_RPC *rpc = &data_004087C0;

    if (data_003DC100 == 0) {
        scePrintf("**** eznetcnf RPC server not currently running\n");
        return -1;
    }
    strcpy(rpc->fname, fname);
    rpc->data[0] = (int)list;
    rpc->data[1] = type;
    sceSifCallRpc(&data_00408780, 1, 0, rpc, 304, rpc, 304, 0, 0);
    scePrintf("%s> ", "libezcnf.c");
    scePrintf("ezNetCnfGetCombinationList returned %d\n", rpc->data[0]);
    return rpc->data[0];
}

ezNetCnfCombination_t *ezNetCnfGetCombination(ezNetCnfCombinationList_t *pList, int iNumber)
{
    if (pList == 0 || pList->listLength == 0) {
        scePrintf("ezNetCnfGetCombination: pList is empty\n");
        return 0;
    }
    if (iNumber <= 0 || pList->listLength < iNumber) {
        scePrintf("ezNetCnfGetCombination: invalid Combination number (%d)\n", iNumber);
        return 0;
    }
    return &pList->list[iNumber - 1];
}

ezNetCnfCombination_t *ezNetCnfGetDefault(ezNetCnfCombinationList_t *pList)
{
    if (pList == 0 || pList->listLength == 0) {
        scePrintf("ezNetCnfGetDefault: pList is empty\n");
        return 0;
    }
    if (pList->listLength < pList->defaultNumber) {
        scePrintf("ezNetCnfGetDefault: defaultNumber exceeds listLength (%d)\n", pList->defaultNumber);
        return 0;
    }
    return &pList->list[pList->defaultNumber - 1];
}

int ezNetCnfGetNetcnfifData(char *fname, char *usrname, int data)
{
    EZCNF_RPC *rpc = &data_004087C0;

    if (data_003DC100 == 0) {
        scePrintf("**** eznetcnf RPC server not currently running\n");
        return -1;
    }
    strcpy(rpc->fname, fname);
    strcpy(rpc->usrname, usrname);
    rpc->data[0] = data;
    sceSifCallRpc(&data_00408780, 2, 0, rpc, 304, rpc, 304, 0, 0);
    scePrintf("%s> ", "libezcnf.c");
    scePrintf("ezNetCnfGetNetcnfifData returned %d\n", rpc->data[0]);
    return rpc->data[0];
}
