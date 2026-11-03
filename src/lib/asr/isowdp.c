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
 * written so far are here, in image order: FreeIsoWordTokenDp, InitNewUtterance (OPEN),
 * ExpandGarbageArrays, ConstructIsoWordTokenDp, InitIsoWordTokenDpProcess, CalcNrNodes,
 * IdleProcess, ScoreCompare. CalcNrNodes reads the child count before its depth test (the
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
    char pad33[31];
    int *wordstates;                    /* +64: each word's first state */
    int pad68;
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
    int f160;                           /* +160: ten times that */
    int f164;                           /* +164: profile 7 */
    int pad168;
    int f172;                           /* +172 */
    void *f176;                         /* +176: profile 7's eight-byte records */
    int ngarbage;                       /* +180 */
    int *garbage1;                      /* +184 */
    int *garbage2;                      /* +188 */
    int f192;                           /* +192 */
    int pad196[4];
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
int ControlIsoWordTokenDp();
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

/* a word's state count: the halfword distance between its first state and the next word's (the
 * table holds offsets; as addresses the base cancels -- the target's `sll 1; sra 1` is the scaled
 * difference divided back, which no difference of the offsets themselves gives) */
#define NBRSTATES(base, tab, w)  (&(base)[(tab)[(w) + 1]] - &(base)[(tab)[w]])

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
 * word's size and the wheel's first slot.
 * OPEN (round 17c; the right size, 14 words): the target loads the word's two table entries
 * next first (`lw v0,4(a0)` then `lw v1,0(a0)`, the entry's address in a0, as SOCOM 1's order),
 * this compile the other way round with the address in v1 -- the base, a local word or table,
 * a statement for the count, the operands reversed all leave it (build/scratch/gcc/iso/v_inu*.py);
 * and the closing loads and stores are scheduled in another order (the target loads +136, +152,
 * +100 and stores +172 last; 120 orders of the first five stores come no closer than 13 words).
 * The node's size is unsigned (sizeof: the target's srl), the allocation an inline storing
 * through the field's address taken before the call (the target's s1 across it), the free list
 * indexed from the scheduler itself (tos + 4n + 104, not through &tos->fa). */
static int InitNewUtterance(ISOWDP *e)
{
    ISOTOS *tos = e->tos;
    WTOKEN **q, **p, **s;
    int n = NBRSTATES((short *)e->wordstates, e->wordstates, e->start->word) - 3;
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
    e->f96 = e->f100;
    e->f128 = e->start;
    e->f92 = e->f152;
    e->f94 = 0;
    e->f132 = 0;
    e->f172 = 0;
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
