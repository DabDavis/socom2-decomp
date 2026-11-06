/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* ctxinfo: the recogniser's context-information object (OVL2 0x4CCE90-0x4CE5F0, SOCOM 1's object at
 * 0x33E418: CreateSecondPassInfo (LOCAL), CreateExEvCtxInfo, CreateCtxInfo, FreeCtxInfo,
 * CalcAccuracyCtxInfo, CreateMenuMasks -- the same six in the same order), ScanSoft's speech
 * recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The
 * GCC front"). Names are SOCOM 1's (s1:calls, the object's order agreeing). CreateExEvCtxInfo and
 * CreateCtxInfo (0x48C, 0xC34) are not written, FreeCtxInfo stands as its matched draft. */
#include "casr.h"

void *heap_Alloc(void *heap, unsigned int size);

/* a word's record for the second pass: its score, its length and the speech-unit classes at its ends */
typedef struct SPINFO {
    int score;                          /* +0 */
    short len;                          /* +4 */
    unsigned char first;                /* +6 */
    unsigned char last;                 /* +7 */
} SPINFO;

/* each word's record: a pronunciation of kind 1 is a phoneme list -- its first and last phoneme's units
 * give the classes, the first's states (less two, when there are three or more) join the score, and the
 * inner phonemes' states the length; any other kind is the user word's class throughout. SOCOM 1's body. */
int CreateSecondPassInfo(CASR *c, unsigned short *lex, int *bow, int *scores, unsigned int n)
{
    int *su = c->lng->GetpSpeechUnit(c->lng);
    unsigned char cls = c->lng->GetUserWordSpeechUnitClass(c->lng);
    SPINFO *info = heap_Alloc(c->heap, n * sizeof(SPINFO));
    unsigned int i;

    if (info == 0)
        return 1;
    c->f588 = info;
    for (i = 0; i < n; i++) {
        unsigned short *p = lex + bow[i];
        unsigned short *e = lex + bow[i + 1];
        unsigned int kind = *p++;
        int len = e - p;

        if (kind == 1) {
            int adj = 0;
            int sum = 0;

            e--;
            info->first = *(unsigned char *)((short *)su + su[*p]);
            info->last = ((unsigned char *)((short *)su + su[*e]))[2];
            if (len >= 3) {
                unsigned int x = *p++;

                e--;
                adj = su[x + 1] - su[x] - 2;
            }
            info->score = *scores++ + adj;
            for (; p <= e; p++) {
                unsigned int x = *p;

                sum += (unsigned short)((unsigned short)(((unsigned short *)su)[x * 2 + 2] - ((unsigned short *)su)[x * 2]) - 2);
            }
            info->len = sum;
        } else {
            info->score = *scores++;
            info->len = len;
            info->last = cls;
            info->first = cls;
        }
        info++;
    }
    return 0;
}

/* the context the engine runs (casr +580), as far as the accuracy reads it: its word boundaries,
 * n + 1 frame numbers */
typedef struct CTXINFO {
    int pad0;
    unsigned int n;                     /* +4 */
    char pad8[16];
    unsigned int *bounds;               /* +24 */
} CTXINFO;

/* a menu, as far as its mask goes */
typedef struct MENU {
    char pad0[32];
    unsigned char *mask;                /* +32: a bit per entry, the last byte's tail clear */
} MENU;

/* the second pass's accuracy window: six times the longest word, no less than 50 times the mean
 * word length, no less than 300 -- 1500 when the engine runs the DP. SOCOM 1's body: the mean
 * taken before the test (the target divides first), one result returned at the end (guard clauses
 * set v0 at each exit; the target keeps it in t1 to a final move), and the word count a variable
 * taken right after `end` -- combine then folds `(p + n) - p` to the target's `(n << 2) >> 2`,
 * where a subtraction written after the `*end` load (which uses end in between) stays `subu` */
unsigned int CalcAccuracyCtxInfo(CASR *c)
{
    CTXINFO *ci = c->run_ctx;
    unsigned int *p = ci->bounds;
    unsigned int *end = p + ci->n;
    unsigned int nw = end - p;
    unsigned int max = 0;
    unsigned int mean, acc;

    mean = (*end - *p) * 50 / nw;
    if (c->use_dp != 0)
        acc = 1500;
    else {
        for (; p < end; p++)
            if (max < p[1] - p[0])
                max = p[1] - p[0];
        acc = max * 6;
        if (acc < mean)
            acc = mean;
        if (acc < 300)
            acc = 300;
    }
    return acc;
}

/* a mask of n entries, all set: SOCOM 1's body (its fill loop unrolled by four here), the fill's
 * bound `size - 1` -- the unroller copies it (the target's `move t2,s1`; a bound variable of its
 * own is used as it stands) */
int CreateMenuMasks(CASR *c, MENU *m, unsigned int n)
{
    unsigned int size = ((n - 1) >> 3) + 1;
    unsigned char *mask;
    unsigned int i;

    mask = heap_Alloc(c->heap, size);
    m->mask = mask;
    if (mask == 0)
        return 1;
    for (i = 0; i < size - 1; i++)
        mask[i] = 0xFF;
    mask[i] = 0xFF << (-(n & 7) & 7);
    return 0;
}
