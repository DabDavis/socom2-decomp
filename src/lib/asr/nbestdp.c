/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* nbestdp: the recogniser's N-best dynamic programming block (OVL2 0x4EB5E8-0x4F0CB8, SOCOM 1's
 * object at 0x358DF0: CollectDummyMemory, InitNBestDynProgProcess, InitDynprogContext,
 * CleanUpParBTProc, PrunDynProg, the two DynProgAccNBest passes, CopyParBT, BacktraceWordsParBT,
 * FindBestBacktraces, FindBestRecognised, ExpandGarbageArrays, NBestDynProgProcess,
 * ControlNBestDynProgProcess, ConstructNBestDynProg, ExtendBacktraceStack, ScoreCompare,
 * IdleProcess, all but the constructor and FindBestRecognised LOCAL there), ScanSoft's speech
 * recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README
 * "The GCC front"). Names are SOCOM 1's. Only the functions written so far are here, in image
 * order: CollectDummyMemory and InitViterbi (OPEN) (round 20c), BacktraceWordsParBT (OPEN) (round
 * 36c), ControlNBestDynProgProcess and ConstructNBestDynProg (round 41c), InitNBestDynProgProcess and
 * PrunDynProg (round 19c; PrunDynProg closed in 20c), InitDynprogContext and SOCOM II's
 * func_004EC130 (round 21c), FindBestRecognised (round 17c), ExtendBacktraceStack, ScoreCompare
 * (NAME@nbestdp in the map: the iso-word object has its own). SOCOM II's rows hold SOCOM 1's in
 * order: 0x4EB5E8 is CollectDummyMemory's position, 0x4EB7D8 InitViterbi's, 0x4EBCB0
 * InitDynprogContext's, 0x4EC130 new (the context's release, which SOCOM 1's
 * ControlNBestDynProgProcess does in place), 0x4EC598 and 0x4ED6E0 the two DP passes. The
 * backtrace stack is a list of three-word records from the scheduler's small-block allocator
 * (fastallo.c), a recycled one first. */

/* the scheduler's small-block allocator, as fastallo.c keeps it */
typedef struct FASTALLO {
    void *heap;                         /* +0 */
    unsigned short chunk_size;          /* +4 */
    unsigned short left;                /* +6 */
    int nchunks;                        /* +8 */
    int **chunks;                       /* +12 */
    int *next;                          /* +16 */
    int *free[65];                      /* +20: a free list per size in words */
} FASTALLO;

/* the scheduler, as far as the DP reads it */
typedef struct DPTOS {
    char pad0[80];
    void *heap;                         /* +80 */
    FASTALLO fa;                        /* +84 */
} DPTOS;

/* a backtrace stack record (three words) */
typedef struct BTSTACK {
    struct BTSTACK *prev;               /* +0 */
    struct BTSTACK *next;               /* +4 */
    int entry;                          /* +8 */
} BTSTACK;

/* a backtrace record the tokens share (five words): the backtrace it continues, the chain of
 * links to the others it joins (BTLINK), its count of users at +12 */
typedef struct BTREC {
    struct BTREC *prev;                 /* +0 */
    struct BTLINK *links;               /* +4 */
    int f8;                             /* +8: the score */
    unsigned short users;               /* +12 */
    unsigned short f14;                 /* +14: the word */
    short *f16;                         /* +16: the end state, null at the utterance's start */
} BTREC;

/* a link of a backtrace's chain (three words): the backtrace it holds and the next link; a
 * backtrace record heads its own chain with the same two fields */
typedef struct BTLINK {
    BTREC *bt;                          /* +0 */
    struct BTLINK *next;                /* +4 */
    int f8;                             /* +8 */
} BTLINK;

/* a token on the active lists at +112 (seven words) and +116 (five) */
typedef struct NBTOKEN {
    struct NBTOKEN *next;               /* +0 */
    short f4;                           /* +4 */
    unsigned char f6;                   /* +6 */
    short *state;                       /* +8 */
    BTREC *bt;                          /* +12 */
    int *f16;                           /* +16 */
    int *f20;                           /* +20 */
    int f24;                            /* +24 */
} NBTOKEN;

/* a link of a word record's list (two words) */
typedef struct NBLINK {
    struct NBLINK *next;                /* +0 */
} NBLINK;

/* a word record on the list at +120 (five words): its backtrace and a list of links */
typedef struct NBWORD {
    struct NBWORD *next;                /* +0 */
    int pad4;
    BTREC *bt;                          /* +8 */
    int pad12;
    NBLINK *links;                      /* +16 */
} NBWORD;

/* a recognised word's record in a result (five words), chained at +16 */
typedef struct NBRWORD {
    int state;                          /* +0: the word's end state, an index into +84 */
    int score;                          /* +4 */
    int word;                           /* +8: its id */
    int f12;                            /* +12 */
    struct NBRWORD *next;               /* +16 */
} NBRWORD;

/* a recognised word sequence, chained best first */
typedef struct NBRESULT {
    int f0;
    int score;                          /* +4 */
    int f8;
    NBRWORD *words;                     /* +12 */
    struct NBRESULT *next;              /* +16 */
} NBRESULT;

/* a backtrace's head as the N-best ranks it */
typedef struct NBTRACE {
    int pad0;
    short score;                        /* +4 */
} NBTRACE;

/* a block's reader in an input queue (tosblk.h's QIN) */
typedef struct NBQIN {
    void *q;
    unsigned short reader;              /* +4 */
    unsigned short size;                /* +6 */
} NBQIN;

typedef struct NBESTDP {
    DPTOS *tos;                         /* +0: the block's head (tosblk.h) */
    char pad4[8];
    NBQIN *inputs;                      /* +12 */
    char pad16[8];
    int (*process)();                   /* +24: the scheduler's process slot */
    char pad28[12];
    void *f40;                          /* +40: the scorer's context (command 35) */
    int (*f44)(void *ctx, unsigned int phen);   /* +44: one phoneme's score */
    void (*f48)(void *ctx, short *scores, void *frame, unsigned char flag, short base);  /* +48 */
    short *(*f52)(void *ctx);           /* +52: the scores' table */
    short *f56;                         /* +56: a score per phoneme, 32767 not yet scored */
    unsigned int f60;                   /* +60: the phonemes */
    int f64;                            /* +64 */
    unsigned int f68;                   /* +68: the context's words; f144's last index */
    unsigned int f72;                   /* +72: a bit per word, in bytes */
    int f76;                            /* +76 */
    int *states;                        /* +80: per word an offset into it, then the states */
    short *f84;                         /* +84: a short per state */
    BTSTACK *stack;                     /* +88: the stack's first record */
    NBRESULT *results;                  /* +92 */
    unsigned int f96;                   /* +96: the garbage arrays' frames */
    int *f100;                          /* +100: a garbage score per frame */
    int *f104;                          /* +104: and the second garbage phoneme's */
    unsigned int frame;                 /* +108: the frames so far */
    NBTOKEN *tok7;                      /* +112 */
    NBTOKEN *tok5;                      /* +116 */
    NBWORD *words;                      /* +120 */
    int *offsets;                       /* +124: as +80, the context's other table */
    unsigned char *active;              /* +128: a bit per word id, the high bit first */
    int **wstates;                      /* +132: per word, its first active state in f136 */
    int *f136;                          /* +136: the active states, word by word */
    int f140;                           /* +140 */
    int *f144;                          /* +144 */
    int *f148;                          /* +148: a word per word, and f152 more */
    unsigned int f152;                  /* +152 */
    unsigned short *flags;              /* +156: a word's flags (bit 0: in the pass) */
    char pad160[4];
    unsigned int f164;                  /* +164: the garbage phonemes (command 34) */
    unsigned int f168;                  /* +168 */
    int f172;                           /* +172 */
    unsigned short f176;                /* +176 */
    char pad178[2];
    int (*pass)();                      /* +180: the DP pass, with or without optional silence */
    unsigned int nresults;              /* +184: the sequences asked for (profile 7) */
    unsigned int nwords;                /* +188 */
    int *wordids;                       /* +192 */
    unsigned int beam;                  /* +196: profile 6 */
    unsigned int beam10;                /* +200: ten times it */
    int f204;                           /* +204 */
    unsigned int maxframes;             /* +208: profile 9 */
    unsigned char pruning;              /* +212 */
    char pad213;
    short level;                        /* +214 */
    short maxlevel;                     /* +216: profile 11 */
    char pad218[6];
    unsigned int rate;                  /* +224 */
    unsigned int f228;                  /* +228: profile 10's fraction, in 128ths */
    unsigned int hist[5];               /* +232 */
    unsigned int hist2[5];              /* +252 */
    unsigned char nhist;                /* +272 */
    char pad273;
    short f274;                         /* +274 */
    NBTOKEN *f276;                      /* +276: the best token */
    short f280;                         /* +280: profile 14 */
    char pad282[2];
    int f284;                           /* +284 */
    int f288;                           /* +288 */
} NBESTDP;

/* the context the pass runs on (ControlNBestDynProgProcess's command): its words' flags and
 * the two state tables (per word an offset into the table, then the states) */
typedef struct NBCTX {
    int optsil;                         /* +0: optional silence between words */
    unsigned int nwords;                /* +4 */
    int f8;                             /* +8 */
    unsigned short *flags;              /* +12 */
    short *f16;                         /* +16 */
    int f20;                            /* +20 */
    int *offsets;                       /* +24 */
    int *states;                        /* +28 */
    unsigned char *active;              /* +32: a bit per state, the high bit first */
} NBCTX;

extern int *fastallo_AllocateMemoryFromChunk(FASTALLO *fa, unsigned short n);
extern unsigned int _tosGetProfileU32(void *b, unsigned short key, unsigned int def);
extern float _tosGetProfileFloat(void *b, unsigned short key, float def);

static unsigned int FindBestBacktraces(NBESTDP *d, NBTRACE ***bt);
static int ScoreCompare();
extern void qsort(void *base, unsigned int n, unsigned int size, int (*cmp)());
static void CleanUpParBTProc(NBESTDP *d, BTREC *bt);
static void ExtendBacktraceStack(NBESTDP *d, BTSTACK *top);
static NBRESULT *BacktraceWordsParBT(NBESTDP *d, NBTOKEN *t);
extern void heap_Free(void *heap, void *p);
extern void *heap_Alloc(void *heap, unsigned int size);
extern unsigned char _tosErrorLog(void *log, unsigned short code);
extern int DynProgAccNBestNoOptSil();
extern int DynProgAccNBestOptSil();
static void NBestDynProgProcess(NBESTDP *d, void **in);
extern void ExpandGarbageArrays(NBESTDP *d);
extern int IdleProcess();
extern int _tosControl(void *b, unsigned char cmd, void *arg, int arg2);

/* a record given back to the scheduler's free list of its size, its first word the link. The
 * list's address is a pointer of the push's own (`fl_`): loop.c hoists each as an invariant of its
 * own -- the target's `addiu s3,s6,132` in each preheader, and in the results' two loops a second
 * register copied from the first (`move t1,t0`) -- where the free list spelled
 * as a field is addressed off tos each time (`lw v0,132(s4)`), and a pointer declared at the top
 * of a loop's body takes another priority in global-alloc (the word records' list and their copy
 * swap s3/s4) */
#define FREE_REC(tos, k, p) { int **fl_ = &(tos)->fa.free[k]; *(int **)(p) = *fl_; *fl_ = (int *)(p); }

/* the pass's memory given back: every token, word record and result to the scheduler's free lists
 * by size (seven words, five, two), a backtrace losing its last user cleaned up. SOCOM 1's body is
 * the middle two lists (0x118 bytes), with a scheduler pointer of its own (read again after the
 * first list: PRE reloads it on that loop's exit, the target's s5); SOCOM II adds the seven-word
 * tokens before them and the results after (0x1EC), through the pointer read at the top (s6).
 * The record is copied before its backtrace's test (a saved register across the call) */

static void CollectDummyMemory(NBESTDP *d)
{
    DPTOS *tos = d->tos;
    NBTOKEN *t, *pt;
    NBWORD *w, *pw;
    NBRESULT *r, *pr;

    for (t = d->tok7; t != 0; ) {
        pt = t;
        if (t->bt != 0 && --t->bt->users == 0)
            CleanUpParBTProc(d, t->bt);
        t = t->next;
        FREE_REC(tos, 7, pt);
    }
    d->frame = 0;
    d->tok7 = 0;
    {
        DPTOS *tos = d->tos;

        for (t = d->tok5; t != 0; ) {
            pt = t;
            if (t->bt != 0 && --t->bt->users == 0)
                CleanUpParBTProc(d, t->bt);
            t = t->next;
            FREE_REC(tos, 5, pt);
        }
        for (w = d->words; w != 0; ) {
            NBLINK *l = w->links;

            pw = w;
            if (w->bt != 0 && --w->bt->users == 0)
                CleanUpParBTProc(d, w->bt);
            while (l != 0) {
                NBLINK *pl = l;

                l = l->next;
                FREE_REC(tos, 2, pl);
            }
            w = w->next;
            FREE_REC(tos, 5, pw);
        }
        d->tok5 = 0;
        d->words = 0;
    }
    for (r = d->results; r != 0; ) {
        NBRWORD *rw = r->words;

        pr = r;
        while (rw != 0) {
            NBRWORD *prw = rw;

            rw = rw->next;
            FREE_REC(tos, 5, prw);
        }
        r = r->next;
        FREE_REC(tos, 5, pr);
    }
    d->results = 0;
}

/* a record of N words from the scheduler's free list of its size, else from a new chunk */
#define ALLOC_REC(tos, k, p) { int **fl_ = &(tos)->fa.free[k]; \
    if (*fl_ != 0) { (p) = (void *)*fl_; *fl_ = *(int **)(p); } \
    else (p) = (void *)fastallo_AllocateMemoryFromChunk(&(tos)->fa, k); }

/* a new utterance's start: the backtrace stack made, the last pass's memory given back, the
 * counters and the score tables cleared, one backtrace record shared by a token for every state of
 * every active word (chained at +112, the count its users), the pruning level reset. SOCOM 1's body
 * with the token and result lists' release moved into CollectDummyMemory. The scheduler, the
 * word's index and the chain's head are spilled (sp+0, +4, +8: declaration order; the clearing
 * loops' counter of their own, else the index takes a register and its scaled copy spills), the
 * token count kept in a caller-saved register round the rare allocations (`sq t1`).
 * OPEN (round 20c, 0x3B8 against 0x3C0, 2 words short): the token loop is unrolled by two with a
 * preconditioning copy as the target's, but cross-jumped elsewhere -- the target's first copy
 * loads its state, steps the pointer and stores the state and the zero at +24 before it joins
 * the last copy's tail (`b; addiu t1,t1,1`), here it joins right after the backtrace's store;
 * and the registers follow (the count t0 here, t1 there; the record allocation's reload of the
 * spilled scheduler a1 here, v0 there). `*s++` in the state's store, the count first or in the
 * for's step, `&d->f84[*s]` leave it (build/scratch/gcc/r20c/v_iv.py) */
static void InitViterbi(NBESTDP *d)
{
    DPTOS *tos = d->tos;
    unsigned int i;
    NBTOKEN *head = 0;
    NBTOKEN *t = 0;
    BTREC *bt;
    unsigned int n, k;

    if (d->stack == 0)
        ExtendBacktraceStack(d, 0);
    CollectDummyMemory(d);
    d->f284 = 0;
    d->f176 = 0xFFFF;
    d->f64 = 0;
    d->f204 = 0;
    d->f274 = 0;
    d->f276 = 0;
    for (k = 0; k <= d->f68; k++)
        d->f144[k] = 0;
    for (k = 0; k < d->nwords + d->f152; k++)
        d->f148[k] = 0;
    ALLOC_REC(tos, 5, bt);
    bt->f16 = 0;
    bt->links = 0;
    bt->prev = 0;
    n = 0;
    for (i = 0; i < d->nwords; i++) {
        int w = d->wordids[i];

        if (d->active[w >> 3] & (128 >> (w & 7))) {
            int *st = d->states;
            int *s = st + st[w];
            int *end = st + st[w + 1];

            for (; s < end; s++) {
                ALLOC_REC(tos, 7, t);
                t->f4 = 0;
                t->bt = bt;
                t->state = d->f84 + *s;
                t->f24 = 0;
                t->f16 = &d->f140;
                t->f6 = 1;
                t->next = head;
                head = t;
                t->f20 = d->f148 + i;
                n++;
            }
        }
    }
    d->tok7 = t;
    bt->users = n;
    d->frame = n;
    d->level = d->maxlevel;
    d->pruning = 0;
    for (k = 0; k < 5; k++) {
        d->hist[k] = 0;
        d->hist2[k] = 0;
    }
    d->nhist = 0;
}

/* the block's profile: SOCOM 1's body, its fraction converted inline where SOCOM 1 called fptoui
 * (0.2f lands an ulp low through 2.96's parser, on the target's 0x3E4CCCCC); the inputs' frame
 * sizes last */
static int InitNBestDynProgProcess(NBESTDP *d)
{
    d->f172 = 0;
    d->beam = _tosGetProfileU32(d, 6, 50);
    d->beam10 = d->beam * 10;
    d->nresults = _tosGetProfileU32(d, 7, 10);
    d->maxframes = _tosGetProfileU32(d, 9, 300);
    d->f228 = _tosGetProfileFloat(d, 10, 0.2f) * 128.0f;
    d->maxlevel = _tosGetProfileU32(d, 11, 5500);
    d->f280 = _tosGetProfileU32(d, 14, 0);
    d->results = 0;
    d->inputs[1].size = _tosGetProfileU32(d, 15, 4);
    d->inputs[0].size = 1;
    return 0;
}

/* a context's tables taken for the pass: the DP pass chosen by its silence option, the ids of
 * the words flagged for it, and per word the states the active bitmap keeps; the block then
 * processes. SOCOM 1's body (0x2B0), which SOCOM II's compile unrolls (0x47C). Measured: the
 * flags walked by a pointer taken before the count loop (the target's load in the entry test's
 * slot; indexed, loop.c hoists it after the test), the ids' loop a counter of its own (one counter
 * for both moves its register), and the filter's out pointer declared before the per-word table
 * (30 of the 120 declaration orders, each with out before ws: the target's `move a2,v0` in the
 * test's slot). The five failures are one `return _tosErrorLog(d, 113)` each, cross-jumped into
 * the last one's place */
static int InitDynprogContext(NBESTDP *d, NBCTX *ctx)
{
    DPTOS *tos = d->tos;
    unsigned int cnt = 0;
    unsigned int nw = 0;
    unsigned int i, j;
    unsigned short *f;

    if (ctx->optsil != 0)
        d->pass = DynProgAccNBestOptSil;
    else
        d->pass = DynProgAccNBestNoOptSil;
    d->f68 = ctx->nwords;
    d->f72 = (d->f68 - 1) / 8 + 1;
    f = ctx->flags;
    for (i = 0; i < d->f68; i++)
        cnt += *f++ & 1;
    d->nwords = cnt;
    d->f76 = ctx->f8;
    d->wordids = heap_Alloc(tos->heap, cnt * sizeof(int));
    d->f288 = ctx->f20;
    d->states = ctx->states;
    d->f84 = ctx->f16;
    if (d->wordids == 0)
        return _tosErrorLog(d, 113);
    d->offsets = ctx->offsets;
    d->flags = ctx->flags;
    for (j = 0; j < d->f68; j++)
        if (d->flags[j] & 1)
            d->wordids[nw++] = j;
    d->wstates = heap_Alloc(tos->heap, (d->f68 + 1) * sizeof(int *));
    d->active = ctx->active;
    d->f152 = d->offsets[d->f68] - d->offsets[0];
    d->f136 = heap_Alloc(tos->heap, (d->f152 + 1) * sizeof(int));
    if (d->wstates == 0 || d->f136 == 0)
        return _tosErrorLog(d, 113);
    {
        int *out = d->f136;
        int **ws = d->wstates;
        int *offs = d->offsets, *o = offs;
        unsigned char *bits = d->active;
        unsigned int n = d->f68;
        unsigned int w, k, end;

        for (w = 0; w < n; w++) {
            *ws++ = out;
            k = *o++;
            end = *o;
            for (; k < end; k++) {
                int s = offs[k];

                if (bits[s >> 3] & (128 >> (s & 7)))
                    *out++ = s;
            }
        }
        *ws = out;
    }
    d->f144 = heap_Alloc(tos->heap, (d->f68 + 1) * sizeof(int));
    if (d->f144 == 0)
        return _tosErrorLog(d, 113);
    d->f148 = heap_Alloc(tos->heap, (d->nwords + d->f152) * sizeof(int));
    if (d->f148 == 0)
        return _tosErrorLog(d, 113);
    d->f56 = d->f52(d->f40);
    d->process = NBestDynProgProcess;
    return 0;
}

/* new in SOCOM II: InitDynprogContext's tables given back (SOCOM 1's ControlNBestDynProgProcess
 * frees them in place before each new context and when the block is destroyed; SOCOM II calls this
 * at both places), and the two tables at +100/+104 that +96 says are held */
void func_004EC130(NBESTDP *d)
{
    DPTOS *tos = d->tos;

    if (d->wordids != 0) {
        heap_Free(tos->heap, d->wordids);
        d->wordids = 0;
    }
    if (d->wstates != 0) {
        heap_Free(tos->heap, d->wstates);
        d->wstates = 0;
    }
    if (d->f136 != 0) {
        heap_Free(tos->heap, d->f136);
        d->f136 = 0;
    }
    if (d->f144 != 0) {
        heap_Free(tos->heap, d->f144);
        d->f144 = 0;
    }
    if (d->f148 != 0) {
        heap_Free(tos->heap, d->f148);
        d->f148 = 0;
    }
    if (d->f96 != 0) {
        heap_Free(tos->heap, d->f100);
        heap_Free(tos->heap, d->f104);
        d->f96 = 0;
    }
}

/* a record pushed after `top` (the first one when top is null). An inline defined here, after
 * InitViterbi (which calls it) and before CleanUpParBTProc (which holds its body twice): 2.96
 * inlines only a body it has seen, and defers the function's own copy to the object's end, where
 * the image has it (0x4F0C00, after ConstructNBestDynProg, as SOCOM 1's) */
static __inline__ void ExtendBacktraceStack(NBESTDP *d, BTSTACK *top)
{
    DPTOS *tos = d->tos;
    int **list = &tos->fa.free[3];
    BTSTACK *e;

    if (*list != 0) {
        e = (BTSTACK *)*list;
        *list = *(int **)e;
    } else
        e = (BTSTACK *)fastallo_AllocateMemoryFromChunk(&tos->fa, 3);
    if (top != 0) {
        top->next = e;
        e->prev = top;
    } else {
        d->stack = e;
        e->prev = 0;
    }
    e->next = 0;
}

/* a backtrace with no user left given back, and every backtrace its chain held that this leaves
 * without users, through the stack of records at +88 (an explicit recursion: each record's entry
 * the backtrace to clean, the stack extended as it deepens); the chain's links to the scheduler's
 * three-word list, the backtrace to its five-word list. SOCOM 1's body (0x180; 0x1A8 here with the
 * free lists through pointers of their own, FREE_REC). The backtrace is given back after the
 * chain's test, whether or not it had one. Measured: the next record's entry read through
 * `top->prev` before top steps (the target's `lw v0,0(s0); lw s2,8(v0); move s0,v0`), and the
 * chain's walker set before the test (the copy then stays out of the test's slot, which takes
 * it from the fall-through instead) */
static void CleanUpParBTProc(NBESTDP *d, BTREC *bt)
{
    BTSTACK *top = d->stack;
    DPTOS *tos = d->tos;
    BTLINK *p, *q;
    BTREC *b, *r;

    top->entry = (int)bt;
    if (top->next == 0)
        ExtendBacktraceStack(d, top);
    top = top->next;
    while (top != d->stack) {
        b = (BTREC *)top->prev->entry;
        top = top->prev;
        p = (BTLINK *)b;
        if (b != 0) {
            do {
                r = p->bt;
                if (r != 0 && --r->users == 0) {
                    top->entry = (int)r;
                    if (top->next == 0)
                        ExtendBacktraceStack(d, top);
                    top = top->next;
                }
                q = p;
                p = p->next;
                if (q != (BTLINK *)b)
                    FREE_REC(tos, 3, q);
            } while (p != 0);
        }
        FREE_REC(tos, 5, b);
    }
}

/* the pruning level's update, once the pass has run a while (or has seen 11 ...): the rate of the
 * last five frames' sums, a saturating fixed-point product, the level moved by it within the
 * maximum and not below a third of where it was; then this frame into the five-entry history.
 * SOCOM 1's body (0x1F4 in both builds). Measured: the flag's store before the flag (`d->pruning
 * = 1; pruning = 1;` -- the store takes a constant of its own, the target's v0), the ring index an
 * int (no `andi` after its increment), `x < 0` and `x > MAX` two tests (one `||` folds into an
 * unsigned compare), the sum an int, the rate read into x itself before the product (`x =
 * d->rate; ... x = diff * (d->f228 * x)`: round 19c's 17 words, registers only, were global-alloc
 * giving x -- a dozen references over the clamps, allocated first -- the first free register, a0;
 * with the rate's load in x's own live range x conflicts with the f228 load local-alloc put in a0
 * and takes a1, the old level then a0: the target's, byte-exact. A rate local of its own leaves x
 * free of a0, whatever the order). */
static void PrunDynProg(NBESTDP *d)
{
    unsigned char pruning = d->pruning;

    if (pruning) {
        unsigned int s1 = 0, s2 = 1;
        int i, diff, x, t;
        short old;

        for (i = 0; i < 5; i++) {
            s1 += d->hist[i];
            s2 += d->hist2[i];
        }
        if (s1 > 0x1FFFFFF)
            d->rate = s1 / (s2 >> 7);
        else
            d->rate = (s1 << 7) / s2;
        x = d->rate;
        diff = d->maxframes - d->frame;
        x = diff * (d->f228 * x);
        if (diff > 0) {
            if (x < 0)
                x = 0x1FFFC000;
            else if (x > 0x1FFFC000)
                x = 0x1FFFC000;
        } else {
            if (x > 0)
                x = -0x1FFFC000;
            else if (x < -0x1FFFC000)
                x = -0x1FFFC000;
        }
        old = d->level;
        t = (x >> 14) + old;
        if (t > d->maxlevel)
            d->level = d->maxlevel;
        else
            d->level = t;
        if (d->level < old / 3)
            d->level = old / 3;
    } else if (d->f176 >= 11 || (d->maxframes >> 2) < d->frame) {
        d->pruning = 1;
        pruning = 1;
    }
    if (pruning) {
        int n = d->nhist;

        d->hist[n] = (d->frame * (unsigned short)d->level) >> 3;
        d->hist2[n] = (d->frame * d->frame) >> 3;
        n++;
        if (n == 5)
            n = 0;
        d->nhist = n;
    }
}

/* a backtrace copied (five words, its user count left 0) with its chain of links (three words
 * each), every backtrace they hold gaining a user. SOCOM 1's body (0x110): the users' store after
 * the first copy (the target loads +16 before it; a store may alias the source's fields) */
static BTREC *CopyParBT(NBESTDP *d, BTREC *src)
{
    DPTOS *tos = d->tos;
    BTREC *n;
    BTLINK *s, *l, *prev;

    ALLOC_REC(tos, 5, n);
    n->f16 = src->f16;
    n->users = 1;
    n->f8 = src->f8;
    n->f14 = src->f14;
    n->prev = src->prev;
    n->prev->users++;
    n->links = 0;
    prev = (BTLINK *)n;
    for (s = src->links; s != 0; s = s->next) {
        ALLOC_REC(tos, 3, l);
        l->f8 = s->f8;
        l->bt = s->bt;
        l->bt->users++;
        l->next = 0;
        if (prev != 0)
            prev->next = l;
        prev = l;
    }
    n->users = 0;
    return n;
}

/* the token's word sequence: a five-word record per word its backtrace chain holds (the newest
 * first; a word's record when the next backtrace ends another word or state), each shared
 * backtrace on the way copied and pushed on the stack at +88; then back down the stack, each
 * backtrace's best link (the lowest score) made its head and the chain re-scored by the
 * difference, the token left holding the new chain (its score moved by the difference) or none
 * (32767). SOCOM 1's body (0x4F4; 0x534 here with the free lists through pointers, ALLOC_REC).
 * OPEN (round 36c, the right size, 218 words, registers and three shapes): the target gives the
 * second loop's best link and the first's next s0, b s1, nb s2, prev and the stack's walker s6, w
 * s8; here nb (59 refs over 129 insns: both loops) outranks the link and b, and w prev. Measured
 * (build/scratch/gcc/r36/v_bw*.py): the loop's record built through r with `w = r` after its stores
 * (the copy first makes the longer-lived w CSE's canonical register: every store through s8); the
 * first record's last two stores go through w in the target (`w = r` after the third: 3 words, but
 * w's priority grows); `b = top->prev->entry; top = top->prev;` (CleanUpParBTProc's) gives the
 * walker a register of its own as the target's but moves delta's zero before the second loop (a
 * word long); the f16 == 0 arm -- the target sets b = 0 and branches to the loop's test, for /
 * continue gives `next = 0` into `b = next`, while / else threads it to the exit; and GCSE keeps
 * the head link's score from the search's start for the swap (a copy in a3) where the target
 * reloads it. A copy of its own in the first loop (c) gives b 69 refs and s0. */
static NBRESULT *BacktraceWordsParBT(NBESTDP *d, NBTOKEN *t)
{
    DPTOS *tos = d->tos;
    BTSTACK *top = d->stack;
    BTREC *b = t->bt;
    BTREC *next, *nb, *prev = 0;
    BTLINK *l, *q, *best;
    NBRWORD *w = 0, *r;
    NBRESULT *res;
    unsigned int min, old, tmp;
    BTREC *tb;
    int delta = 0;

    if (b != 0) {
        ALLOC_REC(tos, 5, r);
        w = r;
        r->next = 0;
        r->f12 = 0;
        r->state = t->state - d->f84;
        r->word = d->f176;
        r->score = t->f4 + d->f284;
    }
    for (; b != 0; b = next) {
        if (b->f16 == 0) {
            next = 0;
            continue;
        }
        next = b->prev;
        if (next != 0 && (next->f14 != b->f14 || next->f16 != b->f16)) {
            ALLOC_REC(tos, 5, r);
            r->next = w;
            r->f12 = 0;
            r->state = b->f16 - d->f84;
            r->word = b->f14;
            r->score = b->f8;
            w = r;
        }
        nb = b;
        if (b != t->bt && b->users >= 2)
            nb = CopyParBT(d, b);
        top->entry = (int)nb;
        if (top->next == 0)
            ExtendBacktraceStack(d, top);
        top = top->next;
        if (nb != b && prev != 0) {
            nb->users++;
            if (--prev->prev->users == 0)
                CleanUpParBTProc(d, prev->prev);
            prev->prev = nb;
        }
        prev = nb;
    }
    if (w != 0) {
        ALLOC_REC(tos, 5, res);
        res->words = w;
        res->score = t->f4 + d->f284;
    } else
        res = 0;
    nb = 0;
    while (top != d->stack) {
        top = top->prev;
        b = (BTREC *)top->entry;
        if (nb == 0) {
            l = b->links;
            if (l != 0) {
                best = 0;
                min = l->f8;
                for (q = l->next; q != 0; q = q->next)
                    if (q->f8 < min) {
                        min = q->f8;
                        best = q;
                    }
                if (best != 0) {
                    tmp = best->f8;
                    best->f8 = l->f8;
                    l->f8 = tmp;
                    tb = best->bt;
                    best->bt = l->bt;
                    l->bt = tb;
                }
                ALLOC_REC(tos, 5, nb);
                nb->prev = l->bt;
                nb->links = l->next;
                nb->f8 = l->f8;
                nb->f16 = b->f16;
                nb->users = 0;
                nb->f14 = b->f14;
                FREE_REC(tos, 3, l);
                b->links = 0;
                delta = (short)(nb->f8 - b->f8);
            }
        } else {
            b = CopyParBT(d, b);
            nb->users++;
            if (--b->prev->users == 0)
                CleanUpParBTProc(d, b->prev);
            old = b->f8;
            b->prev = nb;
            b->f8 = old + delta;
            best = 0;
            min = b->f8;
            for (q = b->links; q != 0; q = q->next)
                if (q->f8 < min) {
                    min = q->f8;
                    best = q;
                }
            if (best != 0) {
                tmp = best->f8;
                best->f8 = b->f8;
                b->f8 = tmp;
                tb = best->bt;
                best->bt = b->prev;
                b->prev = tb;
            }
            nb = b;
            delta = (short)(nb->f8 - old);
        }
    }
    if (nb != 0) {
        nb->users++;
        if (--t->bt->users == 0)
            CleanUpParBTProc(d, t->bt);
        t->bt = nb;
        t->f4 = delta + t->f4;
    } else {
        if (t->bt != 0) {
            if (--t->bt->users == 0)
                CleanUpParBTProc(d, t->bt);
            t->bt = 0;
        }
        t->f4 = 32767;
    }
    return res;
}

/* the backtraces the pass can end on: every seven-word token whose state ends a word the context
 * flags as an end (bit 1) kept and counted, the others given back; the five-word tokens and the
 * word records given back as SOCOM 1's CollectDummyMemory does (SOCOM II's own CollectDummyMemory
 * gives back the seven-word tokens too, so the block is written here); the kept ones in a table
 * from the heap, sorted by score. SOCOM 1's body (0x180) with its call's middle inline (0x288).
 * Measured: the block's token pointer its own (one `t` for the three loops is live across the
 * block's call: a saved register where the target's first loop has a1), and the state's word id
 * summed low half first (`st[1] + (st[2] << 16)`: the target's operands; the shift first swaps
 * the two halves' registers) */
static unsigned int FindBestBacktraces(NBESTDP *d, NBTRACE ***bt)
{
    DPTOS *tos = d->tos;
    NBTOKEN *t, *prev = 0, *r;
    unsigned int n = 0;
    NBTRACE **p;

    for (t = d->tok7; t != 0; ) {
        unsigned short *st = (unsigned short *)t->state;

        r = 0;
        if (st[0] & 1) {
            if (d->flags[st[1] + (st[2] << 16)] & 2)
                n++;
            else
                r = t;
        } else
            r = t;
        if (r != 0) {
            if (--r->bt->users == 0)
                CleanUpParBTProc(d, r->bt);
            t = r->next;
            if (prev != 0)
                prev->next = t;
            else
                d->tok7 = t;
            FREE_REC(tos, 7, r);
        } else {
            prev = t;
            t = t->next;
        }
    }
    {
        DPTOS *tos = d->tos;
        NBTOKEN *t, *pt;
        NBWORD *w, *pw;

        for (t = d->tok5; t != 0; ) {
            pt = t;
            if (t->bt != 0 && --t->bt->users == 0)
                CleanUpParBTProc(d, t->bt);
            t = t->next;
            FREE_REC(tos, 5, pt);
        }
        for (w = d->words; w != 0; ) {
            NBLINK *l = w->links;

            pw = w;
            if (w->bt != 0 && --w->bt->users == 0)
                CleanUpParBTProc(d, w->bt);
            while (l != 0) {
                NBLINK *pl = l;

                l = l->next;
                FREE_REC(tos, 2, pl);
            }
            w = w->next;
            FREE_REC(tos, 5, pw);
        }
        d->tok5 = 0;
        d->words = 0;
    }
    if (n != 0) {
        *bt = heap_Alloc(tos->heap, n * sizeof(NBTRACE *));
        p = *bt;
        for (t = d->tok7; t != 0; t = t->next)
            *p++ = (NBTRACE *)t;
        qsort(*bt, n, sizeof(NBTRACE *), ScoreCompare);
    } else
        *bt = 0;
    return n;
}

/* the best token's state ended a word the context flags as an end (bit 1), the same word as the
 * frame before: the frames it has held so far against the beam (profile 6). An inline (SOCOM 1's
 * too): its result zero-extended once at its join, the target's `andi v0,v0,0xff` */
static __inline__ unsigned char WordEnded(NBESTDP *d)
{
    NBTOKEN *t = d->f276;
    unsigned short *st;
    unsigned int w;

    if (t == 0)
        return 0;
    st = (unsigned short *)t->state;
    while ((*st & 1) == 0)
        st++;
    w = st[1] + (st[2] << 16);
    if ((d->flags[w] & 2) && w == d->f204)
        d->f64++;
    else
        d->f64 = 0;
    d->f204 = w;
    return d->f64 > d->beam;
}

/* one frame: the phonemes' scores cleared and made by the scorer (the frame's flag byte, 1 when
 * none), the frame run through the DP pass, then the two garbage phonemes' running scores; with
 * end detection on (+172), the block stops once the best word has held long enough. SOCOM 1's
 * body (0x334). Measured: each running score is one statement over a conditional (SCORE): 2.96's
 * fold distributes the sum over the conditional's arms, `c ? prev + x + base : prev + y + base`,
 * so each arm adds into the one temporary stored at the join (one `sw v0,0(s4)`, the call arm's
 * halfword store first: the target's), the left side's address taken before the right side, and
 * the right side's first operand -- an array reference -- made a SAVE_EXPR, read once before the
 * test (the target's `lw s2` in the branch's slot); the pieces as statements of their own keep the
 * adds but not the shared store, and put the four saved registers in another order. The frame
 * pointer is a local (the fill loop's halfword stores would make in[1] a reload), the word-end
 * test compares `f64 > beam` (its loads in that order) */
/* a phoneme's score this frame, asked of the scorer the first time (32767: not yet scored) */
#define SCORE(d, scores, k) \
    ((scores)[k] != 32767 ? (scores)[k] : ((scores)[k] = (d)->f44((d)->f40, (k))))

static void NBestDynProgProcess(NBESTDP *d, void **in)
{
    short *scores = d->f56;
    void *frame = in[1];

    if (frame != 0) {
        unsigned char flag = 1;
        unsigned int n = d->f60;
        short base = d->f274;
        unsigned int i;

        if (in[0] != 0)
            flag = *(unsigned char *)in[0];
        d->f284 += base;
        for (i = 0; i < n; i++)
            scores[i] = 32767;
        d->f48(d->f40, scores, frame, flag, base);
        d->f176++;
        if (d->pass(d) != 0) {
            _tosErrorLog(d, 119);
            d->process = IdleProcess;
            if (_tosControl(d, 11, 0, 0) != 0)
                return;
        }
        if (d->f176 >= d->f96)
            ExpandGarbageArrays(d);
        d->f100[d->f176 + 1] = d->f100[d->f176] + SCORE(d, scores, d->f164) + base;
        d->f104[d->f176 + 1] = d->f104[d->f176] + SCORE(d, scores, d->f168) + base;
        if (d->f172 != 0 && WordEnded(d)) {
            d->process = IdleProcess;
            _tosControl(d, 8, (void *)(unsigned int)d->f176, 0);
        }
    }
}

/* the N best word sequences: the best backtraces (sorted, best first) made relative to the best,
 * each turn's best backtraced into a result and moved back past the ones now better (its trace
 * advanced to the next path), the results chained at +92; 1 when there is one. SOCOM 1's body
 * (the same 0x1C4): the count set before the scores are made relative (the target's early `li
 * s4,1`), the moved trace's score in a local (the shifting stores may alias it: reloaded each pass
 * otherwise), and the turns a `for` with its test alone -- a `while` there gets its exit test
 * copied to the top (8 bytes short), the target jumps to the test at the bottom */
int FindBestRecognised(NBESTDP *d)
{
    NBTRACE **bt;
    NBTRACE *top;
    NBRESULT *first, *r;
    unsigned int n, i, j, k;
    short best, score;
    int ret = 0;

    n = FindBestBacktraces(d, &bt);
    if (n != 0) {
        k = 1;
        best = bt[0]->score;
        for (i = 0; i < n; i++)
            bt[i]->score -= best;
        first = BacktraceWordsParBT(d, (NBTOKEN *)bt[0]);
        r = first;
        if (first != 0) {
            for (; k < d->nresults; ) {
                top = bt[0];
                score = top->score;
                for (j = 1; j < n && bt[j]->score < score; j++)
                    bt[j - 1] = bt[j];
                bt[j - 1] = top;
                k++;
                r->next = BacktraceWordsParBT(d, (NBTOKEN *)bt[0]);
                r = r->next;
                if (r == 0)
                    break;
            }
        }
        if (r != 0)
            r->next = 0;
        d->results = first;
        heap_Free(d->tos->heap, bt);
        if (first != 0)
            ret = 1;
    } else
        d->results = 0;
    return ret;
}

/* the block's commands: 26 and 27 its parameters; 28 and 30 the active states (a list or all of them set, or cleared), with each
 * word that wants all its states (flag 0x80 clear) kept, the per-word lists rebuilt; 34 the language's counts; 35 the context; 36 the
 * scorer; 3, 4, 5 and 255 the block's life. SOCOM 1's body, SOCOM II's unrolls (round 41c). */
typedef struct NBPARAMS {
    unsigned int maxframes;             /* +0 */
    unsigned int nresults;              /* +4 */
    int f8;                             /* +8 */
    unsigned int beam10;                /* +12 */
    int f16;                            /* +16 */
} NBPARAMS;

typedef struct NBLANG {
    char pad0[24];
    int (*f24)(struct NBLANG *l);       /* +24 */
    char pad28[36];
    int (*f64)(struct NBLANG *l);       /* +64 */
    int (*f68)(struct NBLANG *l);       /* +68 */
} NBLANG;

extern int qQueueControl(void *q, unsigned char cmd, int arg, int n);
extern void tosBaseBlockDestruct(void *b);
extern void *memset(void *dst, int c, unsigned int n);

static int ControlNBestDynProgProcess(NBESTDP *d, unsigned char cmd, void *arg, int arg2)
{
    switch (cmd) {
    case 27: {
        NBPARAMS *p = arg;

        p->maxframes = d->maxframes;
        p->beam10 = d->beam10;
        p->nresults = d->nresults;
        p->f8 = d->f172 != 0;
        break;
    }
    case 37:
        return qQueueControl(d->inputs->q, 7, d->inputs->reader, (int)arg);
    case 38:
        return qQueueControl(d->inputs->q, 6, d->inputs->reader, 0);
    case 4: {
        d->process = IdleProcess;
        if (FindBestRecognised(d))
            return _tosControl(d, 9, &d->results, 0) == 0;
        else
            return _tosControl(d, 10, 0, 0) == 0;
    }
    case 3:
        InitViterbi(d);
        d->process = NBestDynProgProcess;
        break;
    case 5:
        d->process = IdleProcess;
        break;
    case 34: {
        NBLANG *l = arg;

        d->f60 = l->f24(l);
        d->f164 = (unsigned short)l->f64(l);
        d->f168 = (unsigned short)l->f68(l);
        break;
    }
    case 35:
        func_004EC130(d);
        if (InitDynprogContext(d, arg) != 0)
            return 0;
        break;
    case 36: {
        void **a = arg;

        d->f48 = a[2];
        d->f44 = a[1];
        d->f52 = a[3];
        d->f40 = a[0];
        break;
    }
    case 28: {
        unsigned int *st = arg;
        int n = arg2;
        unsigned short *flags = d->flags;
        unsigned char *bits = d->active;
        int i;

        if (st == 0) {
            if (arg2 != 0) {
                memset(bits, 255, d->f72 - 1);
                bits[d->f72 - 1] = 255 << (-(d->f68 & 7) & 7);
            } else {
                memset(bits, 0, d->f72);
            }
        } else {
            if (n > 0)
                memset(bits, 0, d->f72);
            else
                n = -n;
            for (i = 0; i < n; i++)
                bits[st[i] >> 3] |= 128 >> (st[i] & 7);
        }
        n = d->f68;
        for (i = 0; i < n; i++) {
            if ((flags[i] & 0x80) == 0)
                bits[i >> 3] |= 128 >> (i & 7);
        }
        {
            int *out = d->f136;
            int **ws = d->wstates;
            int *offs = d->offsets, *o = offs;
            unsigned char *b = d->active;
            unsigned int nw = d->f68;
            unsigned int w, k, end;

            for (w = 0; w < nw; w++) {
                *ws++ = out;
                k = *o++;
                end = *o;
                for (; k < end; k++) {
                    int s = offs[k];

                    if (b[s >> 3] & (128 >> (s & 7)))
                        *out++ = s;
                }
            }
            *ws = out;
        }
        InitViterbi(d);
        break;
    }
    case 30: {
        unsigned int *st = arg;
        unsigned int n = -arg2;
        unsigned short *flags = d->flags;
        unsigned char *bits = d->active;
        unsigned int i;

        if (st == 0) {
            memset(bits, 0, d->f72);
        } else {
            for (i = 0; i < n; i++)
                d->active[st[i] >> 3] &= ~(128 >> (st[i] & 7));
        }
        n = d->f68;
        for (i = 0; i < n; i++) {
            if ((flags[i] & 0x80) == 0)
                bits[i >> 3] |= 128 >> (i & 7);
        }
        {
            int *out = d->f136;
            int **ws = d->wstates;
            int *offs = d->offsets, *o = offs;
            unsigned char *b = d->active;
            unsigned int nw = d->f68;
            unsigned int w, k, end;

            for (w = 0; w < nw; w++) {
                *ws++ = out;
                k = *o++;
                end = *o;
                for (; k < end; k++) {
                    int s = offs[k];

                    if (b[s >> 3] & (128 >> (s & 7)))
                        *out++ = s;
                }
            }
            *ws = out;
        }
        InitViterbi(d);
        break;
    }
    case 29:
        *(unsigned char **)arg = d->active;
        *(unsigned int *)arg2 = d->f72;
        break;
    case 26: {
        NBPARAMS p = *(NBPARAMS *)arg;

        if (d->maxframes != p.maxframes)
            d->maxframes = p.maxframes;
        d->beam10 = p.beam10;
        d->beam = p.beam10 / 10;
        d->nresults = p.nresults;
        if (p.f8 != 0)
            d->f172 = 1;
        else
            d->f172 = 0;
        break;
    }
    case 39:
        break;
    case 255: {
        BTSTACK *st, *pr;
        DPTOS *tos;

        CollectDummyMemory(d);
        func_004EC130(d);
        st = d->stack;
        tos = d->tos;
        while (st != 0) {
            pr = st;
            st = st->next;
            FREE_REC(tos, 3, pr);
        }
        d->stack = 0;
        tosBaseBlockDestruct(d);
        break;
    }
    default:
        return 0;
    }
    return 1;
}

extern void *tosBaseBlockConstruct(void *tos, unsigned char id, int ninputs, int noutputs, int (*process)(), int (*init)(),
                                   int (*control)(), unsigned int size);

void *ConstructNBestDynProg(void *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 2, 0, (int (*)())NBestDynProgProcess, (int (*)())InitNBestDynProgProcess,
                                 (int (*)())ControlNBestDynProgProcess, sizeof(NBESTDP));
}

/* the qsort comparator of the backtraces (pointers to them): ascending scores */
typedef struct BTSCORE {
    void *f0;
    short score;                        /* +4 */
} BTSCORE;

static int ScoreCompare(BTSCORE **a, BTSCORE **b)
{
    if ((*a)->score > (*b)->score)
        return 1;
    if ((*a)->score < (*b)->score)
        return -1;
    return 0;
}
