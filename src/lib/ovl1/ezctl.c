/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing */
/* libezctl: Sony's ezNetCtl sample library ("libezctl.c (1.0 beta)"), the client of the eznetctl RPC
 * server (OVL1 0x1E89E8-0x1E8FD4), prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing as
 * libezcnf beside it (README "The GCC front"): the demo's object 0x425EA0 (C), its functions in the
 * same order and sizes -- the demo reached its flag by $gp, this build by lui. Names are the demo's
 * (s1:order, each RPC number checked) and the calls' own messages. Not yet written: exitEzNetCtl (a
 * draft), ezNetCtlSetProperties, dmaIOP. */

extern int scePrintf(const char *fmt, ...);
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
} EZCTL_RPC;                            /* 304 bytes each way */

extern int data_003DC108;               /* 1 once bound to the eznetctl server */
extern sceSifClientData data_00408900;
extern EZCTL_RPC data_00408940;

int initEzNetCtl(void)
{
    int ret;
    int i;

    ret = sceSifSearchModuleByName("eznetctl");
    if (ret < 0)
        scePrintf("\"%s\" module not found (%d)\n", "eznetctl", ret);
    while (1) {
        i = 0x10000;
        if (sceSifBindRpc(&data_00408900, 0x75488909, 0) < 0)
            scePrintf("could not bind to eznetctl service\n");
        if (data_00408900.serve != 0)
            break;
        while (i--)
            ;
    }
    return data_003DC108 = 1;
}

int ezNetCtlSetCombination(char *fname, char *usrname)
{
    EZCTL_RPC *rpc = &data_00408940;

    if (data_003DC108 == 0) {
        scePrintf("**** eznetctl RPC server not currently running\n");
        return -1;
    }
    strcpy(rpc->fname, fname);
    strcpy(rpc->usrname, usrname);
    sceSifCallRpc(&data_00408900, 2, 0, rpc, 304, rpc, 304, 0, 0);
    scePrintf("%s> ", "libezctl.c");
    scePrintf("ezNetCtlSetCombination returned %d\n", rpc->data[0]);
    return rpc->data[0];
}

int ezNetCtlGetStatus(int *status)
{
    if (data_003DC108 == 0) {
        scePrintf("**** eznetctl RPC server not currently running\n");
        return -1;
    }
    sceSifCallRpc(&data_00408900, 3, 0, status, 272, status, 272, 0, 0);
    if (*status != 0) {
        scePrintf("%s> ", "libezctl.c");
        scePrintf("ezNetCtlGetStatus returned %d\n", *status);
    }
    return *status;
}

int ezNetCtlSetAutoMode(int mode)
{
    EZCTL_RPC *rpc = &data_00408940;

    if (data_003DC108 == 0) {
        scePrintf("**** eznetctl RPC server not currently running\n");
        return -1;
    }
    rpc->data[0] = mode;
    sceSifCallRpc(&data_00408900, 4, 0, rpc, 4, rpc, 4, 0, 0);
    return 0;
}

int ezNetCtlUpInterface(int id)
{
    EZCTL_RPC *rpc = &data_00408940;

    if (data_003DC108 == 0) {
        scePrintf("**** eznetctl RPC server not currently running\n");
        return -1;
    }
    rpc->data[0] = id;
    sceSifCallRpc(&data_00408900, 5, 0, rpc, 4, rpc, 4, 0, 0);
    return 0;
}

int ezNetCtlDownInterface(int id)
{
    EZCTL_RPC *rpc = &data_00408940;

    if (data_003DC108 == 0) {
        scePrintf("**** eznetctl RPC server not currently running\n");
        return -1;
    }
    rpc->data[0] = id;
    sceSifCallRpc(&data_00408900, 6, 0, rpc, 4, rpc, 4, 0, 0);
    return 0;
}
