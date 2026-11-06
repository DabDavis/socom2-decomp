/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* isowdp: the recogniser's iso-word token DP block (OVL2 0x4E8AE8-0x4EB5E8, SOCOM 1's object at
 * 0x356970: InitIsoWordTokenDpContext, FreeIsoWordTokenDp, InitNewUtterance, ExtendNodeWheel,
 * RePruneToPho, FrameDPacoust, ExpandGarbageArrays, IsoWordTokenDpProcess, GetNBest,
 * ControlIsoWordTokenDp, ConstructIsoWordTokenDp, InitIsoWordTokenDpProcess, CalcNrNodes,
 * IdleProcess, ScoreCompare -- all but RePruneToPho and the constructor LOCAL there), ScanSoft's
 * speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops
 * (README "The GCC front"). Names are SOCOM 1's, the object's order and sizes agreeing; a static
 * another object also defines is NAME@isowdp in the map (the N-best object has its own
 * ExpandGarbageArrays, ScoreCompare and IdleProcess). SOCOM II's block record is SOCOM 1's with
 * a word more before the node wheel (its fields from SOCOM 1's +68 on are 4 higher). The tokens'
 * nodes are blocks of the scheduler's small-block allocator (fastallo.c), given back to its free
 * lists by size; a tree node's word indexes a table of its states' offsets. Only the functions
 * written so far are here, in image order: InitIsoWordTokenDpContext (round 32c),
 * FreeIsoWordTokenDp, InitNewUtterance, ExtendNodeWheel (OPEN, diagnosed in place),
 * RePruneToPho (round 40c), FrameDPacoust (round 39c, OPEN), ExpandGarbageArrays,
 * IsoWordTokenDpProcess (round 32c), GetNBest and ControlIsoWordTokenDp
 * (round 31c), ConstructIsoWordTokenDp,
 * InitIsoWordTokenDpProcess, CalcNrNodes, IdleProcess, ScoreCompare. CalcNrNodes reads the child count before its depth test (the
 * target's load in the test's delay slot) and steps its child pointer in the for (the unrolled
 * copies at +12, +24, +36 off one register; `c++` in the call gives a pointer per copy). */

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

/* the scheduler, as far as the block reads it */
typedef struct ISOTOS {
    char pad0[80];
    void *heap;                         /* +80 */
    FASTALLO fa;                        /* +84 */
} ISOTOS;

typedef int (*TOSFUNC)();

/* a block's reader in an input queue (tosblk.h's QIN) */
typedef struct QIN {
    void *q;
    unsigned short reader;              /* +4 */
    unsigned short size;                /* +6 */
} QIN;

/* a token's node: a small block of the allocator, len + 2 halfwords */
typedef struct NODE {
    unsigned char f0;                   /* +0 */
    unsigned char len;                  /* +1 */
    short f2[1];                        /* +2: a score per state */
} NODE;

/* a token as the N-best sort sees it */
typedef struct TOKEN {
    void *f0;
    int score;                          /* +4 */
} TOKEN;

/* a token on the wheel */
typedef struct WTOKEN {
    void *f0;                           /* +0 */
    NODE *node;                         /* +4 */
    unsigned short word;                /* +8 */
    unsigned short f10;                 /* +10: 0 at a word's end */
} WTOKEN;

/* the node wheel: a ring of token pointers */
typedef struct WHEEL {
    WTOKEN **base;                      /* +0 */
    WTOKEN **head;                      /* +4 */
    WTOKEN **tail;                      /* +8 */
    WTOKEN **end;                       /* +12: the last slot */
    int size;                           /* +16 */
} WHEEL;

/* the host's callbacks (command 36): a frame scored into the buffer (the input's flag and the
 * frame's offset passed on), the buffer itself, one phoneme's score when the buffer has none */
typedef void (*ISOSCOREFN)(int arg, short *scores, void *frame, unsigned char last, short off);
typedef short *(*ISOBUFFN)(int arg);
typedef int (*ISOPHONFN)(int arg, int from, int to);

typedef struct ISOWDP {
    ISOTOS *tos;                        /* +0: the block's head (tosblk.h) */
    unsigned char id;                   /* +4 */
    char pad5[3];
    unsigned char ninputs;              /* +8 */
    unsigned char noutputs;             /* +9 */
    char pad10[2];
    QIN *inputs;                        /* +12 */
    void *outputs;                      /* +16 */
    TOSFUNC init;                       /* +20 */
    TOSFUNC process;                    /* +24 */
    TOSFUNC control;                    /* +28 */
    unsigned char state;                /* +32 */
    char pad33[7];
    int f40;                            /* +40: command 36's four words: the host's argument */
    ISOSCOREFN f44;                     /* +44: the host's scorer of a frame */
    ISOBUFFN f48;                       /* +48: the host's score buffer, at a new context and each frame */
    short *f52;                         /* +52: the buffer at a new context */
    ISOPHONFN f56;                      /* +56: the host's score of one phoneme */
    int nphen;                          /* +60: the language's phonemes */
    int *wordstates;                    /* +64: each word's first state */
    short nunits;                       /* +68: the language's speech units (SOCOM II's word more) */
    short pad70;
    WHEEL wheel;                        /* +72 */
    short f92;                          /* +92 */
    short f94;                          /* +94 */
    int f96;                            /* +96 */
    int f100;                           /* +100 */
    int nnodes;                         /* +104: the context tree's nodes, the root's included */
    void *f108;                         /* +108 */
    int f112;                           /* +112 */
    void *f116;                         /* +116 */
    int f120;                           /* +120 */
    int pad124;
    WTOKEN *f128;                       /* +128 */
    int f132;                           /* +132 */
    WTOKEN *start;                      /* +136 */
    int pad140[3];
    unsigned short f152;                /* +152 */
    short pad154;
    unsigned int f156;                  /* +156: profile 6 */
    unsigned int f160;                  /* +160: ten times that */
    int f164;                           /* +164: profile 7 */
    unsigned char f168;                 /* +168 */
    char pad169[3];
    int f172;                           /* +172 */
    void *f176;                         /* +176: profile 7's eight-byte records */
    unsigned int ngarbage;              /* +180 */
    int *garbage1;                      /* +184 */
    int *garbage2;                      /* +188 */
    int f192;                           /* +192 */
    int garbage;                        /* +196: the single-word garbage phoneme */
    int sgarbage;                       /* +200: the sentence garbage phoneme */
    int f204;                           /* +204: the context's two words (SOCOM II's) */
    int f208;                           /* +208 */
} ISOWDP;                               /* 212 (ConstructIsoWordTokenDp's) */

/* tos.c */
void *tosBaseBlockConstruct(void *tos, unsigned char id, unsigned char ninputs,
                            unsigned char noutputs, TOSFUNC process, TOSFUNC init,
                            TOSFUNC control, unsigned int size);
unsigned int _tosGetProfileU32(void *b, unsigned short key, unsigned int def);

/* fastallo.c */
extern int *fastallo_AllocateMemoryFromChunk(FASTALLO *fa, unsigned short n);

/* OVL1's heap (heap.c) */
void *heap_Alloc(void *heap, unsigned int size);
void heap_Free(void *heap, void *p);

void *memcpy(void *dst, const void *src, unsigned int n);
void *memset(void *dst, int c, unsigned int n);

/* not written yet: the map's rows */
int FrameDPacoust(ISOWDP *e);
static void IsoWordTokenDpProcess(ISOWDP *e, void **in);
static int ControlIsoWordTokenDp(struct ISOWDP *e, unsigned char cmd, void *arg, void *arg2);
int ExtendNodeWheel(ISOWDP *e, WTOKEN ***a, WTOKEN ***b);

static void FreeIsoWordTokenDp(ISOWDP *e);
static int InitNewUtterance(ISOWDP *e);
static int ExpandGarbageArrays(ISOWDP *e);
static int InitIsoWordTokenDpProcess(ISOWDP *e);
static int CalcNrNodes(void *n, int depth);
static void IdleProcess(void);
static int ScoreCompare(const TOKEN *a, const TOKEN *b);
extern void qsort(void *base, unsigned int n, unsigned int size, int (*cmp)(const void *, const void *));

/* a node given back to the allocator's free list of its size (len + 2 halfwords, in words) */
#define FREENODE(fa, n)                                                 \
    {                                                                   \
        int **list = &(fa)->free[((n)->len * 2 + 4 + 3) >> 2];          \
        *(int **)(n) = *list;                                           \
        *list = (int *)(n);                                             \
    }

/* a block of n words from the allocator: its free list of that size first */
static __inline__ void FastAlloc(ISOTOS *tos, int **pp, unsigned int n)
{
    int **list = &tos->fa.free[n];

    if (*list != 0) {
        *pp = *list;
        *list = *(int **)*list;
    } else
        *pp = fastallo_AllocateMemoryFromChunk(&tos->fa, n);
}

/* the wheel's next slot */
#define NEXTSLOT(w, p)  ((p) < (w)->end ? (p) + 1 : (w)->base)

extern void *heap_Calloc(void *heap, int n, int size);

/* the frame buffer sized for the given frames (frames / 32 + 1 words, at most 10000; the last frame
 * kept at +152) and grown to it: 1 when the heap has none. The helper reads the block's scheduler
 * and does the sizing itself: the target loads e->tos ahead of the sizing (written at the call
 * site, the load issues after it: 5 instructions off) */
static __inline__ int AllocFrames(ISOWDP *e, int frames)
{
    ISOTOS *tos = e->tos;
    unsigned int n = frames / 32 + 1;

    if (n > 10000)
        n = 10000;
    e->f152 = n * 32 - 1;
    if (e->f112 < n) {
        if (e->f108 != 0)
            heap_Free(tos->heap, e->f108);
        e->f108 = heap_Alloc(tos->heap, n * 4);
        if (e->f108 == 0)
            return 1;
        e->f112 = n;
    }
    return 0;
}

/* the N-best's records re-allocated for k of them: 1 when the heap has none. The helper reads the
 * block's scheduler after its count (the target's k load ahead of e->tos's at both sites) */
static __inline__ unsigned char AllocNBest(ISOWDP *e, unsigned int k)
{
    ISOTOS *tos = e->tos;

    if (e->f176 != 0)
        heap_Free(tos->heap, e->f176);
    e->f176 = heap_Alloc(tos->heap, k * 8);
    if (e->f176 == 0)
        return 1;
    return 0;
}

/* a new context (command 35): its word tree's root kept as the start token and its nodes counted
 * (the root's own one added), SOCOM II's two words taken when given, the pass made the process,
 * profile 9 kept, the frame buffer sized from profile 11 (its frames / 32 + 1, at most 10000), the
 * wheel made, 300 halfwords allocated, the utterance started and the N-best's records allocated;
 * then the host's function called on its argument. 1 on any failure. SOCOM 1's body (round 32c)
 * with the two words; the children stepped in the for, as CalcNrNodes steps them; the two
 * allocations' helpers read e->tos themselves (the function's own tos, loaded at the entry, serves
 * only the 300 halfwords), the frames helper sizing its buffer from the profile's frames. */
int InitIsoWordTokenDpContext(ISOWDP *e, WTOKEN *root, int *words)
{
    ISOTOS *tos = e->tos;
    WTOKEN *child;
    unsigned int i, n;
    int total;

    e->start = root;
    if (words != 0) {
        e->f204 = words[0];
        e->f208 = words[1];
    }
    n = root->f10;
    total = 1;
    child = root->f0;
    for (i = 0; i < n; i++, child++)
        total += CalcNrNodes(child, 3);
    e->nnodes = total;
    e->process = (TOSFUNC)IsoWordTokenDpProcess;
    e->f100 = _tosGetProfileU32(e, 9, 3000);
    if (AllocFrames(e, (short)_tosGetProfileU32(e, 11, 5000)))
        return 1;
    if (ExtendNodeWheel(e, 0, 0) != 0)
        return 1;
    e->f116 = heap_Calloc(tos->heap, 300, 2);
    if (e->f116 == 0)
        return 1;
    e->f120 = 300;
    if (InitNewUtterance(e) != 0)
        return 1;
    if (AllocNBest(e, e->f164))
        return 1;
    e->f52 = e->f48(e->f40);
    return 0;
}

static void FreeIsoWordTokenDp(ISOWDP *e)
{
    ISOTOS *tos = e->tos;
    WTOKEN **p;

    if (e->wheel.base != 0) {
        for (p = e->wheel.head; p != e->wheel.tail; p = NEXTSLOT(&e->wheel, p)) {
            FREENODE(&tos->fa, (*p)->node);
            (*p)->node = 0;
        }
        heap_Free(tos->heap, e->f108);
        e->f108 = 0;
        e->f112 = 0;
        heap_Free(tos->heap, e->f116);
        e->f116 = 0;
        e->f120 = 0;
        heap_Free(tos->heap, e->wheel.base);
        e->wheel.base = 0;
        e->wheel.size = 0;
    }
    if (e->ngarbage != 0) {
        heap_Free(tos->heap, e->garbage1);
        heap_Free(tos->heap, e->garbage2);
        e->ngarbage = 0;
        e->garbage2 = 0;
        e->garbage1 = 0;
    }
    if (e->f176 != 0) {
        heap_Free(e->tos->heap, e->f176);
        e->f176 = 0;
    }
}

/* a new utterance: the wheel's tokens give their nodes back, the root token takes a node of its
 * word's size and the wheel's first slot. The word's state count is the halfword distance from
 * its first state to the next word's (the table holds offsets; as addresses the base cancels --
 * the target's `sll 1; sra 1` is the scaled difference divided back). The first state is a
 * pointer of its own, taken first: the next word's entry is then the address's last use, and the
 * first scheduling pass (which prefers the insn that frees a register) loads it first, as the
 * target does (one expression loads the word's own entry first). The closing stores end with
 * `f96 = f100`, the block's last use of e: it frees two registers and is stored as soon as its
 * load lands, the other three value stores follow in source order and the zero stores, which
 * free nothing, after them -- the target's order (round 17c's 120 orders kept that store first).
 * The node's size is unsigned (sizeof: the target's srl), the allocation an inline storing
 * through the field's address taken before the call (the target's s1 across it), the free list
 * indexed from the scheduler itself (tos + 4n + 104, not through &tos->fa). */
static int InitNewUtterance(ISOWDP *e)
{
    ISOTOS *tos = e->tos;
    WTOKEN **q, **p, **s;
    short *states = (short *)e->wordstates + e->wordstates[e->start->word];
    int n = (short *)e->wordstates + e->wordstates[e->start->word + 1] - states - 3;
    unsigned int size;

    for (s = e->wheel.head; s != e->wheel.tail; s = NEXTSLOT(&e->wheel, s)) {
        FREENODE(&tos->fa, (*s)->node);
        (*s)->node = 0;
    }
    p = e->wheel.head;
    size = (sizeof(NODE) + n * sizeof(short) + 3) >> 2;
    e->f192 = -1;
    *p = e->start;
    FastAlloc(tos, (int **)&e->start->node, size);
    e->start->node->len = n;
    e->start->node->f2[0] = 0;
    e->start->node->f0 = 0;
    p = NEXTSLOT(&e->wheel, p);
    if (p == e->wheel.head) {
        if (ExtendNodeWheel(e, &q, &p) != 0)
            return 1;
    }
    e->wheel.tail = p;
    e->f128 = e->start;
    e->f92 = e->f152;
    e->f94 = 0;
    e->f132 = 0;
    e->f172 = 0;
    e->f96 = e->f100;
    return 0;
}

/* the node wheel made (256 slots) or grown by 200: the ring copied into a new block, its pointers
 * rebased (*a and *b both to *b's slot), then the 200 new slots opened at *a -- the ring's part
 * before *a moved up into them when *b was 200 slots in or more, else the first 200 wrapped round
 * to the new end and the rest moved down. SOCOM 1's body, the wheel 4 bytes on. OPEN (round 28c,
 * the right size, every block the target's but the first allocation's four closing stores --
 * 0x4E90E8's tail, end, head, size; 24 orders tried, none closer than a word): the registers
 * rotate through s0-s2. The target keeps the null test's load of the base apart from `old` (a
 * `move s0,v0` in the grow path), and old then outranks p and w in global-alloc (s0, s1, s2);
 * here CSE gives the test and old one pseudo, live from the entry, which ranks below both (p s0,
 * w s1, old s2). The test through e->wheel, old read later or through e, the declarations in
 * three orders, old for the copy's and free's base: no change. */
int ExtendNodeWheel(ISOWDP *e, WTOKEN ***a, WTOKEN ***b)
{
    WHEEL *w = &e->wheel;
    ISOTOS *tos = e->tos;
    WTOKEN **old, **p, **s, **d;
    int n, hi, ti, bi;

    if (w->base == 0) {
        n = 256;
        w->base = heap_Alloc(tos->heap, n * sizeof(WTOKEN *));
        if (w->base == 0)
            return 1;
        w->head = w->base;
        w->tail = w->base;
        w->end = w->base + n - 1;
        w->size = n;
        return 0;
    }
    old = w->base;
    n = w->size + 200;
    hi = w->head - old;
    ti = w->tail - old;
    bi = *b - old;
    p = heap_Alloc(tos->heap, n * sizeof(WTOKEN *));
    if (p == 0)
        return 1;
    memcpy(p, w->base, w->size * sizeof(WTOKEN *));
    heap_Free(tos->heap, w->base);
    w->base = p;
    if (old != p) {
        w->head = p + hi;
        w->tail = p + ti;
        *a = *b = p + bi;
    }
    w->end = w->base + n - 1;
    w->size = n;
    if (bi < 200) {
        for (s = w->base, d = w->base + n - 200; s < *a; )
            *d++ = *s++;
        if (w->head < *a)
            w->head += n - 200;
        if (w->tail < *a)
            w->tail += n - 200;
        *a += n - 200;
    } else {
        for (s = w->base, d = w->base + n - 200; d <= w->end; )
            *d++ = *s++;
        for (s = w->base + 200, d = w->base; s < *a; )
            *d++ = *s++;
        if (w->head < w->base + 200)
            w->head += n - 200;
        else if (w->head < *a)
            w->head -= 200;
        if (w->tail < w->base + 200)
            w->tail += n - 200;
        else if (w->tail < *a)
            w->tail -= 200;
        *a -= 200;
    }
    return 0;
}

int RePruneToPho(ISOWDP *e, WTOKEN ***out, int *count, int n, int best);
/* the wheel pruned back when it grew past the frame's budget: each token's best score histogrammed
 * from the best (bins of 32), the threshold found by narrowing the bin a hundredfold twice over, the
 * tokens at or past it dropped from the ring and their nodes given back, the scores kept for the next
 * re-prune when few enough went. SOCOM 1's body (round 40c, 870 words). Measured, each moved the
 * allocation or the layout to the target's:
 * - the histogram's and the scores' tables through pointers of their own (h, sc: `lw v1,4(s7)`), the
 *   ring's base through w, never e -- e's references drop below h's and w's, and it is spilled;
 * - the retry a `while` on the histogram's test with the `step > 1` test an inner break: the first is
 *   the exit expand_end_loop rolls to the bottom (the entry `b`), its label keeps jump.c from copying
 *   it above the loop, and h's, sum's and step's in-loop references count double (h: 22 refs over 490,
 *   it passes sc and w);
 * - `size = cnt * 2` set before the memset, inside the loop: the insn is in the same block as its
 *   use, and far enough ahead (life >= 4) for loop.c to find it desirable, so it is hoisted (slot 32);
 *   set before the loop PRE reuses it for the compaction's `o + cnt`, which the target recomputes;
 * - one set of walkers s, se, o for all five loops (t0, t3, t1 throughout); `nx` once for the first
 *   ring walk, a second for the thinning; tk shared by both walks (a2: global, not v0) but the
 *   thinning's node a pseudo of its own;
 * - the first double split round the three small assignments (the reorg fills the first dpmul's slot
 *   with tos's store), the rest of the small assignments right after the second conversion;
 * - `target = 1; flag = 1; if (t != 0) target = t;`: CSE stores flag from target's register. */
typedef struct ISOHIST {
    int *p;
    unsigned int n;
} ISOHIST;

typedef struct ISOSCORES {
    short *p;
    unsigned int cap;
    int n;
} ISOSCORES;

int RePruneToPho(ISOWDP *e, WTOKEN ***out, int *count, int n, int best)
{
    ISOTOS *tos;
    unsigned int prev;
    int cnt;
    int flag;
    ISOSCORES *sc;
    int size;
    unsigned int target;
    unsigned int sum = 0;
    double total, dn, x2;
    unsigned int bins;
    ISOHIST *h;
    WHEEL *w;
    int *hist;
    unsigned int k, x;
    float base, step, scale, top;
    int thresh;
    short *s, *se, *o;
    WTOKEN **a, **nx;
    NODE *nd;
    WTOKEN *tk;

    cnt = *count;
    tos = e->tos;
    prev = 0;
    total = (double)e->f96;
    total *= 0.95;
    dn = (double)cnt;
    flag = 0;
    h = (ISOHIST *)&e->f108;
    w = &e->wheel;
    sc = (ISOSCORES *)&e->f116;
    total = total * dn;
    x2 = (double)n;
    target = (unsigned int)(total / x2);
    if ((double)target < dn * 0.5) {
        unsigned int t = (unsigned int)(dn * 0.5);

        target = 1;
        flag = 1;
        if (t != 0)
            target = t;
    }
    bins = (unsigned int)(e->f92 - best) >> 5;
    hist = h->p;
    x = bins + 1;
    if (h->n < x) {
        int *hh;

        h->n = x;
        hh = heap_Alloc(tos->heap, x * 4);
        if (hh == 0)
            return 1;
        hist = hh;
        heap_Free(tos->heap, h->p);
        h->p = hist;
    }
    memset(hist, 0, bins * 4 + 4);
    if (sc->cap < cnt) {
        short *s = heap_Alloc(tos->heap, cnt * 2);

        if (s == 0)
            return 1;
        memcpy(s, sc->p, sc->cap * 2);
        heap_Free(tos->heap, sc->p);
        sc->p = s;
        sc->cap = cnt;
    }
    {
        if (sc->n != 0) {
            o = sc->p + sc->n;
            s = sc->p;
            for (; s < o; s++)
                hist[(*s - best) >> 5]++;
            if (w->end - w->tail >= sc->n)
                a = w->tail + sc->n;
            else
                a = w->base + sc->n - (w->end - w->tail) - 1;
        } else {
            a = w->tail;
            o = sc->p;
        }
        while (a != *out) {
            int min;

            tk = *a;
            nd = tk->node;

            s = nd->f2;
            min = *s;
            se = s + nd->f0;
            for (s = nd->f2 + 1; s <= se; s++)
                if (*s < min)
                    min = *s;
            hist[(min - best) >> 5]++;
            *o++ = min;
            nx = a + 1;
            if (a >= w->end)
                nx = w->base;
            a = nx;
        }
    }
    k = 0;
    if (target != 0) {
        int *hh = hist;

        do {
            unsigned int y = *hh++;

            k++;
            prev = sum;
            sum += y;
        } while (k <= bins && sum < target);
    }
    base = 0.0f;
    step = 32.0f;
    scale = 1.0f;
    while ((double)(unsigned int)hist[k - 1] > (double)target * 0.2) {
        if (!(step > 1.0f))
            break;
        base += step * (float)(k - 1);
        top = base + step;
        step *= 0.01f;
        scale *= 100.0f;
        if (h->n < 100) {
            int *hh = heap_Alloc(tos->heap, 400);

            if (hh == 0)
                return 1;
            heap_Free(tos->heap, h->p);
            h->p = hh;
            hist = hh;
            h->n = 100;
        }
        size = cnt * 2;
        memset(hist, 0, 400);
        {
            se = (short *)((char *)sc->p + size);
            o = sc->p;
            for (; o < se; o++) {
                float f = (float)(*o - best);

                if (base <= f && f < top)
                    hist[(short)(int)(scale * (f - base)) >> 5]++;
            }
        }
        k = 0;
        sum = prev;
        if (prev < target) {
            int *hh = hist;

            do {
                unsigned int y = *hh++;

                k++;
                prev = sum;
                sum += y;
            } while (k < 100 && sum < target);
        }
    }
    thresh = best + (int)(base + step * ((float)k - (float)(sum - target) / (float)(sum - prev)));
    if (thresh < (best < -33792 ? -33792 : best)) {
        thresh = best + 1;
        if (thresh < -33792)
            thresh = -33792;
    }
    e->f92 = thresh;
    {
        WTOKEN **o2, **nx2;

        o2 = a = w->tail;
        o = sc->p;
        while (a != *out) {
            if (*o++ < thresh) {
                *o2 = *a;
                nx2 = o2 + 1;
                if (o2 >= w->end)
                    nx2 = w->base;
                o2 = nx2;
            } else {
                int **list;
                NODE *nd2;

                tk = *a;
                nd2 = tk->node;
                list = (int **)((char *)tos + ((nd2->len * 2 + 7) & 0x3FC) + 104);
                *(int **)nd2 = *list;
                *list = (int *)tk->node;
                tk->node = 0;
                (*count)--;
            }
            nx2 = a + 1;
            if (a >= w->end)
                nx2 = w->base;
            a = nx2;
        }
        if (w->end - w->tail >= *count)
            *out = w->tail + *count;
        else
            *out = w->base + *count - (w->end - w->tail) - 1;
    }
    if (flag != 0 || (double)*count > (double)target * 1.0526315789473684) {
        o = sc->p;
        se = o + cnt;
        for (s = o; s < se; s++)
            if (*s < thresh)
                *o++ = *s;
        sc->n = *count;
    } else
        sc->n = 0;
    return 0;
}


/* the garbage arrays grown by 150 entries (each array holds ngarbage + 1 words) */

/* one frame over the wheel: each token's states from the last back, its followers entered from its last state,
 * a token surviving onto the ring again, its node freed when nothing beat the threshold; the threshold eased by
 * 100 a frame up to +152, a re-prune when the ring would outgrow +96. SOCOM 1's body. OPEN (round 39c, written:
 * 586 words of 587, the frame's slots and the whole layout the target's). Measured shapes, each moved it closer:
 * `hd` a register twin of the address-taken `in`; a guard and a do-while with `done = 0` inside the guard (the
 * store then lands in the entry test's delay slot); `kk = k * 2` a variable in both arms (PRE carries it over the
 * join) and the sum `(ws + kk) + ws[word]`; `(x - nun - 1) & 0xFFFF` not a halfword cast (`addiu; andi`, where a
 * cast goes through `li at,0xffff`); a slot pointer `ps` with the host's result stored through it and read back
 * (CSE forwards it narrowed: the target's sll/sra); `nn != 0` first; nun an unsigned short. Open: the target
 * re-reads t->word in the first arm (`lhu v0,8(s6)`) and keeps an `andi` on the follower's word in that arm, both
 * as if the arm began after a label in CSE1's view -- a bitfield gives the andi on the first read too (tried); so
 * registers from there on: v in s0 and ps in s1 (swapped here), the reloads' a3 for a2 (kk's global a2 is live
 * only in the target's shape). */
int FrameDPacoust(ISOWDP *e)
{
    WTOKEN **out, **in, **hd;
    int count;
    ISOTOS *tos;
    WTOKEN **last, **end;
    NODE *node;
    unsigned short *st;
    int best;
    int k, kept, n, done;
    short *scores;
    int *ws;
    int *f208;
    unsigned short *f204;
    unsigned short nun;
    int thresh;
    WTOKEN *chend;
    WHEEL *w;
    WTOKEN *t, *ch;
    unsigned short *lastst;
    short *p, *q;
    int kk;
    unsigned short word;
    unsigned int w2;
    int lim;
    int v;
    short *ps;
    int m, sc;
    unsigned int u;

    w = &e->wheel;
    best = 0x7BFF;
    tos = e->tos;
    scores = e->f48(e->f40);
    ws = e->wordstates;
    thresh = e->f92 + 100;
    e->pad124 = 0;
    f208 = (int *)e->f208;
    if ((short)e->f152 < thresh)
        thresh = (short)e->f152;
    f204 = (unsigned short *)e->f204;
    e->f92 = thresh;
    nun = e->nunits;
    last = w->tail;
    hd = w->head;
    end = w->end;
    if (hd < last)
        n = last - hd;
    else
        n = w->size - (hd - last);
    lim = e->f96;
    if (lim > 300)
        lim = 300;
    count = 0;
    in = hd;
    out = last;
    if (hd != last) {
    done = 0;
    do {
        t = *hd;
        kept = 0;
        done++;
        node = t->node;
        word = t->word;
        k = node->f0;
        if (word < nun) {
            kk = k * 2;
            st = (unsigned short *)((char *)ws + kk) + ws[t->word] + 2;
            lastst = (unsigned short *)ws + ws[t->word + 1] - 1;
        } else {
            int j = (word - nun - 1) & 0xFFFF;

            kk = k * 2;
            st = f204 + f208[j] + k;
            lastst = f204 + f208[j + 1] - 1;
        }
        p = (short *)((char *)node + kk) + 1;
        if (st == lastst && t->f10 != 0) {
            ch = t->f0;
            chend = ch + t->f10;
            do {
                w2 = ch->word;
                if (w2 < nun)
                    u = ((unsigned short *)ws + ws[(unsigned short)w2])[2];
                else
                    u = f204[f208[(w2 - nun - 1) & 0xFFFF]];
                m = *p;
                ps = &scores[u];
                sc = *ps;
                v = m + sc;
                if (sc == 0x7FFF) {
                    *ps = ((int (*)(int, unsigned int))e->f56)(e->f40, u);
                    v = m + *ps;
                }
                if (v < thresh) {
                    NODE *nn = ch->node;

                    if (nn != 0) {
                        if (v < nn->f2[0]) {
                            nn->f2[0] = v;
                            if (v < best) {
                                best = v;
                                e->f128 = ch;
                            }
                        }
                    } else {
                        int len;
                        unsigned int idx;

                        w2 = ch->word;
                        if (w2 < nun) {
                            int j = (unsigned short)w2;

                            len = ws[j + 1] - ws[j] - 3;
                        } else {
                            int j = (w2 - nun - 1) & 0xFFFF;

                            len = f208[j + 1] - f208[j] - 1;
                        }
                        idx = (unsigned int)(len * 2 + 7) >> 2;
                        FastAlloc(tos, (int **)&nn, idx);
                        nn->len = len;
                        ch->node = nn;
                        *out = ch;
                        nn->f0 = 0;
                        nn->f2[0] = v;
                        if (v < best) {
                            e->f128 = ch;
                            best = v;
                        }
                        count++;
                        out = out < end ? out + 1 : w->base;
                        if (out == in) {
                            if (ExtendNodeWheel(e, &out, &in) != 0)
                                return 0;
                            end = w->end;
                            last = w->tail;
                        }
                        if (count >= lim) {
                            int x = count * n / done;
                            int a = count + 200;

                            if (x >= e->f96) {
                                if (RePruneToPho(e, &out, &count, x, best) != 0)
                                    return 1;
                                thresh = e->f92;
                                a = count + 200;
                            }
                            if (a < 300)
                                a = 300;
                            lim = e->f96 < a ? e->f96 : a;
                        }
                    }
                }
                ch++;
            } while (ch < chend);
        }
        u = st[1];
        m = *p;
        ps = &scores[u];
        sc = *ps;
        v = m + sc;
        if (sc == 0x7FFF) {
            *ps = ((int (*)(int, unsigned int))e->f56)(e->f40, u);
            v = m + *ps;
        }
        if (v < thresh) {
            kept = 1;
            *out = t;
            node->f0++;
            if (v < best) {
                e->f128 = t;
                best = v;
            }
            count++;
            out = out < end ? out + 1 : w->base;
            if (out == in) {
                if (ExtendNodeWheel(e, &out, &in) != 0)
                    return 1;
                end = w->end;
                last = w->tail;
            }
            p[1] = v;
        }
        while (k != 0) {
            q = p - 1;
            u = st[0];
            m = *q < *p ? *q : *p;
            ps = &scores[u];
            sc = *ps;
            v = m + sc;
            if (sc == 0x7FFF) {
                *ps = ((int (*)(int, unsigned int))e->f56)(e->f40, u);
                v = m + *ps;
            }
            if (v < thresh) {
                if (kept == 0) {
                    kept = 1;
                    *out = t;
                    node->f0 = k;
                    count++;
                    out = out < end ? out + 1 : w->base;
                    if (out == in) {
                        if (ExtendNodeWheel(e, &out, &in) != 0)
                            return 1;
                        end = w->end;
                        last = w->tail;
                    }
                }
                *p = v;
                if (v < best) {
                    best = v;
                    e->f128 = t;
                }
            } else
                *p = 0x7BFF;
            p = q;
            k--;
            st--;
        }
        u = st[0];
        m = *p;
        sc = scores[u];
        v = m + sc;
        if (sc == 0x7FFF) {
            sc = ((int (*)(int, unsigned int))e->f56)(e->f40, u);
            v = m + (short)sc;
            scores[st[0]] = sc;
        }
        if (v < thresh) {
            if (kept == 0) {
                *out = t;
                node->f0 = 0;
                count++;
                out = out < end ? out + 1 : w->base;
                if (out == in) {
                    if (ExtendNodeWheel(e, &out, &in) != 0)
                        return 1;
                    end = w->end;
                    last = w->tail;
                }
            }
            *p = v;
            if (v < best) {
                e->f128 = t;
                best = v;
            }
        } else if (kept != 0)
            *p = 0x7BFF;
        else {
            NODE *f = t->node;
            int **fl = (int **)((char *)tos + (((unsigned int)f->len * 2 + 7) & 0x3FC) + 104);

            *(int **)f = *fl;
            *fl = (int *)t->node;
            t->node = 0;
        }
        if (count >= lim) {
            int x = count * n / done;
            int a = count + 200;

            if (x >= e->f96) {
                if (RePruneToPho(e, &out, &count, x, best) != 0)
                    return 1;
                thresh = e->f92;
                a = count + 200;
            }
            if (a < 300)
                a = 300;
            lim = e->f96 < a ? e->f96 : a;
        }
        hd = in < end ? in + 1 : w->base;
        in = hd;
    } while (hd != last);
    }
    w->head = hd;
    w->tail = out;
    e->f94 = best;
    return 0;
}

static int ExpandGarbageArrays(ISOWDP *e)
{
    int *a, *b;
    int err = 0;

    a = heap_Alloc(e->tos->heap, (e->ngarbage + 151) * sizeof(int));
    if (a == 0)
        err = 1;
    else {
        b = heap_Alloc(e->tos->heap, (e->ngarbage + 151) * sizeof(int));
        if (b == 0)
            err = 1;
        else {
            if (e->ngarbage != 0) {
                memcpy(a, e->garbage1, (e->ngarbage + 1) * sizeof(int));
                heap_Free(e->tos->heap, e->garbage1);
                memcpy(b, e->garbage2, (e->ngarbage + 1) * sizeof(int));
                heap_Free(e->tos->heap, e->garbage2);
            } else {
                *a = 0;
                *b = 0;
            }
            e->garbage1 = a;
            e->garbage2 = b;
            e->ngarbage += 150;
        }
    }
    return err;
}

/* the language's accessors (language.c's _LangVirtualTable), as command 34 hands them over */
typedef struct ISOLANG ISOLANG;
struct ISOLANG {
    void *data;
    void *s1[5];
    int (*GetNbrPhenome)(ISOLANG *l);                           /* +24, slot 6 */
    void *s7[9];
    int (*GetSingleWordGarbagePhenome)(ISOLANG *l);             /* +64, slot 16 */
    int (*GetSentenceGarbagePhenome)(ISOLANG *l);               /* +68, slot 17 */
    void *s18[2];
    short (*GetNbrSpeechUnit)(ISOLANG *l);                      /* +80, slot 20 */
    void *s21[21];
    int *(*GetpSpeechUnit)(ISOLANG *l);                         /* +168, slot 42 */
};

/* commands 26, 27 and 36's four words */
typedef struct ISOPARAM {
    int f0;
    unsigned int f4;                    /* +4: command 26's N-best count */
    int f8;
    unsigned int f12;                   /* +12 */
} ISOPARAM;

extern unsigned char qQueueControl(void *q, int what, int reader, void *arg);
extern unsigned char _tosControl(void *log, int id, int a, int b);
extern void tosBaseBlockDestruct(void *b);

/* a frame's pass (the block's process; in[0] the input's flag, in[1] the frame): nothing without a
 * frame; the host's buffer cleared to 0x7FFF for every phoneme, the frame counted and its offset
 * summed, the frame scored by the host (the flag, 1 without one: the last frame), the pass's node
 * budget set (on the start token alone before the last frame, the smaller of the budget and the
 * tree's nodes; else, the tree's nodes reached, the budget and the frame limit back), the DP made,
 * the garbage arrays grown when full and both garbage paths extended by their phonemes' scores (a
 * missing score asked of the host and kept), the frames since a word's end counted, and the end
 * of the utterance sent once enough have passed with the flag at +168 set. SOCOM 1's body (round
 * 32c), the budget's min SOCOM II's. The input's flag read before the frame's count is stored (the
 * target's load ahead of the store: no strict aliasing) and `last = 1` after the frame test. Each
 * garbage path one statement with the score as a ?: in the sum: fold distributes the sum into the
 * arms (the adds in both, one store at the join), the row's old value a SAVE_EXPR loaded before
 * the test and the destination's address computed first (its own register across the host's
 * call); the assignment's value is the stored short (the call's register stored, its sign
 * extension summed). The host's phoneme scorer takes the phoneme twice (`move a1,a2`). */
static void IsoWordTokenDpProcess(ISOWDP *e, void **in)
{
    void *frame = in[1];
    int n = e->nphen;
    unsigned char last;
    unsigned char *flag;
    short *scores;
    short off;
    unsigned int i;

    if (frame == 0)
        return;
    last = 1;
    scores = e->f48(e->f40);
    off = e->f94;
    flag = in[0];
    e->f192++;
    if (flag != 0)
        last = *flag;
    e->f132 += off;
    for (i = 0; i < n; i++)
        scores[i] = 0x7FFF;
    e->f44(e->f40, scores, frame, last, off);
    if (last == 0 && e->f128 == e->start)
        e->f96 = e->f100 < e->nnodes ? e->f100 : e->nnodes;
    else if (e->f96 == e->nnodes) {
        e->f96 = e->f100;
        e->f92 = e->f152;
    }
    if (FrameDPacoust(e) != 0)
        return;
    if (e->f192 >= e->ngarbage && ExpandGarbageArrays(e) != 0)
        return;
    e->garbage1[e->f192 + 1] = e->garbage1[e->f192]
        + (scores[e->garbage] != 0x7FFF ? scores[e->garbage]
           : (scores[e->garbage] = e->f56(e->f40, e->garbage, e->garbage))) + off;
    e->garbage2[e->f192 + 1] = e->garbage2[e->f192]
        + (scores[e->sgarbage] != 0x7FFF ? scores[e->sgarbage]
           : (scores[e->sgarbage] = e->f56(e->f40, e->sgarbage, e->sgarbage))) + off;
    if (e->f128 != 0 && e->f128->f10 == 0)
        e->f172++;
    else
        e->f172 = 0;
    if (e->f168 != 0 && e->f172 >= e->f156) {
        e->process = (TOSFUNC)IdleProcess;
        _tosControl(e, 8, e->f192, 0);
    }
}

#define WORST   (-33792)                /* 0xFFFF7C00: no score yet */

/* the best n of the wheel's tokens that end a word (no successor, not the start token, at their
 * node's last state), each scored by its node's last entry over the block's base: the first n as
 * they come, then each better than the worst kept replacing it (the worst searched for again only
 * after a replacement); sorted by score. SOCOM 1's body (round 31c): the wheel walked by a for
 * with the records' base read before it (inside the empty-wheel test it takes the second
 * register); the first n appended through an index (`best[cnt]`: a walker of its own gives the
 * base the lower priority); a replacement's two stores written score first (sched1 issues the
 * later of the two first); a node's scores an array at +2 (the node pointer first in the sum). */
static unsigned int GetNBest(ISOWDP *e, unsigned int n)
{
    WHEEL *w = &e->wheel;
    TOKEN *best;
    WTOKEN **p;
    WTOKEN *t;
    NODE *node;
    unsigned int cnt, k, wi;
    int worst, score;
    void *word;

    worst = WORST;
    cnt = 0;
    wi = 0;
    best = e->f176;
    for (p = w->head; p != w->tail; p = NEXTSLOT(w, p)) {
        {
            t = *p;
            if (t->f10 == 0 && t != e->start) {
                node = t->node;
                if (node->f0 == node->len) {
                    score = node->f2[node->f0] + e->f132;
                    word = t->f0;
                    if (cnt < n) {
                        best[cnt].score = score;
                        best[cnt].f0 = word;
                        cnt++;
                    } else {
                        if (worst == WORST)
                            for (k = 0; k < n; k++)
                                if (worst < best[k].score) {
                                    worst = best[k].score;
                                    wi = k;
                                }
                        if (score < worst) {
                            best[wi].score = score;
                            best[wi].f0 = word;
                            worst = WORST;
                        }
                    }
                }
            }
        }
    }
    if (cnt != 0)
        qsort(best, cnt, sizeof(TOKEN), (int (*)(const void *, const void *))ScoreCompare);
    return cnt;
}

/* the block's commands: 3 runs the pass, 5 idles it, 4 ends the utterance (the N-best sent, 9, or
 * none, 10), 26/27 set and read the N-best's parameters, 34 takes the language, 35 the context,
 * 36 four words, 37/38 start and stop the input, 39 frees, 0xFF destroys. SOCOM 1's body (round
 * 31c): the cases in SOCOM 1's order (37, 38, 4, 3, 5, 34 ... 27: GCC lays the bodies out as
 * written); qQueueControl returning an unsigned char (as an int its return is a sibling call); the
 * garbage phonemes narrowed at the store (the accessors return int: the target's andi); the
 * record count read back from the block for the division (one load of the word); the records'
 * allocation through an inline returning 1 on failure, as SOCOM 1's (plainly written, the failure's
 * `move v0,zero` is folded into the null test). */
static int ControlIsoWordTokenDp(ISOWDP *e, unsigned char cmd, void *arg, void *arg2)
{
    ISOPARAM *p = arg;
    ISOLANG *l = arg;
    int n;

    switch (cmd) {
    case 37:
        return qQueueControl(e->inputs->q, 7, e->inputs->reader, arg);
    case 38:
        return qQueueControl(e->inputs->q, 6, e->inputs->reader, 0);
    case 4:
        n = GetNBest(e, e->f164);
        if (InitNewUtterance(e) != 0)
            return 0;
        e->process = (TOSFUNC)IdleProcess;
        if (n != 0)
            return _tosControl(e, 9, (int)&e->f176, n) == 0;
        return _tosControl(e, 10, 0, 0) == 0;
    case 3:
        e->process = (TOSFUNC)IsoWordTokenDpProcess;
        break;
    case 5:
        e->process = (TOSFUNC)IdleProcess;
        break;
    case 34:
        e->nphen = l->GetNbrPhenome(l);
        e->garbage = (unsigned short)l->GetSingleWordGarbagePhenome(l);
        e->sgarbage = (unsigned short)l->GetSentenceGarbagePhenome(l);
        e->wordstates = l->GetpSpeechUnit(l);
        e->nunits = l->GetNbrSpeechUnit(l);
        break;
    case 35:
        FreeIsoWordTokenDp(e);
        if (InitIsoWordTokenDpContext(e, arg, arg2) != 0)
            return 0;
        break;
    case 36:
        e->f44 = (ISOSCOREFN)p->f8;
        e->f56 = (ISOPHONFN)p->f4;
        e->f48 = (ISOBUFFN)p->f12;
        e->f40 = p->f0;
        break;
    case 39:
        FreeIsoWordTokenDp(e);
        return 1;
    case 26: {
        unsigned int k;

        e->f100 = p->f0;
        k = p->f4;
        e->f164 = k;
        if (AllocNBest(e, k))
            return 0;
        e->f168 = p->f8;
        e->f160 = p->f12;
        e->f156 = e->f160 / 10;
        break;
    }
    case 27:
        p->f0 = e->f100;
        p->f12 = e->f160;
        p->f4 = e->f164;
        p->f8 = e->f168;
        break;
    case 28:
    case 29:
    case 30:
        break;
    case 0xFF:
        FreeIsoWordTokenDp(e);
        tosBaseBlockDestruct(e);
        return 1;
    default:
        return 0;
    }
    return 1;
}

void *ConstructIsoWordTokenDp(void *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 2, 0, (TOSFUNC)IsoWordTokenDpProcess,
                                 (TOSFUNC)InitIsoWordTokenDpProcess, (TOSFUNC)ControlIsoWordTokenDp,
                                 sizeof(ISOWDP));
}

static int InitIsoWordTokenDpProcess(ISOWDP *e)
{
    unsigned int n = _tosGetProfileU32(e, 7, 10);
    ISOTOS *tos = e->tos;

    e->f164 = n;
    if (e->f176 != 0)
        heap_Free(tos->heap, e->f176);
    e->f176 = heap_Alloc(tos->heap, n * 8);
    if (e->f176 == 0)
        return 1;
    e->f156 = _tosGetProfileU32(e, 6, 30);
    e->f160 = e->f156 * 10;
    e->inputs[1].size = _tosGetProfileU32(e, 15, 4);
    e->inputs[0].size = 1;
    return 0;
}

/* a tree node: its children (12 bytes each) and their count */
typedef struct TNODE {
    struct TNODE *children;             /* +0 */
    char pad4[6];
    unsigned short nchildren;           /* +10 */
} TNODE;

/* the nodes of a tree down to depth levels */
static int CalcNrNodes(void *t, int depth)
{
    TNODE *n = t;
    int count = 1;
    unsigned short nc = n->nchildren;
    unsigned int i;
    TNODE *c;

    depth--;
    if (depth > 0) {
        c = n->children;
        for (i = 0; i < nc; i++, c++)
            count += CalcNrNodes(c, depth);
    }
    return count;
}

static void IdleProcess(void)
{
}

/* the qsort comparator of the tokens' scores */
static int ScoreCompare(const TOKEN *a, const TOKEN *b)
{
    if (a->score < b->score)
        return -1;
    if (a->score > b->score)
        return 1;
    return 0;
}
