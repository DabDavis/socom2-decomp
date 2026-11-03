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

/* a training utterance (three at +980): its frames, its segmentation's length and the
 * segmentation itself (freed when the utterance is rejected) */
typedef struct UTTREC {
    unsigned int frames;                /* +0: below parameter +428, too short */
    int n;                              /* +4: the segmentation's length */
    void *seg;                          /* +8 */
} UTTREC;

#define CASR_MAGIC      0x218DF4F5      /* +552 of a live handle */
#define CASR_ERR_NULL   6
#define CASR_ERR_HANDLE 11
#define CASR_ERR_ARG    18

struct LANGVT;
struct CASR;

/* the engine's log head: the scheduler's TOSLOG (tos.c) -- the engine itself (its scheduler record
 * is the context's head), block 0 */
typedef struct CASRLOG {
    struct CASR *tos;
    unsigned char block;
} CASRLOG;

/* the host's callbacks as casrOpen takes them (asrspi.c's g_APIRecogCallBacks), copied whole */
typedef struct CASRCBTABLE {
    int (*ts_cb)(int ms, void *info);                   /* +508: a talk spurt */
    int (*start_cb)(int ms, void *info);                /* +512: speech started */
    int (*result_cb)(void *result, void *info);         /* +516: a result, or none */
    int (*state_cb)(int state, int ms, void *info);     /* +520 */
    int (*train_cb)(int state, void *info);             /* +524 */
    int (*abnorm_cb)(int code, int ms, void *info);     /* +528: an abnormal condition */
    int (*gain_cb)(unsigned short gain, void *info);    /* +532: the engine asks for a gain */
    int (*askgain_cb)(void *info);                      /* +536: ... for the gain in use */
} CASRCBTABLE;

/* what every callback is handed: a word casrOpen clears, and the host's data */
typedef struct CASRINFO {
    void *f0;
    void *user;
} CASRINFO;

typedef struct CASR {
    char pad0[50];
    unsigned short state;               /* +50 */
    int last_error;                     /* +52 */
    int last_warning;                   /* +56 */
    char pad60[304];
    CASRLOG event[1];                   /* +364: the engine's log head (_tosControl's) */
    int queue;                          /* +372 */
    unsigned short queue_id;            /* +376 */
    CBSLOT cur;                         /* +380: the sample format's slot in use */
    CBSLOT slot[4];                     /* +388 */
    /* the parameters (InitParamList's defaults; casrGetParam's ids in the comments) */
    unsigned int p420;                  /* +420: id 5, read in hundredths */
    int p424;                           /* +424: id 15 */
    int p428;
    int p432;
    int p436;
    int exev;                           /* +440: id 18, extra events on */
    int p444[3];                        /* +444: ids 11, 12 */
    int f456;                           /* +456: id 2 */
    int p460;                           /* +460: id 1 */
    int p464;                           /* +464: id 4 */
    int p468;                           /* +468: id 9 */
    int p472;                           /* +472: id 10 */
    int p476;                           /* +476: id 17 */
    int gender;                         /* +480: id 14 */
    short p484[2];                      /* +484: ids 8, 7 (read as 50 - x) */
    int p488;                           /* +488: id 6 */
    int p492;                           /* +492: id 13 */
    char pad496[8];
    int p504;                           /* +504: id 3 */
    CASRCBTABLE cb;                     /* +508 */
    int pad540;
    int (*train)(struct CASR *c, int a1, int a2, int state);   /* +544: cbTrain */
    unsigned char gender_on[4];         /* +548 */
    unsigned int magic;                 /* +552: CASR_MAGIC */
    CASRINFO info[1];                   /* +556: handed to every callback */
    void *heap;                         /* +564 */
    void *data;                         /* +568: the activated data */
    struct IMPORT *ctx;                 /* +572: the activated context */
    unsigned char use_dp;               /* +576 */
    unsigned char iso_word;             /* +577 */
    char pad578[2];
    void *run_ctx;                      /* +580: the context the engine runs (_tosControl 36) */
    char pad584[12];
    void *sp_ctx;                       /* +596: the second pass's (CreateSecondPassContext) */
    int pad600;
    void *iso_ctx;                      /* +604: the iso-word context, run again after a second pass */
    int f608;                           /* +608 */
    struct LANGVT *lng;                 /* +612 */
    char pad616[4];
    int uw_state;                       /* +620: also the sample format */
    int f624;                           /* +624 */
    void *f628;                         /* +628 */
    char *lab_buf[3];                   /* +632: each training utterance's labels */
    char *lab_pos[3];                   /* +644: where the next label goes */
    char *lab_end[3];                   /* +656: the buffer's end */
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
    int *frame_base;                    /* +912: the frame count the talk spurts are told from */
    unsigned char training;             /* +916: a user-word training is under way */
    char pad917[3];
    int f920;                           /* +920: cleared on a cancel */
    int f924;                           /* +924 */
    int uw_recs_needed;                 /* +928 */
    unsigned short uw_recs_done;        /* +932 */
    char pad934[6];
    unsigned char rejected;             /* +940: the last utterance was too short */
    char pad941[39];
    UTTREC utt[3];                      /* +980 */
    short uw_silence;                   /* +1016: the language's user-word silence phoneme */
    short pad1018;
    struct TRAINREC *trainrecs;         /* +1020: the training's records (train.c) */
    unsigned short nstates;             /* +1024: the training's states (train.c) */
    short pad1026;
    float *smooth[5];                   /* +1028: each record's smoothing matrix (train.c) */
    float *train1048;                   /* +1048: the smoothing's row sums; freed by TrainFree */
    float f1052;                        /* +1052: 0.0001 (casrOpen) */
    int pad1056;
    void *train1060;                    /* +1060: freed by TrainFree */
    int pad1064;
    short **segtab;                     /* +1068 */
    int pad1072;
    int last_state;                     /* +1076 */
    char pad1080[8];
    int f1088;                          /* +1088: parameter 16 */
    int pad1092;
    int f1096;                          /* +1096 */
    char pad1100[32];
} CASR;                                 /* 1132 bytes (casrOpen's allocation) */

typedef struct LANGVT LANGVT;

/* the language file's accessors (language.c's _LangVirtualTable, slot for slot) */
struct LANGVT {
    void *data;                                     /* +0 */
    void *s1[3];
    int (*GetNbrCodeBook)(LANGVT *l);                 /* +16, slot 4 */
    void *s5[10];
    short (*GetUWSilencePhenome)(LANGVT *l);          /* +60, slot 15 */
    void *s16[9];
    int (*GetNbrPhenUserWordTraining)(LANGVT *l);     /* +100, slot 25 */
    void *s26[20];
    void *(*GetpPhenUserWordTraining)(LANGVT *l);     /* +184, slot 46 */
    void *s47[5];
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
