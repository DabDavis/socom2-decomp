/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing */
/* libezcnf: Sony's ezNetCnf sample library ("libezcnf.c~ (1.0 beta)"), the client of the eznetcnf
 * RPC server ("**** eznetcnf RPC server not currently running") (OVL1 0x1E8530-0x1E89E8),
 * prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing like the microphone input beside it
 * (README "The GCC front"). The two list accessors name themselves in their own messages. */

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
