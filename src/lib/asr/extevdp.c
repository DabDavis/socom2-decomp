/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The recogniser's extra-event block (OVL2 0x4E6C20-0x4E71F8, SOCOM 1's object at 0x354AD0: the
 * same five functions in the same order), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96
 * at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM 1's
 * (s1:align). The block runs a dynamic programme over the extra events' states (the silences and
 * noises a recognition may meet between words): each event is a left-to-right chain of states,
 * entered from the first state, and every frame each state takes the best of itself and its
 * predecessor, then adds its phoneme's score for the frame (cached per phoneme, 32767 when not
 * yet asked). Command 6 hands it the scorer, command 8 the events, command 2 resets it, command 1
 * reads the best score so far. */
#include "tosblk.h"

#define MAXSCORE        0x7FFFF7FF      /* a state not reached */
#define NOSCORE         32767           /* a phoneme not yet scored this frame */

/* a state of the programme */
typedef struct DPCELL {
    int score;                          /* +0 */
    unsigned int phen;                  /* +4: its phoneme */
} DPCELL;

/* command 6's argument: the scorer */
typedef struct EXEVSCORER {
    void *ctx;                                          /* +0 */
    int (*getscore)(void *ctx, unsigned int phen);      /* +4 */
    void *pad8;
    short *(*getcache)(void *ctx);                      /* +12 */
    int (*framescore)(void *ctx);                       /* +16 */
} EXEVSCORER;

/* command 8's argument: the events */
typedef struct EXEVSETUP {
    unsigned int nstates;               /* +0 */
    unsigned short *phen;               /* +4: each state's phoneme */
    unsigned short *ranges;             /* +8: event i's states are [ranges[i], ranges[i + 1]) */
    unsigned int nevents;               /* +12 */
    int penalty;                        /* +16: entering an event */
    int start;                          /* +20: the first state's score on reset */
} EXEVSETUP;

typedef struct EXEVDP {
    TOSBLOCK b;
    QIN *in;                            /* +40 */
    void *ctx;                          /* +44 */
    int (*getscore)(void *ctx, unsigned int phen);      /* +48 */
    int (*framescore)(void *ctx);       /* +52 */
    short *(*getcache)(void *ctx);      /* +56 */
    short *cache;                       /* +60 */
    unsigned int nstates;               /* +64 */
    unsigned short *phen;               /* +68 */
    int best;                           /* +72 */
    int acc;                            /* +76: the frames' scores summed */
    unsigned short frame;               /* +80 */
    int penalty;                        /* +84 */
    int start;                          /* +88 */
    int pad92;
    unsigned int nevents;               /* +96 */
    unsigned short *ranges;             /* +100 */
    DPCELL *cells;                      /* +104 */
} EXEVDP;

static void DynProgExtraEventsProcess(EXEVDP *e);
static __inline__ void FreeCells(TOS *tos, EXEVDP *e);
static int ControlExtraEventDP(EXEVDP *e, unsigned char cmd, void *arg);
static int ProcessExtraEventDP(EXEVDP *e, void **in);
static int InitExtraEventDP(EXEVDP *e);

static void DynProgExtraEventsProcess(EXEVDP *e)
{
    short *cache = e->cache;
    DPCELL *first = e->cells;
    DPCELL *last = &first[e->nstates - 1];
    int best = last->score;
    int prev = 0x7FFFFFFF;
    int cur;
    DPCELL *p, *end;
    unsigned short i, j;

    for (i = 1; i <= e->nevents; i++) {
        cur = first->score + e->penalty;
        p = &e->cells[e->ranges[i]];
        end = &e->cells[e->ranges[i + 1]] - 1;
        for (; p < end; p++) {
            prev = cur;
            cur = p->score;
            if (prev < cur)
                p->score = prev;
        }
        if (p->score < best)
            last->score = p->score;
        if (prev < p->score)
            p->score = prev;
    }
    if (best < first->score)
        first->score = last->score;
    p = e->cells;
    for (j = 0; j < e->nstates; j++) {
        p->score += cache[p->phen] == NOSCORE ? (cache[p->phen] = e->getscore(e->ctx, p->phen))
                                              : cache[p->phen];
        if (p->score > MAXSCORE)
            p->score = MAXSCORE;
        p++;
    }
    if (first->score < last->score)
        e->best = first->score;
    else
        e->best = last->score;
}

/* no name of the demo's: inlined there too (its argument loaded before the test, as the
 * target does in both commands) */
static __inline__ void FreeCells(TOS *tos, EXEVDP *e)
{
    if (e->cells != 0) {
        heap_Free(tos->heap, e->cells);
        e->cells = 0;
    }
}

static int ControlExtraEventDP(EXEVDP *e, unsigned char cmd, void *arg)
{
    TOS *tos;
    DPCELL *c;
    unsigned int i;

    switch (cmd) {
    case 1:
        if (arg != 0)
            *(int *)arg = e->best + e->acc;
        break;
    case 2:
        e->frame = 0xFFFF;
        e->best = 0;
        e->acc = 0;
        c = e->cells;
        c->score = e->start;
        c->phen = e->phen[0];
        c++;
        for (i = 1; i < e->nstates; i++) {
            c->score = MAXSCORE;
            c->phen = e->phen[i];
            c++;
        }
        e->b.process = (TOSFUNC)ProcessExtraEventDP;
        break;
    case 3:
        break;
    case 8:
        FreeCells(e->b.tos, e);
        tos = e->b.tos;
        e->nevents = ((EXEVSETUP *)arg)->nevents;
        e->penalty = ((EXEVSETUP *)arg)->penalty;
        e->start = ((EXEVSETUP *)arg)->start;
        e->nstates = ((EXEVSETUP *)arg)->nstates;
        e->phen = ((EXEVSETUP *)arg)->phen;
        e->ranges = ((EXEVSETUP *)arg)->ranges;
        e->cache = e->getcache(e->ctx);
        e->cells = heap_Alloc(tos->heap, e->nstates * sizeof(DPCELL));
        if (e->cells == 0) {
            _tosErrorLog(e, 1);
            return 1;
        }
        break;
    case 6:
        e->getscore = ((EXEVSCORER *)arg)->getscore;
        e->getcache = ((EXEVSCORER *)arg)->getcache;
        e->framescore = ((EXEVSCORER *)arg)->framescore;
        e->ctx = ((EXEVSCORER *)arg)->ctx;
        break;
    case 7:
        break;
    case TOS_DESTROY:
        FreeCells(e->b.tos, e);
        tosBaseBlockDestruct(&e->b);
        break;
    default:
        return 0;
    }
    return 1;
}

TOSBLOCK *ConstructExtraEventDp(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 1, 0, (TOSFUNC)ProcessExtraEventDP,
                                 (TOSFUNC)InitExtraEventDP, (TOSFUNC)ControlExtraEventDP,
                                 sizeof(EXEVDP));
}

static int ProcessExtraEventDP(EXEVDP *e, void **in)
{
    if (in[0] != 0) {
        e->frame++;
        e->acc += e->framescore(e->ctx);
        DynProgExtraEventsProcess(e);
    }
    return 0;
}

static int InitExtraEventDP(EXEVDP *e)
{
    e->in = e->b.inputs;
    e->in->size = _tosGetProfileU32(e, 15, 4);
    return 0;
}
