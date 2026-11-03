/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* queue: the recogniser's data queues between its blocks (OVL2 0x509730-0x50AED8, SOCOM 1's object
 * at 0x3745D0: qQueueControl .. qCheckDeQueueOne, the same functions in the same order),
 * ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops (README "The GCC front"). The names are SOCOM 1's (s1:calls, s1:order). A queue
 * is a list of fixed-size elements with one read pointer per reader: a reader's pointer is its
 * next element, the queue itself when it has read everything, &first when the reader is stopped
 * and 0 when it is disabled. An element of up to seven words comes from the scheduler's fast
 * allocator and goes back on its free list; SOCOM 1's static qFreeElem, which does that, is
 * inlined everywhere here (its out-of-line copy is gone). */

typedef struct QELEM {
    struct QELEM *next;
    /* the element's data follows */
} QELEM;

typedef struct FASTALLO {
    void *heap;
    unsigned short chunk_size;
    unsigned short left;
    int nchunks;
    int **chunks;
    int *next;
    QELEM *free[65];                    /* +20: by size in words */
} FASTALLO;

typedef struct TOS {
    char pad0[60];
    int nenqueues;                      /* +60 */
    char pad64[16];
    void *heap;                         /* +80 */
    FASTALLO fa;                        /* +84 */
} TOS;

typedef struct QUEUE {
    TOS *tos;                           /* +0 */
    unsigned char id;                   /* +4: the queue's id in error reports */
    char pad5[3];
    unsigned short elemsize;            /* +8: bytes */
    unsigned short maxelems;            /* +10: 0 for unbounded */
    unsigned char state;                /* +12: 1 running, 2 stopped */
    unsigned char nreaders;             /* +13 */
    char pad14[2];
    QELEM *first;                       /* +16 */
    QELEM *read[1];                     /* +20: one per reader */
} QUEUE;

#define Q_STOPPED(q)    ((QELEM *)&(q)->first)
#define Q_END(q)        ((QELEM *)(q))

typedef struct TOSLOG {
    TOS *tos;
    unsigned char block;
} TOSLOG;

extern void *func_00243298(void *heap, int size);                  /* the heap's malloc */
extern void *func_00243398(void *heap, int count, int size);       /* the heap's calloc */
extern void heap_Free(void *heap, void *p);
extern void *memcpy(void *dst, const void *src, unsigned int n);
extern void *memset(void *p, int c, unsigned int n);
extern QELEM *fastallo_AllocateMemoryFromChunk(FASTALLO *fa, unsigned short n);
extern unsigned char _tosErrorLog(TOSLOG *log, unsigned short code);
extern int _tosGetProfileU32(void *block, int key, int def);


/* SOCOM 1's qFreeElem: an element of up to seven words back on the fast allocator's free list
 * of its size, a bigger one to the heap */
static __inline__ void qFreeElem(QUEUE *q, QELEM *e)
{
    if (q->elemsize + sizeof(QELEM) < 32) {
        QELEM **list = &q->tos->fa.free[(q->elemsize + sizeof(QELEM) + 3) >> 2];

        e->next = *list;
        *list = e;
    } else
        heap_Free(q->tos->heap, e);
}

/* the other way: a small element from its free list, else from the allocator's chunks */
static __inline__ QELEM *qAllocElem(QUEUE *q)
{
    QELEM *e;

    if (q->elemsize + sizeof(QELEM) < 32) {
        unsigned short n = (q->elemsize + sizeof(QELEM) + 3) >> 2;
        QELEM **list = &q->tos->fa.free[n];

        if (*list != 0) {
            e = *list;
            *list = e->next;
        } else
            e = fastallo_AllocateMemoryFromChunk(&q->tos->fa, n);
    } else
        e = func_00243298(q->tos->heap, q->elemsize + sizeof(QELEM));
    return e;
}

typedef struct QREQ {
    QUEUE *q;
    unsigned short reader;
} QREQ;

int qCheckInputQueues(QREQ *req, unsigned int n);
void qQueueReset(QUEUE *q);
static void qUpdateReadPtrsAndIncNbrOfEnqueues(QUEUE *q, QELEM *e);
void qFreeUnusedElemements(QUEUE *q);
int qQueueJumpBack(QUEUE *q, unsigned short n);
int qQueueJumpBackOne(QUEUE *q, unsigned short reader, unsigned short n);

/* the queue's own settings out of the scheduler's profile: the element size (2), whether the
 * readers start running (3) and the bound (4) */
QUEUE *qQueueInit(QUEUE *q)
{
    int size;
    unsigned int i;

    size = _tosGetProfileU32(q, 2, -1);
    if (size == -1 || size == q->elemsize) {
        qQueueReset(q);
        if ((unsigned char)_tosGetProfileU32(q, 3, 1)) {
            for (i = 0; i < q->nreaders; i++)
                if (q->read[i] != Q_STOPPED(q))
                    q->read[i] = Q_END(q);
        } else {
            for (i = 0; i < q->nreaders; i++)
                q->read[i] = 0;
        }
        q->maxelems = (unsigned char)_tosGetProfileU32(q, 4, 0);
        return q;
    }
    return 0;
}

/* a new element for the readers at the end; past the bound, the oldest elements no reader still
 * needs are freed (the first one some reader points at stops it). Each loop has its own counter. */
static void qUpdateReadPtrsAndIncNbrOfEnqueues(QUEUE *q, QELEM *e)
{
    QELEM *p;
    QELEM *t;
    unsigned int count;
    unsigned int i;
    unsigned int j;
    unsigned int k;

    if (q->maxelems != 0) {
        count = 0;
        for (p = q->first; p != 0; p = p->next)
            count++;
        if (q->maxelems < count) {
            count -= q->maxelems;
            p = q->first;
            for (i = 0; i < count; i++) {
                t = p;
                for (j = 0; j < q->nreaders; j++)
                    if (q->read[j] == t)
                        break;
                if (j < q->nreaders)
                    break;
                p = p->next;
                qFreeElem(q, t);
            }
            q->first = p;
        }
    }
    for (k = 0; k < q->nreaders; k++)
        if (q->read[k] == Q_END(q))
            q->read[k] = e;
    q->tos->nenqueues++;
}

/* the elements no reader points at any more freed from the front -- none while the queue is
 * bounded and a reader is disabled */
void qFreeUnusedElemements(QUEUE *q)
{
    QELEM *e = q->first;
    QELEM *p;
    unsigned int i;
    unsigned int j;

    if (e == 0)
        return;
    if (q->maxelems != 0)
        for (j = 0; j < q->nreaders; j++)
            if (q->read[j] != Q_STOPPED(q) && q->read[j] == 0)
                return;
    while (e != 0) {
        for (i = 0; i < q->nreaders; i++)
            if (q->read[i] == e)
                break;
        if (i != q->nreaders)
            break;
        p = e;
        e = e->next;
        qFreeElem(q, p);
    }
    q->first = e;
}

QUEUE *qQueueConstruct(TOS *tos, unsigned char id, unsigned char nreaders)
{
    QUEUE *q = func_00243398(tos->heap, 1, sizeof(QUEUE) - sizeof(QELEM *) + nreaders * sizeof(QELEM *));
    TOSLOG log;

    if (q == 0) {
        log.tos = tos;
        log.block = id;
        _tosErrorLog(&log, 2);
        return 0;
    }
    q->nreaders = nreaders;             /* this order schedules as the target's */
    q->id = id;
    q->tos = tos;
    return q;
}

QUEUE *qQueueInitEx(QUEUE *q, unsigned short elemsize, unsigned char run, unsigned short maxelems)
{
    unsigned short size;
    unsigned int i;

    if (elemsize != 0)
        size = elemsize;
    else
        size = _tosGetProfileU32(q, 2, -1);
    q->elemsize = size;
    qQueueReset(q);
    if (run == 0xFF)
        run = _tosGetProfileU32(q, 3, 1);
    if (run) {
        for (i = 0; i < q->nreaders; i++)
            q->read[i] = Q_END(q);
    } else {
        for (i = 0; i < q->nreaders; i++)
            q->read[i] = 0;
    }
    if (maxelems != 0xFFFF)
        q->maxelems = maxelems;
    else
        q->maxelems = _tosGetProfileU32(q, 4, 0);
    return q;
}

QUEUE *qQueueReDimension(QUEUE *q, unsigned short elemsize)
{
    QELEM *e = q->first;
    QELEM *p;
    unsigned short size;

    if (elemsize != 0)
        size = elemsize;
    else
        size = _tosGetProfileU32(q, 2, -1);
    while (e != 0) {
        p = e;
        e = e->next;
        qFreeElem(q, p);
    }
    q->elemsize = size;
    q->first = 0;
    return q;
}

/* n elements copied in at the end; each reader at the end now reads the first of them.
 * OPEN (the size differs): byte-exact as written without -funroll-loops, but this 2.96 unrolls the
 * loop by two (preconditioned on n) where the target's is rolled, though the object's other
 * counted loops are unrolled (qQueueJumpBackOne's, by four). Something in the vendor's body
 * stops the unroller and leaves the code alone: two empty `asm volatile` statements in it do
 * (4 words off, the barrier holding memcpy's argument out of its delay slot); a signed or long
 * counter, a while or do-while loop, the step in the for header do not
 * (build/scratch/gcc/p/q5.c). */
int qEnQueue(QUEUE *q, char *data, unsigned int n)
{
    QELEM *first = 0;
    QELEM *last;
    QELEM *e;
    QELEM *p;
    unsigned int i;

    if (q->state == 2 && q->maxelems == 0)
        return 0;
    last = 0;
    p = q->first;
    if (p != 0) {
        while (p->next != 0)
            p = p->next;
        last = p;
    }
    for (i = 0; i < n; i++) {
        e = qAllocElem(q);
        if (first == 0)
            first = e;
        if (last != 0)
            last->next = e;
        else
            q->first = e;
        last = e;
        memcpy(e + 1, data, q->elemsize);
        data += q->elemsize;
    }
    if (last != 0)
        last->next = 0;
    qUpdateReadPtrsAndIncNbrOfEnqueues(q, first);
    return 1;
}

/* n elements copied out for one reader, only if n are there */
int qDeQueue(QUEUE *q, int reader, char *buf, unsigned int n)
{
    QELEM *e = q->read[reader];
    QELEM *p;
    unsigned int i;

    if (e == 0 || e == Q_END(q))
        return 0;
    p = e;
    for (i = 0; i < n; i++) {
        if (p == 0)
            return 0;
        p = p->next;
    }
    for (i = 0; i < n; i++) {
        memcpy(buf, e + 1, q->elemsize);
        buf += q->elemsize;
        e = e->next;
    }
    if (e != 0)
        q->read[reader] = e;
    else
        q->read[reader] = Q_END(q);
    qFreeUnusedElemements(q);
    return 1;
}

void *qEnQueueOne(QUEUE *q)
{
    QELEM *e;
    QELEM *p;

    if (q->state == 2 && q->maxelems == 0)
        return 0;
    e = qAllocElem(q);
    p = q->first;
    if (p != 0) {
        while (p->next != 0)
            p = p->next;
        p->next = e;
    } else
        q->first = e;
    e->next = 0;
    qUpdateReadPtrsAndIncNbrOfEnqueues(q, e);
    return e + 1;
}

/* inline: qCheckDeQueueOne has it inlined (as SOCOM 1's has) */
__inline__ void *qDeQueueOne(QUEUE *q, int reader)
{
    QELEM *e = q->read[reader];

    if (e == Q_STOPPED(q) || e == 0 || e == Q_END(q))
        return 0;
    q->read[reader] = e->next ? e->next : Q_END(q);
    return e + 1;
}

void qQueueReset(QUEUE *q)
{
    QELEM *e = q->first;
    QELEM *p;
    unsigned int i;

    while (e != 0) {
        p = e;
        e = e->next;
        qFreeElem(q, p);
    }
    for (i = 0; i < q->nreaders; i++)
        if (q->read[i] != Q_STOPPED(q))
            q->read[i] = Q_END(q);
    q->first = 0;
    q->state = 1;
}

/* every running reader moved back to the n-th element from the end */
int qQueueJumpBack(QUEUE *q, unsigned short n)
{
    QELEM *e;
    unsigned int count;
    unsigned int i;

    if (n != 0) {
        count = 0;
        for (e = q->first; e != 0; e = e->next)
            count++;
        if (count < n)
            return 0;
        count -= n;
        e = q->first;
        for (i = 0; i < count; i++)
            e = e->next;
        for (i = 0; i < q->nreaders; i++)
            if (q->read[i] != Q_STOPPED(q) && q->read[i] != 0)
                q->read[i] = e;
        qFreeUnusedElemements(q);
    }
    return 1;
}

int qQueueJumpBackOne(QUEUE *q, unsigned short reader, unsigned short n)
{
    QELEM *e;
    unsigned int count;
    unsigned int i;

    if (n != 0) {
        count = 0;
        for (e = q->first; e != 0; e = e->next)
            count++;
        if (count < n)
            return 0;
        count -= n;
        e = q->first;
        for (i = 0; i < count; i++)
            e = e->next;
        q->read[reader] = e;
    }
    return 1;
}

unsigned int qQueueNbrElements(QUEUE *q)
{
    QELEM *e;
    unsigned int n = 0;

    for (e = q->first; e != 0; e = e->next)
        n++;
    return n;
}

/* one element from each of n queues, when every one has one: the scheduler's read of a
 * block's inputs */
int qCheckDeQueueOne(QREQ *req, unsigned int n, void **out)
{
    unsigned int i;
    void *p;

    if (!qCheckInputQueues(req, n)) {
        memset(out, 0, n * sizeof(void *));
        return 0;
    }
    for (i = 0; i < n; i++) {
        p = 0;
        if (req->q != 0)
            p = qDeQueueOne(req->q, req->reader);
        *out++ = p;
        req++;
    }
    return 1;
}
