/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_thread.c: three thread-status queries over the kernel's ReferThreadStatus (OVL2
 * 0x63CD18-0x63CF20), in SCEA's RT library, prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC
 * front"). They follow rt_table.c and precede rt_time.c, read none of rt_table.c's statics nor any
 * of their own, and have no $Header and no SOCOM 1 counterpart: the file name (between rt_table.c
 * and rt_time.c in the alphabetical link order) and every name are ours. Error code 2 is a bad
 * argument. */

struct ThreadParam {                    /* Sony's eekernel.h */
    int status;                         /* 0x00 */
    void *entry;                        /* 0x04 */
    void *stack;                        /* 0x08 */
    int stackSize;                      /* 0x0C */
    void *gpReg;                        /* 0x10 */
    int initPriority;                   /* 0x14 */
    int currentPriority;                /* 0x18 */
    unsigned int attr;                  /* 0x1C */
    unsigned int option;                /* 0x20 */
    int waitType;                       /* 0x24 */
    int waitId;                         /* 0x28 */
    int wakeupCount;                    /* 0x2C */
};                                      /* 0x30 */

extern int ReferThreadStatus(int tid, struct ThreadParam *info);
extern void *memset(void *p, int c, unsigned int n);

/* the threads whose status is not 0 */
int func_0063CD18(int *pnCount)
{
    struct ThreadParam param;
    int nCount = 0;
    int i;

    if (pnCount == 0)
        return 2;
    *pnCount = 0;
    for (i = 0; i < 256; i++) {
        if (ReferThreadStatus(i, &param) != -1 && param.status != 0)
            nCount++;
    }
    *pnCount = nCount;
    return 0;
}

/* the thread slots with status 0 that still hold a priority or a stack */
int func_0063CDA0(int *pnCount)
{
    struct ThreadParam param;
    int nCount = 0;
    int i;

    if (pnCount == 0)
        return 2;
    *pnCount = 0;
    for (i = 0; i < 256; i++) {
        if (ReferThreadStatus(i, &param) != -1 && param.status == 0
            && (param.currentPriority != 0 || param.stackSize != 0))
            nCount++;
    }
    *pnCount = nCount;
    return 0;
}

/* one thread's status, copied out when the caller's buffer holds it */
int func_0063CE38(int nThread, struct ThreadParam *pParam, unsigned int nSize, unsigned int *pnSize)
{
    struct ThreadParam param;

    if (pnSize == 0)
        return 2;
    *pnSize = 0;
    if (pParam == 0)
        return 2;
    if (nSize < sizeof(struct ThreadParam))
        return 2;
    memset(&param, 0, sizeof(struct ThreadParam));
    if (ReferThreadStatus(nThread, &param) != -1) {
        *pParam = param;
        *pnSize = sizeof(struct ThreadParam);
    }
    return 0;
}
