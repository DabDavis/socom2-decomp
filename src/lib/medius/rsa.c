/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* RSA.c: the RT library's RSA over LargeInt.c (OVL2 0x62B168-0x62BCA8), SCEA's rt_crypt, prebuilt
 * with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). SOCOM 1's RSA object (0x409418:
 * RSAGenerateKeyPair, RSAEncryptBlock, RSADecryptBlock, RSAEncrypt, RSADecrypt) in its order, the
 * sizes agreeing (RSADecryptBlock 0x298 and RSADecrypt 0x94 both builds): the names are SOCOM 1's
 * (s1:order). A key or a block is 64 bytes (512 bits) of a byte record, so every copy is an
 * unaligned block move (`ldl/ldr`, `sdl/sdr` both sides); a block goes into the low half of a
 * 32-word LINT whose high half is cleared. SOCOM 1 built the public exponent 17 on the stack;
 * SOCOM II's encrypting pair each passes a static one (0x655530, 0x655570: one per function).
 *
 * The parameters, read off the wrappers: RSADecryptBlock(modulus, exponent, input, output) --
 * EMOD(result, input, exponent, modulus) is input^exponent mod modulus -- and RSADecrypt walks a
 * buffer of whole blocks, returning the bytes done.
 *
 * Round 18b: the object starts at 0x62B168, a function earlier than round 17b put it -- SOCOM II has
 * two key-pair generators, RSAGenerateKeyPair (SOCOM 1's: two lintPrime(p, 256) draws, asserting at
 * RSA.c:164) and a twin 0x62B548 whose primes come from prime.c's table (RSA.c:293), each behind its
 * own copy of SOCOM 1's KM_GenerateRSAKeyPair (0x6305C8, 0x630600). Both are SOCOM 1's algorithm over
 * SCEA's six reused LINTs: the inner loop's first branch (the IsGreater case, ending `d = q; break;`)
 * is rolled to the loop's end by 2.96, the target's layout; the check is `assert(0)` (the
 * expression text "0"), its line spelled with a `#line`. */

#define LINT_WORDS  32

typedef struct LINT {
    unsigned int d[LINT_WORDS];
} LINT;                                 /* 0x80 */

typedef struct RSA_BLOCK {
    unsigned char b[64];
} RSA_BLOCK;

extern void *memset(void *p, int c, unsigned int n);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern void func_006283F8(LINT *pResult, LINT *pBase, LINT *pExp, LINT *pMod);     /* EMOD */

/* LargeInt.c's C and its assembly core (largeint_asm.s) */
extern void lintSetSmallValue(LINT *p, unsigned int nValue);
extern int IsZero(LINT *p);
extern int IsOne(LINT *p);
extern int IsGreater(LINT *a, LINT *b);
extern int IsSMaller(LINT *a, LINT *b);
extern void lintAddIP(LINT *a, LINT *b);
extern void lintSubIP(LINT *a, LINT *b);
extern void func_00626D28(LINT *p, unsigned int n);                 /* p += n */
extern void func_00626F70(LINT *pResult, LINT *a, LINT *b);         /* result = a - b */
extern void func_00627580(LINT *a, LINT *b, LINT *pQuot, LINT *pRem);       /* a / b, a % b */
extern void func_006296A0(LINT *pResult, LINT *a, LINT *b);         /* result = a * b */
extern void func_00629868(LINT *pResult, LINT *a, LINT *b, LINT *pMod);     /* a * b mod m */
extern void lintPrime(LINT *p, int nBits);
extern void func_0062A3B0(LINT *p);                                 /* prime.c: a 256-bit prime */
extern void __assert(const char *file, int line, const char *failedexpr);
#define assert(e)   ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

/* two primes p and q, n = p * q the public key; d the inverse of 17 mod (p - 1)(q - 1), by a binary
 * extended Euclid on (17, phi mod 17), the private key; a pair whose inverse comes out zero is drawn
 * again (the new prime pairs with the last) */
void RSAGenerateKeyPair(RSA_BLOCK *pPublic, RSA_BLOCK *pPrivate)
{
    LINT p;
    LINT q;
    LINT d;
    LINT a;
    LINT b;
    LINT n;

    lintSetSmallValue(&d, 0);
    lintPrime(&p, 256);
    do {
        q = p;
        lintPrime(&p, 256);
        func_006296A0(&n, &p, &q);
        *pPublic = *(RSA_BLOCK *)&n;
        func_00626D28(&n, 1);
        lintSubIP(&n, &p);
        lintSubIP(&n, &q);
        lintSetSmallValue(&q, 1);
        lintSetSmallValue(&a, 17);
        func_00627580(&n, &a, &d, &b);
        lintAddIP(&d, &q);
        func_00626F70(&b, &a, &b);
        for (;;) {
            if (IsGreater(&a, &b)) {
                if (IsSMaller(&q, &d))
                    lintAddIP(&q, &n);
                lintSubIP(&q, &d);
                lintSubIP(&a, &b);
                if (IsOne(&a)) {
                    d = q;
                    break;
                }
            } else if (IsSMaller(&a, &b)) {
                if (IsSMaller(&d, &q))
                    lintAddIP(&d, &n);
                lintSubIP(&d, &q);
                lintSubIP(&b, &a);
                if (IsOne(&b))
                    break;
            } else {
                lintSetSmallValue(&d, 0);
                break;
            }
        }
    } while (IsZero(&d));
    lintSetSmallValue(&p, 17);
    func_00629868(&p, &d, &p, &n);
    if (!IsOne(&p))
#line 164 "/public/work/dev/rt_crypt/src/RSA.c"
        assert(0);
    *pPrivate = *(RSA_BLOCK *)&d;
}

/* the same with the primes from prime.c's table (SOCOM II's, behind KM_GenerateRSAKeyPair's second
 * twin 0x630600) */
void func_0062B548(RSA_BLOCK *pPublic, RSA_BLOCK *pPrivate)
{
    LINT p;
    LINT q;
    LINT d;
    LINT a;
    LINT b;
    LINT n;

    lintSetSmallValue(&d, 0);
    func_0062A3B0(&p);
    do {
        q = p;
        func_0062A3B0(&p);
        func_006296A0(&n, &p, &q);
        *pPublic = *(RSA_BLOCK *)&n;
        func_00626D28(&n, 1);
        lintSubIP(&n, &p);
        lintSubIP(&n, &q);
        lintSetSmallValue(&q, 1);
        lintSetSmallValue(&a, 17);
        func_00627580(&n, &a, &d, &b);
        lintAddIP(&d, &q);
        func_00626F70(&b, &a, &b);
        for (;;) {
            if (IsGreater(&a, &b)) {
                if (IsSMaller(&q, &d))
                    lintAddIP(&q, &n);
                lintSubIP(&q, &d);
                lintSubIP(&a, &b);
                if (IsOne(&a)) {
                    d = q;
                    break;
                }
            } else if (IsSMaller(&a, &b)) {
                if (IsSMaller(&d, &q))
                    lintAddIP(&d, &n);
                lintSubIP(&d, &q);
                lintSubIP(&b, &a);
                if (IsOne(&b))
                    break;
            } else {
                lintSetSmallValue(&d, 0);
                break;
            }
        }
    } while (IsZero(&d));
    lintSetSmallValue(&p, 17);
    func_00629868(&p, &d, &p, &n);
    if (!IsOne(&p))
#line 293 "/public/work/dev/rt_crypt/src/RSA.c"
        assert(0);
    *pPrivate = *(RSA_BLOCK *)&d;
}

void RSADecryptBlock(RSA_BLOCK *pMod, RSA_BLOCK *pExp, RSA_BLOCK *pIn, RSA_BLOCK *pOut);
unsigned int RSADecrypt(RSA_BLOCK *pMod, RSA_BLOCK *pExp, unsigned char *pIn, unsigned char *pOut,
                        unsigned int nLength);

void RSAEncryptBlock(RSA_BLOCK *pMod, RSA_BLOCK *pIn, RSA_BLOCK *pOut)
{
    static RSA_BLOCK e = { { 17 } };

    RSADecryptBlock(pMod, &e, pIn, pOut);
}

void RSADecryptBlock(RSA_BLOCK *pMod, RSA_BLOCK *pExp, RSA_BLOCK *pIn, RSA_BLOCK *pOut)
{
    LINT exp;
    LINT mod;
    LINT in;
    LINT result;

    memcpy(&exp, pExp, 64);
    memset((char *)&exp + 64, 0, 64);
    memcpy(&mod, pMod, 64);
    memset((char *)&mod + 64, 0, 64);
    memcpy(&in, pIn, 64);
    memset((char *)&in + 64, 0, 64);
    func_006283F8(&result, &in, &exp, &mod);
    memcpy(pOut, &result, 64);
}

void RSAEncrypt(RSA_BLOCK *pMod, unsigned char *pIn, unsigned char *pOut, unsigned int nLength)
{
    static RSA_BLOCK e = { { 17 } };

    RSADecrypt(pMod, &e, pIn, pOut, nLength);
}

unsigned int RSADecrypt(RSA_BLOCK *pMod, RSA_BLOCK *pExp, unsigned char *pIn, unsigned char *pOut,
                        unsigned int nLength)
{
    unsigned int nDone = 0;

    while (nLength >= 64) {
        RSADecryptBlock(pMod, pExp, (RSA_BLOCK *)pIn, (RSA_BLOCK *)pOut);
        nDone += 64;
        nLength -= 64;
        pIn += 64;
        pOut += 64;
    }
    return nDone;
}
