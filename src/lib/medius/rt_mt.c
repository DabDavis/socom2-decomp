/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* The RT library's random numbers (OVL2 0x638CF0-0x639058, SOCOM 1's object at 0x410F10: seedMT ..
 * setNewSeed), SCEA's Medius/DME network stack, prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC
 * front"). Shawn Cokus's MT19937 (seedMT, reloadMT, randomMT and their constants are his), kept by
 * SCEA in one static state gMT (SOCOM 1's local symbol, 0x9C8 bytes: the 625-word state and the
 * index) whose index counts up to N, plus a seeding from an array and a random-character string.
 * The object has no CVS $Header; its name is unknown -- it links between rt_msg.c and rt_mutex.c,
 * where the RT library's alphabetical link order puts an rt_mt.c (and setNewSeed_platform's object,
 * after it, an rt_mt_platform.c). Names are SOCOM 1's (s1:order: the object in the same order). */

#define N               (624)
#define M               (397)
#define K               (0x9908B0DFU)
#define hiBit(u)        ((u) & 0x80000000U)
#define loBit(u)        ((u) & 0x00000001U)
#define loBits(u)       ((u) & 0x7FFFFFFFU)
#define mixBits(u, v)   (hiBit(u) | loBits(v))

typedef unsigned int uint32;

typedef struct MTState {
    uint32 state[N + 1];                /* N+1 as Cokus's, "to not violate ANSI C" */
    int index;                          /* 0x9C4: the last word handed out; N+1 before any seed */
} MTState;

extern MTState data_00655638;           /* SOCOM 1's gMT */
#define gMT data_00655638

extern char data_00665608[];            /* the 64 characters randomMTSetCharArray draws from */

extern void setNewSeed_platform(unsigned long seed);
#if Z_PORT
void setNewSeed(uint32 seed);   /* the port: called before its definition (implicit in C89 here) */
#endif

void seedMT(uint32 seed)
{
    register uint32 x = (seed | 1U) & 0xFFFFFFFFU, *s = gMT.state;
    register int j;

    for (gMT.index = N - 1, *s++ = x, j = N; --j; *s++ = (x *= 69069U) & 0xFFFFFFFFU)
        ;
}

void seedMTUsingArray(uint32 *pArray, unsigned int nLength)
{
    register uint32 x = 0xFFFFFFFFU, *s = gMT.state;
    register int j;

    if (nLength >= N)
        nLength = N - 1;
    for (gMT.index = N - 1, *s++ = x, j = N + 1; --j; *s++ = x) {
        if (nLength != 0) {
            x ^= *pArray++;
            nLength--;
        }
        x *= 69069U;
    }
}

void reloadMT(void)
{
    register uint32 *p0 = gMT.state, *p2 = gMT.state + 2, *pM = gMT.state + M, s0, s1;
    register int j;

    if (gMT.index >= N + 1)
        setNewSeed(0x3579ABCDU);

    gMT.index = 0;

    for (s0 = gMT.state[0], s1 = gMT.state[1], j = N - M + 1; --j; s0 = s1, s1 = *p2++)
        *p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

    for (pM = gMT.state, j = M; --j; s0 = s1, s1 = *p2++)
        *p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

    s1 = gMT.state[0], *p0 = *pM ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);
}

uint32 randomMT(void)
{
    uint32 y;

    if (++gMT.index >= N)
        reloadMT();

    y = gMT.state[gMT.index];
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9D2C5680U;
    y ^= (y << 15) & 0xEFC60000U;
    return y ^ (y >> 18);
}

void randomMTSetCharArray(char *pBuf, unsigned int nLength)
{
    unsigned int i = 0;

    if (nLength != 0) {
        for (; i < nLength - 1; i++)
            pBuf[i] = data_00665608[randomMT() & 0x3F];
        pBuf[nLength - 1] = 0;
    }
}

void setNewSeed(uint32 seed)
{
    setNewSeed_platform(seed);
}
