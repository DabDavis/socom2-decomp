/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, LDPDA.c (OVL1 0x249FA8-0x24AB48): the pitch detector's small entry points --
 * its pitch table (89 lags in sixteenths, a .rodata table compared whole), its 8-byte block made,
 * freed and set. The asserts carry the codec's own expressions, file and line by `#line` (as in
 * sase_encsc.c). GCC round 18a: the band edge 0x24A908 (its count decremented in place, the
 * total scaled in its own variable) and the voicing candidate 0x24A9F0 (its thresholds a static
 * const table, compared whole; `if (c) r = last; else r = index;` is if-converted after the
 * compare -- the target's `move` in the branch's slot -- where `r = index; if (c) r = last;`
 * assigns first and re-forms the address from it, 2 words long). GCC round 27a: the
 * autocorrelation of the resampled power spectrum 0x24A438 (its Q15 accumulator set before the
 * spectrum's pointer: two words the other way; the host's memcpy, data_001D55B4, copies the lags
 * out). GCC round 44a: the peaks' filter 0x24A210 and the LPC envelope 0x24A5A8 (round 78, below). The detector itself, 0x249FA8.
 * Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

typedef struct {
	float f0;
	float f4;
} LdPdaMblk;

extern void *(*data_001D55A8)(int n, int size);  /* the host's allocator (calloc's arguments) */
extern void (*data_001D55AC)(void *);           /* the host's free callback */

static const float fPitchTable[89] = {
	8.0f, 8.3125f, 8.625f, 9.0f, 9.375f, 9.8125f, 10.25f, 10.75f, 11.25f, 11.875f,
	12.5625f, 13.3125f, 13.6875f, 14.125f, 14.625f, 15.125f, 15.625f, 16.1875f, 16.8125f, 17.5f,
	18.1875f, 19.0f, 19.8125f, 20.4375f, 21.0625f, 21.75f, 22.5f, 23.25f, 24.125f, 25.0f,
	26.0f, 26.75f, 27.625f, 28.5f, 29.4375f, 30.4375f, 31.5f, 32.625f, 33.625f, 34.6875f,
	35.8125f, 37.0f, 38.25f, 39.625f, 40.8125f, 42.125f, 43.5f, 44.9375f, 46.5625f, 47.9375f,
	49.5f, 51.125f, 52.875f, 54.4375f, 56.1875f, 58.0f, 59.9375f, 61.8125f, 63.75f, 65.875f,
	67.875f, 70.0f, 72.3125f, 74.5f, 76.8125f, 79.3125f, 81.75f, 84.3125f, 86.875f, 89.5625f,
	92.375f, 95.25f, 98.25f, 101.1875f, 104.375f, 107.5f, 110.875f, 114.1875f, 117.75f, 121.3125f,
	125.125f, 128.9375f, 133.0f, 137.0625f, 141.25f, 145.625f, 150.0625f, 154.625f, 159.4375f,
};

extern void (*data_001D55B4)(void *pDst, const void *pSrc, int n);	/* the host's memcpy */
extern void func_00251AF8(float *pfIn, int n, float *pfRe, float *pfIm);
extern int func_00251C00(float *pfRe, float *pfIm, int n);
extern void func_00251B38(float *pfRe, float *pfIm, int n, float *pfOut);

/* the autocorrelation of a power spectrum resampled to 64 bins (Q15 steps through iBins of it,
 * the ends zero), by the inverse transform; its first iOrder + 1 lags copied out */
void func_0024A438(const float *pfPower, int iBins, float *pfRs, int iOrder)
{
	float afSpec[130];
	float afRe[65], afIm[65];
	float *pf;
	float fStep;
	int i, iStep, iAcc;

#line 712 "../../SaseEncVad/source/LDPDA.c"
	assert(pfPower != NellyNull);
	assert(pfRs != NellyNull);
	fStep = (float)iBins * 0.015625F * 32768.0F;
	iStep = (int)((fStep > 0.0F) ? fStep + 0.5F : fStep - 0.5F);
	afSpec[0] = 0.0F;
	afSpec[1] = 0.0F;
	iAcc = iStep + 16384;
	pf = &afSpec[2];
	for (i = 1; i < 64; i++) {
		*pf++ = pfPower[iAcc >> 15];
		*pf++ = 0.0F;
		iAcc += iStep;
	}
	afSpec[128] = 0.0F;
	afSpec[129] = 0.0F;
	func_00251B38(afRe, afIm, 128, afSpec);
	func_00251C00(afRe, afIm, 128);
	func_00251AF8(afSpec, 128, afRe, afIm);
	data_001D55B4(pfRs, afSpec, (iOrder + 1) * sizeof(float));
}

void func_0024A7C0(const float **pfPitchTable, int *piSize)
{
#line 244 "../../SaseEncVad/source/LDPDA.c"
	assert(pfPitchTable != NellyNull);
	assert(piSize != NellyNull);
	*piSize = 89;
	*pfPitchTable = fPitchTable;
}

int func_0024A840(LdPdaMblk **hLD_PDA_mblk)
{
	LdPdaMblk *p;

#line 276 "../../SaseEncVad/source/LDPDA.c"
	assert(hLD_PDA_mblk != NellyNull);
	p = data_001D55A8(1, sizeof(LdPdaMblk));
	*hLD_PDA_mblk = p;
	if (p == NellyNull)
		return 1;
	p->f0 = 100.0f;
	p->f4 = 0;
	return 0;
}

int func_0024A8B0(LdPdaMblk **hLD_PDA_mblk)
{
#line 313 "../../SaseEncVad/source/LDPDA.c"
	assert(hLD_PDA_mblk != NellyNull);
	if (*hLD_PDA_mblk != NellyNull) {
		data_001D55AC(*hLD_PDA_mblk);
		*hLD_PDA_mblk = NellyNull;
	}
	return 0;
}

void func_0024AAE8(LdPdaMblk *hLD_PDA_mblk, float x, float y)
{
#line 480 "../../SaseEncVad/source/LDPDA.c"
	assert(hLD_PDA_mblk != NellyNull);
	hLD_PDA_mblk->f0 = x;
	hLD_PDA_mblk->f4 = y;
}

/* the band edge: from the top, the bin where the power summed downward first reaches 5 % of
 * the whole, two bins up, at least 76 */
int func_0024A908(const float *pfPower, int iLength)
{
	float fSum, fAcc;
	int i;

#line 357 "../../SaseEncVad/source/LDPDA.c"
	assert(pfPower != NellyNull);
	iLength--;
	fSum = 0.0F;
	for (i = iLength; i >= 0; i--)
		fSum += pfPower[i];
	fSum *= 0x1.999998p-5F;				/* 0.05, one ulp short */
	fAcc = 0.0F;
	for (i = iLength; i >= 0; i--) {
		fAcc += pfPower[i];
		if (fAcc >= fSum)
			break;
	}
	i += 2;
	if (i < 76)
		i = 76;
	return i;
}

/* the voicing candidate: iIndex kept when its value holds 0.75 of the first, the last bin taken
 * over it when that holds its own share (1.1 of the candidate's, 0.88 of the first's) */
static const float afPvThres[3] = { 0x1.8p-1F, 0x1.199998p+0F, 0x1.c28f5cp-1F };

int func_0024A9F0(const float *pfPv, int iLength, int iIndex)
{
	int iLast = iLength - 1;
	int iRet;

#line 425 "../../SaseEncVad/source/LDPDA.c"
	assert(pfPv != NellyNull);
	iRet = 0;
	if (iIndex < iLast) {
		if (iIndex == 0 || pfPv[iIndex] >= afPvThres[0] * pfPv[0]) {
			if (pfPv[iLast] >= afPvThres[1] * pfPv[iIndex])
				iRet = iLast;
			else
				iRet = iIndex;
		}
	} else if (pfPv[iLast] >= afPvThres[2] * pfPv[0])
		iRet = iLast;
	return iRet;
}

extern float ceilf(float);
extern void func_0024D370(const float *pfMag, int iStart, int iEnd, int iMaxPeaks, int *piPeakFreq, int *piNPeaks);
extern void func_0024C8E8(const float *pAutoCorr, int iLpcOrder, float *pfLPCoef, float *pfPARCOR, float *pfResidEnrg);
extern void func_0024D330(const float *pfIn, float *pfOut, int n, float fGamma);
extern void func_0024A5A8(const float *pfAlpc, int iOrder, float *pfMaskEnv, int iBins);

/* the power spectrum's peaks, those above its LPC envelope's fifth of the residual and above the frame's floor kept;
 * four sentinels after them */
void func_0024A210(const float *pfPower, int iBins, int *piPeakFreq, int *piNPeaks, float fEnergy)
{
	float afLpc[8];
	float afRs[8];
	float afParcor[8];
	float afEnv[100];
	float fResid, fFloor, fMag;
	int iOrder, i, n, k;

#line 574 "../../SaseEncVad/source/LDPDA.c"
	assert(pfPower != NellyNull);
	assert(piPeakFreq != NellyNull);
	func_0024D370(pfPower, 0, 97, 48, piPeakFreq, piNPeaks);
	iOrder = (int)ceilf((float)iBins * 5.0F / 96.0F);
	if (iOrder > 7)
		iOrder = 7;
	func_0024A438(pfPower, iBins, afRs, iOrder);
	afRs[0] *= 0x1.00001p+0F;
	func_0024C8E8(afRs, iOrder, afLpc, afParcor, &fResid);
	func_0024D330(afLpc, afLpc, iOrder, 0x1.f5c28ep-1F);
	func_0024A5A8(afLpc, iOrder, afEnv, iBins);
	fResid *= 0x1.999998p-3F;
	fFloor = fEnergy * 0x1.5798eep-11F;
	n = 0;
	for (i = 0; i < *piNPeaks; i++) {
		k = piPeakFreq[i];
		if (k < iBins) {
			fMag = pfPower[k];
			if (fResid <= fMag * afEnv[k] && fFloor < fMag) {
				piPeakFreq[n] = k;
				n++;
			}
		}
	}
	piPeakFreq[n] = 0x7FFF;
	piPeakFreq[n + 1] = 0x7FFF;
	piPeakFreq[n + 2] = 0x7FFF;
	piPeakFreq[n + 3] = 0x7FFF;
	*piNPeaks = n;
}

extern void (*data_001D55B0)(void *p, int c, int n);	/* the host's memset */
extern void func_00251B78(float *pfRe, float *pfIm, int n);

/* the LPC envelope: the predictor's power spectrum at iBins points (Q15 steps through the 128-point transform of the
 * coefficients, zero-padded), its last point the one before when it falls off the end. The step's product is the
 * expression in the loop, not an accumulator (loop.c's giv: `li a2,16384` after the test), j++ in the arm keeps (j + 1) << 2
 * from folding. GCC round 78 (open since 44a, 8 bytes short): the loop's body is two statements on one index -- `j = 2 *
 * (...) >> 15; pfMaskEnv[i] = afSpec[j] * afSpec[j]; j++; pfMaskEnv[i] += afSpec[j] * afSpec[j];` -- the store and the
 * reload of pfMaskEnv[i] between them is what holds k * 8 in two registers (`sll a0,v0,3; move v0,a0`); the doubled
 * value straight into j (no k in the loop) gives the sra v0. And the asserts name the parameters pfAlpc and pfMaskEnv
 * (the image's strings: a content mismatch the register miss had hidden). */
void func_0024A5A8(const float *pfAlpc, int iOrder, float *pfMaskEnv, int iBins)
{
	float afSpec[130];
	float afRe[65], afIm[65];
	float fStep;
	int i, k, j, iStep;

#line 811 "../../SaseEncVad/source/LDPDA.c"
	assert(pfAlpc != NellyNull);
	assert(pfMaskEnv != NellyNull);
	data_001D55B4(afSpec, pfAlpc, (iOrder + 1) * 4);
	i = iOrder + 1;
	data_001D55B0(&afSpec[i], 0, (130 - i) * 4);
	func_00251AF8(afSpec, 128, afRe, afIm);
	func_00251B78(afRe, afIm, 128);
	func_00251B38(afRe, afIm, 128, afSpec);
	fStep = 64.0F / (float)iBins * 32768.0F;
	iStep = (int)((fStep > 0.0F) ? fStep + 0.5F : fStep - 0.5F);
	for (i = 0; i < iBins - 1; i++) {
		j = 2 * ((iStep * i + 0x4000) >> 15);
		pfMaskEnv[i] = afSpec[j] * afSpec[j];
		j++;
		pfMaskEnv[i] += afSpec[j] * afSpec[j];
	}
	k = (iStep * i + 0x4000) >> 15;
	j = 2 * k;
	if (j >= 128)
		pfMaskEnv[i] = pfMaskEnv[i - 1];
	else {
		j++;
		pfMaskEnv[i] = afSpec[2 * k] * afSpec[2 * k] + afSpec[j] * afSpec[j];
	}
}

extern void func_0024EE38(const float *pfIn, int iLength, float *pfOut);
extern void func_0024ECF8(const float *pfX, int iLength, int *piMaxIndex, float *pfXmax);
extern float func_0024AD48(float fXval);
extern int func_0024FFF8(const float *pfSpec, int iBins, const int *piPeak, int iNPeaks, float *pfPitch, float *pfVoicing,
			 int *piBest, float fPrevPitch, float fPrevVoicing);
extern int func_00251638(int iBins, float fPitch);
extern float func_002513F0(const float *pfSW, int iHarm, float fPitch);

/* the pitch detector: the power spectrum and its peaks, the spectrum normalised to its maximum and compressed, the candidates
 * over the peaks scored against the magnitudes and the best one kept in the block */
float func_00249FA8(const float *pfSW, LdPdaMblk *hLD_PDA_mblk)
{
	float afSpec[102];
	float afVoicing[4];
	float afPitch[4];
	float afScore[4];
	int aiPeak[52];
	int i;
	float fMaxPower;
	int iNPeaks, iBest;
	int iBins, n;
	float fPitch;

#line 144 "../../SaseEncVad/source/LDPDA.c"
	assert(pfSW != NellyNull);
	assert(hLD_PDA_mblk != NellyNull);
	func_0024EE38(pfSW, 102, afSpec);
	func_0024ECF8(afSpec, 102, &i, &fMaxPower);
#line 154 "../../SaseEncVad/source/LDPDA.c"
	assert(fMaxPower >= 0.0F);
	iBins = func_0024A908(afSpec, 96);
	func_0024A210(afSpec, iBins, aiPeak, &iNPeaks, fMaxPower);
	if (fMaxPower > 0.0F)
		fMaxPower = 1.0F / fMaxPower;
	for (i = iBins - 1; i >= 0; i--) {
		afSpec[i] *= fMaxPower;
		afSpec[i] = func_0024AD48(afSpec[i]);
	}
	n = func_0024FFF8(afSpec, iBins, aiPeak, iNPeaks, afPitch, afVoicing, &iBest, hLD_PDA_mblk->f0, hLD_PDA_mblk->f4);
	iBins = func_00251638(iBins, afPitch[n - 1]);
	for (i = n - 1; i >= 0; i--)
		afScore[i] = func_002513F0(pfSW, iBins, afPitch[i]);
	iBest = func_0024A9F0(afScore, n, iBest);
	fPitch = afPitch[iBest];
	func_0024AAE8(hLD_PDA_mblk, fPitch, afVoicing[iBest]);
	return fPitch;
}
