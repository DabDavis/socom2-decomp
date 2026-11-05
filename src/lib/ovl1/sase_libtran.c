/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, libtran.c (OVL1 0x24D8B8-0x24ECF8): the transforms -- the real FFT's
 * passes, the LPC conversions (reflection coefficients to their arcsines and back, the step-up
 * recursion to the predictor), the FFT driver and its bit reversal. Asserting as the codec does
 * (file and line by `#line`, as in sase_encsc.c). Written (GCC round 18a): the de-interleave
 * 0x24E8D0, k <-> ASK 0x24E918/0x24E9C0, the step-up 0x24EA68, the FFT driver 0x24EB60 and its
 * bit reversal 0x24EC40 (Numerical Recipes' four1 shape, 0-based: the pair swapped at [j], [j-1]).
 * The passes before them (0x24D8B8-0x24E788) are not written but the inverse driver 0x24DA70 (GCC
 * round 27a, as first written: the real-input merge, the bit reversal, the passes, the 1/N scale
 * unless the flags' bit 3 is set; its asserts at lines 819-822, after the forward driver's 758 though
 * it links first).
 *
 * Shapes read off the target: a count tested `!= 0` (bnez after the step) is `while (n != 0) {
 * ...; n--; }` -- `while (n--)`, a for loop or a do-while each give other code; the copy loop's
 * `while (n-- > 0)` keeps the old count for its bgtz. TWIDDLE_PERIOD compares unsigned (sltiu): a
 * sizeof's quotient, as the twiddle table's own length would be. The short assert texts ("N >= 8",
 * "log2N>0") sit in MAIN's small data at 0x1D5578/0x1D5580 (GCC's -G8 section for an 8-byte
 * string), still addressed by lui/addiu.
 * Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

extern float asinf(float);
extern float sinf(float);

#define MAX_LPC_ORDER 18
#define TWIDDLE_PERIOD (sizeof(float[512]) / sizeof(float))

extern void func_0024DBB8(float *pfInOut, int log2N);
extern void func_0024E408(float *pfInOut, int log2N);
extern void func_0024DFB8(float *pfInOut, int log2N);
extern void func_0024E5B0(float *pfInOut, int log2N);
void func_0024EC40(float *pfData, int nn);

/* the cepstral scale: 1, then 1 / (n 2 ln 2) -- the vendor's six-digit decimals as they converted
 * (0x3E9EB8, 44 floats; the file name follows it in .rodata) */
static const float afCsScale[44] = {
	1.0F, 0x1.715476p-1F, 0x1.715486p-2F, 0x1.ec7086p-3F,
	0x1.715486p-3F, 0x1.27768p-3F, 0x1.ec710cp-4F, 0x1.a617cp-4F,
	0x1.7154p-4F, 0x1.484b5cp-4F, 0x1.277706p-4F, 0x1.0c9a76p-4F,
	0x1.ec7p-5F, 0x1.c68ec4p-5F, 0x1.a617cp-5F, 0x1.89f40ap-5F,
	0x1.7154p-5F, 0x1.5b9a5ap-5F, 0x1.484b5cp-5F, 0x1.370478p-5F,
	0x1.2775fap-5F, 0x1.19652ap-5F, 0x1.0c9b84p-5F, 0x1.00ecfap-5F,
	0x1.ec7p-6F, 0x1.d8be72p-6F, 0x1.c68ec4p-6F, 0x1.b5bb38p-6F,
	0x1.a615a8p-6F, 0x1.97891ep-6F, 0x1.89f40ap-6F, 0x1.7d3d42p-6F,
	0x1.7154p-6F, 0x1.66234ap-6F, 0x1.5b9a5ap-6F, 0x1.51ac9ap-6F,
	0x1.484944p-6F, 0x1.3f6c26p-6F, 0x1.370478p-6F, 0x1.2f09d8p-6F,
	0x1.277814p-6F, 0x1.204294p-6F, 0x1.19652ap-6F, 0x1.12dba4p-6F,
};
#define MAX_CS_TABLE_ORDER (sizeof(afCsScale) / sizeof(float) - 1)

/* the LPC cepstrum (GCC round 33a): nCs coefficients of the predictor pfLPC (1-based, iOrder
 * deep) and its gain, by the recursion n c[n] = -n a[n] - sum a[k] c[n-k] -- the first min(n, iOrder)
 * with the n a[n] term, the rest without -- then each scaled by its table entry. The table's bound
 * compares unsigned (sltiu): a sizeof's quotient, as TWIDDLE_PERIOD is. */
void func_0024D8B8(const float *pfLPC, int iOrder, float *pfCs, int nCs, float fGain)
{
	int iOrder_Cs = nCs - 1;
	int i, k, m;
	float fSum;

#line 235 "../../SaseEncVad/source/libtran.c"
	assert(iOrder_Cs >= 0);
	assert(iOrder_Cs <= MAX_CS_TABLE_ORDER);
	m = (iOrder_Cs < iOrder) ? iOrder_Cs : iOrder;
	pfCs[0] = fGain;
	for (i = 1; i <= m; i++) {
		fSum = -(float)i * pfLPC[i];
		for (k = 1; k < i; k++)
			fSum -= pfLPC[k] * pfCs[i - k];
		pfCs[i] = fSum;
	}
	for (; i <= iOrder_Cs; i++) {
		fSum = 0.0F;
		for (k = 1; k <= iOrder; k++)
			fSum -= pfLPC[k] * pfCs[i - k];
		pfCs[i] = fSum;
	}
	for (i = 1; i <= iOrder_Cs; i++)
		pfCs[i] = afCsScale[i] * pfCs[i];
}

/* the inverse FFT of 2^log2N points: the real-input merge, the bit reversal, the passes, and
 * the 1/N scale unless iFlags' bit 3 asks for none */
void func_0024DA70(float *pfInOut, int log2N, int iFlags)
{
	float fScale;
	int i, N = 1 << log2N;

#line 819 "../../SaseEncVad/source/libtran.c"
	assert(pfInOut != NellyNull);
	assert(N >= 8);
	assert(N <= TWIDDLE_PERIOD);
	assert(log2N >= 3);
	func_0024E5B0(pfInOut, log2N);
	func_0024EC40(pfInOut, N >> 1);
	func_0024DFB8(pfInOut, log2N - 1);
	if (!(iFlags & 8)) {
		fScale = 1.0F / N;
		for (i = 0; i < N; i++)
			pfInOut[i] *= fScale;
	}
}

/* an interleaved complex vector split into its two halves */
void func_0024E8D0(const float *pfIn, float *pfRe, float *pfIm, int iLength)
{
	while (iLength-- > 0) {
		*pfRe++ = *pfIn++;
		*pfIm++ = *pfIn++;
	}
}

/* the reflection coefficients' arcsines */
void func_0024E918(int iLPCorder, const float *pfk, float *pfASK)
{
#line 503 "../../SaseEncVad/source/libtran.c"
	assert(pfk != NellyNull && pfASK != NellyNull);
	assert(iLPCorder > 0);
	while (iLPCorder != 0) {
		*pfASK++ = asinf(*pfk++);
		iLPCorder--;
	}
}

/* and back */
void func_0024E9C0(int iLPCorder, const float *pfASK, float *pfk)
{
#line 548 "../../SaseEncVad/source/libtran.c"
	assert(pfASK != NellyNull && pfk != NellyNull);
	assert(iLPCorder > 0);
	while (iLPCorder != 0) {
		*pfk++ = sinf(*pfASK++);
		iLPCorder--;
	}
}

/* the step-up recursion: reflection coefficients to the predictor, through a scratch array the
 * two buffers alternate with so that the last order lands in pfA */
void func_0024EA68(const float *pfk, float *pfA, int iOrder)
{
	float afTmp[MAX_LPC_ORDER + 1];
	float *pfPrev, *pfT;
	float fK;
	int i, j;

#line 604 "../../SaseEncVad/source/libtran.c"
	assert(iOrder <= MAX_LPC_ORDER);
	pfA[0] = 1.0F;
	if (iOrder & 1) {
		pfPrev = pfA;
		pfA = afTmp;
	} else
		pfPrev = afTmp;
	for (i = 0; i < iOrder; i++) {
		pfT = pfA;
		pfA = pfPrev;
		pfPrev = pfT;
		fK = pfk[i];
		pfA[i + 1] = -fK;
		for (j = 1; j <= i; j++)
			pfA[j] = pfPrev[j] - fK * pfPrev[i + 1 - j];
	}
}

/* the FFT of 2^log2N points: the bit reversal, the passes, the real-input split */
void func_0024EB60(float *pfInOut, int log2N)
{
	int N = 1 << log2N;

#line 758 "../../SaseEncVad/source/libtran.c"
	assert(pfInOut != NellyNull);
	assert(N >= 8);
	assert(N <= TWIDDLE_PERIOD);
	assert(log2N>0);
	func_0024EC40(pfInOut, N >> 1);
	func_0024DBB8(pfInOut, log2N - 1);
	func_0024E408(pfInOut, log2N);
}

/* the bit reversal of nn complex points */
void func_0024EC40(float *pfData, int nn)
{
	float fTemp;
	int n, m, i, j;

	n = nn << 1;
	j = 1;
	for (i = 1; i < n; i += 2) {
		if (j > i) {
			fTemp = pfData[j];
			pfData[j] = pfData[i];
			pfData[i] = fTemp;
			fTemp = pfData[j - 1];
			pfData[j - 1] = pfData[i - 1];
			pfData[i - 1] = fTemp;
		}
		m = n >> 1;
		while (m >= 2 && j > m) {
			j -= m;
			m >>= 1;
		}
		j += m;
	}
}
