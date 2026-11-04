/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* prime.c: the RT library's 256-bit prime source (OVL2 0x62A3B0-0x62A5A8), SCEA's rt_crypt, prebuilt
 * with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). rsa.c's second key-pair generator (0x62B548)
 * draws its two primes here, where RSAGenerateKeyPair searches with lintPrime (LargeInt.c). The
 * object has no `$Header`; the name is ours (it sits between peer.c and rc.c in the link). Round 18b.
 *
 * SOCOM II does not search: a prime is a fixed 256-bit base (0x6514A8, a LINT whose high 96 words
 * are zero) plus the sum of the first n of 8191 sixteen-bit gaps (0x651528, entry 0 unused), n drawn
 * from the Mersenne twister (rt_mt.c), seeded once with 0x5BEF6DF9 (the flag at 0x655528) -- so every
 * key pair comes from the same 8192 primes. The copies in and out are memcpy (GCC's block move with
 * the run-time alignment test of the side whose alignment it does not know), not struct assignments.
 * The base and the table are the image's, referenced by their addresses (16 KB of data). */

typedef struct LINT {
    unsigned int d[32];
} LINT;                                 /* 0x80 */

extern void *memcpy(void *d, const void *s, unsigned int n);
extern void setNewSeed(unsigned int seed);
extern unsigned int randomMT(void);
extern void lintIncrement(LINT *p, unsigned int n);                 /* LargeInt.c: p += n */

extern LINT data_006514A8;                      /* the base */
extern unsigned short data_00651528[8192];      /* the gaps */
extern int data_00655528;                       /* seeded */

LINT *func_0062A3B0(LINT *pPrime)
{
    LINT p;
    unsigned int n;
    unsigned int nSum;

    memcpy(&p, &data_006514A8, sizeof p);
    if (data_00655528 == 0) {
        data_00655528 = 1;
        setNewSeed(0x5BEF6DF9);
    }
    n = (randomMT() >> 5) & 0x1FFF;
    nSum = 0;
    for (; n != 0; n--)
        nSum += data_00651528[n];
    lintIncrement(&p, nSum);
    memcpy(pPrime, &p, sizeof p);
    return pPrime;
}
