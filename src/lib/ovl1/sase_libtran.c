/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, libtran.c (OVL1 0x24D8B8-0x24ECF8): the transforms -- the real FFT's
 * passes, the LPC conversions (reflection coefficients to their arcsines and back, the step-up
 * recursion to the predictor), the FFT driver and its bit reversal. Asserting as the codec does
 * (file and line by `#line`, as in sase_encsc.c). Written (GCC round 18a): the de-interleave
 * 0x24E8D0, k <-> ASK 0x24E918/0x24E9C0, the step-up 0x24EA68, the FFT driver 0x24EB60 and its
 * bit reversal 0x24EC40 (Numerical Recipes' four1 shape, 0-based: the pair swapped at [j], [j-1]).
 * The passes before them (0x24D8B8-0x24E788) are not written.
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
void func_0024EC40(float *pfData, int nn);

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
