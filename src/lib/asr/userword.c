/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops -fno-optimize-sibling-calls */
/* userword: the recogniser's user-word training (OVL2 0x4E1B28-0x4E42A4, SOCOM 1's object at 0x34FFE0:
 * userwordAcknowledge .. cbDoUserWordTraining), ScanSoft's recogniser, prebuilt with ee-gcc 2.96 at
 * -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front") and, this object, without
 * sibling calls: DoLabelStop returns casrEnableLabelInQueue's int through a jal, byte-exact only
 * under -fno-optimize-sibling-calls (10 words off without; DoLabelStart likewise). Only the
 * functions written so far are here, in image order; names are SOCOM 1's (s1:align), the context
 * casr.h's, its user-word fields by offset. OPEN: cbDoUserWordTraining (1 word; 10 before round
 * 11c, when the flag test and the last call were separate guards and 2.96 folded the last into
 * sltu: as one `&&` the final `return 0` is a join and keeps its branch): the flag test is `bnezl`
 * here and `bnez` in the target -- reorg takes the return's `move v0,zero` into the slot and
 * annuls it, though the fall-through is a call that sets v0. tosCallControlFunc (tos.c) and
 * UpdateExevStatus (casr.c) miss by the same word; round 12c toggled every optimisation switch of
 * 2.96-ee-001003-1 on all three and none moves it -- a later SCE 2.96's reorg (README). */

#include "casr.h"

typedef struct UWDATA {
    int w[9];
} UWDATA;                               /* 36 bytes, kept at +944 */

extern int casrEnableLabelInQueue(CASR *c, unsigned char enable);
extern int _tosControl(void *event, int id, int a, int b);
extern int ExitUserWordTraining(CASR *c);
extern int DoUserWordTraining(CASR *c);

int func_004E3ED8(CASR *c, int a1, int a2, UWDATA *pData)
{
    *(UWDATA *)((char *)c + 944) = *pData;
    return 0;
}

int DoLabelStop(CASR *c)
{
    if (_tosControl(c->event, 32, 0, 0) != 0)
        return 1;
    return casrEnableLabelInQueue(c, 1);
}

int DoLabelStart(CASR *c)
{
    if (casrEnableLabelInQueue(c, 0) != 0)
        return 1;
    return _tosControl(c->event, 31, 0, 0);
}

int cbCancelUserWordTraining(CASR *c)
{
    *(int *)((char *)c + 920) = 0;
    *(int *)((char *)c + 924) = 0;
    if (*((unsigned char *)c + 916) != 0 && ExitUserWordTraining(c) != 0)
        return 1;
    return 0;
}

int cbDoUserWordTraining(CASR *c)
{
    if (DoUserWordTraining(c) != 0)
        return 1;
    if (*((unsigned char *)c + 940) == 0 && ExitUserWordTraining(c) != 0)
        return 1;
    return 0;
}
