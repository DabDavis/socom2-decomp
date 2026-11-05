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
 * written so far are here, in image order: FreeIsoWordTokenDp, InitNewUtterance,
 * ExpandGarbageArrays, ControlIsoWordTokenDp (round 31c), ConstructIsoWordTokenDp,
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
    short f2;                           /* +2 */
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
} WTOKEN;

/* the node wheel: a ring of token pointers */
typedef struct WHEEL {
    WTOKEN **base;                      /* +0 */
    WTOKEN **head;                      /* +4 */
    WTOKEN **tail;                      /* +8 */
    WTOKEN **end;                       /* +12: the last slot */
    int size;                           /* +16 */
} WHEEL;

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
    int f40;                            /* +40: command 36's four words */
    int f44;                            /* +44 */
    int f48;                            /* +48 */
    int pad52;
    int f56;                            /* +56 */
    int nphen;                          /* +60: the language's phonemes */
    int *wordstates;                    /* +64: each word's first state */
    short nunits;                       /* +68: the language's speech units (SOCOM II's word more) */
    short pad70;
    WHEEL wheel;                        /* +72 */
    short f92;                          /* +92 */
    short f94;                          /* +94 */
    int f96;                            /* +96 */
    int f100;                           /* +100 */
    int pad104;
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
    int f156;                           /* +156: profile 6 */
    unsigned int f160;                  /* +160: ten times that */
    int f164;                           /* +164: profile 7 */
    unsigned char f168;                 /* +168 */
    char pad169[3];
    int f172;                           /* +172 */
    void *f176;                         /* +176: profile 7's eight-byte records */
    int ngarbage;                       /* +180 */
    int *garbage1;                      /* +184 */
    int *garbage2;                      /* +188 */
    int f192;                           /* +192 */
    int garbage;                        /* +196: the single-word garbage phoneme */
    int sgarbage;                       /* +200: the sentence garbage phoneme */
    int pad204[2];
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

/* not written yet: the map's rows */
int func_004EAB30();                    /* IsoWordTokenDpProcess */
static int ControlIsoWordTokenDp(struct ISOWDP *e, unsigned char cmd, void *arg, void *arg2);
int ExtendNodeWheel(ISOWDP *e, WTOKEN ***a, WTOKEN ***b);

static void FreeIsoWordTokenDp(ISOWDP *e);
static int InitNewUtterance(ISOWDP *e);
static int ExpandGarbageArrays(ISOWDP *e);
static int InitIsoWordTokenDpProcess(ISOWDP *e);
static int CalcNrNodes(void *n, int depth);
static void IdleProcess(void);

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
    e->start->node->f2 = 0;
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

/* the garbage arrays grown by 150 entries (each array holds ngarbage + 1 words) */
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

extern int GetNBest(ISOWDP *e, unsigned int n);
extern int InitIsoWordTokenDpContext(ISOWDP *e, void *ctx, void *arg2);
extern unsigned char qQueueControl(void *q, int what, int reader, void *arg);
extern unsigned char _tosControl(void *log, int id, int a, int b);
extern void tosBaseBlockDestruct(void *b);

/* the N-best's records re-allocated for k of them: 1 when the heap has none */
static __inline__ unsigned char AllocNBest(ISOWDP *e, ISOTOS *tos, unsigned int k)
{
    if (e->f176 != 0)
        heap_Free(tos->heap, e->f176);
    e->f176 = heap_Alloc(tos->heap, k * 8);
    if (e->f176 == 0)
        return 1;
    return 0;
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
        e->process = (TOSFUNC)func_004EAB30;
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
        e->f44 = p->f8;
        e->f56 = p->f4;
        e->f48 = p->f12;
        e->f40 = p->f0;
        break;
    case 39:
        FreeIsoWordTokenDp(e);
        return 1;
    case 26: {
        unsigned int k;
        ISOTOS *tos;

        e->f100 = p->f0;
        tos = e->tos;
        k = p->f4;
        e->f164 = k;
        if (AllocNBest(e, tos, k))
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
    return tosBaseBlockConstruct(tos, id, 2, 0, (TOSFUNC)func_004EAB30,
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
