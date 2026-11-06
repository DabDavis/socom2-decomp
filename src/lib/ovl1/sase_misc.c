/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder: QP0SC3.c's pitch-index dequantiser (OVL1 0x250C68: exp of a line in the
 * index, its two constants .lit4 floats through $gp, the call a sibling `j expf`), asserting as
 * its own file and line (by `#line`, as in sase_encsc.c). libsnd.c's circular-buffer release,
 * here since round 12a, is in sase_libsnd.c with the rest of its object (GCC round 18a).
 * Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8).
 *
 * A float constant is written as a decimal that converts to the target's bits under this GCC, which
 * does not round to nearest: 0.011747969314455986f, the target's 0x3C407A8E exactly, comes out one
 * ulp low (0x3C407A8D); a decimal between it and the next float up gives it. */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

extern float expf(float x);

#define BITS_PITCH_SC3 8

float func_00250C68(int iIndex)
{
#line 134 "../../SaseEncVad/source/QP0SC3.c"
	assert((iIndex >= 0) && (iIndex < (1<<BITS_PITCH_SC3)));
	return expf(5.0751733779907227f - (float)iIndex * 0.0117479697f);
}

extern void func_0024A7C0(const float **ppfTable, int *piLength);
extern int func_0024B720(const float *pfCodeBook, int iLength, float fValue);
extern void func_00248A18(float *pfMax, int n, const float *pfAmp, float fStep, float fFreq, float fEnd, float fBias,
			  const int *piHarm, float *pfMaxFreq);

/* RefineC0.c, kept here by address: the pitch refined around its table entry by the harmonic comb, twice, the second search
 * narrower: the comb's best fundamental, its value over the amplitudes' sum. The comb's float parameters sit before its last
 * two pointers (`mov.s $f15` before `move a3`); the first window is a local, the second two expressions, CSE's temporary. */
int func_00250CD8(const float *pfAmp, const float *pfFreq, int iLength, float *pfSum, float *pfPitch, float *pfGain, float fPitch)
{
	int aiHarm[100];
	const float *pfTable;
	int iTableLen;
	float fMax, fBest, deltapitch, fSum, fW, fLo, fHi, fStep;
	float fScale = 2048.0F;
	int i;
	unsigned int k;

	for (i = 0; i < iLength; i++)
		aiHarm[i] = (int)(pfFreq[i] * fScale + 0x1.0624dcp-10F + 0.5F);
	func_0024A7C0(&pfTable, &iTableLen);
	fBest = fPitch;
	k = func_0024B720(pfTable, iTableLen, fPitch);
	if (k != 0) {
		deltapitch = pfTable[k] - pfTable[k - 1];
		if (k < iTableLen - 1)
			deltapitch = (pfTable[k + 1] - pfTable[k - 1]) * 0.5F;
	} else
		deltapitch = pfTable[1] - pfTable[0];
#line 97 "../../SaseEncVad/source/RefineC0.c"
	assert(deltapitch > 0);
	fSum = 0x1.b7cdfcp-34F;
	for (i = 0; i < iLength; i++)
		fSum += pfAmp[i];
	fMax = 0.0F;
	fW = deltapitch * 0.75F;
	fStep = deltapitch * 0.375F;
	fLo = fBest - fW;
	if (fLo < 8.0F)
		fLo = 8.0F;
	fHi = fBest + fW;
	if (fHi > 160.0F)
		fHi = 160.0F;
	func_00248A18(&fMax, iLength, pfAmp, fStep, fLo, fHi, fSum, aiHarm, &fBest);
	fStep = deltapitch * 0.125F;
	fLo = fBest - deltapitch * 0.25F;
	if (fLo < 8.0F)
		fLo = 8.0F;
	fHi = fBest + deltapitch * 0.25F;
	if (fHi > 160.0F)
		fHi = 160.0F;
	func_00248A18(&fMax, iLength, pfAmp, fStep, fLo, fHi, fSum, aiHarm, &fBest);
	*pfSum = fSum;
	*pfGain = fMax * 0.5F / fSum;
	*pfPitch = fBest;
	return 0;
}
