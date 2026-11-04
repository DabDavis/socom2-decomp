/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The recogniser's user-word scoring block (OVL2 0x4E5960-0x4E6C20, SOCOM 1's object at 0x3539F0:
 * the same eleven functions in the same order, and one more at the end, func_004E6BC0, that
 * nothing calls), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM 1's (s1:align,
 * the statics s1:order). The block scores a list of user words, each a single phoneme model (the
 * cells), against the phoneme scores of each input frame, with the language's garbage phoneme
 * around them when command 4 asks for it: a Viterbi pass whose back pointers are nodes taken from
 * a free list of chunks (AllocateBacktrace adds one when the list runs dry). Command 4 sets the
 * words up, 7 hands it the language, 1 starts, 2 sends the result (_tosControl 12), 3 stops.
 * DynProgScoreUserWords (0x4E6000, the pass itself) closed in round 21c. */
#include "tosblk.h"

#define WORSTSCORE      0x77FF          /* a cell not reached */

/* a queue as its readers see it (queue.c): the bytes of a frame at +8 */
typedef struct QUEUE {
    char pad0[8];
    unsigned short size;                /* +8 */
} QUEUE;

/* the language's table of accessors (language.c's _LangVirtualTable, copied to its head) */
typedef struct LANG {
    void *pad0[6];
    unsigned short (*GetNbrPhenome)(struct LANG *l);                /* 6 */
    void *pad7[4];
    unsigned short (*GetNbrErgodicPhenomes)(struct LANG *l);        /* 11 */
    void *pad12[4];
    unsigned short (*GetSingleWordGarbagePhenome)(struct LANG *l);  /* 16 */
    void *pad17[22];
    short *(*GetpErgodicPhenomes)(struct LANG *l);                  /* 39 */
} LANG;

/* a back pointer: the word ended, when, with which score, and the node before it */
typedef struct BTNODE {
    int count;                          /* +0: the nodes pointing at it */
    struct BTNODE *next;                /* +4: the free list's link, or the node before */
    int score;                          /* +8 */
    int score2;                         /* +12 */
    unsigned short word;                /* +16 */
    short pad18;
    int frame;                          /* +20 */
} BTNODE;

/* a model in the pass */
typedef struct WORDCELL {
    short score;                        /* +0 */
    short pad2;
    BTNODE *bt;                         /* +4 */
    unsigned short phen;                /* +8 */
    short pad10;
} WORDCELL;

/* command 4's argument */
typedef struct SCOREUWSETUP {
    unsigned short nwords;              /* +0 */
    short pad2;
    unsigned short ngarbage;            /* +4: 0 or 1 */
    short pad6;
    unsigned short *words;              /* +8: each word's phoneme */
    unsigned short *garbage;            /* +12 */
} SCOREUWSETUP;

/* what command 2 sends (_tosControl 12) */
typedef struct SCOREUWRESULT {
    int score;                          /* +0 */
    unsigned int wordscore;             /* +4 */
    unsigned int nframes;               /* +8 */
    float perframe;                     /* +12 */
    int garbagerate;                    /* +16 */
    unsigned int start;                 /* +20 */
    unsigned int end;                   /* +24 */
    unsigned int nnodes;                /* +28 */
    BTNODE *bt;                         /* +32 */
} SCOREUWRESULT;

typedef struct DPSCOREUW {
    TOSBLOCK b;
    QIN *in;                            /* +40 */
    QOUT *out;                          /* +44 */
    WORDCELL *cells;                    /* +48 */
    unsigned short chunksize;           /* +52: bytes (profile 17) */
    unsigned int nchunks;               /* +56 */
    BTNODE **chunks;                    /* +60 */
    BTNODE *freelist;                   /* +64 */
    unsigned short nwords;              /* +68 */
    unsigned short ncells;              /* +70 */
    unsigned short ngarbage;            /* +72 */
    unsigned short nphen;               /* +74 */
    unsigned short nergodic;            /* +76 */
    short *ergodic;                     /* +80 */
    unsigned short *words;              /* +84 */
    unsigned short garbage;             /* +88 */
    unsigned short swgarbage;           /* +90 */
    int base;                           /* +92 */
    int base2;                          /* +96 */
    short best;                         /* +100 */
    short first;                        /* +102 */
    unsigned char flag;                 /* +104: commands 5 and 6 */
    unsigned char flag105;              /* +105 */
    int frame;                          /* +108 */
    unsigned int maxframes;             /* +112 */
    unsigned short (*makeactive)(struct DPSCOREUW *e, unsigned short *dist,
                                 unsigned char *active);    /* +116 */
    void *pad120;
    void (*sendresult)(struct DPSCOREUW *e, void *arg, void *arg2);     /* +124 */
} DPSCOREUW;

int _tosControl(void *b, unsigned char cmd, void *arg, int arg2);
void qQueueReset(void *q);

int DynProgScoreUserWords(DPSCOREUW *e, short *frame);

static BTNODE *AllocateBacktrace(DPSCOREUW *e, BTNODE *prev);
static void InitViterbi(DPSCOREUW *e);
static void SendResultScoreUserWords(DPSCOREUW *e, void *arg, void *arg2);
static void DpScoreUwProcess(DPSCOREUW *e, short **in);
static __inline__ unsigned char CheckAlloc(void *b, void *p, unsigned short code);
static int InitScoreUserWords(DPSCOREUW *e, SCOREUWSETUP *s);
static int ControlDpScoreUw(DPSCOREUW *e, unsigned char cmd, void *arg, void *arg2);
static void IdleDpScoreUwProcess(DPSCOREUW *e, short **in);
static int InitDpScoreUw(DPSCOREUW *e);
static unsigned short MakeScoreUserWordsActiveDist(DPSCOREUW *e, unsigned short *dist,
                                                   unsigned char *active);

static BTNODE *AllocateBacktrace(DPSCOREUW *e, BTNODE *prev)
{
    TOS *tos = e->b.tos;
    unsigned int n = e->nchunks;
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

static void InitViterbi(DPSCOREUW *e)
{
    BTNODE *last;
    BTNODE *chunk, *root;
    WORDCELL *w;
    unsigned int i, j, k;
    unsigned int n;
    unsigned int per;

    e->frame = -1;
    last = 0;
    n = 0;
    e->first = 0;
    e->base = 0;
    e->base2 = 0;
    per = e->chunksize / sizeof(BTNODE) - 1;
    for (i = 0; i < e->nchunks; i++) {
        chunk = e->chunks[i];
        if (last != 0)
            last->next = chunk;
        for (j = 0; j < per; j++)
            chunk[j].next = &chunk[j + 1];
        last = &chunk[j];
    }
    if (last != 0)
        last->next = 0;
    root = e->chunks[0];
    root->count = 0;
    root->word = 0;
    root->score = 0;
    root->frame = 0;
    e->freelist = root->next;
    root->next = 0;
    root->word = 0;
    root->count = 0;
    w = e->cells;
    for (k = 0; k < e->ngarbage; k++) {
        w->phen = e->garbage;
        if (k == 0)
            w->score = 0;
        else
            w->score = WORSTSCORE;
        w->bt = root;
        w++;
        n++;
    }
    for (k = 0; k < e->nwords; k++) {
        w->phen = e->words[k];
        w->score = WORSTSCORE;
        w->bt = root;
        w++;
        n++;
    }
    for (k = 0; k < e->ngarbage; k++) {
        w->phen = e->garbage;
        w->score = WORSTSCORE;
        w->bt = root;
        w++;
        n++;
    }
    if (e->ngarbage == 0)
        e->cells->score = 0;
    root->count = n;
}

static void SendResultScoreUserWords(DPSCOREUW *e, void *arg, void *arg2)
{
    WORDCELL *last = &e->cells[e->ncells - 1];
    BTNODE *bt = last->bt;
    BTNODE *node, *free, *p, *q, *next;
    SCOREUWRESULT r;
    short best;
    int sum, cnt, frame;
    unsigned int score, start, end, nframes, nnodes;
    unsigned char ingarbage;

    nnodes = 0;
    if (e->ngarbage == 0) {
        node = e->freelist;
        free = node->next;
        best = e->best;
        if (free == 0)
            free = AllocateBacktrace(e, node);
        e->freelist = free;
        node->next = bt;
        bt->count++;
        node->word = last->phen;
        node->count = 0;
        frame = e->frame;
        node->frame = frame;
        node->score = best + e->base;
        node->score2 = e->base2;
        bt = node;
        bt->frame = frame + 1;
    }
    p = bt;
    sum = 0;
    cnt = 0;
    if (e->ngarbage == 0) {
        start = 0;
        end = e->frame + 1;
        score = e->base + e->best;
    } else {
        next = bt->next;
        q = bt;
        ingarbage = 1;
        end = bt->frame;
        nnodes = 1;
        score = bt->score;
        while (next != 0) {
            if (p->word == e->garbage) {
                if (!ingarbage) {
                    ingarbage = 1;
                    sum -= p->score - p->score2;
                    cnt -= p->frame;
                }
            } else if (ingarbage) {
                ingarbage = 0;
                sum += p->score - p->score2;
                cnt += p->frame;
            }
            q = p;
            p = next;
            next = next->next;
            nnodes++;
        }
        start = q->frame;
        score -= q->score;
    }
    nframes = end - start;
    r.wordscore = score;
    r.nframes = nframes;
    if (nframes != 0)
        r.perframe = (float)score / (float)nframes;
    else
        r.perframe = 0;
    r.start = start;
    r.end = end;
    r.score = e->base + e->best;
    r.nnodes = nnodes;
    r.bt = bt;
    if (cnt == 0)
        r.garbagerate = 100;
    else
        r.garbagerate = sum / cnt;
    if (_tosControl(e, 12, &r, 2) != 0)
        _tosErrorLog(e, 1);
}

/* new in SOCOM II: walks each cell's back pointers to the last node and keeps nothing (a check
 * whose report is compiled out). A public inline: DynProgScoreUserWords holds its body, and 2.96
 * defers its own copy to the object's end, after MakeScoreUserWordsActiveDist (0x4E6BC0) */
__inline__ void func_004E6BC0(DPSCOREUW *e)
{
    WORDCELL *c = e->cells;
    BTNODE *p;
    unsigned short i;

    for (i = 0; i < e->ncells; i++) {
        p = c->bt;
        while (p->next != 0)
            p = p->next;
        c++;
    }
}

/* a back pointer let go: when nothing points at it any more, it and the nodes before it that only
 * it held go back to the free list */
#define RELEASE(e, b)                                   \
    if (--(b)->count <= 0) {                            \
        BTNODE *bt_ = (b), *p_ = bt_, *n_ = bt_->next;  \
        while (n_->count < 2) {                         \
            p_ = n_;                                    \
            n_ = p_->next;                              \
        }                                               \
        n_->count--;                                    \
        p_->next = (e)->freelist;                       \
        (e)->freelist = bt_;                            \
    }

/* one frame of the pass: the scores made relative to the last frame's best and clamped, then each
 * cell entered from the one before it when that is cheaper than staying (a new back pointer), or
 * from the garbage phoneme's last cell when command 5 allows it; the best cell's score and the
 * frame's best kept. SOCOM 1's body (0x458), with two loops at the end that SOCOM II added (0x4DC):
 * the back pointer check (func_004E6BC0, inlined) and an empty walk over the cells. Byte-exact
 * (round 21c) by three shapes, read off cc1's dumps (tools/ccdump.py -f; global-alloc's priority
 * is floor_log2(refs) * refs / live length, build/scratch/gcc/prio.py): (1) the previous cell is
 * copied at the first cell's top (`pc = c`) and the empty walk at the end steps `c` itself -- c's
 * last use is then after pc's, so CSE keeps c as the pair's canonical register (make_regs_eqv
 * prefers the one that lives longer: with pc outliving c the first cell's uses became pc's and
 * `c++` came out `addiu s1,s3,12`); pc, live from the first cell, then ranks below min and takes
 * the caller-saved a2 (`sq` round the allocation), min the last saved register, s8 -- the target's;
 * loop.c deletes the walk's dead stepping; (2) the first cell's addend is the int variable `f`
 * (the target's `lh`: `c->score += frame[...]` narrows the sum to a halfword add, `lhu`);
 * (3) RELEASE is a macro on its expression: the cell's `c->bt` read again after the decrement, as
 * the target's (-fno-strict-aliasing) */
int DynProgScoreUserWords(DPSCOREUW *e, short *frame)
{
    WORDCELL *c, *pc;
    BTNODE *bt, *prevbt, *gbt, *node;
    int min, score, prevscore, gscore, f;
    short first;
    unsigned int i;

    min = 32767;
    first = e->first;
    e->base += first;
    e->base2 += frame[e->swgarbage];
    c = e->cells;
    for (i = 0; i < e->ncells; i++) {
        c->score -= first;
        if (c->score > 30719)
            c->score = 30719;
        c++;
    }
    c = e->cells;
    pc = c;
    score = c->score;
    bt = c->bt;
    f = frame[c->phen];
    c->score += f;
    if (c->score < min)
        min = c->score;
    bt->count++;
    gscore = c->score;
    gbt = c->bt;
    c++;
    for (i = 1; i < e->ncells; i++) {
        prevbt = bt;
        bt = c->bt;
        prevscore = score;
        score = c->score;
        bt->count++;
        f = frame[c->phen];
        if (e->flag != 0 && c->phen == e->garbage && gscore < prevscore && gscore < score) {
            c->score = gscore + f;
            if (c->score < min)
                min = c->score;
            RELEASE(e, c->bt);
            c->bt = gbt;
            gbt->count++;
        } else if (prevscore < score) {
            c->score = prevscore + f;
            if (c->score < min)
                min = c->score;
            node = e->freelist;
            if (node->next == 0)
                e->freelist = AllocateBacktrace(e, node);
            else
                e->freelist = node->next;
            node->next = prevbt;
            prevbt->count++;
            node->word = pc->phen;
            node->count = 0;
            node->frame = e->frame;
            node->score = prevscore + e->base;
            node->score2 = e->base2;
            node->count++;
            RELEASE(e, c->bt);
            c->bt = node;
        } else {
            c->score += f;
            if (c->score < min)
                min = c->score;
        }
        if (c->phen == e->garbage) {
            gbt = c->bt;
            gscore = score;
        }
        RELEASE(e, prevbt);
        pc = c;
        c++;
    }
    RELEASE(e, bt);
    e->first = min;
    e->best = pc->score;
    func_004E6BC0(e);
    for (i = 0, c = e->cells; i < e->ncells; i++, c++)
        ;
    return 0;
}

static void DpScoreUwProcess(DPSCOREUW *e, short **in)
{
    short *frame = in[0];
    char *q;

    if (frame != 0) {
        if (++e->frame == 0)
            e->first = e->cells->score = frame[e->cells->phen];
        else if (DynProgScoreUserWords(e, frame) != 0) {
            _tosErrorLog(e, 0x77);
            e->b.process = (TOSFUNC)IdleDpScoreUwProcess;
            if (_tosControl(e, 11, 0, 0) != 0) {
                e->b.process = (TOSFUNC)IdleDpScoreUwProcess;
                return;
            }
        }
        q = qEnQueueOne(e->out->q);
        e->makeactive(e, (unsigned short *)q, (unsigned char *)(q + e->in->size - 2));
        if (e->maxframes != 0 && e->maxframes < e->frame) {
            e->b.process = (TOSFUNC)IdleDpScoreUwProcess;
            if (_tosControl(e, 8, (void *)e->frame, 0) != 0)
                _tosErrorLog(e, 1);
        }
    }
}

/* an allocation's check: its error logged, or 0 (no name of the demo's: inlined there too; the
 * target tests the inline's unsigned char result, `andi` at the join of both arms) */
static __inline__ unsigned char CheckAlloc(void *b, void *p, unsigned short code)
{
    if (p == 0)
        return _tosErrorLog(b, code);
    return 0;
}

static int InitScoreUserWords(DPSCOREUW *e, SCOREUWSETUP *s)
{
    TOS *tos;
    unsigned int i;
    unsigned short *garbage;

    e->nwords = s->nwords;
    e->words = s->words;
    e->ngarbage = s->ngarbage;
    garbage = s->garbage;
    if (e->ngarbage >= 2) {
        _tosErrorLog(e, 1);
        return 1;
    }
    e->first = 0;
    e->frame = -1;
    e->base = 0;
    e->base2 = 0;
    if (e->ngarbage != 0) {
        e->garbage = *garbage;
        e->ncells = e->nwords + 2;
    } else {
        e->ncells = e->nwords;
        e->garbage = 0;
    }
    tos = e->b.tos;
    for (i = 0; i < e->nchunks; i++)
        heap_Free(tos->heap, e->chunks[i]);
    if (e->chunks != 0)
        heap_Free(tos->heap, e->chunks);
    e->chunks = 0;
    e->nchunks = 0;
    if (e->cells != 0)
        heap_Free(tos->heap, e->cells);
    e->cells = 0;
    if (AllocateBacktrace(e, 0) == 0)
        return 1;
    e->cells = heap_Alloc(e->b.tos->heap, e->ncells * sizeof(WORDCELL));
    if (CheckAlloc(e, e->cells, 113) != 0)
        return 1;
    e->flag105 = 0;
    e->in->size = ((QUEUE *)e->in->q)->size;
    e->out->size = ((QUEUE *)e->out->q)->size;
    return 0;
}

static int ControlDpScoreUw(DPSCOREUW *e, unsigned char cmd, void *arg, void *arg2)
{
    TOS *tos;
    unsigned int i;
    char *q;

    switch (cmd) {
    case 2:
        e->b.process = (TOSFUNC)IdleDpScoreUwProcess;
        e->sendresult(e, arg, arg2);
        break;
    case 1:
        InitViterbi(e);
        qQueueReset(e->out->q);
        q = qEnQueueOne(e->out->q);
        e->makeactive(e, (unsigned short *)q, (unsigned char *)(q + e->in->size));
        e->b.process = (TOSFUNC)DpScoreUwProcess;
        break;
    case 3:
        e->b.process = (TOSFUNC)IdleDpScoreUwProcess;
        break;
    case 7:
        e->nphen = ((LANG *)arg)->GetNbrPhenome(arg);
        e->nergodic = ((LANG *)arg)->GetNbrErgodicPhenomes(arg);
        e->ergodic = ((LANG *)arg)->GetpErgodicPhenomes(arg);
        e->swgarbage = ((LANG *)arg)->GetSingleWordGarbagePhenome(arg);
        if (e->out->q != 0)
            e->out->size = ((QUEUE *)e->out->q)->size;
        break;
    case 4:
        if (InitScoreUserWords(e, arg) != 0)
            return 0;
        break;
    case TOS_DESTROY:
        tos = e->b.tos;
        for (i = 0; i < e->nchunks; i++)
            heap_Free(tos->heap, e->chunks[i]);
        if (e->chunks != 0)
            heap_Free(tos->heap, e->chunks);
        e->chunks = 0;
        e->nchunks = 0;
        if (e->cells != 0)
            heap_Free(tos->heap, e->cells);
        e->cells = 0;
        tosBaseBlockDestruct(&e->b);
        break;
    case 5:
        e->flag = (unsigned char)(int)arg;
        break;
    case 6:
        *(unsigned char *)arg = e->flag;
        break;
    default:
        return 0;
    }
    return 1;
}

TOSBLOCK *ConstructDpScoreUw(TOS *tos, unsigned char id)
{
    DPSCOREUW *e = (DPSCOREUW *)tosBaseBlockConstruct(tos, id, 1, 1,
                                                      (TOSFUNC)IdleDpScoreUwProcess,
                                                      (TOSFUNC)InitDpScoreUw,
                                                      (TOSFUNC)ControlDpScoreUw,
                                                      sizeof(DPSCOREUW));

    e->makeactive = MakeScoreUserWordsActiveDist;
    e->sendresult = SendResultScoreUserWords;
    return &e->b;
}

static void IdleDpScoreUwProcess(DPSCOREUW *e, short **in)
{
}

static int InitDpScoreUw(DPSCOREUW *e)
{
    unsigned short n = _tosGetProfileU32(e, 19, 1);

    e->in = e->b.inputs;
    e->out = e->b.outputs;
    e->in->size = n * sizeof(short);
    e->out->size = n * (sizeof(short) + sizeof(char));
    e->chunksize = _tosGetProfileU32(e, 17, 400);
    if (e->chunksize > 65000)
        e->chunksize = 65000;
    else if (e->chunksize < 200)
        e->chunksize = 200;
    e->maxframes = 0;
    e->flag = 0;
    return 0;
}

static unsigned short MakeScoreUserWordsActiveDist(DPSCOREUW *e, unsigned short *dist,
                                                   unsigned char *active)
{
    WORDCELL *c = e->cells;
    unsigned int n;
    unsigned int i;
    unsigned short phen;

    memset(active, 0, e->nphen);
    n = e->nergodic;
    memset(active, 1, n);
    for (i = 0; i < e->ncells; i++) {
        phen = c->phen;
        if (active[phen] == 0) {
            active[phen] = 1;
            dist[++n] = phen;
        }
        c++;
    }
    dist[0] = n;
    return n;
}
