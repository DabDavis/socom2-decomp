/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* tos: the recogniser's block scheduler (OVL2 0x50AED8-0x50D560, SOCOM 1's object at 0x3759E8,
 * SetAndControlQSizes .. DisabledProcess): ScanSoft's speech recogniser, prebuilt with ee-gcc
 * 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM
 * 1's. Only the functions written so far are here, in image order. A block reports through a log
 * record (the scheduler and its block id); the first error or warning is kept and passed to the
 * host's callback. The loggers return the code as an unsigned char, which is why their callers'
 * `return _tosErrorLog(...)` is no sibling call (2.96 declines one when the modes differ). */

typedef struct TOS TOS;

typedef void (*TOSLOGCB)(TOS *t, int code);

/* the host's callbacks, copied in by tosInit (24 bytes) */
typedef struct TOSCB {
    unsigned char (*setmode)(TOS *t, unsigned short mode);   /* +0 */
    void *f4;
    unsigned char (*input_queue)(TOS *t, void *q, unsigned short reader);   /* +8 */
    unsigned char (*output_queue)(TOS *t, void *q);                         /* +12 */
    TOSLOGCB error_cb;                  /* +16 */
    TOSLOGCB warning_cb;                /* +20 */
} TOSCB;

struct TOS {
    unsigned short nblocks;             /* +0 */
    unsigned short blockbase;           /* +2: the first block's id */
    struct TOSBLOCK **blocks;           /* +4: by id - blockbase */
    unsigned short nqueues;             /* +8 */
    unsigned short queuebase;           /* +10: the first queue's id */
    void **queues;                      /* +12: by id - queuebase */
    TOSCB cb;                           /* +16 */
    struct PROFILE *profiles;           /* +40: one per block id */
    struct TOSCMDS ***cmds;             /* +44: [arch][mode], each block's command list */
    unsigned short arch;                /* +48 */
    unsigned short mode;                /* +50 */
    int error;                          /* +52: the first error, block << 16 | code */
    int warning;                        /* +56 */
    int again;                          /* +60: a block asks for another pass */
    struct TOSPROC *procs;              /* +64: each block id's constructor, the architecture's */
    unsigned char *writers;             /* +68: {queue, -, block} for each block writing a queue */
    unsigned char *readers;             /* +72: {queue, block} for each block reading one, ended by 255 */
    void **f76;                         /* +76: the frames a block's inputs give it; freed with the tables */
    void *heap;                         /* +80 */
    struct FASTALLO { char pad[280]; } fa;   /* +84: the small-block allocator (fastallo.c) */
};

typedef struct TOSLOG {
    TOS *tos;                           /* +0 */
    unsigned char block;                /* +4 */
} TOSLOG;

/* a block: the log record's head (the scheduler, its id), its input queues (each a queue and
 * the block's reader in it), its output queues, the three functions the scheduler calls, and the
 * block's own data after the table */
typedef struct QREQ {
    void *q;
    unsigned short reader;
} QREQ;

typedef unsigned char (*TOSFUNC)();
typedef void (*TOSPROCFN)();        /* a block's process function returns nothing */

typedef struct TOSBLOCK {
    TOS *tos;                           /* +0 */
    unsigned char id;                   /* +4 */
    char pad5[3];
    unsigned char ninputs;              /* +8 */
    unsigned char noutputs;             /* +9 */
    char pad10[2];
    QREQ *inputs;                       /* +12 */
    QREQ *outputs;                      /* +16 */
    TOSFUNC init;                       /* +20 */
    TOSPROCFN process;                  /* +24 */
    TOSFUNC control;                    /* +28 */
    unsigned char state;                /* +32: 0 while disabled */
    char pad33[3];
    TOSPROCFN saved;                    /* +36: the process function while disabled */
} TOSBLOCK;

extern unsigned char qQueueControl(void *q, unsigned char cmd, int arg, int n);
extern void heap_Free(void *heap, void *p);
static void DisabledProcess();

/* the input-queue commands, inlined into tosBaseBlockControl (GNU extern inlines: their own
 * copies are defined further on, at their place) */
extern __inline__ unsigned char tosBaseBlockStopInputQueues(TOSBLOCK *b)
{
    unsigned char err = 0;
    unsigned int i;

    for (i = 0; i < b->ninputs; i++)
        err |= !qQueueControl(b->inputs[i].q, 6, b->inputs[i].reader, 0);
    return err;
}

extern __inline__ unsigned char tosBaseBlockDisableInputQueues(TOSBLOCK *b)
{
    unsigned char err = 0;
    unsigned int i;

    for (i = 0; i < b->ninputs; i++)
        if (b->inputs[i].q != 0)
            err |= !qQueueControl(b->inputs[i].q, 8, b->inputs[i].reader, 0);
    return err;
}

extern __inline__ unsigned char tosBaseBlockStartInputQueues(TOSBLOCK *b, int n)
{
    unsigned char err = 0;
    unsigned char cmd = n ? 7 : 5;
    unsigned int i;

    for (i = 0; i < b->ninputs; i++)
        if (b->inputs[i].q != 0)
            err |= !qQueueControl(b->inputs[i].q, cmd, b->inputs[i].reader, n);
    return err;
}

extern void *func_00243398(void *heap, int count, int size);       /* the heap's calloc */

/* the architecture's blocks (InitialProcs): each id's constructor */
typedef struct TOSPROC {
    unsigned char id;
    TOSBLOCK *(*construct)(TOS *t, unsigned char id);   /* +4 */
} TOSPROC;

/* a queue as the scheduler sees it: the scheduler, its id, its element's bytes */
typedef struct TOSQUEUE {
    TOS *tos;                           /* +0 */
    unsigned char id;                   /* +4: 255 for a queue outside the tables */
    char pad5[3];
    unsigned short elemsize;            /* +8 */
} TOSQUEUE;

void *tosGetQueuePtr(TOS *t, unsigned char id);

/* a block's n-th output queue: set, the queue's element size taken from the block's when it has
 * none; the scheduler's own (the record a block id's constructor may hand back is the scheduler)
 * through the host's callback, at the block's control slot. A 1 is a failure. The callback's
 * failure returns 1 and its success falls to the shared `return 0` (round 27c): so its result is
 * tested by branches, the target's `beqz v0; li v0,1` -- `return cb(...) != 0`, or its own
 * `return 0` after the test, is a store-flag (`sltu`) the target does not have. */
static __inline__ unsigned char RegisterOutputQueue(TOSBLOCK *b, TOS *t, TOSQUEUE *q, unsigned char n)
{
    if (b == (TOSBLOCK *)t) {
        if (((TOS *)b)->cb.output_queue((TOS *)b, q) != 0)
            return 1;
    } else if (b != 0 && q != 0) {
        b->outputs[n].q = q;
        if (q->elemsize == 0)
            q->elemsize = b->outputs[n].reader;
    }
    return 0;
}

/* a block's n-th input queue and its reader id: set and the reader opened (10); with no block the
 * reader disabled (8), the scheduler's through the host's callback. A 1 is a failure. */
static __inline__ unsigned char RegisterInputQueue(TOSBLOCK *b, TOS *t, TOSQUEUE *q, unsigned short n,
                                                   unsigned short reader)
{
    if (b == 0) {
        if (q != 0 && qQueueControl(q, 8, reader, 0) == 0)
            return 1;
        return 0;
    }
    if (b == (TOSBLOCK *)t) {
        if (((TOS *)b)->cb.input_queue((TOS *)b, q, reader) != 0)
            return 1;
    } else {
        b->inputs[n].q = q;
        b->inputs[n].reader = reader;
        if (q != 0 && qQueueControl(q, 10, reader, 0) == 0)
            return 1;
    }
    return 0;
}

/* block i built: its id's constructor, its output queues (QueueWriters' entries naming it), its
 * input queues (QueueReaders'), each reader numbered down from the queue's count of readers, then
 * its init; a 1 on any failure. tosGetQueuePtr is called out of line: its body comes later.
 * OPEN (148 words, 2 short since round 27c's callback tests): the walks, tests and calls are the
 * target's, but the output count is
 * kept in s3 and narrowed at each use there (`andi v1,s3,0xff` before the bump) where 2.96 copies
 * it into the inline's argument registers here, and the readers' and writers' saved registers
 * trade places (s5/s6). Its definition here is what tosCallControlFunc needed (below). */
static unsigned char ConstructBlock(TOS *t, unsigned char id, int i)
{
    TOSPROC *p = t->procs;
    unsigned short nin = 0;
    TOSBLOCK *b;
    unsigned char *w, *r, *counts;
    unsigned char nout;

    while (p->id != id)
        p++;
    b = p->construct(t, id);
    if (b == 0)
        return 1;
    t->blocks[i] = b;
    nout = 0;
    for (w = t->writers; w[0] != 0xFF; w += 3)
        if (w[2] == id && RegisterOutputQueue(b, t, tosGetQueuePtr(t, w[0]), nout++))
            return 1;
    counts = func_00243398(t->heap, t->nqueues, 1);
    if (counts == 0)
        return 1;
    for (r = t->readers; r[0] != 0xFF; r += 2)
        counts[r[0] - t->queuebase]++;
    for (r = t->readers; r[0] != 0xFF; r += 2) {
        unsigned char k = r[0] - t->queuebase;

        counts[k]--;
        if (r[1] == id && RegisterInputQueue(b, t, t->queues[k], nin++, counts[k])) {
            heap_Free(t->heap, counts);
            return 1;
        }
    }
    heap_Free(t->heap, counts);
    return b->init(b) != 0;
}

/* a block or a queue by its id: the scheduler numbers its blocks from blockbase and its queues
 * from queuebase (GNU extern inlines: their own copies are defined further on) */
extern __inline__ TOSBLOCK *tosGetBlockPtr(TOS *t, unsigned char id)
{
    int i = id - t->blockbase;

    if (i >= 0 && i < t->nblocks)
        return t->blocks[i];
    return 0;
}

extern __inline__ void *tosGetQueuePtr(TOS *t, unsigned char id)
{
    int i = id - t->queuebase;

    if (i >= 0 && i < t->nqueues)
        return t->queues[i];
    return 0;
}

extern TOSQUEUE *qQueueConstruct(TOS *t, unsigned char id, unsigned char nreaders);
extern int qQueueInit(void *q);

/* a reader of a queue added to a block: the first free input slot takes the queue and the reader,
 * which is opened (10); with no block the reader disabled (8), the scheduler's through the host's
 * callback. A 1 is a failure. */
static __inline__ unsigned char AddInputQueue(TOSBLOCK *b, TOS *t, TOSQUEUE *q, unsigned short reader)
{
    char n;

    if (b != 0) {
        if (b == (TOSBLOCK *)t) {
            if (((TOS *)b)->cb.input_queue((TOS *)b, q, reader) != 0)
                return 1;
        } else {
            for (n = 0; n < b->ninputs; n++) {
                if (b->inputs[n].q == 0) {
                    b->inputs[n].q = q;
                    b->inputs[n].reader = reader;
                    break;
                }
            }
            if (q != 0 && qQueueControl(q, 10, reader, 0) == 0)
                return 1;
        }
    } else if (q != 0 && qQueueControl(q, 8, reader, 0) == 0)
        return 1;
    return 0;
}

/* queue i built: its readers counted (QueueReaders' entries naming it), the queue made, each block
 * writing it (QueueWriters' entries, together) given it as its next output, each block reading it
 * given it as a free input with its reader numbered down; then the queue initialised. A 1 on any
 * failure. The writers' output numbers are counted per block from the table's head. SOCOM 1's
 * static (round 27c, byte-exact): the reader's block found as each entry's own (no range test, the
 * scheduler for id 0), its slot the first free input (a char counter, as DestructQueue's); the
 * input helper's no-block case last (`if (b != 0) ... else if`: the target's 8-path after the
 * 10-path, each its own qQueueControl call). */
static unsigned char ConstructQueue(TOS *t, unsigned char id, int i)
{
    unsigned char *counts = func_00243398(t->heap, t->nblocks, 1);
    unsigned char *w, *r;
    unsigned int nreaders = 0;
    TOSQUEUE *q;

    if (counts == 0)
        return 1;
    for (w = t->writers; w[0] != id; w += 3)
        counts[w[2]]++;
    for (r = t->readers; r[0] != 255; r += 2)
        if (r[0] == id)
            nreaders++;
    q = qQueueConstruct(t, id, nreaders);
    if (q == 0) {
        heap_Free(t->heap, counts);
        return 1;
    }
    t->queues[i] = q;
    for (; w[0] == id; w += 3) {
        TOSBLOCK *b = w[2] != 0 ? t->blocks[w[2] - t->blockbase] : (TOSBLOCK *)t;

        if (RegisterOutputQueue(b, t, q, counts[w[2]]++)) {
            heap_Free(t->heap, counts);
            return 1;
        }
    }
    heap_Free(t->heap, counts);
    for (r = t->readers; r[0] != 255; r += 2) {
        if (id == r[0]) {
            unsigned char k = id - t->queuebase;
            TOSBLOCK *b = r[1] != 0 ? t->blocks[r[1] - t->blockbase] : (TOSBLOCK *)t;

            if (AddInputQueue(b, t, t->queues[k], --nreaders))
                return 1;
        }
    }
    return qQueueInit(q) == 0;
}

int tosBaseBlockControl(TOSBLOCK *b, unsigned char cmd, int arg, int n);

extern void qQueueReset(void *q);


/* a queue reset and freed: every block writing it and every block reading it forgets it, and the
 * scheduler's slot is cleared (block id 0 is the scheduler itself, which keeps no slots) */
unsigned char DestructQueue(TOSQUEUE *q)
{
    unsigned char id = q->id;
    TOS *t = q->tos;
    unsigned char *w;
    unsigned char *r;

    qQueueReset(q);
    if (id != 255) {
        for (w = t->writers; w[0] != id; w += 3)
            ;
        do {
            TOSBLOCK *b = w[2] != 0 ? t->blocks[w[2] - t->blockbase] : (TOSBLOCK *)t;
            char i;

            if (b != (TOSBLOCK *)t && b != 0) {
                for (i = 0; i < b->noutputs; i++) {
                    if (b->outputs[i].q == q) {
                        b->outputs[i].q = 0;
                        break;
                    }
                }
            }
            w += 3;
        } while (w[0] == id);
    }
    for (r = t->readers; r[0] != 255; r += 2) {
        if (id == r[0]) {
            unsigned char j = id - t->queuebase;
            void *qq;
            TOSBLOCK *b;
            char i;

            b = r[1] != 0 ? t->blocks[r[1] - t->blockbase] : (TOSBLOCK *)t;
            qq = t->queues[j];
            if (b != 0 && b != (TOSBLOCK *)t) {
                for (i = 0; i < b->ninputs; i++) {
                    if (b->inputs[i].q == qq) {
                        b->inputs[i].q = 0;
                        break;
                    }
                }
            }
        }
    }
    if (q->id != 255)
        t->queues[q->id - t->queuebase] = 0;
    heap_Free(t->heap, q);
    return 0;
}

/* every block's control called to destroy it (255), every queue destroyed, the tables freed; a 1
 * when any refused */
static unsigned char DestructArchitecture(TOS *t)
{
    unsigned char err = 0;
    int i;

    for (i = 0; i < t->nblocks; i++) {
        struct TOSBLOCK *b = t->blocks[i];

        if (b != 0 && b->control != 0)
            err |= !b->control(b, 255, 0, 0);
    }
    for (i = 0; i < t->nqueues; i++) {
        void *q = t->queues[i];

        if (q != 0)
            err |= !qQueueControl(q, 255, 0, 0);
    }
    heap_Free(t->heap, t->blocks);
    heap_Free(t->heap, t->queues);
    heap_Free(t->heap, t->f76);
    return err;
}

/* a command (msg[1]) to a block or a queue (msg[0]): a block's own control first (always the
 * destroy, 255, other commands only while it is enabled), the scheduler's commands (248 up) to
 * the block as well; a queue's to the queue; 248 to an id with nothing there constructs it.
 * The branch over ConstructBlock is `bnez` with the epilogue's `move v0,s2` in its slot only with
 * ConstructBlock defined above (round 13c): reorg leaves a slot unannulled before a call to a
 * function the object has already compiled (`bnezl` with it only declared -- the word rounds 11c
 * and 12c held open as a toolchain question). */
int tosCallControlFunc(TOS *t, unsigned char *msg, int arg, int n)
{
    unsigned char ret = 0;
    TOSBLOCK *b = tosGetBlockPtr(t, msg[0]);

    if (b != 0) {
        unsigned char enabled = b->state;

        if (enabled != 0 || msg[1] == 255)
            ret = b->control(b, msg[1], arg, n);
        if (msg[1] >= 248)
            ret |= tosBaseBlockControl(b, msg[1], arg, n);
        if (enabled == 0 && ret == 0)
            ret = 1;
    } else {
        void *q = tosGetQueuePtr(t, msg[0]);

        if (q != 0) {
            ret = qQueueControl(q, msg[1], arg, n);
        } else {
            if (msg[1] == 248) {
                int i = msg[0] - t->blockbase;

                if (i >= 0 && i < t->nblocks) {
                    if (t->blocks[i] == 0)
                        ConstructBlock(t, msg[0], i);
                } else {
                    i = msg[0] - t->queuebase;
                    if (i < 0 || i >= t->nqueues || t->queues[i] == 0)
                        ConstructQueue(t, msg[0], i);
                }
            }
            ret = 1;
        }
    }
    return ret;
}

/* a block's command: the mode it needs (0xFFFF any, 0xFFFE the next one), the steps -- {id, cmd}
 * pairs sent through tosCallControlFunc, ended by a zero cmd, the scheduler run after each when
 * `run` is set -- and a function called last */
typedef struct TOSCMD {
    unsigned char cmd;                  /* +0 */
    unsigned char run;                  /* +1 */
    unsigned short mode;                /* +2 */
    unsigned char (*post)(TOS *t, unsigned char block, unsigned char cmd, int arg, int n);   /* +4 */
    unsigned char steps[2];             /* +8 */
} TOSCMD;

/* a mode's command lists, one per block, ended by a null list */
typedef struct TOSCMDS {
    unsigned char block;                /* +0 */
    TOSCMD **cmds;                      /* +4 */
} TOSCMDS;

extern int qCheckDeQueueOne(QREQ *req, unsigned int n, void **out);
extern void qFreeUnusedElemements(void *q);

/* a block run once: a frame from each of its input queues (into the scheduler's buffer at +76),
 * its process function called on them, the elements every reader has taken freed */
static __inline__ void RunBlock(TOSBLOCK *b)
{
    unsigned char n = b->ninputs;
    void **in = b->tos->f76;
    QREQ *q;
    unsigned int i;
    unsigned char m;

    qCheckDeQueueOne(b->inputs, n, in);
    b->process(b, in, n);
    m = b->ninputs;
    q = b->inputs;
    for (i = 0; i < m; q++, i++)
        if (q->q != 0)
            qFreeUnusedElemements(q->q);
}

/* the scheduler's pass: every block in turn (ids null) or the blocks a zero-ended id list names,
 * again while a block asks for another pass (+60). SOCOM 1's body (round 33c). The block's
 * process returns nothing (through a void function pointer the address takes v1, `jalr v1`; a
 * value call's takes v0); the
 * free loop's count and table are locals read after the call (a bound read through b each pass
 * leaves the loop rolled), the count a variable of its own (one shared with the calls' count
 * outranks b: s1/s2 swapped), the walker stepped before the count (`q++, i++`: reorg's slot takes
 * the join's first insn); the id list's step inside the load (`*p++`: an unannulled slot) and its
 * table a variable apart from the first loop's walker (one pseudo for both: 4 registers off). */
unsigned char tosRun(TOS *t, unsigned char *ids)
{
    unsigned short n;
    unsigned char i;
    TOSBLOCK **pb, **blocks;
    unsigned char *p;

    if (ids == 0) {
        n = t->nblocks;
        do {
            t->again = 0;
            for (i = 0, pb = t->blocks; i < n; i++) {
                TOSBLOCK *b = *pb++;

                if (b != 0)
                    RunBlock(b);
            }
        } while (t->again != 0);
    } else {
        do {
            t->again = 0;
            blocks = t->blocks - t->blockbase;
            for (p = ids; *p != 0; ) {
                TOSBLOCK *b = blocks[*p++];

                if (b != 0)
                    RunBlock(b);
            }
        } while (t->again != 0);
    }
    return 0;
}

unsigned char _tosErrorLog(TOSLOG *log, unsigned short code);

/* a block's command (the log record names the block), through the mode's command list */
unsigned char _tosControl(TOSLOG *log, unsigned char cmd, int arg, int n)
{
    TOS *t = log->tos;
    unsigned char block = log->block;
    unsigned char ret = 0;
    TOSCMDS *e;
    TOSCMD **l;
    TOSCMD *c;
    unsigned char *p;

    for (e = t->cmds[t->arch][t->mode]; e->cmds != 0 && e->block != block; e++)
        ;
    l = e->cmds;
    if (l == 0)
        return _tosErrorLog(log, 5);
    while (*l != 0 && (*l)->cmd != cmd)
        l++;
    c = *l;
    if (c == 0)
        return _tosErrorLog(log, 5);
    if (c->mode != 0xFFFF && c->mode != t->mode) {
        if (c->mode == 0xFFFE)
            t->mode++;
        else
            t->mode = c->mode;
        ret = t->cb.setmode(t, t->mode);
        if (ret != 0)
            return ret;
    }
    p = c->steps;
    if (c->run == 0) {
        for (; p[1] != 0; p += 2)
            ret |= !tosCallControlFunc(t, p, arg, n);
    } else {
        for (; p[1] != 0; p += 2) {
            ret |= !tosCallControlFunc(t, p, arg, n);
            tosRun(t, 0);
        }
    }
    if (c->post != 0)
        ret |= c->post(t, block, cmd, arg, n);
    return ret;
}

/* the scheduler's commands to a block: 252 disables it (its process function parked, its input
 * queues disabled), 251 enables it again and starts its queues, 250 stops them, 249 starts them
 * moved back arg, 248 is a no-op; a 1 when done */
int tosBaseBlockControl(TOSBLOCK *b, unsigned char cmd, int arg, int n)
{
    int ret = 1;

    switch (cmd) {
    case 252:
        if (b->state != 0) {
            b->saved = b->process;
            b->process = (TOSPROCFN)DisabledProcess;
            b->state = 0;
            ret = !tosBaseBlockDisableInputQueues(b);
        } else
            ret = 0;
        break;
    case 250:
        if (b->state != 0)
            ret = !tosBaseBlockStopInputQueues(b);
        else
            ret = 0;
        break;
    case 251:
        b->state = 1;
        arg = 0;
        if (b->process == (TOSPROCFN)DisabledProcess)
            b->process = b->saved;
        /* fall through */
    case 249:
        if (b->state != 0)
            ret = !tosBaseBlockStartInputQueues(b, arg);
        else
            ret = 0;
        break;
    case 248:
        break;
    default:
        ret = 0;
        break;
    }
    return ret;
}

extern unsigned char ConstructArchitecture(TOS *t, void *a, void *b, void *c);   /* not yet written */
extern unsigned char SetAndControlQSizes(TOS *t);                                       /* not yet written */
extern int qQueueInit(void *q);
extern void fastallo_Init(struct FASTALLO *fa, void *heap, unsigned short chunk_size);
extern void fastallo_Terminate(struct FASTALLO *fa);

/* every block's init, the queues' sizes settled, every queue's init: a 1 at the first failure */
static __inline__ unsigned char InitArchitecture(TOS *t)
{
    int i;

    for (i = 0; i < t->nblocks; i++) {
        TOSBLOCK *b = t->blocks[i];

        if (b != 0 && b->init != 0 && b->init(b) != 0)
            return 1;
    }
    if (SetAndControlQSizes(t) != 0)
        return 1;
    for (i = 0; i < t->nqueues; i++) {
        void *q = t->queues[i];

        if (q != 0 && qQueueInit(q) == 0)
            return 1;
    }
    return 0;
}

/* the scheduler set up: its heap and allocator, the host's callbacks, the profiles and command
 * lists, the architecture built and initialised; a 1 (the allocator torn down) on failure */
unsigned char tosInit(TOS *t, void *heap, TOSCB *cb, struct PROFILE *profiles, void *a, void *b,
                      void *c, struct TOSCMDS ***cmds, unsigned short arch, unsigned short mode)
{
    t->heap = heap;
    fastallo_Init(&t->fa, heap, 4096);
    t->profiles = profiles;
    t->cmds = cmds;
    t->error = 0;
    t->cb = *cb;
    t->arch = arch;
    t->mode = mode;
    if (ConstructArchitecture(t, a, b, c) == 0 && InitArchitecture(t) == 0)
        return 0;
    fastallo_Terminate(&t->fa);
    return 1;
}

/* inline: tosBaseBlockConstruct has it inlined (the log record still built on the stack) */
__inline__ unsigned char _tosErrorLog(TOSLOG *log, unsigned short code)
{
    TOS *t = log->tos;
    int code32 = log->block << 16 | code;

    if (t->error == 0)
        t->error = code32;
    if (t->cb.error_cb != 0)
        t->cb.error_cb(t, t->error);
    return code;
}

unsigned char _tosWarningLog(TOSLOG *log, unsigned short code)
{
    TOS *t = log->tos;
    int code32 = log->block << 16 | code;

    if (t->warning == 0)
        t->warning = code32;
    if (t->cb.warning_cb != 0)
        t->cb.warning_cb(t, t->warning);
    return code;
}

/* a block allocated with its queue tables and data (size rounded to words) after it; a failed
 * allocation is the scheduler's error 2. OPEN (16 words): _tosErrorLog is inlined here in the
 * target (the log record still built on the stack, so the logger is a public __inline__ -- its
 * own copy stays byte-exact so), but the target computes the code inside the `error == 0` test
 * where this inlining computes it before (computing it inside in the logger's source breaks the
 * logger, 22 words), and keeps the outputs' address in a1 for the stores that follow. */
TOSBLOCK *tosBaseBlockConstruct(TOS *tos, unsigned char id, unsigned char ninputs, unsigned char noutputs,
                                TOSFUNC process, TOSFUNC init, TOSFUNC control, unsigned int size)
{
    TOSBLOCK *b;
    TOSLOG log;
    QREQ *inputs;

    if (size & 3)
        size = size - (size & 3) + 4;
    b = func_00243398(tos->heap, 1, (ninputs + noutputs) * sizeof(QREQ) + size);
    log.tos = tos;
    log.block = id;
    inputs = (QREQ *)((char *)b + size);
    if (b == 0) {
        _tosErrorLog(&log, 2);
        return 0;
    }
    b->outputs = inputs + ninputs;
    b->state = 1;
    b->tos = tos;
    b->id = id;
    b->ninputs = ninputs;
    b->noutputs = noutputs;
    b->init = init;
    b->control = control;
    b->process = (TOSPROCFN)process;
    b->inputs = inputs;
    return b;
}

/* the block's readers stopped in its input queues (8), its slot in the scheduler cleared, the
 * block freed */
void tosBaseBlockDestruct(TOSBLOCK *b)
{
    TOS *tos = b->tos;
    unsigned char n = b->ninputs;
    int i;

    for (i = 0; i < n; i++)
        if (b->inputs[i].q != 0)
            qQueueControl(b->inputs[i].q, 8, b->inputs[i].reader, 0);
    tos->blocks[b->id - tos->blockbase] = 0;
    heap_Free(b->tos->heap, b);
}

/* the block's readers in its input queues disabled (6), stopped (8) or started (5, or 7 moved
 * back n); a 1 when any queue refused */
unsigned char tosBaseBlockStopInputQueues(TOSBLOCK *b)
{
    unsigned char err = 0;
    unsigned int i;

    for (i = 0; i < b->ninputs; i++)
        err |= !qQueueControl(b->inputs[i].q, 6, b->inputs[i].reader, 0);
    return err;
}

unsigned char tosBaseBlockDisableInputQueues(TOSBLOCK *b)
{
    unsigned char err = 0;
    unsigned int i;

    for (i = 0; i < b->ninputs; i++)
        if (b->inputs[i].q != 0)
            err |= !qQueueControl(b->inputs[i].q, 8, b->inputs[i].reader, 0);
    return err;
}

unsigned char tosBaseBlockStartInputQueues(TOSBLOCK *b, int n)
{
    unsigned char err = 0;
    unsigned char cmd = n ? 7 : 5;
    unsigned int i;

    for (i = 0; i < b->ninputs; i++)
        if (b->inputs[i].q != 0)
            err |= !qQueueControl(b->inputs[i].q, cmd, b->inputs[i].reader, n);
    return err;
}

/* a block's profile: two lists of {key, value} ended by key 0, one of floats and one of
 * integers; a key is looked up in the list of its own type first, then in the other one,
 * converted */
typedef struct PROFENT {
    unsigned short key;
    union {
        float f;
        unsigned int u;
    } v;
} PROFENT;

typedef struct PROFILE {
    PROFENT *floats;                    /* +0 */
    PROFENT *ints;                      /* +4 */
} PROFILE;

unsigned int _tosGetProfileU32(TOSBLOCK *b, unsigned short key, unsigned int def)
{
    PROFENT *e = b->tos->profiles[b->id].ints;
    PROFENT *f = b->tos->profiles[b->id].floats;
    int found = 0;

    if (e != 0) {
        while (e->key != key && e->key != 0)
            e++;
        if (e->key != 0) {
            def = e->v.u;
            found = 1;
        }
    }
    if (!found && f != 0) {
        while (f->key != key && f->key != 0)
            f++;
        if (f->key != 0)
            def = f->v.f;
    }
    return def;
}

float _tosGetProfileFloat(TOSBLOCK *b, unsigned short key, float def)
{
    PROFENT *f = b->tos->profiles[b->id].floats;
    PROFENT *e = b->tos->profiles[b->id].ints;
    int found = 0;

    if (f != 0) {
        while (f->key != key && f->key != 0)
            f++;
        if (f->key != 0) {
            def = f->v.f;
            found = 1;
        }
    }
    if (!found && e != 0) {
        while (e->key != key && e->key != 0)
            e++;
        if (e->key != 0)
            return e->v.u;
    }
    return def;
}

static void DisabledProcess()
{
}

