/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, Coder.c (OVL1 0x2488A0-0x248AD0): the frame coder 0x2488A0 (its asserts
 * Coder.c's) and after it the harmonic comb search 0x248A18 -- no assert names that one's file, so
 * it is kept here by address (the objects link alphabetically: CodePv.c, Coder.c, DecVcFry.c).
 * Written in the GCC lane's round 26a: 0x248A18; round 44a: 0x2488A0. Placeholders: the codec's names are not in
 * SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8: the cosine table's pointer is a $gp word). */

extern float *data_001D5598;		/* the cosine table, 256 points a turn (0x3EA7E8) */

extern void __assert(const char *file, int line, const char *expr);
extern int func_00248718(float fPv);
extern int func_00248760(float fPv, float fRef, float fMid);
extern void func_002487E8(float fPvPrev, int iIndex, float *pfPv, float *pfMid);
extern void func_0024B028(float *pfIn, int *piIndex, int iLength, float fMid);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern int func_00250BE0(float x);
extern float func_00250C68(int iIndex);
extern float func_00250CC0(float a, float b, float c, float d);

typedef struct {
	float fOut;			/* 0x00 */
	float fPv;			/* 0x04 */
	float afPad[8];
	float afLevel[2];		/* 0x28: the level, the midpoint */
	float afPad2[8];
	float fPvIn;			/* 0x50 */
	float afCoefs[12];		/* 0x54 */
	float afCoefsPrev[12];		/* 0x84 */
} CoderParam;

typedef struct {
	float afPad[14];
	float fPvPrev;			/* 0x38 */
	float fLevelPrev;		/* 0x3C */
} CoderEnc;

#define CODER_ASSERT(e, line) ((e) ? (void)0 : __assert("../../SaseEncVad/source/Coder.c", line, #e))

/* the frame coder: the four scalar parameters quantised and decoded back into the parameter block, the coefficient
 * set saved (a builtin memcpy, 48 bytes through ldl/ldr: its memory conflicts with every store, so the int store above
 * it stays above it, where a struct assignment's would not) and quantised, the encoder's history updated. OPEN (18 words,
 * right size, GCC round 44a: every instruction but two saved registers, as the CodePv's next door): the target gives the
 * encoder block s1 and the level pointer s2, here the other way round -- global-alloc's order, both 8 references, the
 * pointer's 55 insns live to the block's 58 (priority 24/55 against 24/58); locals, typed parameters and the tail's order
 * do not move it. func_002487E8's float parameter is its FIRST (its load precedes the store and a1, a2 as the target's). */
void func_002488A0(int *piQuanIndex, void *pvEncMblk, void *pvParamMblk)
{
	CoderParam *pParam = pvParamMblk;
	float *pfLevel = pParam->afLevel;
	CoderEnc *pEnc = pvEncMblk;

	CODER_ASSERT((piQuanIndex != 0) && (pvEncMblk != 0), 99);
	CODER_ASSERT(pvParamMblk != 0, 100);
	piQuanIndex[1] = func_00248760(pEnc->fPvPrev, pParam->fPv, pfLevel[1]);
	func_002487E8(pEnc->fPvPrev, piQuanIndex[1], &pParam->fPv, &pParam->afLevel[1]);
	piQuanIndex[2] = func_00250BE0(pfLevel[0]);
	pfLevel[0] = func_00250C68(piQuanIndex[2]);
	pParam->fOut = func_00250CC0(pEnc->fLevelPrev, pfLevel[0], pEnc->fPvPrev, pfLevel[1]);
	piQuanIndex[0] = func_00248718(pParam->fPvIn);
	memcpy(pParam->afCoefsPrev, pParam->afCoefs, sizeof(pParam->afCoefs));
	func_0024B028(pParam->afCoefs, piQuanIndex + 3, 12, pfLevel[1]);
	pEnc->fPvPrev = pfLevel[1];
	pEnc->fLevelPrev = pfLevel[0];
}

/* the comb's best fundamental between fFreq and fEnd: at each step the amplitudes summed over the
 * harmonics' phases (the step in 1/4096 turns, the phase's top byte the table's index). The scale
 * is a local initialised at its declaration -- its lui/mtc1 before the loop's entry test, as the
 * target's (a literal is hoisted into the preheader after it: 4 words); the 0.5 stays a literal. */
void func_00248A18(float *pfMax, int n, const float *pfAmp, const int *piHarm, float *pfMaxFreq,
		   float fStep, float fFreq, float fEnd, float fBias)
{
	int i, iStep;
	float fSum;
	float fScale = 4096.0F;

	for (; fFreq <= fEnd; fFreq += fStep) {
		iStep = (int)(fFreq * fScale + 0.5F);
		fSum = fBias;
		for (i = 0; i < n; i++)
			fSum += pfAmp[i] * data_001D5598[(unsigned int)(iStep * piHarm[i] + 0x800000) >> 24];
		if (*pfMax < fSum) {
			*pfMax = fSum;
			*pfMaxFreq = fFreq;
		}
	}
}
