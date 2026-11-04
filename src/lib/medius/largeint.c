/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* LargeInt.c: the RT library's big integers (OVL2 0x626B10-, rt_crypt/src/LargeInt.c by the asserts its
 * prime functions make -- "/public/work/dev/rt_crypt/src/LargeInt.c", "nBits == lintGetBits(pPrime)"),
 * SCEA's rt_crypt, prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). SOCOM 1's object
 * (0x406C28: lintSetSmallValue, IsEven, IsZero ... lintPrime) rewritten: a number is still 32 words,
 * least significant first, but the tests walk up from word 0, IsGreater and its three siblings are
 * gone between IsEqual and the bit functions, and a word is added and taken with its carry where
 * SOCOM 1 added numbers. Names are SOCOM 1's where position and body agree (s1:order; IsEven
 * s1:exact); the rest keep placeholders. Only the functions written so far are here, in image
 * order (round 8b).
 *
 * Shapes, measured: IsEqual indexes plain word pointers (the target's address sums come index
 * first; a struct member's base first); lintGetBit puts the mask first, `(1 << n) & word`.
 * OPEN: func_00626D28 (17 words, the right size with the nonzero words' case written first and a
 * walking pointer: the target lays word 0's case at the loop's head -- entered straight from the
 * initialisation -- and the others' after the test, this one the other way about) and
 * func_00626D78 (28s: the same layout question, indexed). */

#define LINT_WORDS  32

typedef struct LINT {
    unsigned int d[LINT_WORDS];
} LINT;                                 /* 0x80 */

extern LINT data_006513E0;              /* zero */

void lintSetSmallValue(LINT *p, unsigned int nValue)
{
    *p = data_006513E0;
    p->d[0] = nValue;
}

int IsEven(LINT *p)
{
    return !(p->d[0] & 1);
}

int IsZero(LINT *p)
{
    int i;

    for (i = 0; i < LINT_WORDS; i++)
        if (p->d[i] != 0)
            return 0;
    return 1;
}

int IsOne(LINT *p)
{
    int i;

    if (p->d[0] != 1)
        return 0;
    for (i = 1; i < LINT_WORDS; i++)
        if (p->d[i] != 0)
            return 0;
    return 1;
}

int IsEqual(LINT *a, LINT *b)
{
    unsigned int *pa = a->d;
    unsigned int *pb = b->d;
    int i;

    for (i = 0; i < LINT_WORDS; i++)
        if (pa[i] != pb[i])
            return 0;
    return 1;
}

int lintGetBit(LINT *p, unsigned int nBit)
{
    return ((1 << nBit) & p->d[nBit >> 5]) != 0;
}

void lintSetBit(LINT *p, unsigned int nBit)
{
    p->d[nBit >> 5] |= 1 << nBit;
}

void lintClearBit(LINT *p, unsigned int nBit)
{
    p->d[nBit >> 5] &= ~(1 << nBit);
}

/* a word added, the carry run up (0x626D28) */
void func_00626D28(LINT *p, unsigned int n)
{
    unsigned int *pd = p->d;
    unsigned int i;

    for (i = 0; i < LINT_WORDS; i++, pd++) {
        if (i != 0) {
            (*pd)++;
            if (*pd != 0)
                break;
        } else {
            *pd += n;
            if (*pd >= n)
                break;
        }
    }
}

/* a word taken, the borrow run up (0x626D78) */
void func_00626D78(LINT *p, unsigned int n)
{
    unsigned int *pd = p->d;
    unsigned int i;

    for (i = 0; i < LINT_WORDS; i++) {
        if (i != 0) {
            if (pd[i] != 0) {
                pd[i]--;
                break;
            }
            pd[i] = 0xFFFFFFFF;
        } else {
            if (pd[i] >= n) {
                pd[i] -= n;
                break;
            }
            pd[i] -= n;
        }
    }
}

extern void *memcpy(void *d, const void *s, unsigned int n);
extern void lintAddIP(LINT *a, LINT *b);       /* largeint_asm.s */
extern void lintSubIP(LINT *a, LINT *b);

/* the sum and difference into a third number, through a copy (round 18b: SOCOM 1's lintAdd and
 * lintSub at their place, over the in-place assembly; memcpy both ways, GCC's block move with the
 * run-time alignment test of the side it does not know) */
void lintAdd(LINT *pResult, LINT *a, LINT *b)
{
    LINT t;

    memcpy(&t, a, sizeof t);
    lintAddIP(&t, b);
    memcpy(pResult, &t, sizeof t);
}

void lintSub(LINT *pResult, LINT *a, LINT *b)
{
    LINT t;

    memcpy(&t, a, sizeof t);
    lintSubIP(&t, b);
    memcpy(pResult, &t, sizeof t);
}
