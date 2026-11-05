/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* triggerlr: the recogniser's speech trigger block (OVL2 0x505D90-0x506D94, SOCOM 1's object at
 * 0x370C90: TriggerLR_FindSpeech, ProcessTriggerLR, TriggerLR_Reset, ControlTriggerLR,
 * InitTriggerLR -- all LOCAL there -- and ConstructTriggerLR; SOCOM II has no row for
 * TriggerLR_Reset, which its control function holds), ScanSoft's speech recogniser, prebuilt with
 * ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are
 * SOCOM 1's (s1:align, the demo's object order); the block's fields are named by their use, the
 * voicing measure's (+276 on) being voicing.c's VOICING. ConstructTriggerLR (over its draft) and
 * ProcessTriggerLR (OPEN) are written (round 24c); TriggerLR_FindSpeech, ControlTriggerLR and
 * InitTriggerLR are not. */
#include "tosblk.h"

typedef struct TRIGGERLR {
    TOSBLOCK b;
    unsigned int nframes;               /* +40: the input frames counted */
    unsigned int start;                 /* +44: the speech's first frame, nframes - lead */
    int split;                          /* +48: report the start as command 203 (else 200) */
    int f52;
    int keep;                           /* +56: keep the inputs running at the start */
    int armed;                          /* +60: the end not yet reported */
    int f64;                            /* +64: SOCOM II's (SOCOM 1's fields from here are 4 lower) */
    unsigned int f68;                   /* +68 */
    int f72;
    unsigned int maxlead;               /* +76: the lead's ceiling */
    unsigned int f80;                   /* +80 */
    char pad84[24];
    unsigned int lead;                  /* +108: the frames seen before the speech, up to maxlead */
    char pad112[32];
    int state;                          /* +144: 0 before the speech, 1 in it, 2 its end reported */
    int found;                          /* +148: TriggerLR_FindSpeech's verdict (3: speech) */
    char pad152[260];
} TRIGGERLR;                            /* 412 bytes (ConstructTriggerLR's) */

extern int Voicing_AddSignal(void *v, short *in);
extern unsigned char qQueueControl(void *q, unsigned char cmd, int arg, int n);
extern unsigned char _tosControl(void *log, unsigned char cmd, int arg, int n);
int TriggerLR_FindSpeech(TRIGGERLR *p, void **in);
int InitTriggerLR(TRIGGERLR *p);
int ControlTriggerLR(TRIGGERLR *p, unsigned char cmd);

/* one frame of the four inputs: its signal into the voicing measure while no speech is found (the
 * lead counted up to its ceiling), then the speech search; at the speech's start the start frame
 * reported (command 203 or 200, after stopping the first input unless it is kept -- the inputs
 * then disabled, else the state 1), and in the speech the end reported once (command 201, the
 * state 2). SOCOM 1's body. Declared int and returning nothing (the scheduler's TOSFUNC): each
 * failure returns what the call left in v0, as the target's paths do; the end test `f80 > f68`
 * (operand 0 expanded first: the target loads +80 first).
 * OPEN (round 24c, 0x194 against 0x18C, every block the target's but one): the target cross-jumps
 * the two state stores (state 1's arm branches to state 2's `sw v0,144(s0)`, `li v0,1` in the
 * keep test's slot). Here jump2 -- after sched2 in 2.96 -- threads state 1's jump past the
 * fall-off-the-end `(clobber (reg v0))` an int function returning nothing gets, so its tail is
 * compared with that clobber and never merges (-dJ); an explicit `return;` after `state = 1`
 * gives both tails `sw; clobber`, but the store's 1 then takes v1 (the block clobbers v0) and the
 * two still differ (build/scratch/gcc/r24c/v_ptl*.py, 11 shapes). */
static int ProcessTriggerLR(TRIGGERLR *p, void **in)
{
    if (in[0] != 0) {
        p->nframes++;
        if (p->state == 0) {
            if (Voicing_AddSignal(p, in[0]) != 0)
                return;
            p->lead++;
            if (p->lead > p->maxlead)
                p->lead = p->maxlead;
        }
        if (TriggerLR_FindSpeech(p, in) != 0)
            return;
        if (p->state == 0) {
            if (p->found == 3) {
                p->start = p->nframes - p->lead;
                if (p->keep == 0 && qQueueControl(p->b.inputs->q, 8, p->b.inputs->reader, 0) == 0)
                    return;
                if (p->split != 0) {
                    if (_tosControl(p, 203, p->lead, p->nframes) != 0)
                        return;
                } else if (_tosControl(p, 200, p->lead, p->nframes) != 0)
                    return;
                if (p->keep == 0)
                    tosBaseBlockDisableInputQueues(&p->b);
                else
                    p->state = 1;
            }
        } else if (p->state == 1) {
            if (p->armed != 0 && p->f80 > p->f68) {
                p->armed = 0;
                if (_tosControl(p, 201, p->nframes, 0) == 0)
                    p->state = 2;
            }
        }
    }
}

TOSBLOCK *ConstructTriggerLR(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 4, 0, (TOSFUNC)ProcessTriggerLR, (TOSFUNC)InitTriggerLR,
                                 (TOSFUNC)ControlTriggerLR, sizeof(TRIGGERLR));
}
