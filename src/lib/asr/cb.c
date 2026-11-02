/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The recogniser's engine API and callbacks (OVL2 0x4DE490-0x4DF2C8, the tail of SOCOM 1's
 * casr object at 0x348248): ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops (README "The GCC front": casrStoreSegmentation's store
 * stays above the load of the next node). Names are SOCOM 1's (s1:exact/align/order); the engine
 * context's fields are named by their use here. A handle is checked against CASR_MAGIC; 2.96's
 * if-conversion turns SOCOM 1's plain guard into a movz. */

#include "casr.h"

typedef struct IMPORT {
    void *data;
    void *heap;
} IMPORT;

extern int func_00509730(int queue, int what, int id, int arg);
extern int func_004DC178(CASR *c);
extern int casrDoIsoWordSecondPass(CASR *c);
extern int casrDoNormalSecondPass(CASR *c);
extern unsigned char _tosErrorLog(void *log, unsigned short code);
extern void *func_00243298(void *heap, int size);                  /* the heap's malloc */

/* OPEN (and casrGetLastWarning): the target if-converts the magic test into a flag (li 11,
 * movz 0) and branches on it, after testing the null handle by itself; SOCOM 1's build has this
 * plain guard. The guard, a ?: macro, an inline checker, a single-exit flag: all compile to
 * branches (12 words of 16). */
int casrGetLastError(CASR *c)
{
    int err;

    if (c == 0 || c->magic != CASR_MAGIC)
        return CASR_ERR_HANDLE;
    err = c->last_error;
    c->last_error = 0;
    return err;
}

int casrGetLastWarning(CASR *c)
{
    int err;

    if (c == 0 || c->magic != CASR_MAGIC)
        return CASR_ERR_HANDLE;
    err = c->last_warning;
    c->last_warning = 0;
    return err;
}

int casrGetSignalLevel(CASR *c, unsigned short *level)
{
    if (c == 0)
        return CASR_ERR_NULL;
    if (c->magic != CASR_MAGIC)
        return CASR_ERR_HANDLE;
    if (level == 0)
        return _tosErrorLog(c->event, CASR_ERR_ARG);
    *level = *c->signal;
    return 0;
}

int casrGetState(CASR *c, int *state)
{
    if (c == 0)
        return CASR_ERR_NULL;
    if (c->magic != CASR_MAGIC)
        return CASR_ERR_HANDLE;
    if (state == 0)
        return _tosErrorLog(c->event, CASR_ERR_ARG);
    *state = c->state;
    return 0;
}

/* OPEN: the target sets the result 0 before testing ctx, keeps the type it loads in a1 and
 * stores data before heap; the guard and single-exit shapes tried miss by 18-25 words. */
IMPORT *casrImportContext(void *heap, int *ctx)
{
    IMPORT *p = 0;

    if (ctx != 0 && *ctx == 6) {
        p = func_00243298(heap, sizeof(IMPORT));
        if (p != 0) {
            p->heap = heap;
            p->data = ctx;
        }
    }
    return p;
}

IMPORT *casrImportData(void *heap, void *data)
{
    IMPORT *p;

    if (data == 0)
        return 0;
    p = func_00243298(heap, sizeof(IMPORT));
    if (p == 0)
        return 0;
    p->data = data;
    p->heap = heap;
    return p;
}

short *func_004DE910(CASR *c, int i, int j)
{
    return c->segtab[i * 3 + j];
}

void casrStoreSegmentation(CASR *c, int i, int j, SEGLABEL *l)
{
    short *p = c->segtab[i * 3 + j];

    if (l != 0) {
        do {
            *p++ = l->label;
            l = l->next;
        } while (l != 0);
    } else
        *p = -1;
}

int casrEnableLabelInQueue(CASR *c, unsigned char enable)
{
    if (enable)                         /* the two calls are cross-jumped into one */
        return func_00509730(c->queue, 5, c->queue_id, 0) == 0;
    return func_00509730(c->queue, 8, c->queue_id, 0) == 0;
}

/* OPEN: the target tests the result at the join with a branch (bnez; li 1; move 0) where every
 * shape tried (?:, if/return, != 0, a flag variable) compiles to sltu. */
int casrDoSecondPass(CASR *c)
{
    int ok;

    if (c->use_dp)
        ok = func_004DC178(c);
    else if (c->iso_word)
        ok = casrDoIsoWordSecondPass(c);
    else
        ok = casrDoNormalSecondPass(c);
    return ok ? 1 : 0;
}

int cbStateHandler(CASR *c, unsigned short state)
{
    int r;

    if (c->state_cb != 0) {
        r = c->state_cb(state, c->frames * 110, c->info);
        if (c->last_state == 0 && r != 0) {
            c->last_state = r;
            _tosErrorLog(c->event, 27);
        }
    }
    return 0;
}

int cbInitArchitectureQueues(CASR *c, unsigned char arch, unsigned char queues)
{
    if (arch == 1 && queues == 4)
        return 0;
    return 1;
}

int cbRegisterInputQueue(CASR *c, int queue, unsigned short id)
{
    c->queue = queue;
    c->queue_id = id;
    return 0;
}

int func_004DEB58(CASR *c, int cb)
{
    int *p;

    if (c->slot[0].cb == 0)
        p = &c->slot[0].cb;
    else if (c->slot[1].cb == 0)
        p = &c->slot[1].cb;
    else if (c->slot[2].cb == 0)
        p = &c->slot[2].cb;
    else if (c->slot[3].cb == 0)
        p = &c->slot[3].cb;
    else
        return 1;
    *p = cb;
    return 0;
}

int cbDoSecondPass(CASR *c, int a1, int a2, int arg1, int arg2)
{
    c->second_pass = 1;
    c->sp_arg2 = arg2;
    c->sp_arg1 = arg1;
    return 0;
}

int cbNewUserWordUtterance(CASR *c)
{
    c->f674 = 0;
    c->frames = 0;
    c->f906 = 0;
    c->f672 = 0;
    c->uw_state = 4;
    return 0;
}

/* OPEN: the target tests ==1, ==0, ==2 with no range check, its case 0 storing through the
 * join (reorg); a switch tests slti 2 first, the if-chains tried miss by 14-19 words. */
int cbGenderDisable(CASR *c, int a1, int a2, int which, int mode)
{
    c->gender_on[which] = 0;
    switch (mode) {
    case 0:
        c->gender = 1;
        break;
    case 1:
        c->gender = 2;
        break;
    case 2:
        c->gender = 4;
        break;
    default:
        return 1;
    }
    return 0;
}
