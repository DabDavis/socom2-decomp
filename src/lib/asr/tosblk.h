/* tosblk.h: the recogniser's processing blocks as the scheduler (tos.c) sees them, for the block
 * objects (the delay line, the look-ahead, the stacker ...): a block is the scheduler's record --
 * its log head (the scheduler and the block's id), its input and output queue tables, the three
 * functions the scheduler calls -- with the block's own data after it, from +40. Every block
 * object has the same four functions: Construct (tosBaseBlockConstruct with the block's size),
 * Init (reads the block's profile, sizes its queue elements, allocates), Process (one input
 * frame, or a null frame when the input is flushed) and Control (a command; 0xFF destroys).
 * Layouts are read off the code; names are the fields' uses. */
#ifndef TOSBLK_H
#define TOSBLK_H

typedef struct TOS {
    char pad0[80];
    void *heap;                         /* +80: the engine's heap (OVL1's heap_*) */
} TOS;

/* a block's reader in an input queue: the queue, the reader's id, the bytes it takes a frame */
typedef struct QIN {
    void *q;
    unsigned short reader;              /* +4 */
    unsigned short size;                /* +6 */
} QIN;

/* an output queue: the queue and the bytes the block writes a frame */
typedef struct QOUT {
    void *q;
    unsigned short size;                /* +4 */
    unsigned short pad6;
} QOUT;

typedef int (*TOSFUNC)();

typedef struct TOSBLOCK {
    TOS *tos;                           /* +0 */
    unsigned char id;                   /* +4 */
    char pad5[3];
    unsigned char ninputs;              /* +8 */
    unsigned char noutputs;             /* +9 */
    char pad10[2];
    QIN *inputs;                        /* +12 */
    QOUT *outputs;                      /* +16 */
    TOSFUNC init;                       /* +20 */
    TOSFUNC process;                    /* +24 */
    TOSFUNC control;                    /* +28 */
    unsigned char state;                /* +32 */
    char pad33[7];
} TOSBLOCK;

/* the scheduler's control commands a block answers (Control's second argument) */
#define TOS_RESET       100
#define TOS_FLUSH       101
#define TOS_DESTROY     0xFF

/* tos.c */
TOSBLOCK *tosBaseBlockConstruct(TOS *tos, unsigned char id, unsigned char ninputs,
                                unsigned char noutputs, TOSFUNC process, TOSFUNC init,
                                TOSFUNC control, unsigned int size);
void tosBaseBlockDestruct(TOSBLOCK *b);
unsigned char _tosErrorLog(void *log, unsigned short code);
unsigned char _tosWarningLog(void *log, unsigned short code);
unsigned int _tosGetProfileU32(void *b, unsigned short key, unsigned int def);
float _tosGetProfileFloat(void *b, unsigned short key, float def);
unsigned char tosBaseBlockStopInputQueues(TOSBLOCK *b);
unsigned char tosBaseBlockDisableInputQueues(TOSBLOCK *b);
unsigned char tosBaseBlockStartInputQueues(TOSBLOCK *b, int n);

/* queue.c */
void *qEnQueueOne(void *q);

/* OVL1's heap (heap.c) */
void *heap_Calloc(void *heap, unsigned int n, unsigned int size);
void *heap_Alloc(void *heap, unsigned int size);
void heap_Free(void *heap, void *p);

/* newlib */
void *memcpy(void *dst, const void *src, unsigned int n);
void *memset(void *dst, int c, unsigned int n);

#endif
