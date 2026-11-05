/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing */
/* libezctl: Sony's ezNetCtl sample library ("libezctl.c (1.0 beta)"), the client of the eznetctl RPC
 * server (OVL1 0x1E89E8-0x1E8FD4), prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing as
 * libezcnf beside it (README "The GCC front"): the demo's object 0x425EA0 (C), its functions in the
 * same order and sizes -- the demo reached its flag by $gp, this build by lui. Names are the demo's
 * (s1:order, each RPC number checked) and the calls' own messages. exitEzNetCtl stays a draft; dmaIOP
 * byte-exact, ezNetCtlSetProperties since GCC round 31a (below). */

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

typedef struct {
    unsigned int data;
    unsigned int addr;
    unsigned int size;
    unsigned int mode;
} sceSifDmaData;

extern unsigned int sceSifSetDma(sceSifDmaData *sdd, int len);
extern int sceSifDmaStat(unsigned int id);

extern int data_003DC108;               /* 1 once bound to the eznetctl server */
extern sceSifClientData data_00408900;
extern EZCTL_RPC data_00408940;

int dmaIOP(void *src, unsigned int dst, int size, int nowait);

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

/* the network properties handed to the server: RPC 0 asks where its buffer is, the properties are
 * DMAed there (4928 bytes), RPC 1 applies them (the demo's, 0x120 as here; the states lane, round
 * 34: the rows lie in its range, the GCC lane not running).
 * The result is one variable for both arms (GCC round 31a): the -2 path sets it and stores it, the
 * other reads the word back after RPC 1's report -- so the -2 is the return value's own register
 * (`li v0,-2; b; sw v0`, past the final reload) where the demo's text, which returns the word on
 * both paths, keeps it in v1 and jumps to the reload (3 instructions); each early return of the -2
 * rematerialises it (0x128), the variable initialised to -2 is 0x130. */
int ezNetCtlSetProperties(void *props)
{
    EZCTL_RPC *rpc = &data_00408940;
    int ret;

    if (data_003DC108 == 0) {
        scePrintf("**** eznetctl RPC server not currently running\n");
        return -1;
    }
    sceSifCallRpc(&data_00408900, 0, 0, rpc, 4, rpc, 4, 0, 0);
    if (rpc->data[0] == 0) {
        scePrintf("%s> ", "libezctl.c");
        scePrintf("eCtlGetPropertiesAddress returned 0\n");
        ret = -2;
        rpc->data[0] = ret;
    } else {
        dmaIOP(props, rpc->data[0], 4928, 0);
        sceSifCallRpc(&data_00408900, 1, 0, rpc, 4, rpc, 4, 0, 0);
        scePrintf("%s> ", "libezctl.c");
        scePrintf("ezNetCtlSetProperties returned %d\n", rpc->data[0]);
        ret = rpc->data[0];
    }
    return ret;
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

/* a block sent to the IOP over SIF DMA, its size padded to 16 (a warning when it is not), the
 * transfer queued until it is taken and, unless told not to, waited for (the demo's, its printf
 * calls scePrintf here) */
int dmaIOP(void *src, unsigned int dst, int size, int nowait)
{
    sceSifDmaData d;
    unsigned int id;

    d.data = (unsigned int)src;
    d.addr = dst;
    d.size = (size + 15) & ~15;
    d.mode = 0;
    if (size & 0xf) {
        scePrintf("%s> ", "libezctl.c");
        scePrintf("warning: SIF DMA transfer size %d padded to %d\n", size, d.size);
    }
    while ((id = sceSifSetDma(&d, 1)) == 0)
        ;
    if (!nowait)
        while (sceSifDmaStat(id) >= 0)
            ;
    return id;
}
