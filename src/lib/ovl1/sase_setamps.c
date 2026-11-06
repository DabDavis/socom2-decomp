/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* Nellymoser's decoder, SetAmps.c (OVL1 0x255E68): each harmonic's amplitude and phase read off the two frames' envelopes at its
 * warped frequency, by the asserts' own names (GCC round 45a). Asserting as the codec does (file and line by `#line`, as in
 * sase_encsc.c); built as sase_decapi.c is (ee-gcc 2.96, -O2 -G0). Placeholders: the codec's names are not in SOCOM 1. */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))
extern int printf(const char *fmt, ...);

#define CSENV_NFFT_2 64

/* the harmonic's frequency warped (a quadratic above 97, linear below), a quarter of it the envelope's index; both frames' envelopes
 * interpolated there and mixed by fMix, the gain added from iFrom on */
void func_00255E68(float fGain, float fF0, float fUnused, float fMix, int iNumHarm, int iFrom, float *pfHist, float *pfAmp,
		   float *pfEnv_1, float *pfPhaseOut, float *pfPhase_1)
{
	float *pfEnv = pfHist + 514;
	float *pfPhase = pfHist + 579;
	float fFreq, fWarp, fWarpedFreq;
	float fA, fC, fFrac;
	int i, sIndex;

#line 140 "../../SaseDec/source/SetAmps.c"
	assert((pfEnv != NellyNull) && (pfEnv_1 != NellyNull));
	assert((pfPhase != NellyNull) && (pfPhase_1 != NellyNull));
	pfEnv[64] = pfEnv[63];
	pfPhase[64] = pfPhase[63];
	fFreq = fF0;
	for (i = 0; i < iNumHarm; i++) {
		if (97.0F < fFreq)
			fWarp = fFreq * (fFreq * -0x1.a3fffep-10F + 0x1.67p+0F) + 0x1.cap+0F;
		else
			fWarp = fFreq * 0x1.45p+0F;
		fWarpedFreq = fWarp * 0.25F;
		fFreq += fF0;
#line 168 "../../SaseDec/source/SetAmps.c"
		assert(fWarpedFreq >= 0.0F);
		sIndex = (int)fWarpedFreq;
#line 171 "../../SaseDec/source/SetAmps.c"
		assert(sIndex < CSENV_NFFT_2);
		fFrac = fWarpedFreq - sIndex;
		fA = pfEnv_1[sIndex] + fFrac * (pfEnv_1[sIndex + 1] - pfEnv_1[sIndex]);
		pfAmp[i] = fA + fMix * (pfEnv[sIndex] + fFrac * (pfEnv[sIndex + 1] - pfEnv[sIndex]) - fA);
		fC = pfPhase_1[sIndex] + fFrac * (pfPhase_1[sIndex + 1] - pfPhase_1[sIndex]);
		pfPhaseOut[i] = fC + fMix * (pfPhase[sIndex] + fFrac * (pfPhase[sIndex + 1] - pfPhase[sIndex]) - fC);
		if (i >= 100)
			printf("a mess in SetAmp\n");
	}
	for (i = iFrom; i < iNumHarm; i++)
		pfAmp[i] += fGain;
}
