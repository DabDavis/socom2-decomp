/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* tos: the recogniser's block scheduler (OVL2 0x50B2C0-0x50D560, SOCOM 1's object at 0x3759E8):
 * ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops (README "The GCC front"). Names are SOCOM 1's. A block reports through a log
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
};

typedef struct TOSLOG {
    TOS *tos;                           /* +0 */
    unsigned char block;                /* +4 */
} TOSLOG;

unsigned char _tosErrorLog(TOSLOG *log, unsigned short code)
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

/* a block: the log record's head (the scheduler, its id), then its input queues, each a queue
 * and the block's reader in it */
typedef struct QREQ {
    void *q;
    unsigned short reader;
} QREQ;

typedef struct TOSBLOCK {
    TOS *tos;                           /* +0 */
    unsigned char id;                   /* +4 */
    char pad5[3];
    unsigned char ninputs;              /* +8 */
    char pad9[3];
    QREQ *inputs;                       /* +12 */
} TOSBLOCK;

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
