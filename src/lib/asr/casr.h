/* casr.h: the ScanSoft recogniser's engine context, as far as the hand-written units use it
 * (fields named by their use; offsets from the code). */
#ifndef CASR_H
#define CASR_H

typedef struct SEGLABEL {
    int pad0;
    struct SEGLABEL *next;              /* +4 */
    char pad8[8];
    unsigned short word;                /* +16: the phoneme (dpscoreuw.c's BTNODE) */
    short pad18;
    unsigned short label;               /* +20: its first frame */
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

/* the scheduler's small-block allocator (fastallo.c's FASTALLO): blocks of up to 64 words, a free
 * list per size in words */
typedef struct FASTALLO {
    void *heap;                         /* +0 */
    unsigned short chunk_size;          /* +4: bytes */
    unsigned short left;                /* +6: words left in the current chunk */
    int nchunks;                        /* +8 */
    int **chunks;                       /* +12 */
    int *next;                          /* +16: the current chunk's first free word */
    int *free[65];                      /* +20 */
} FASTALLO;

/* a context tree's node (12 bytes, three words of the allocator): its children, or at a leaf the
 * word it ends (an index, or ~ the first of a list of words), its phoneme and child count */
typedef struct TNODE {
    struct TNODE *children;             /* +0 */
    int pad4;
    unsigned short ph;                  /* +8 */
    unsigned short nchildren;           /* +10 */
} TNODE;

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
    char pad60[20];
    void *tos_heap;                     /* +80: the scheduler's heap */
    FASTALLO fa;                        /* +84: its small-block allocator (free lists from +104) */
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
    int p496;                           /* +496: id 1's copy (casrSetParam) */
    int p500;                           /* +500: id 4 doubled (casrSetParam) */
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
    char pad584[8];
    struct UWTABLES *uw_tables;         /* +592: the user words' tables (intCreateCtxTree's) */
    void *sp_ctx;                       /* +596: the second pass's (CreateSecondPassContext) */
    int pad600;
    TNODE *iso_ctx;                     /* +604: the iso-word context's tree, run again after a second pass */
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
    void **trans;                       /* +920: where DoUserWordTraining puts the trained transcription */
    int *trans_size;                    /* +924: and its size in bytes */
    int uw_recs_needed;                 /* +928 */
    unsigned short uw_recs_done;        /* +932 */
    char pad934[6];
    unsigned char rejected;             /* +940: the last utterance was too short */
    char pad941[3];
    /* +944: the user-word scoring's result (dpscoreuw.c's SCOREUWRESULT, cbScoreUserWordResultAvailable) */
    int uw_score;                       /* +944 */
    unsigned int uw_wordscore;          /* +948 */
    unsigned int uw_nframes;            /* +952 */
    float uw_perframe;                  /* +956 */
    int f960;                           /* +960: the last label scoring's result (CheckConsistency) */
    unsigned int uw_start;              /* +964 */
    unsigned int uw_end;                /* +968 */
    unsigned int uw_nnodes;             /* +972: the back pointers, the two silences included */
    SEGLABEL *uw_bt;                    /* +976: the last of them */
    UTTREC utt[3];                      /* +980 */
    short uw_silence;                   /* +1016: the language's user-word silence phoneme */
    short pad1018;
    struct TRAINREC *trainrecs;         /* +1020: the training's records (train.c) */
    unsigned short nstates;             /* +1024: the training's states (train.c) */
    short pad1026;
    float *smooth[5];                   /* +1028: each record's smoothing matrix (train.c) */
    float *train1048;                   /* +1048: the smoothing's row sums; freed by TrainFree */
    float f1052;                        /* +1052: 0.0001 (casrOpen): the probabilities' floor */
    float f1056;                        /* +1056: the log probabilities' scale (train.c) */
    float *train1060;                   /* +1060: one state's probabilities; freed by TrainFree */
    unsigned short maxcw;               /* +1064: the largest codebook (TrainInit) */
    short pad1066;
    short **segtab;                     /* +1068 */
    int pad1072;
    int last_state;                     /* +1076 */
    void *tree_words;                   /* +1080: the context tree's word lists (freed with the tree) */
    int tree_nwords;                    /* +1084 */
    int f1088;                          /* +1088: parameter 16 */
    int f1092;                          /* +1092: casrActivateContext's third argument (casrSetParam) */
    int f1096;                          /* +1096 */
    char pad1100[16];
    void *uw_states;                    /* +1116: the user words' states (freed with the tree) */
    int *uw_offsets;                    /* +1120: each user-word phoneme's first state */
    short uw_n;                         /* +1124 */
    short pad1126;
    int f1128;                          /* +1128 */
} CASR;                                 /* 1132 bytes (casrOpen's allocation) */

typedef struct LANGVT LANGVT;

/* the language file's accessors (language.c's _LangVirtualTable, slot for slot) */
struct LANGVT {
    void *data;                                     /* +0 */
    void *s1[3];
    int (*GetNbrCodeBook)(LANGVT *l);                 /* +16, slot 4 */
    unsigned int (*GetNbrState)(LANGVT *l);           /* +20, slot 5 */
    void *s6[5];
    int (*GetNbrErgodicPhenomes)(LANGVT *l);          /* +44, slot 11 */
    void *s12[2];
    short (*GetSilencePhenome)(LANGVT *l);            /* +56, slot 14 */
    short (*GetUWSilencePhenome)(LANGVT *l);          /* +60, slot 15 */
    void *s16[3];
    unsigned int (*GetNbrWarpFactors)(LANGVT *l);     /* +76, slot 19 */
    int (*GetNbrSpeechUnit)(LANGVT *l);               /* +80, slot 20 */
    void *s21[4];
    int (*GetNbrPhenUserWordTraining)(LANGVT *l);     /* +100, slot 25 */
    void *s26[5];
    unsigned int (*GetSecondCdbSize)(void *cb);       /* +124, slot 31 */
    void *s32;
    unsigned int (*GetNbrGastone)(void *cb);          /* +132, slot 33 */
    struct WARP *(*GetpWarpFactors)(LANGVT *l);       /* +136, slot 34 */
    unsigned short *(*GetpGastone)(LANGVT *l);        /* +140, slot 35 */
    void *s36[5];
    void *(*GetpCodeBook)(LANGVT *l, int n);          /* +164, slot 41 */
    int *(*GetpSpeechUnit)(LANGVT *l);                /* +168, slot 42 */
    void *s43[3];
    void *(*GetpPhenUserWordTraining)(LANGVT *l);     /* +184, slot 46 */
    void *s47[5];
    int (*CheckBitField)(LANGVT *l, int mask);        /* +208, slot 52 */
    void *s53[8];
    unsigned char *(*GetpProbMatrix)(void *cb);       /* +244, slot 61 */
    void *s62[17];
};

/* a warp factor (the language's, _langGetpWarpFactors): the genders it serves and its value */
typedef struct WARP {
    unsigned int mask;                  /* +0 */
    int value;                          /* +4 */
} WARP;

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
