/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing */
/* The recogniser's engine callbacks and second pass (OVL2 0x4DE910-0x4DF2C8): ScanSoft's
 * speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing (README "The GCC
 * front": casrStoreSegmentation's store stays above the load of the next node). Names are
 * SOCOM 1's (s1:exact/align); the engine context's fields are named by their use here. */

#include "casr.h"

extern int func_00509730(int queue, int what, int id, int arg);
extern int func_004DC178(CASR *c);
extern int casrDoIsoWordSecondPass(CASR *c);
extern int casrDoNormalSecondPass(CASR *c);
extern void func_0050CDE8(void *event, int what);

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
            func_0050CDE8(c->event, 27);
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
