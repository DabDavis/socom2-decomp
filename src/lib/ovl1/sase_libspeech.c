/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, libspeech.c (OVL1 0x24C8E8-0x24D8B8): the speech analysis helpers -- the
 * LPC recursion over the autocorrelation, the pitch and peak pickers, the parabolic peak
 * interpolation, the STFT phase. Asserting as the codec does (file and line by `#line`, as in
 * sase_encsc.c). Written in the GCC lane's round 18a: the parabolic interpolation 0x24D498 (its
 * guarded division `if (fA != 0.0F) fX = ...; else fX = 0.0F;`: the else's zero copied from the
 * compare's in the branch's slot; `fX = 0.0F; if (...)` 4 words off, the test inverted 27). The
 * GCC round 27a: the LPC recursion 0x24C8E8 (the reflection coefficients stored through a
 * stepped pointer -- indexed, the three pointer parameters take other saved registers -- and the
 * update loop counting in its own variable: one counter for both inner loops is 19 words of
 * temporaries), the peak-pair features 0x24D190, the peak picker 0x24D370 (its end decremented in
 * place, `iEnd--`: `i < iEnd - 1` trades two saved registers) and the envelope's LPC 0x24D770 (the
 * spectrum filled by two counters stepping 4 and 2: from one, i*4 and i*2, loop.c reverses a
 * counter of 63, the target's 252). Not written: 0x24CAE8, 0x24CD28, 0x24CF10, 0x24D560.
 * Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

#define MAX_LPC_ORDER 18

/* the LPC recursion (Levinson-Durbin) over the autocorrelation: the predictor's coefficients
 * (built in two buffers taken in turn, the caller's chosen by the order's parity so that the last
 * pass lands in it), the reflection coefficients clamped to +-0.9999, the residual energy */
void func_0024C8E8(const float *pAutoCorr, int iLpcOrder, float *pfLPCoef, float *pfPARCOR, float *pfResidEnrg)
{
	float afTmp[MAX_LPC_ORDER + 1];
	float *pfA, *pfB, *pfT;
	float fErr, fK, fSum;
	int i, j, k;

#line 73 "../../SaseEncVad/source/libspeech.c"
	assert(iLpcOrder <= MAX_LPC_ORDER);
	assert(pAutoCorr != NellyNull && pfLPCoef != NellyNull);
	assert(pfPARCOR != NellyNull && pfResidEnrg != NellyNull);
	fErr = pAutoCorr[0];
	if (fErr == 0.0F)
		fErr = 0x1.1039d4p-123F;		/* 1e-37 */
	if (iLpcOrder & 1) {
		pfA = afTmp;
		pfB = pfLPCoef;
	} else {
		pfA = pfLPCoef;
		pfB = afTmp;
	}
	pfB[0] = 1.0F;
	pfA[0] = 1.0F;
	for (i = 0; i < iLpcOrder; i++) {
		fSum = 0.0F;
		for (j = 0; j <= i; j++)
			fSum += pfA[j] * pAutoCorr[i + 1 - j];
		fK = fSum / fErr;
		if (fK > 0x1.fff2e4p-1F)		/* 0.9999 */
			fK = 0x1.fff2e4p-1F;
		if (fK < -0x1.fff2e4p-1F)
			fK = -0x1.fff2e4p-1F;
		pfT = pfA;
		pfA = pfB;
		pfB = pfT;
		*pfPARCOR++ = fK;
		pfA[i + 1] = -fK;
		for (k = 1; k <= i; k++)
			pfA[k] = pfB[k] - fK * pfB[i + 1 - k];
		fErr *= 1.0F - fK * fK;
	}
	*pfResidEnrg = fErr;
}

/* the peak pairs' features, at most 100: each peak's magnitude and frequency, then each pair a
 * peak and two apart's magnitude product and frequency distance; from ten on, only those whose
 * magnitude (product) is over 0.05 kept */
void func_0024D190(const float *pfMag, const float *pfFreq, int iNPeaks, float *pfAmp, float *pfDist, int *piNum)
{
	int i, j, k, n;

	n = iNPeaks;
	if (n > 100)
		n = 100;
	k = 0;
	for (i = 0; i < n; i++) {
		pfAmp[k] = pfMag[i];
		pfDist[k] = pfFreq[i];
		k++;
	}
	n = iNPeaks - 1;
	if (100 - k < n)
		n = 100 - k;
	for (i = 0; i < n; i++) {
		pfAmp[k] = pfMag[i + 1] * pfMag[i];
		pfDist[k] = pfFreq[i + 1] - pfFreq[i];
		k++;
	}
	n = iNPeaks - 2;
	if (100 - k < n)
		n = 100 - k;
	for (i = 0; i < n; i++) {
		pfAmp[k] = pfMag[i + 2] * pfMag[i];
		pfDist[k] = pfFreq[i + 2] - pfFreq[i];
		k++;
	}
	*piNum = k;
	if (k >= 10) {
		j = 0;
		for (i = 0; i < k; i++) {
			if (0x1.999998p-5F < pfAmp[i]) {	/* 0.05 */
				pfAmp[j] = pfAmp[i];
				pfDist[j] = pfDist[i];
				j++;
			}
		}
		*piNum = j;
	}
}

/* the local peaks of the magnitude strictly inside (iStart, iEnd - 1): a bin at least its left
 * neighbour and above its right one, the next bin skipped, up to iMaxPeaks of them */
void func_0024D370(const float *pfMag, int iStart, int iEnd, int iMaxPeaks, int *piPeakFreq, int *piNPeaks)
{
	int i, n;

#line 170 "../../SaseEncVad/source/libspeech.c"
	assert(pfMag != NellyNull);
	assert(piPeakFreq != NellyNull);
	assert(piNPeaks != NellyNull);
	n = 0;
	iEnd--;
	for (i = iStart + 1; i < iEnd; i++) {
		if (pfMag[i - 1] <= pfMag[i] && pfMag[i + 1] < pfMag[i]) {
			n++;
			*piPeakFreq++ = i;
			i++;
			if (n == iMaxPeaks)
				break;
		}
	}
	*piNPeaks = n;
}

/* the vertex of the parabola through three samples about a peak: its offset from the middle
 * sample returned, its height stored */
float func_0024D498(float *pfYpeak, float fY_1, float fY0, float fY1)
{
	float fA, fB, fX;

#line 256 "../../SaseEncVad/source/libspeech.c"
	assert((fY0 >= fY_1) && (fY0 >= fY1));
	fB = (fY1 - fY_1) * 0.5F;
	fA = (fY_1 + fY1) * 0.5F - fY0;
	if (fA != 0.0F)
		fX = -fB / (2.0F * fA);
	else
		fX = 0.0F;
	*pfYpeak = (fA * fX + fB) * fX + fY0;
	return fX;
}

extern void func_00251678(const float *pfIn, float *pfOut);
extern float func_00251850(float fX);		/* the encoder's fast exp2 (shared) */
extern float func_002517D8(float fX);		/* the encoder's fast log2 (shared) */
extern void func_00251AF8(float *pfIn, int n, float *pfRe, float *pfIm);
extern int func_00251C00(float *pfRe, float *pfIm, int n);
extern void func_00251B38(float *pfRe, float *pfIm, int n, float *pfOut);
extern void func_0024EEA0(const float *pfIn, int iLength, float *pfOut, float fScale);
extern void func_0024C8E8(const float *pAutoCorr, int iLpcOrder, float *pfLPC, float *pfRC, float *pfErr);

/* the LPC of a log envelope: 65 of its 256 points (every fourth, and the last) raised back to
 * powers, inverse-transformed into an autocorrelation (scaled by 1/128, its lags lag-windowed by
 * 0.999999 each), the recursion's residual energy returned as half its log2 */
void func_0024D770(float *pfLogEnv, int iLpcOrder, float *pfGain, float *pfLPC, float *pfRC)
{
	float afSpec[130];
	float afRe[65], afIm[65];
	float fErr;
	int i, j;

	func_00251678(pfLogEnv, pfLogEnv);
	for (i = 0, j = 0; i < 256; i += 4, j += 2) {
		afSpec[j] = func_00251850(pfLogEnv[i]);
		afSpec[j + 1] = 0.0F;
	}
	afSpec[128] = func_00251850(pfLogEnv[255]);
	afSpec[129] = 0.0F;
	func_00251AF8(afSpec, 128, afRe, afIm);
	func_00251C00(afRe, afIm, 128);
	func_00251B38(afRe, afIm, 128, afSpec);
	func_0024EEA0(afSpec, 128, afSpec, 0.0078125F);
	func_0024EEA0(&afSpec[1], iLpcOrder, &afSpec[1], 0x1.ffffdep-1F);
	func_0024C8E8(afSpec, iLpcOrder, pfLPC, pfRC, &fErr);
	*pfGain = func_002517D8(fErr) * 0.5F;
}
