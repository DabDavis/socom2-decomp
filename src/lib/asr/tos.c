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
    void *f8;
    void *f12;
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
    char pad60[8];
    unsigned char *writers;             /* +68: {queue, -, block} for each block writing a queue */
    unsigned char *readers;             /* +72: {queue, block} for each block reading one, ended by 255 */
    void *f76;                          /* +76: freed with the tables */
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
    TOSFUNC process;                    /* +24 */
    TOSFUNC control;                    /* +28 */
    unsigned char state;                /* +32: 0 while disabled */
    char pad33[3];
    TOSFUNC saved;                      /* +36: the process function while disabled */
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

extern unsigned char ConstructBlock(TOS *t, unsigned char id, int i);    /* not yet written */
extern unsigned char ConstructQueue(TOS *t, unsigned char id, int i);    /* not yet written */
int tosBaseBlockControl(TOSBLOCK *b, unsigned char cmd, int arg, int n);

extern void qQueueReset(void *q);

/* a queue as the scheduler sees it: the scheduler and its id */
typedef struct TOSQUEUE {
    TOS *tos;                           /* +0 */
    unsigned char id;                   /* +4: 255 for a queue outside the tables */
} TOSQUEUE;

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
 * OPEN (1 word): the branch over ConstructBlock is `bnez` in the target and `bnezl` here (its slot,
 * the epilogue's `move v0,s2`, annulled: reorg takes v0 as live past the call) -- the queue
 * branch after it fills alike in both; the block test spelled through the inline lookup, a goto
 * past the call, an assignment in the test, constructors returning void or a value: the same. */
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

extern unsigned char tosRun(TOS *t, int n);        /* not yet written */
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
            b->process = (TOSFUNC)DisabledProcess;
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
        if (b->process == (TOSFUNC)DisabledProcess)
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
    b->process = process;
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

