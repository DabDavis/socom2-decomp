/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The recogniser's user-word generation block (OVL2 0x4E4628-0x4E5960, SOCOM 1's object at
 * 0x352770: AllocateBacktrace, InitViterbi, InitDpGenUw, SendResultUserWords, DynProgUserWords,
 * DpGenUwProcess, ControlDpGenUw, ConstructDpGenUw, IdleDpGenUwProcess,
 * MakeUserWordsActiveDist -- SOCOM II moves command 23's set-up out of ControlDpGenUw into a
 * function of its own, func_004E5538, where the DpScoreUw object has InitScoreUserWords),
 * ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops (README "The GCC front"). Names are SOCOM 1's (s1:align, the statics s1:order
 * and s1:slot). The block runs a phoneme loop over the frames of a word being trained (twenty
 * phoneme cells and the garbage phoneme's two), back pointers taken from a free list of chunks
 * as in the DpScoreUw object. MakeUserWordsActiveDist, the object's last function, is
 * ctxutil.c's (byte-exact there). DynProgUserWords (rounds 16c-17c) is byte-exact, its shapes
 * at it. SendResultUserWords (round 12c) walks the backtrace as `for (; bt->next != 0; bt =
 * bt->next)` (a `next` variable copies the test's load differently), its score's nonzero arm
 * first; the frame counter and the +112 word are unsigned (both convert by the halving trick). */
#include "tosblk.h"

#define NWORDCELLS      20              /* the phoneme cells, then the garbage phoneme's two */
#define WORSTSCORE      0x7B86          /* a cell not reached */
#define SILENT          0x3FC           /* an ergodic phoneme's score, the garbage one's aside */

/* a queue as its readers see it (queue.c): the bytes of a frame at +8 */
typedef struct QUEUE {
    char pad0[8];
    unsigned short size;                /* +8 */
} QUEUE;

/* the language's table of accessors (language.c's _LangVirtualTable, copied to its head) */
typedef struct LANG {
    void *pad0[6];
    int (*GetNbrPhenome)(struct LANG *l);                           /* 6 */
    void *pad7[4];
    unsigned short (*GetNbrErgodicPhenomes)(struct LANG *l);        /* 11 */
    void *pad12[13];
    int (*GetNbrPhenUserWordTraining)(struct LANG *l);              /* 25 */
    void *pad26[13];
    short *(*GetpErgodicPhenomes)(struct LANG *l);                  /* 39 */
} LANG;

/* a back pointer */
typedef struct BTNODE {
    struct BTNODE *next;                /* +0: the free list's link, or the node before */
    short f4;                           /* +4 */
    short count;                        /* +6 */
    int f8;                             /* +8 */
} BTNODE;

/* a phoneme cell */
typedef struct GENCELL {
    unsigned short phen;                /* +0 */
    short score;                        /* +2 */
    BTNODE *bt;                         /* +4 */
} GENCELL;

/* command 23's argument */
typedef struct GENUWSETUP {
    unsigned short nwords;              /* +0 */
    short pad2;
    unsigned short *words;              /* +4: the cells' phonemes */
    unsigned short garbage;             /* +8 */
} GENUWSETUP;

/* commands 26 and 27's argument */
typedef struct GENUWPARAMS {
    int pad0[2];
    int f8;                             /* +8: the block's +52 (profile 35) */
    int f12;                            /* +12: the block's +54 (profile 20) */
} GENUWPARAMS;

typedef struct DPGENUW {
    TOSBLOCK b;
    QIN *in;                            /* +40 */
    QOUT *out;                          /* +44 */
    short f48;                          /* +48 */
    unsigned short f50;                 /* +50: the phonemes, as command 23 found them */
    short f52;                          /* +52: profile 35 */
    short f54;                          /* +54: profile 20 */
    GENCELL *cells;                     /* +56 */
    BTNODE **chunks;                    /* +60 */
    BTNODE *freelist;                   /* +64 */
    unsigned int f68;                   /* +68: profile 2, tenths */
    int f72;                            /* +72 */
    unsigned short nergodic;            /* +76 */
    short *ergodic;                     /* +80 */
    short f84;                          /* +84: -nergodic */
    int nphen;                          /* +88 */
    int ntraining;                      /* +92 */
    unsigned short nwords;              /* +96 */
    unsigned short *words;              /* +100 */
    unsigned short garbage;             /* +104 */
    int f108;                           /* +108 */
    unsigned int f112;                  /* +112 */
    int pad116;
    short f120;                         /* +120 */
    short pad122[3];
    unsigned char f128;                 /* +128 */
    unsigned int frame;                 /* +132 */
    unsigned int maxframes;             /* +136: profile 11, hundreds */
    unsigned short (*makeactive)(struct DPGENUW *e, unsigned short *dist,
                                 unsigned char *active);    /* +140 */
    void *pad144;
    void (*sendresult)(struct DPGENUW *e, void *arg, void *arg2);       /* +148 */
    int pad152[20];
    int f232;                           /* +232 */
    int pad236;
    unsigned short nchunks;             /* +240 */
    unsigned short chunksize;           /* +242: bytes (profile 17) */
} DPGENUW;

int _tosControl(void *b, unsigned char cmd, void *arg, int arg2);
void qQueueReset(void *q);

/* ctxutil.c */
unsigned short MakeUserWordsActiveDist(DPGENUW *e, unsigned short *dist, unsigned char *active);

static BTNODE *AllocateBacktrace(DPGENUW *e, BTNODE *prev);
static void InitViterbi(DPGENUW *e);
static int InitDpGenUw(DPGENUW *e);
static void SendResultUserWords(DPGENUW *e, void *arg, void *arg2);
static int DynProgUserWords(DPGENUW *e, short *frame);
static void DpGenUwProcess(DPGENUW *e, short **in);
static __inline__ void FreeBacktrace(TOS *tos, DPGENUW *e);
static int func_004E5538(DPGENUW *e, GENUWSETUP *s);
static int ControlDpGenUw(DPGENUW *e, unsigned char cmd, void *arg, void *arg2);
static void IdleDpGenUwProcess(DPGENUW *e, short **in);

static BTNODE *AllocateBacktrace(DPGENUW *e, BTNODE *prev)
{
    TOS *tos = e->b.tos;
    unsigned short n = e->nchunks;
    BTNODE **chunks;
    BTNODE *chunk;
    unsigned int i, per;

    chunks = heap_Alloc(tos->heap, n * sizeof(BTNODE *) + sizeof(BTNODE *));
    if (chunks == 0) {
        _tosErrorLog(e, 2);
        return 0;
    }
    if (n != 0) {
        memcpy(chunks, e->chunks, n * sizeof(BTNODE *));
        heap_Free(tos->heap, e->chunks);
    }
    e->chunks = chunks;
    chunk = heap_Alloc(tos->heap, e->chunksize / sizeof(BTNODE) * sizeof(BTNODE));
    if (chunk == 0) {
        _tosErrorLog(e, 2);
        return 0;
    }
    if (prev != 0) {
        per = e->chunksize / sizeof(BTNODE) - 1;
        prev->next = chunk;
        for (i = 0; i < per; i++)
            chunk[i].next = &chunk[i + 1];
        chunk[i].next = 0;
    }
    e->chunks[n++] = chunk;
    e->nchunks = n;
    return chunk;
}

/* the backtrace chunks chained into one free list (each chunk's nodes walked by the chunk pointer
 * itself, the list's last node where the walk stops), the root node taken off it, the twenty word
 * cells and the two garbage cells started from the root. Byte-exact since GCC round 75: the walk
 * steps `chunk` (indexing gives a copy of it, `move a2,t1`), the chunk counter is not zeroed ahead
 * of the loop (with `i = 0` above CSE shares one zero for i and `last`, and the entry test reads it:
 * `sltu` where the target tests the count `beqz`), and f108 is cleared before f232. OPEN from round
 * 9c to 74 as "the cells' loop unrolled by five and the statement order not measured" -- neither:
 * the cells' loop matched as written. */
static void InitViterbi(DPGENUW *e)
{
    BTNODE *last;
    BTNODE *chunk, *root;
    GENCELL *c;
    unsigned short i;
    unsigned int j, k;
    unsigned int per;

    e->frame = -1;
    e->f48 = 0;
    e->f72 = 0;
    last = 0;
    e->f120 = 0;
    per = e->chunksize / sizeof(BTNODE) - 1;
    for (i = 0; i < e->nchunks; i++) {
        chunk = e->chunks[i];
        if (last != 0)
            last->next = chunk;
        for (j = 0; j < per; j++) {
            chunk->next = chunk + 1;
            chunk++;
        }
        last = chunk;
    }
    if (last != 0)
        last->next = 0;
    root = e->chunks[0];
    root->f4 = -1;
    root->count = 0;
    e->freelist = root->next;
    root->next = 0;
    root->f4 = 0;
    root->f8 = 0;
    c = e->cells;
    for (k = 0; k < NWORDCELLS; k++) {
        c->score = WORSTSCORE;
        c->phen = e->words[k];
        c->bt = root;
        c++;
    }
    c->score = 0;
    c->phen = e->garbage;
    c->bt = root;
    c++;
    c->score = WORSTSCORE;
    c->phen = e->garbage;
    c->bt = root;
    root->count = NWORDCELLS + 2;
    e->f108 = 0;
    e->f232 = 0;
}

static int InitDpGenUw(DPGENUW *e)
{
    unsigned short n;

    e->f68 = (unsigned short)((int)_tosGetProfileU32(e, 2, 1) / 10);
    n = _tosGetProfileU32(e, 19, 1);
    e->in = e->b.inputs;
    e->out = e->b.outputs;
    e->in->size = n * sizeof(short);
    e->out->size = n * (sizeof(short) + sizeof(char));
    e->chunksize = _tosGetProfileU32(e, 17, 1500);
    if (e->chunksize > 65000)
        e->chunksize = 65000;
    else if (e->chunksize < 200)
        e->chunksize = 200;
    e->maxframes = (unsigned short)_tosGetProfileU32(e, 11, 0) * 100;
    e->f54 = _tosGetProfileU32(e, 20, 120);
    e->f52 = _tosGetProfileU32(e, 35, 900);
    e->nchunks = 0;
    return 0;
}

/* the result the scheduler takes (command 12): the score, the frames spoken outside the garbage
 * phoneme, the words' labels */
typedef struct UWRESULT {
    float score;                        /* +0 */
    int frames;                         /* +4 */
    int n;                              /* +8 */
    unsigned short labels[1];           /* +12: n, and one more */
} UWRESULT;

/* the recognised labels off the garbage phoneme's last cell's backtrace (it runs from the end:
 * stored from the last slot down) */
static void SendResultUserWords(DPGENUW *e, void *arg, void *arg2)
{
    TOS *tos = e->b.tos;
    BTNODE *bt;
    UWRESULT *r;
    unsigned short n = 0;
    int frames = 0;
    int start;
    int i;

    bt = e->cells[NWORDCELLS + 1].bt;
    start = bt->f8;
    for (; bt->next != 0; bt = bt->next) {
        if ((unsigned short)bt->f4 == e->garbage) {
            frames += start - bt->f8;
            start = 0;
        } else if (start == 0)
            start = bt->f8;
        n++;
    }
    if (n != 0)
        n--;
    r = heap_Alloc(tos->heap, n * 2 + 14);
    if (r == 0) {
        _tosErrorLog(e, 1);
        return;
    }
    r->frames = frames;
    r->n = n;
    if (e->frame != 0)
        r->score = (float)(e->f112 + e->f108) / (float)e->frame;
    else
        r->score = 0;
    bt = e->cells[NWORDCELLS + 1].bt;
    for (i = n - 1; i >= 0; i--) {
        r->labels[i] = bt->f4;
        bt = bt->next;
    }
    if (_tosControl(e, 12, r, 1) != 0)
        _tosErrorLog(e, 1);
    heap_Free(tos->heap, r);
}

/* a back pointer for phen off the free list (a chunk more when the list runs dry), after prev,
 * into res; the caller's `node` is the macro's working pointer, one variable for every expansion
 * (the target's s0 in both). res is set inside the do-while: after its LOOP_END note the first
 * scheduling pass treats an insn as a barrier, so a `res = node` written after the macro stays
 * below the stores, where the target's copy rises to just after `node->next = prev` */
#define NEWBACKTRACE(e, res, prev, phen)                \
    do {                                                \
        node = (e)->freelist;                           \
        if (node->next == 0)                            \
            (e)->freelist = AllocateBacktrace((e), node); \
        else                                            \
            (e)->freelist = node->next;                 \
        node->next = (prev);                            \
        (prev)->count++;                                \
        node->f4 = (phen);                              \
        node->count = 0;                                \
        node->f8 = (e)->frame;                          \
        (e)->f232++;                                    \
        (res) = node;                                   \
    } while (0)

/* a back pointer let go: when nothing points at it any more, it and the nodes before it that only
 * it held go back to the free list */
static __inline__ void ReleaseBacktrace(DPGENUW *e, BTNODE *bt)
{
    BTNODE *p, *n;

    if (--bt->count <= 0) {
        p = bt;
        n = bt->next;
        e->f232--;
        while (n->count < 2) {
            p = n;
            n = p->next;
            e->f232--;
        }
        n->count--;
        p->next = e->freelist;
        e->freelist = bt;
    }
}

/* the word cell with the best score and the one with the worst (the last of equals) */
static __inline__ GENCELL *BestWordCell(GENCELL *cells)
{
    GENCELL *best = cells, *c = cells + 1;
    unsigned short i;

    for (i = 1; i < NWORDCELLS; i++, c++)
        if (c->score < best->score)
            best = c;
    return best;
}

static __inline__ GENCELL *WorstWordCell(GENCELL *cells)
{
    GENCELL *worst = cells, *c = cells + 1;
    unsigned short i;

    for (i = 1; i < NWORDCELLS; i++, c++)
        if (c->score >= worst->score)
            worst = c;
    return worst;
}

/* one frame of the phoneme loop: every cell's score less the best's, a cell entered from the
 * garbage phoneme or from the best word cell when that is cheaper than staying, the garbage
 * phoneme's second cell entered from the words; then each of the word's phonemes tried in the worst
 * word cell. Byte-exact (round 17c; open since round 16c at 0x60C) by four shapes, each read off
 * cc1's dumps (tools/ccdump.py --dump "-dG -dS"): (1) the garbage penalty is the field read in
 * both cells, `e->f52` -- GCSE's PRE then holds it as a halfword pseudo across the first cell's
 * if/else and combine folds only the first use into `lh`, the target's `lh` and later `lhu` + sign
 * extension (a short local is promoted and loaded once: 0x60C); (2) the eq arm sets node as it
 * shares the garbage node (`wnode = node = gnode`): with node set in that block GCSE's copy
 * propagation cannot turn `wnode = gnode` into `wnode = node` ("COPY-PROP: Replacing reg 91 in
 * insn 434 with reg 94", then wnode replaced by node after the join: wnode merged into the node,
 * the frame pointer's register order shifted); (3) the macro sets its result inside its
 * do-while (above); (4) the clamp loop has a pointer of its own (`p`, the target's a0: sharing `c`
 * with the later cells gives one pseudo, a3) and the words loop steps `w` itself (`w++`, `*w`: an
 * index makes the giv a copy of w, `lw v0; move t1,v0`). */
static int DynProgUserWords(DPGENUW *e, short *frame)
{
    GENCELL *p, *cells, *best, *from, *c, *worst;
    BTNODE *gnode, *wnode, *old, *node, *prev;
    unsigned short *w;
    unsigned short phen, bphen;
    int min, enter, wordexit, gscore, s;
    unsigned short i;
    unsigned int j;

    p = e->cells;
    for (i = 1; i < NWORDCELLS + 2; i++) {
        if (p->score > 31622)
            p->score = 31622;
        p++;
    }
    cells = e->cells;
    best = BestWordCell(cells);
    min = best->score;
    wordexit = min + e->f54;
    c = &cells[NWORDCELLS];
    enter = c->score + e->f52;
    if (c->score < min)
        min = c->score;
    bphen = best->phen;
    if (enter < wordexit) {
        phen = c->phen;
        from = c;
    } else {
        phen = bphen;
        enter = wordexit;
        from = best;
    }
    gscore = frame[c->phen];
    c++;
    if (c->score < min)
        min = c->score;
    if (c->score + e->f52 < enter) {
        phen = c->phen;
        enter = c->score + e->f52;
        from = c;
    }
    prev = from->bt;
    NEWBACKTRACE(e, gnode, prev, phen);
    gnode->count++;
    if (bphen != phen) {
        prev = best->bt;
        NEWBACKTRACE(e, wnode, prev, bphen);
        wnode->count++;
    } else {
        wnode = node = gnode;
        wnode->count++;
    }

    c = e->cells;
    for (j = 0; j < NWORDCELLS; j++) {
        if (c->score < enter)
            c->score = c->score + frame[c->phen] - min;
        else {
            old = c->bt;
            c->score = frame[c->phen] + enter - min;
            c->bt = gnode;
            gnode->count++;
            ReleaseBacktrace(e, old);
        }
        frame[c->phen] = 1024;
        c++;
    }
    c->score = c->score + gscore - min;
    c++;
    if (c->score <= wordexit)
        c->score = c->score + gscore - min;
    else {
        old = c->bt;
        c->score = wordexit + gscore - min;
        c->bt = wnode;
        wnode->count++;
        ReleaseBacktrace(e, old);
    }
    e->f112 = (unsigned short)c->score;

    worst = WorstWordCell(e->cells);
    w = e->words;
    for (j = 0; j < e->nwords; j++, w++) {
        s = enter + frame[*w] - min;
        if (s < worst->score) {
            worst->phen = *w;
            worst->score = s;
            old = worst->bt;
            worst->bt = gnode;
            gnode->count++;
            ReleaseBacktrace(e, old);
            worst = WorstWordCell(e->cells);
        }
    }
    ReleaseBacktrace(e, gnode);
    ReleaseBacktrace(e, wnode);
    e->f108 += (unsigned short)min;
    return 0;
}

static void DpGenUwProcess(DPGENUW *e, short **in)
{
    short *frame = in[0];
    short *p;
    char *q;
    int i;

    if (frame != 0) {
        p = frame;
        for (i = 0; i < e->nergodic; i++) {
            if (i != e->garbage)
                *p = SILENT;
            p++;
        }
        if (++e->frame == 0)
            e->f120 = e->cells[NWORDCELLS].score = frame[e->garbage];
        else if (DynProgUserWords(e, frame) != 0) {
            _tosErrorLog(e, 0x77);
            e->b.process = (TOSFUNC)IdleDpGenUwProcess;
            _tosControl(e, 11, 0, 0);
        }
        q = qEnQueueOne(e->out->q);
        e->makeactive(e, (unsigned short *)q, (unsigned char *)(q + e->in->size - 2));
        if (e->maxframes != 0 && e->maxframes < e->frame) {
            e->b.process = (TOSFUNC)IdleDpGenUwProcess;
            if (_tosControl(e, 8, (void *)e->frame, 0) != 0)
                _tosErrorLog(e, 1);
        }
    }
}

/* the back pointers' chunks and the cells freed (no name of the demo's: inlined there too) */
static __inline__ void FreeBacktrace(TOS *tos, DPGENUW *e)
{
    unsigned int i;

    for (i = 0; i < e->nchunks; i++)
        heap_Free(tos->heap, e->chunks[i]);
    if (e->chunks != 0)
        heap_Free(tos->heap, e->chunks);
    e->chunks = 0;
    e->nchunks = 0;
    if (e->cells != 0)
        heap_Free(tos->heap, e->cells);
    e->cells = 0;
}

/* command 23: the word's phonemes (SOCOM II's own function, where SOCOM 1's ControlDpGenUw did
 * this in place; the DpScoreUw object's InitScoreUserWords sits at the same place) */
static int func_004E5538(DPGENUW *e, GENUWSETUP *s)
{
    TOS *tos = e->b.tos;
    QIN *in = e->in;

    e->nwords = s->nwords;
    e->words = s->words;
    e->garbage = s->garbage;
    in->size = ((QUEUE *)in->q)->size;
    e->f50 = e->nphen;
    FreeBacktrace(e->b.tos, e);
    if (AllocateBacktrace(e, 0) == 0
        || (e->cells = heap_Alloc(tos->heap, (NWORDCELLS + 2) * sizeof(GENCELL))) == 0)
        return _tosErrorLog(e, 113);
    e->f128 = 0;
    InitViterbi(e);
    return 0;
}

static int ControlDpGenUw(DPGENUW *e, unsigned char cmd, void *arg, void *arg2)
{
    char *q;

    switch (cmd) {
    case 4:
        e->b.process = (TOSFUNC)IdleDpGenUwProcess;
        e->sendresult(e, arg, arg2);
        break;
    case 3:
        InitViterbi(e);
        qQueueReset(e->out->q);
        q = qEnQueueOne(e->out->q);
        e->makeactive(e, (unsigned short *)q, (unsigned char *)(q + e->in->size));
        e->b.process = (TOSFUNC)DpGenUwProcess;
        break;
    case 5:
        e->b.process = (TOSFUNC)IdleDpGenUwProcess;
        break;
    case 34:
        e->nergodic = ((LANG *)arg)->GetNbrErgodicPhenomes(arg);
        e->ergodic = ((LANG *)arg)->GetpErgodicPhenomes(arg);
        e->f84 = -e->nergodic;
        e->nphen = ((LANG *)arg)->GetNbrPhenome(arg);
        e->ntraining = ((LANG *)arg)->GetNbrPhenUserWordTraining(arg);
        if (e->out->q != 0)
            e->out->size = ((QUEUE *)e->out->q)->size;
        break;
    case 23:
        if (func_004E5538(e, arg) != 0)
            return 0;
        break;
    case TOS_DESTROY:
        FreeBacktrace(e->b.tos, e);
        tosBaseBlockDestruct(&e->b);
        break;
    case 26:
        e->f54 = ((GENUWPARAMS *)arg)->f12;
        e->f52 = ((GENUWPARAMS *)arg)->f8;
        break;
    case 27:
        ((GENUWPARAMS *)arg)->f12 = e->f54;
        ((GENUWPARAMS *)arg)->f8 = e->f52;
        break;
    case 21:
        break;
    default:
        return 0;
    }
    return 1;
}

TOSBLOCK *ConstructDpGenUw(TOS *tos, unsigned char id)
{
    DPGENUW *e = (DPGENUW *)tosBaseBlockConstruct(tos, id, 1, 1, (TOSFUNC)IdleDpGenUwProcess,
                                                  (TOSFUNC)InitDpGenUw, (TOSFUNC)ControlDpGenUw,
                                                  sizeof(DPGENUW));

    e->makeactive = MakeUserWordsActiveDist;
    e->sendresult = SendResultUserWords;
    return &e->b;
}

static void IdleDpGenUwProcess(DPGENUW *e, short **in)
{
}
