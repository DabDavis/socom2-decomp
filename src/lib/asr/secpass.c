/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* secpass: the recogniser's second-pass tree context (OVL2 0x4D1C18-0x4D3318, SOCOM 1's object at
 * 0x342708: GetSecondPassInfo, putWordInDict (LOCAL), CreateSecondPassTreeContext), ScanSoft's
 * speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops
 * (README "The GCC front"). Names are SOCOM 1's. The language and the context are reached through
 * their virtual tables (language.c's _LangVirtualTable, contextv2.c's CONTEXTV2), slot for slot.
 * Round 27c: GetSecondPassInfo; putWordInDict (0x2F8) and CreateSecondPassTreeContext (0x1180) are
 * not written. */

typedef struct LANGVT LANGVT;
struct LANGVT {
    void *data;
    void *s1[22];
    int (*GetUserWordSpeechUnitClass)(LANGVT *l);   /* +92, slot 23 */
    void *s24[18];
    int *(*GetpSpeechUnit)(LANGVT *l);              /* +168, slot 42 */
};

typedef struct CTXVT CTXVT;
struct CTXVT {
    void *data;
    void *s1[15];
    unsigned short *(*GetpLex)(CTXVT *c);           /* +64 */
    int *(*GetpBeginOfWords)(CTXVT *c);             /* +68 */
};

typedef struct CASR {
    char pad[612];
    LANGVT *lng;                        /* +612 */
} CASR;

/* what the second pass keeps of a word: its states' count and its first and last speech units'
 * classes */
typedef struct SPINFO {
    int f0;
    short nstates;                      /* +4 */
    unsigned char first;                /* +6 */
    unsigned char last;                 /* +7 */
} SPINFO;

/* a word's second-pass record from its first pronunciation: a pronunciation of one item (1) is
 * its speech units' states less two each, its ends' classes from the units' first and last
 * entries (the ends left out of the count when there are three or more); any other is its length
 * and the user-word class at both ends. SOCOM 1's body (round 27c): the word's pronunciation
 * table indexed through its own pointer (`w = bow + *bow; w[word]`: the target scales the word
 * first), the item count taken and the length formed before the test (both arms' `n`), the else
 * arm's three stores in the reverse of the target's order (sched1 issues the last of equal stores
 * first: written nstates, first, last they come out last, first, nstates -- 3 words). */
int GetSecondPassInfo(CASR *c, CTXVT *ctx, SPINFO *info, int word)
{
    int *su = c->lng->GetpSpeechUnit(c->lng);
    unsigned char cls = c->lng->GetUserWordSpeechUnitClass(c->lng);
    unsigned short *lex = ctx->GetpLex(ctx);
    int *bow = ctx->GetpBeginOfWords(ctx);
    int *w = bow + *bow;
    unsigned short *p = lex + w[word];
    unsigned short *end = lex + w[word + 1];
    int n;
    unsigned short type;

    type = *p++;
    n = end - p;
    if (type == 1) {
        int k = 0;

        end--;
        info->first = ((unsigned short *)su)[su[*p]];
        info->last = ((unsigned short *)su)[su[*end] + 1];
        if (n >= 3) {
            p++;
            end--;
        }
        for (; p <= end; p++)
            k += su[*p + 1] - su[*p] - 2;
        info->nstates = k;
    } else {
        info->last = cls;
        info->first = cls;
        info->nstates = n;
    }
    return 0;
}

/* a word's states appended to the dictionary at *out: each speech unit's states but its first two
 * (a one-item pronunciation, its ends dropped when three or more) or the pronunciation itself, each
 * shifted left two, the last marked with bit 0, then the word's id in two halves; *count grows by
 * the states and the id. SOCOM 1's body (0x1F4 there, its copies not unrolled), round 28c: the
 * state count before the states' address (the target's sll after the loads, then the subu), a
 * counter of its own per copy loop (one `j` is one pseudo for both: t0 where the target has t1 and
 * a1), the mark through `*(o - 1)` (its address one register for the load and the store: `o[-1]`
 * folds the -2 into both) and the id's halves at o[0], o[1] before one `o += 2`. */
static void putWordInDict(CASR *c, CTXVT *ctx, int word, int *count, unsigned short **out,
                          unsigned int id)
{
    int *su = c->lng->GetpSpeechUnit(c->lng);
    unsigned short *lex = ctx->GetpLex(ctx);
    int *bow = ctx->GetpBeginOfWords(ctx);
    int *w = bow + *bow;
    unsigned short *p = lex + w[word];
    unsigned short *end = lex + w[word + 1];
    unsigned short *o = *out;
    unsigned int n, k, j, i, total = 0;
    unsigned short type;

    type = *p++;
    n = end - p;
    if (type == 1) {
        end--;
        if (n >= 3) {
            p++;
            end--;
        }
        for (; p <= end; p++) {
            unsigned short *s;

            k = su[*p + 1] - su[*p] - 2;
            s = (unsigned short *)su + su[*p] + 2;
            for (j = 0; j < k; j++)
                *o++ = s[j] << 2;
            total += k;
        }
    } else {
        total = n;
        for (i = 0; i < n; i++)
            *o++ = p[i] << 2;
    }
    *(o - 1) |= 1;
    o[0] = id;
    o[1] = id >> 16;
    o += 2;
    *count += total + 2;
    *out = o;
}
