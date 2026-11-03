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

struct TOS {
    char pad0[32];
    TOSLOGCB error_cb;                  /* +32 */
    TOSLOGCB warning_cb;                /* +36 */
    struct PROFILE *profiles;           /* +40: one per block id */
    char pad44[8];
    int error;                          /* +52: the first error, block << 16 | code */
    int warning;                        /* +56 */
    char pad60[20];
    void *heap;                         /* +80 */
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

typedef int (*TOSFUNC)();

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
    unsigned char state;                /* +32 */
} TOSBLOCK;

extern void *func_00243398(void *heap, int count, int size);       /* the heap's calloc */

/* inline: tosBaseBlockConstruct has it inlined (the log record still built on the stack) */
__inline__ unsigned char _tosErrorLog(TOSLOG *log, unsigned short code)
{
    TOS *t = log->tos;
    int code32 = log->block << 16 | code;

    if (t->error == 0)
        t->error = code32;
    if (t->error_cb != 0)
        t->error_cb(t, t->error);
    return code;
}

unsigned char _tosWarningLog(TOSLOG *log, unsigned short code)
{
    TOS *t = log->tos;
    int code32 = log->block << 16 | code;

    if (t->warning == 0)
        t->warning = code32;
    if (t->warning_cb != 0)
        t->warning_cb(t, t->warning);
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

extern int qQueueControl(void *q, unsigned char cmd, int arg, int n);

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
