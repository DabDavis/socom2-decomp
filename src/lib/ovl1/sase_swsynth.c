/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* Nellymoser's decoder, SWSynth.c (OVL1 0x256130-0x256EC0 about): the high-frequency
 * regeneration's synthesis -- its block, the onset phase, the noise generator (0x256380: the
 * encoder's minimal-standard RNG again, SWSynth.c:359; libmath.c's 0x24AEB0 is the same text).
 * Asserting as the codec does (file and line by `#line`, as in sase_encsc.c); the decoder is
 * built at -G0 (sase_decapi.c), so its float constants are lui/ori immediates, not .lit4 entries.
 * Written in the GCC lane's round 19a: the noise generator 0x256380. Round 18a: the onset phase 0x256AB0 (its rounded turn count an int
 * of its own: the conversion inside the product puts 2 pi's lui/ori in both arms, 32 words; an
 * if/else into a float, 7). Placeholders: the codec's names
 * are not in SOCOM 1. Built as sase_decapi.c is (ee-gcc 2.96, -O2 -G0). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

typedef struct {
	float fPhase;
	float f4;
	float fPrevFreq;
} HFRegenMblk;

#define DELTA_PHASE	0x1.921fb4p-8F		/* 2 pi / 1024 */
#define INV_TWOPI	0x1.45f306p-3F		/* 1 / (2 pi) */
#define TWOPI		0x1.921fb4p+2F

/* the noise generator: libmath.c's minimal-standard RNG (0x24AEB0) again, the same text, its
 * assert at SWSynth.c:359 -- the high half the accumulator, updated in place (GCC round 19a) */
void func_00256380(int *plSeed)
{
	unsigned int lo, hi;
	unsigned int ulLo = *plSeed & 0xFFFF;
	int lHi = *plSeed >> 16;

#line 359 "../../SaseDec/source/SWSynth.c"
	assert(*plSeed != 0);
	hi = 16807 * lHi;
	lo = ((hi << 17) >> 1) + 16807 * ulLo;
	hi >>= 15;
	hi += lo >> 31;
	lo = (lo << 1) >> 1;
	hi += lo;
	hi += hi >> 31;
	*plSeed = (hi << 1) >> 1;
}

/* the phase advanced by the mean of the last and current frequencies over iLength samples,
 * wrapped into [-pi, pi] by the nearest whole turn */
void func_00256AB0(HFRegenMblk *pvHFRegenMblk, float *pfOnsetPhase, int iLength, float fFreq)
{
	float fPhase, fTurns;
	int iTurns;

#line 437 "../../SaseDec/source/SWSynth.c"
	assert(pvHFRegenMblk != NellyNull);
	assert(pfOnsetPhase != NellyNull);
	fPhase = pvHFRegenMblk->fPhase
		 + (fFreq + pvHFRegenMblk->fPrevFreq) * DELTA_PHASE * (float)iLength;
	fTurns = fPhase * INV_TWOPI;
	iTurns = (int)((fTurns > 0.0F) ? fTurns + 0.5F : fTurns - 0.5F);
	fPhase -= (float)iTurns * TWOPI;
	*pfOnsetPhase = fPhase;
	pvHFRegenMblk->fPrevFreq = fFreq;
	pvHFRegenMblk->fPhase = fPhase;
}

extern void (*data_001D55AC)(void *);           /* the host's free callback (-G0: lui and an offset) */

/* a gain curve: a quadratic above 97, linear below */
float func_00256130(float fX)
{
	if (fX > 97.0F)
		return fX * (fX * -0x1.a3fffep-10F + 0x1.670000p+0F) + 0x1.ca0000p+0F;
	return fX * 0x1.450000p+0F;
}

/* a block freed through the host and its pointer cleared */
void func_002569D8(void **ppv)
{
	if (*ppv != 0) {
		data_001D55AC(*ppv);
		*ppv = 0;
	}
}

/* the bands scaled down so they sum to no more than 32767 */
void func_00256EC0(int iLength, float *pf)
{
	float fScale = 1.0F, fSum = 0.0F;
	int i;

	for (i = 0; i < iLength; i++)
		fSum += pf[i];
	if (fSum > 32767.0F)
		fScale = 32767.0F / fSum;
	while (iLength-- > 0) {
		*pf *= fScale;
		pf++;
	}
}
