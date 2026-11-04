/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* LargeInt.c: the RT library's big integers (OVL2 0x626B10-, rt_crypt/src/LargeInt.c by the asserts its
 * prime functions make -- "/public/work/dev/rt_crypt/src/LargeInt.c", "nBits == lintGetBits(pPrime)"),
 * SCEA's rt_crypt, prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). SOCOM 1's object
 * (0x406C28: lintSetSmallValue, IsEven, IsZero ... lintPrime) rewritten: a number is still 32 words,
 * least significant first, but the tests walk up from word 0, IsGreater and its three siblings are
 * gone between IsEqual and the bit functions (into LargeInt's assembly, largeint_asm.s, with the
 * shifts, the in-place add and subtract, MODIP, MULT and XMOD), and the operations that returned a
 * number by value take a result pointer and copy it out (memcpy, GCC's block move). Names are
 * SOCOM 1's where position and body agree (s1:order; IsEven s1:exact), in image order: the whole C
 * part of the object, 0x626B10-0x628B44, but the Montgomery exponentiation 0x6283F8 and its helper
 * 0x628390 (round 8b; round 18b lintAdd ... QRMOD; round 19b QUO, GCD, EMOD, lintIsbSPRP, LucasTest,
 * lintRandomize, lintPrime).
 *
 * Shapes, measured: IsEqual indexes plain word pointers (the target's address sums come index
 * first; a struct member's base first); lintGetBit puts the mask first, `(1 << n) & word`; QUO's
 * divisor-length exit is a `break` in an else (an `if (...) break;` before the shift lies within the
 * 30 insns 2.96's expand_end_loop scans, so the loop was rotated up to it and its test never copied
 * above the loop -- the target's test is); lintPrime's delta walk is an index (2.96 makes it the
 * target's pointer; a pointer in the source is set before the counter's first test).
 * OPEN: lintIncrement (17 words, the right size with the nonzero words' case written first and a
 * walking pointer: the target lays word 0's case at the loop's head -- entered straight from the
 * initialisation -- and the others' after the test, this one the other way about),
 * lintDecrement (28s: the same layout question, indexed). */

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
void lintIncrement(LINT *p, unsigned int n)
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
void lintDecrement(LINT *p, unsigned int n)
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

extern void MODIP(LINT *a, LINT *b);   /* largeint_asm.s: a %= b in place */

/* the remainder into a third number, through a copy (SOCOM 1's MOD, lintPrime's call; round 18b) */
void MOD(LINT *pResult, LINT *a, LINT *b)
{
    LINT t;

    memcpy(&t, a, sizeof t);
    MODIP(&t, b);
    memcpy(pResult, &t, sizeof t);
}

extern unsigned int lintGetBits(LINT *p);       /* largeint_asm.s */
extern int IsNotSmaller(LINT *a, LINT *b);
extern void ShiftLeft(LINT *p, unsigned int n);
extern void ShiftRight(LINT *p, unsigned int n);

/* the quotient into a third number (0x627270, SOCOM 1's QUO by position -- after MOD, before QRMOD
 * -- and by its calls, in order): QRMOD's long division over copies of both, the quotient copied
 * out (memcpy, as MOD) where SOCOM 1 returned it by value */
void QUO(LINT *pResult, LINT *a, LINT *b)
{
    LINT r;
    LINT d;
    LINT q;
    unsigned int nBitsR;
    unsigned int nBitsD;
    unsigned int nBitsB;

    memcpy(&r, a, sizeof r);
    memcpy(&d, b, sizeof d);
    nBitsR = lintGetBits(&r);
    nBitsD = lintGetBits(&d);
    nBitsB = nBitsD;
    lintSetSmallValue(&q, 0);
    if (nBitsD < nBitsR) {
        ShiftLeft(&d, nBitsR - nBitsD);
        nBitsD = nBitsR;
    }
    while (nBitsR >= nBitsB) {
        if (nBitsR < nBitsD) {
            ShiftRight(&d, nBitsD - nBitsR);
            nBitsD = nBitsR;
        }
        if (IsNotSmaller(&r, &d)) {
            lintSubIP(&r, &d);
            nBitsR = lintGetBits(&r);
            lintSetBit(&q, nBitsD - nBitsB);
        } else {
            if (nBitsB < nBitsD) {
                ShiftRight(&d, 1);
                nBitsD--;
            } else
                break;
        }
    }
    memcpy(pResult, &q, sizeof q);
}

/* the quotient and remainder of a by b (SOCOM 1's QRMOD by its calls, in order): the
 * divisor shifted up to the remainder's length, then subtracted down a bit at a time */
void QRMOD(LINT *a, LINT *b, LINT *pQuot, LINT *pRem)
{
    LINT d;
    unsigned int nBitsR;
    unsigned int nBitsD;
    unsigned int nBitsB;

    memcpy(&d, b, sizeof d);
    memcpy(pRem, a, sizeof d);
    nBitsR = lintGetBits(pRem);
    nBitsD = lintGetBits(&d);
    nBitsB = nBitsD;
    lintSetSmallValue(pQuot, 0);
    if (nBitsD < nBitsR) {
        ShiftLeft(&d, nBitsR - nBitsD);
        nBitsD = nBitsR;
    }
    for (;;) {
        if (nBitsR < nBitsB)
            return;
        if (nBitsR < nBitsD) {
            ShiftRight(&d, nBitsD - nBitsR);
            nBitsD = nBitsR;
        }
        if (IsNotSmaller(pRem, &d)) {
            lintSubIP(pRem, &d);
            nBitsR = lintGetBits(pRem);
            lintSetBit(pQuot, nBitsD - nBitsB);
        } else {
            if (nBitsB >= nBitsD)
                return;
            ShiftRight(&d, 1);
            nBitsD--;
        }
    }
}

/* the greatest common divisor into a third number (SOCOM 1's GCD, lintPrime's call): Euclid's
 * remainders taken in place on copies of both, the one left nonzero copied out */
void GCD(LINT *pResult, LINT *a, LINT *b)
{
    LINT x;
    LINT y;

    memcpy(&x, a, sizeof x);
    memcpy(&y, b, sizeof y);
    for (;;) {
        MODIP(&x, &y);
        if (IsZero(&x)) {
            memcpy(pResult, &y, sizeof y);
            return;
        }
        MODIP(&y, &x);
        if (IsZero(&y)) {
            memcpy(pResult, &x, sizeof x);
            return;
        }
    }
}

extern void XMOD(LINT *pResult, LINT *a, LINT *b, LINT *n);   /* largeint_asm.s: a * b mod n */
extern void __assert(const char *file, int line, const char *failedexpr);
#define assert(e)   ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

/* a to the e mod n into a fourth number (0x627AF0, SOCOM 1's EMOD by its body at its place -- after
 * GCD, MULT and XMOD gone into the assembly): the bits of e from the bottom, b squared mod n each
 * step and r multiplied by it where the bit is set; a zero modulus asserts and gives itself back */
void EMOD(LINT *pResult, LINT *a, LINT *e, LINT *n)
{
    LINT r;
    LINT b;
    unsigned int nBits;
    unsigned int nWord;
    unsigned int i;
    unsigned int nMask = 1;

    nBits = lintGetBits(e);
    MOD(&b, a, n);
    if (IsZero(n)) {
#line 663 "/public/work/dev/rt_crypt/src/LargeInt.c"
        assert(0);
        memcpy(pResult, n, sizeof *pResult);
        return;
    }
    lintSetSmallValue(&r, 1);
    nWord = 0;
    for (i = 0; i < nBits; i++) {
        if (i != 0) {
            XMOD(&b, &b, &b, n);
            nMask <<= 1;
            if (nMask == 0) {
                nMask = 1;
                nWord++;
            }
        }
        if (nMask & e->d[nWord])
            XMOD(&r, &r, &b, n);
    }
    memcpy(pResult, &r, sizeof r);
}

extern void func_006283F8(LINT *pResult, LINT *a, LINT *e, LINT *n);    /* a to the e mod n, below */

/* the strong probable-prime test of n to base b (SOCOM 1's lintIsbSPRP, lintPrime's call): n - 1 =
 * d * 2^s, then b^d and its squares against 1 and n - 1. SOCOM 1's GCD test of n and b is gone, n - 1
 * is the word decrement's and the power the new exponentiation's.
 * The squaring loop is labels and gotos, no C loop statement (round 20b): the target enters it by a
 * jump to its one test at the bottom (`sltu v0,r,s; bnez body`, r++ in the slot -- the body's first
 * insn, stolen by reorg) and keeps x's address in one register. Any loop statement (for, while,
 * do-while, a goto into a do-while; round 19b's eleven spellings) gets loop notes, so jump.c copies
 * the test above the loop (duplicate_loop_exit_test) and loop.c moves x's address into a second
 * register; with no loop notes neither pass runs. The first IsEqual's failure branch holds r = 1 and
 * jumps to the test (reorg steals the 1 into its slot), and its `return 1` is the block the loop's
 * IsEqual cross-jumps to -- `if (IsEqual) return 1; r = 1; goto test;` inverts that branch (34s), and
 * `r++ < s` in the test copies r first (`move v0,s2`). */
int lintIsbSPRP(LINT *n, LINT *b)
{
    LINT nm1;
    LINT d;
    LINT x;
    unsigned int s;
    unsigned int r;

    memcpy(&nm1, n, sizeof nm1);
    lintDecrement(&nm1, 1);
    memcpy(&d, &nm1, sizeof d);
    for (s = 0; !lintGetBit(&d, s); s++)
        ;
    if (s != 0)
        ShiftRight(&d, s);
    func_006283F8(&x, b, &d, n);
    if (IsOne(&x))
        return 1;
    if (!IsEqual(&x, &nm1)) {
        r = 1;
        goto test;
    }
    return 1;
loop:
    r++;
    XMOD(&x, &x, &x, n);
    if (IsEqual(&x, &nm1))
        return 1;
    if (IsOne(&x))
        return 0;
test:
    if (r < s)
        goto loop;
    return 0;
}

/* the Lucas-style check that follows the probable-prime tests (SOCOM 1's LucasTest, lintPrime's
 * last call; the same size in both builds): b to the (n - 1) / 2 mod n is not 1 and its square is */
int LucasTest(LINT *n, LINT *b)
{
    LINT x;
    LINT e;

    memcpy(&e, n, sizeof e);
    lintDecrement(&e, 1);
    if (IsEven(&e)) {
        ShiftRight(&e, 1);
        func_006283F8(&x, b, &e, n);
        if (IsOne(&x))
            return 0;
        XMOD(&x, &x, &x, n);
        if (IsOne(&x))
            return 1;
    }
    return 0;
}

extern void setNewSeed(unsigned int nSeed);    /* rt_mt.c */
extern unsigned int randomMT(void);

/* nBits random bits into p, the top one set (SOCOM 1's lintRandomize, lintPrime's first call): the
 * twister seeded once, a word xored in per 32 bits, the last word's bits shifted down under its
 * top bit, the words above cleared */
void lintRandomize(LINT *p, unsigned int nBits)
{
    static char bHasInitialized = 0;
    unsigned int i = 0;

    if (nBits > 1024)
        nBits = 1024;
    if (!bHasInitialized) {
        bHasInitialized = 1;
        setNewSeed(0x2EB3AE77);
    }
    while (nBits > 32) {
        p->d[i] ^= randomMT();
        nBits -= 32;
        i++;
    }
    p->d[i] = (randomMT() | 0x80000000) >> (32 - nBits);
    for (i++; i < LINT_WORDS; i++)
        p->d[i] = 0;
}

extern LINT gCommonPrimes;              /* the small primes' products, LargeInt.c's .data */
extern LINT gCommonPrimes2;
extern unsigned int gPrimeSearchDeltas[];

/* a random nBits prime into pPrime (SOCOM 1's lintPrime, RSAGenerateKeyPair's draw; its thirteen
 * calls SOCOM 1's in order): an odd candidate with its second bit set, moved to 210k + 1, then
 * stepped by the deltas until it shares no factor with the small primes and passes the probable-
 * prime tests -- 127 steps, then a fresh draw. In place here, where SOCOM 1 returned a copy */
void lintPrime(LINT *pPrime, unsigned int nBits)
{
    LINT t;
    int bFound = 1;
    unsigned int nTries = 0;
    unsigned int i;

    do {
        lintRandomize(pPrime, nBits);
        pPrime->d[0] |= 1;
#line 864 "/public/work/dev/rt_crypt/src/LargeInt.c"
        assert(nBits == lintGetBits(pPrime));
        lintSetBit(pPrime, nBits - 2);
        lintSetSmallValue(&t, 210);
        MOD(&t, pPrime, &t);
        lintIncrement(pPrime, 210 - t.d[0]);
        i = 0;
        while ((++nTries & 0x7F) != 0) {
            lintIncrement(pPrime, gPrimeSearchDeltas[i++]);
            bFound = 0;
            GCD(&t, pPrime, &gCommonPrimes);
            if (IsOne(&t)) {
                GCD(&t, pPrime, &gCommonPrimes2);
                bFound = IsOne(&t) != 0;
            }
            if (bFound) {
                lintSetSmallValue(&t, 2);
                bFound = lintIsbSPRP(pPrime, &t);
                if (bFound) {
                    lintSetSmallValue(&t, 3);
                    bFound = LucasTest(pPrime, &t);
                    if (bFound)
                        return;
                }
            }
            if (gPrimeSearchDeltas[i] & 1) {
                lintIncrement(pPrime, gPrimeSearchDeltas[i]);
                i = 0;
            }
        }
    } while (!bFound);
}
