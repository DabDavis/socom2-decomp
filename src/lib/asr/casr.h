/* casr.h: the ScanSoft recogniser's engine context, as far as the hand-written units use it
 * (fields named by their use; offsets from the code). */
#ifndef CASR_H
#define CASR_H

typedef struct SEGLABEL {
    int pad0;
    struct SEGLABEL *next;              /* +4 */
    char pad8[12];
    unsigned short label;               /* +20 */
} SEGLABEL;

typedef struct CBSLOT {
    int cb;
    int data;
} CBSLOT;

typedef struct SEGINFO {
    int n;                              /* the segment's length */
    int pad[2];
} SEGINFO;

#define CASR_MAGIC      0x218DF4F5      /* +552 of a live handle */
#define CASR_ERR_NULL   6
#define CASR_ERR_HANDLE 11
#define CASR_ERR_ARG    18

struct LANGVT;

typedef struct CASR {
    char pad0[50];
    unsigned short state;               /* +50 */
    int last_error;                     /* +52 */
    int last_warning;                   /* +56 */
    char pad60[304];
    char event[8];                      /* +364: the engine's event (func_0050CDE8) */
    int queue;                          /* +372 */
    unsigned short queue_id;            /* +376 */
    CBSLOT cur;                         /* +380: the sample format's slot in use */
    CBSLOT slot[4];                     /* +388 */
    int p420;                           /* +420: the parameter list (InitParamList) */
    int p424;
    int p428;
    int p432;
    int p436;
    int exev;                           /* +440: extra events on */
    int p444[3];                        /* +444 */
    int f456;                           /* +456 */
    char pad460[20];
    int gender;                         /* +480 */
    int p484;                           /* +484 */
    int p488;                           /* +488 */
    char pad492[28];
    int (*state_cb)(int state, int ms, void *info);    /* +520 */
    int (*train_cb)(int state, void *info);             /* +524 */
    char pad528[20];
    unsigned char gender_on[4];         /* +548 */
    unsigned int magic;                 /* +552: CASR_MAGIC */
    char info[8];                       /* +556: handed to every callback */
    void *heap;                         /* +564 */
    void *data;                         /* +568: the activated data */
    struct IMPORT *ctx;                 /* +572: the activated context */
    unsigned char use_dp;               /* +576 */
    unsigned char iso_word;             /* +577 */
    char pad578[30];
    int f608;                           /* +608 */
    struct LANGVT *lng;                 /* +612 */
    char pad616[4];
    int uw_state;                       /* +620: also the sample format */
    int f624;                           /* +624 */
    void *f628;                         /* +628 */
    int f632[3];                        /* +632 */
    int f644[3];                        /* +644 */
    char pad656[12];
    int frames;                         /* +668 */
    unsigned char f672;
    unsigned char f673;
    unsigned char f674;
    unsigned char second_pass;          /* +675 */
    int train_state;                    /* +676 */
    int sp_arg1;                        /* +680 */
    int sp_arg2;                        /* +684 */
    char pad688[218];
    unsigned short f906;
    unsigned short *signal;             /* +908: the input level */
    char pad912[16];
    int uw_recs_needed;                 /* +928 */
    unsigned short uw_recs_done;        /* +932 */
    char pad934[50];
    SEGINFO seginfo[3];                 /* +984 */
    char pad1020[48];
    short **segtab;                     /* +1068 */
    int pad1072;
    int last_state;                     /* +1076 */
    char pad1080[16];
    int f1096;                          /* +1096 */
} CASR;

typedef struct LANGVT LANGVT;

/* the language file's accessors (language.c's _LangVirtualTable, slot for slot) */
struct LANGVT {
    void *data;                                     /* +0 */
    void *s1[3];
    int (*GetNbrCodeBook)(LANGVT *l);                 /* +16, slot 4 */
    void *s5[20];
    int (*GetNbrPhenUserWordTraining)(LANGVT *l);     /* +100, slot 25 */
    void *s26[26];
    int (*CheckBitField)(LANGVT *l, int mask);        /* +208, slot 52 */
    void *s53[26];
};

typedef struct SAMPLEFMT {
    int pad[2];
    unsigned short frame;               /* +8 */
} SAMPLEFMT;

/* every entry point's handle check: 2.96 if-converts the magic test into `li 11; movz` */
static __inline__ int casrCheckHandle(CASR *c)
{
    if (c == 0)
        return CASR_ERR_NULL;
    if (c->magic != CASR_MAGIC)
        return CASR_ERR_HANDLE;
    return 0;
}

#endif
