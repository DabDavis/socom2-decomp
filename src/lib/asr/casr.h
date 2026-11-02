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

#define CASR_MAGIC      0x218DF4F5      /* +552 of a live handle */
#define CASR_ERR_NULL   6
#define CASR_ERR_HANDLE 11
#define CASR_ERR_ARG    18

typedef struct CASR {
    char pad0[50];
    unsigned short state;               /* +50 */
    int last_error;                     /* +52 */
    int last_warning;                   /* +56 */
    char pad60[304];
    char event[8];                      /* +364: the engine's event (func_0050CDE8) */
    int queue;                          /* +372 */
    unsigned short queue_id;            /* +376 */
    char pad378[10];
    CBSLOT slot[4];                     /* +388 */
    char pad420[60];
    int gender;                         /* +480 */
    char pad484[36];
    int (*state_cb)(int state, int ms, void *info);    /* +520 */
    char pad524[24];
    unsigned char gender_on[4];         /* +548 */
    unsigned int magic;                 /* +552: CASR_MAGIC */
    char info[20];                      /* +556 */
    unsigned char use_dp;               /* +576 */
    unsigned char iso_word;             /* +577 */
    char pad578[42];
    int uw_state;                       /* +620 */
    char pad624[44];
    int frames;                         /* +668 */
    unsigned char f672;
    unsigned char f673;
    unsigned char f674;
    unsigned char second_pass;          /* +675 */
    int pad676;
    int sp_arg1;                        /* +680 */
    int sp_arg2;                        /* +684 */
    char pad688[218];
    unsigned short f906;
    unsigned short *signal;             /* +908: the input level */
    char pad912[156];
    short **segtab;                     /* +1068 */
    int pad1072;
    int last_state;                     /* +1076 */
} CASR;

#endif
