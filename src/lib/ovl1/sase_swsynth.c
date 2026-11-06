/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* Nellymoser's decoder, SWSynth.c (OVL1 0x256130-0x256EC0 about): the high-frequency
 * regeneration's synthesis -- its block, the onset phase, the noise generator (0x256380: the
 * encoder's minimal-standard RNG again, SWSynth.c:359; libmath.c's 0x24AEB0 is the same text).
 * Asserting as the codec does (file and line by `#line`, as in sase_encsc.c); the decoder is
 * built at -G0 (sase_decapi.c), so its float constants are lui/ori immediates, not .lit4 entries.
 * Round 28a: the random phases 0x256DB0, which inline the noise generator -- so it is `inline`.
 * Written in the GCC lane's round 27a: the frame wrappers 0x256190 and 0x256280 (the gain pair
 * chosen by if/else into a pointer of its own -- the target's `move v0,v1` and `addiu v0,v1,8` from
 * the parameter kept in v1; a conditional expression shares one register, a pre-assignment is a
 * word short). Round 19a: the noise generator 0x256380. Round 18a: the onset phase 0x256AB0 (its rounded turn count an int
 * of its own: the conversion inside the product puts 2 pi's lui/ori in both arms, 32 words; an
 * if/else into a float, 7). Placeholders: the codec's names
 * are not in SOCOM 1. Built as sase_decapi.c is (ee-gcc 2.96, -O2 -G0). Round 45a: the frames 0x256420, 0x2566D8. */

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

/* the band offsets and their pitch thresholds (GCC round 26a, 0x256A18's) */
static const float afBandOffset[6] = {
	0x1.dd6bf0p+1F, 0x1.d4c4dcp+1F, 0x1.cbbfccp+1F, 0x1.c24facp+1F, 0x1.b86406p+1F, 0x1.ade7bep+1F,
};
static const float afBandThresh[5] = { 96.0F, 88.0F, 80.0F, 72.0F, 64.0F };

extern float func_002517D8(float fX);	/* the encoder's fast log2 (shared) */

/* the gain offset of the band fX falls in, plus half the log2 of fY */
float func_00256A18(float fX, float fY)
{
	int i;

	for (i = 0; i < 5; i++)
		if (afBandThresh[i] < fX)
			break;
	return afBandOffset[i] + func_002517D8(fY) * 0.5F + -0x1.42e68p+2F;
}

extern float func_00251850(float fX);	/* the encoder's fast exp2 (shared) */

/* the bands' phases advanced by a growing step, the even bands' by an offset from 1/pi that grows
 * by 1/(20 pi) a band past fX * 0.1875; the second array's log gains raised by the fast exp2
 * (GCC round 26a) */
void func_00256BC8(float fStep, int n, float fX, float *pfPhase, float *pfGain)
{
	int i, iLimit;
	float fAcc, fOff;

	iLimit = (int)(fX * 0.1875F);
	fAcc = fStep;
	fOff = 0x1.45f306p-2F;
	for (i = 0; i < n; i++) {
		pfPhase[i] += fAcc;
		fAcc += fStep;
		if (i >= iLimit)
			fOff += 0x1.04c26ap-6F;
		if ((i & 1) == 0)
			pfPhase[i] += fOff;
		pfGain[i] = func_00251850(pfGain[i]);
	}
}

/* two arrays upsampled in place by iFactor from the top down (GCC round 26a): the samples from
 * iHold up held iFactor times, those below it followed by iFactor - 1 zeros */
void func_00256CC8(int iLength, int n, int iFactor, float *pfB, float *pfA, int iHold)
{
	int i, j, k;

	k = iLength - 1;
	for (i = n - 1; i >= iHold; i--)
		for (j = 0; j < iFactor; j++) {
			pfA[k] = pfA[i];
			pfB[k] = pfB[i];
			k--;
		}
	for (; i >= 0; i--) {
		pfA[k] = pfA[i];
		pfB[k] = pfB[i];
		k--;
		for (j = 1; j < iFactor; j++) {
			pfA[k] = 0.0F;
			pfB[k] = 0.0F;
			k--;
		}
	}
}

/* the noise generator: libmath.c's minimal-standard RNG (0x24AEB0) again, the same text, its
 * assert at SWSynth.c:359 -- the high half the accumulator, updated in place (GCC round 19a).
 * `inline` (GCC round 28a): nothing calls 0x256380 -- the random phases 0x256DB0 inline it, assert
 * and all, and the public copy is the one GCC emits besides */
inline void func_00256380(int *plSeed)
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

/* random phases in [0, 2 pi) for the bands iStart up to iEnd, the noise generator inlined */
void func_00256DB0(int iStart, int iEnd, int *plSeed, float *pfPhase)
{
	int i;

	for (i = iStart; i < iEnd; i++) {
		func_00256380(plSeed);
		pfPhase[i] = (float)*plSeed * (TWOPI / 2147483648.0F);
	}
}

/* the phase advanced by the mean of the last and current frequencies over iLength samples,
 * wrapped into [-pi, pi] by the nearest whole turn */
void func_00256AB0(HFRegenMblk *pvHFRegenMblk, float *pfOnsetPhase, float fFreq, int iLength)
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

/* a frame's synthesis state: the synthesiser's six buffers, the rate and the position in the
 * period (in samples, 160 a frame at rate 1), a hold flag, the synthesiser's own state */
typedef struct {
	void *pv0, *pv4, *pv8, *pv12, *pv16, *pv20;
	int ai24[5];
	float fRate;
	float fPos;
	int ai52[3];
	short sHold;
	short s66;
	int iState;
} SWSynthFrame;

void func_00256420(HFRegenMblk *hHFRegenMblk, short *psSynSpeech, float *pfMemBuffer, float *pfEnv_1, float *pfPhase_1, float *pfPhase,
		   float *pfAmp, int *plUVPhaseRandSeed, float fPitch, float fVoice, float fMix);
void func_002566D8(HFRegenMblk *hHFRegenMblk, short *psSynSpeech, float *pfMemBuffer, float *pfEnv_1, float *pfPhase_1, float *pfPhase,
		   float *pfAmp, int *plUVPhaseRandSeed, float *pfStepOut, int *piNOut, float fPitch, float fVoice, float fMix);

/* one frame synthesised while the position lies inside the period (from the pair of gains for
 * its first or last quarter), the position stepped by 80 and wrapped unless held */
void func_00256190(SWSynthFrame *p, const float *pfPair, short *iX, int *piPos)
{
	const float *pf;
	float fLen, fRatio;

	fLen = p->fRate * 160.0F;
	if (p->fPos <= fLen) {
		fRatio = p->fPos / fLen;
		if (fRatio < 0.75F)
			pf = pfPair;
		else
			pf = pfPair + 2;
		func_00256420(p->pv0, iX, p->pv4, p->pv8, p->pv12, p->pv16, p->pv20, &p->iState, pf[0], pf[1], fRatio);
		*piPos += 80;
		p->fPos += 80.0F;
	}
	if (p->sHold == 0 && fLen < p->fPos)
		p->fPos -= fLen;
}

/* the same through the other synthesiser, two more arguments passed on */
void func_00256280(SWSynthFrame *p, const float *pfPair, short *iX, int *piPos, float *iY, int *iZ)
{
	const float *pf;
	float fLen, fRatio;

	fLen = p->fRate * 160.0F;
	if (p->fPos <= fLen) {
		fRatio = p->fPos / fLen;
		if (fRatio < 0.75F)
			pf = pfPair;
		else
			pf = pfPair + 2;
		func_002566D8(p->pv0, iX, p->pv4, p->pv8, p->pv12, p->pv16, p->pv20, &p->iState, iY, iZ,
		              pf[0], pf[1], fRatio);
		p->fPos += 80.0F;
		*piPos += 80;
	}
	if (p->sHold == 0 && fLen < p->fPos)
		p->fPos -= fLen;
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

extern void (*data_001D55B0)(void *p, int c, int n);	/* the host's memset */
extern void func_00255E68(float fGain, float fF0, float fUnused, float fMix, int iNumHarm, int iFrom, float *pfHist, float *pfAmp, float *pfEnv_1, float *pfPhaseOut, float *pfPhase_1);
extern void func_00254510(float fPitch, int iPeaks, int iLength, short *psOut, float *pfSpec, const float *pfPhase, const float *pfAmp);

/* a frame of the high-frequency regeneration: the harmonics of the pitch (at most 100, below the spectrum's top), each spread by a
 * factor over its band when the voicing leaves room; their amplitudes and phases set, the onset phase, the noise above the voicing,
 * the amplitude smoothing, and the inverse transform into the caller's samples */
void func_00256420(HFRegenMblk *hHFRegenMblk, short *psSynSpeech, float *pfMemBuffer, float *pfEnv_1, float *pfPhase_1, float *pfPhase,
		   float *pfAmp, int *plUVPhaseRandSeed, float fPitch, float fVoice, float fMix)
{
	float fOnset;
	float fF0, fV, fStep, fGain;
	int iHarm, iFactor, iN, iFrom, iNoise;

#line 166 "../../SaseDec/source/SWSynth.c"
	assert(hHFRegenMblk != NellyNull && psSynSpeech != NellyNull);
	assert(pfMemBuffer != NellyNull);
	assert(plUVPhaseRandSeed != NellyNull);
	fF0 = 512.0F / fPitch;
	fV = fVoice * 256.0F;
	iHarm = (int)(fPitch * 0.5F);
	if ((int)(fF0 * iHarm + 0.5F) >= 256)
		iHarm--;
	if (iHarm > 100)
		iHarm = 100;
	if (fV < 256.0F)
		iFactor = 100 / iHarm;
	else
		iFactor = 1;
	fStep = fF0 / iFactor;
	iN = iHarm * iFactor;
	iFrom = (int)(fV * 0x1p-9F * fPitch);
	fGain = func_00256A18(fPitch, fStep);
	data_001D55B0(pfMemBuffer, 0, 2056);
	func_00255E68(fGain, fF0, fStep, fMix, iHarm, iFrom, pfMemBuffer, pfAmp, pfEnv_1, pfPhase, pfPhase_1);
	iNoise = iFrom * iFactor;
	func_00256AB0(hHFRegenMblk, &fOnset, fF0, 80);
	func_00256BC8(fOnset, iHarm, fPitch, pfPhase, pfAmp);
	func_00256CC8(iN, iHarm, iFactor, pfPhase, pfAmp, iFrom);
	func_00256DB0(iNoise, iN, plUVPhaseRandSeed, pfPhase);
	func_00256EC0(iN, pfAmp);
	func_00254510(fStep, iN, 160, psSynSpeech, pfMemBuffer, pfPhase, pfAmp);
}

/* the same frame prepared and not transformed: the step and the count returned for the caller's own (0x256280, the three- and
 * four-handle frames) */
void func_002566D8(HFRegenMblk *hHFRegenMblk, short *psSynSpeech, float *pfMemBuffer, float *pfEnv_1, float *pfPhase_1, float *pfPhase,
		   float *pfAmp, int *plUVPhaseRandSeed, float *pfStepOut, int *piNOut, float fPitch, float fVoice, float fMix)
{
	float fOnset;
	float fF0, fV, fStep, fGain;
	int iHarm, iFactor, iN, iFrom, iNoise;

#line 273 "../../SaseDec/source/SWSynth.c"
	assert(hHFRegenMblk != NellyNull && psSynSpeech != NellyNull);
	assert(pfMemBuffer != NellyNull);
	assert(plUVPhaseRandSeed != NellyNull);
	fF0 = 512.0F / fPitch;
	fV = fVoice * 256.0F;
	iHarm = (int)(fPitch * 0.5F);
	if ((int)(fF0 * iHarm + 0.5F) >= 256)
		iHarm--;
	if (iHarm > 100)
		iHarm = 100;
	if (fV < 256.0F)
		iFactor = 100 / iHarm;
	else
		iFactor = 1;
	fStep = fF0 / iFactor;
	iN = iHarm * iFactor;
	iFrom = (int)(fV * 0x1p-9F * fPitch);
	fGain = func_00256A18(fPitch, fStep);
	data_001D55B0(pfMemBuffer, 0, 2056);
	func_00255E68(fGain, fF0, fStep, fMix, iHarm, iFrom, pfMemBuffer, pfAmp, pfEnv_1, pfPhase, pfPhase_1);
	iNoise = iFrom * iFactor;
	func_00256AB0(hHFRegenMblk, &fOnset, fF0, 80);
	func_00256BC8(fOnset, iHarm, fPitch, pfPhase, pfAmp);
	func_00256CC8(iN, iHarm, iFactor, pfPhase, pfAmp, iFrom);
	func_00256DB0(iNoise, iN, plUVPhaseRandSeed, pfPhase);
	func_00256EC0(iN, pfAmp);
	*pfStepOut = fStep;
	*piNOut = iN;
}
