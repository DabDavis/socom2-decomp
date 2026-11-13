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
extern const float *data_001D5570;	/* the FFT's quarter-wave sine table, read forwards (0x3EA038) */
extern const float *data_001D5574;	/* ... and its end, read backwards for the cosine (0x3EA238) */
extern float *data_001D5594;		/* the sine table, 256 points a turn (0x3EA6E8) */
extern float *data_001D5598;		/* the cosine table, a quarter turn on (0x3EA7E8) */
extern const float data_001D559C;	/* 256 / 2pi, the phase's scale to a table index */

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

#define TWIDDLE_TABLE_SIZE (TWIDDLE_PERIOD / 4 + 1)

/* the forward passes of the N-point complex FFT (log2N = the driver's log2N - 1), decimation in
 * time after the bit reversal (GCC lane OVL1 round 1, unwritten before; m2c refused it, "unsupported
 * wide integer operation"): the two first stages written out -- W = 1, then W = 1 and -j -- then
 * stage by stage with the twiddles read as the real split reads them (the cosine backwards from the
 * quarter-wave table's end, the sine forwards: 0x1D5574/0x1D5570, $gp words), the index rising
 * through the first half of each group and falling through the second, TWIDDLE_TABLE_SIZE checked
 * unsigned (sltiu 130: a sizeof's quotient). Read off the target:
 *  - each element through its own pointer, set from pfBase in the body (`addiu a0,s0,8` each
 *    iteration, the stores in the source's order: four pointers may alias); written pfBase[2] the
 *    offsets fold into the loads and the stores reorder, written pfInOut[j + 2] they fold too;
 *  - the pass counters count down (`k > 0`: blez, then bgtz) and share one variable with the twiddle
 *    loops' (both in s1); the group loop counts up (dbra: blez, then bnez); the stage loop is
 *    `i = 3; i <= log2N` (slti 3 -- from 2, `2 < log2N`, the 2 is loaded into a register);
 *  - the declaration order is the spill slots' (span 4, nGroups 8, the stage counter 12); iWStep set
 *    first gives it t0 and the string's %hi a3, as the target has them.
 * OPEN (60 words, the integer side exact): the FP registers alone -- the target's are those of global
 * pseudos shared between the passes and the twiddle loops (the pass's sum r and the twiddle's ti both
 * f7, its diff r, the sine and tr f4); 1,900 role assignments of the float variables swept
 * (build/scratch/ovl1gcc-r1/gen_*.py), 47 words at best, none a shape a vendor would write. */
void func_0024DBB8(float *pfInOut, int log2N)
{
	int N = 1 << log2N;
	int iSpan, iWIndex, iWStep, nGroups, i, j, k;
	float *pfBase, *pfAr, *pfAi, *pfBr, *pfBi;
	const float *pfSin, *pfCos;
	float fR, fI, fTr, fTi, fC, fS;

#line 910 "../../SaseEncVad/source/libtran.c"
	assert(pfInOut != NellyNull);
	assert(N >= 4);
	assert(N <= TWIDDLE_PERIOD/2);
	assert(log2N >= 2);
	pfBase = pfInOut;
	for (k = N >> 1; k > 0; k--) {
		pfAr = pfBase;
		pfAi = pfBase + 1;
		pfBr = pfBase + 2;
		pfBi = pfBase + 3;
		fR = *pfAr + *pfBr;
		fTr = *pfAr - *pfBr;
		fI = *pfAi + *pfBi;
		fTi = *pfAi - *pfBi;
		*pfBr = fTr;
		*pfAr = fR;
		*pfBi = fTi;
		*pfAi = fI;
		pfBase += 4;
	}
	pfBase = pfInOut;
	for (k = N >> 2; k > 0; k--) {
		pfAr = pfBase;
		pfAi = pfBase + 1;
		pfBr = pfBase + 4;
		pfBi = pfBase + 5;
		fR = *pfAr + *pfBr;
		fTr = *pfAr - *pfBr;
		fI = *pfAi + *pfBi;
		fTi = *pfAi - *pfBi;
		*pfBr = fTr;
		*pfAr = fR;
		*pfBi = fTi;
		*pfAi = fI;
		pfBase += 2;
		pfAr = pfBase;
		pfAi = pfBase + 1;
		pfBr = pfBase + 4;
		pfBi = pfBase + 5;
		fR = *pfAr + *pfBi;
		fTr = *pfAr - *pfBi;
		fI = *pfAi - *pfBr;
		fTi = *pfAi + *pfBr;
		*pfBr = fTr;
		*pfAr = fR;
		*pfBi = fTi;
		*pfAi = fI;
		pfBase += 6;
	}
	iWStep = 64;
	nGroups = N >> 3;
	iSpan = 4;
	pfCos = data_001D5574;
	iWIndex = 0;
	pfSin = data_001D5570;
	for (i = 3; i <= log2N; i++) {
		pfBase = pfInOut;
		for (j = 0; j < nGroups; j++) {
			for (k = iSpan >> 1; k > 0; k--) {
				fC = pfCos[-iWIndex];
				fS = pfSin[iWIndex];
				pfAr = pfBase;
				pfAi = pfBase + 1;
				pfBr = pfBase + (iSpan << 1);
				pfBi = pfBr + 1;
				fR = *pfBr;
				fI = *pfBi;
				fTi = fR * fS - fI * fC;
				fTr = fR * fC + fI * fS;
				fR = *pfAr;
				fI = *pfAi;
				*pfBr = fR - fTr;
				*pfAr = fTr + fR;
				*pfBi = fI + fTi;
				*pfAi = fI - fTi;
				pfBase += 2;
#line 1039 "../../SaseEncVad/source/libtran.c"
				assert(iWIndex <= TWIDDLE_TABLE_SIZE);
				iWIndex += iWStep;
			}
			for (k = iSpan >> 1; k > 0; k--) {
				fC = pfCos[-iWIndex];
				fS = pfSin[iWIndex];
				pfAr = pfBase;
				pfAi = pfBase + 1;
				pfBr = pfBase + (iSpan << 1);
				pfBi = pfBr + 1;
				fR = *pfBr;
				fI = *pfBi;
				fTi = fR * fS + fI * fC;
				fTr = fR * fC - fI * fS;
				fR = *pfAr;
				fI = *pfAi;
				*pfBr = fR + fTr;
				*pfAr = fR - fTr;
				*pfBi = fI + fTi;
				*pfAi = fI - fTi;
				pfBase += 2;
#line 1069 "../../SaseEncVad/source/libtran.c"
				assert(iWIndex <= TWIDDLE_TABLE_SIZE);
				assert(iWIndex >= 0);
				iWIndex -= iWStep;
			}
			pfBase += iSpan << 1;
		}
		iWStep >>= 1;
		iSpan <<= 1;
		nGroups >>= 1;
	}
}

/* the inverse passes: the forward passes conjugated (W = 1 and +j, the twiddle products' signs
 * swapped), the table index checked at each group's second half and pfBase against the data's end
 * there (`sltu` against pfInOut + 2N hoisted). Read off the target: N lives to that assert, so it is
 * spilled (slot 20), and the declaration order is again the slots' -- iWStep 4, iSpan 8, nGroups 12,
 * the stage counter 16, N 20 (declared after them); the pass counters count up here (dbra: blez,
 * then bnez) where the forward passes' count down; the stage's updates in the order span, step,
 * groups. OPEN (66 words, the integer side exact): the FP registers, as the forward passes'. */
void func_0024DFB8(float *pfInOut, int log2N)
{
	int iWStep, iSpan, nGroups, i, N = 1 << log2N;
	int iWIndex, j, k;
	float *pfBase, *pfAr, *pfAi, *pfBr, *pfBi;
	const float *pfSin, *pfCos;
	float fR, fI, fTr, fTi, fC, fS;

#line 1153 "../../SaseEncVad/source/libtran.c"
	assert(pfInOut != NellyNull);
	assert(N >= 4);
	assert(N <= TWIDDLE_PERIOD);
	assert(log2N >= 2);
	pfBase = pfInOut;
	for (k = 0; k < (N >> 1); k++) {
		pfAr = pfBase;
		pfAi = pfBase + 1;
		pfBr = pfBase + 2;
		pfBi = pfBase + 3;
		fR = *pfAr + *pfBr;
		fTr = *pfAr - *pfBr;
		fI = *pfAi + *pfBi;
		fTi = *pfAi - *pfBi;
		*pfBr = fTr;
		*pfAr = fR;
		*pfBi = fTi;
		*pfAi = fI;
		pfBase += 4;
	}
	pfBase = pfInOut;
	for (k = 0; k < (N >> 2); k++) {
		pfAr = pfBase;
		pfAi = pfBase + 1;
		pfBr = pfBase + 4;
		pfBi = pfBase + 5;
		fR = *pfAr + *pfBr;
		fTr = *pfAr - *pfBr;
		fI = *pfAi + *pfBi;
		fTi = *pfAi - *pfBi;
		*pfBr = fTr;
		*pfAr = fR;
		*pfBi = fTi;
		*pfAi = fI;
		pfBase += 2;
		pfAr = pfBase;
		pfAi = pfBase + 1;
		pfBr = pfBase + 4;
		pfBi = pfBase + 5;
		fR = *pfAr - *pfBi;
		fTr = *pfAr + *pfBi;
		fI = *pfAi + *pfBr;
		fTi = *pfAi - *pfBr;
		*pfBr = fTr;
		*pfAr = fR;
		*pfBi = fTi;
		*pfAi = fI;
		pfBase += 6;
	}
	iWStep = 64;
	iSpan = 4;
	iWIndex = 0;
	nGroups = N >> 3;
	pfCos = data_001D5574;
	pfSin = data_001D5570;
	for (i = 3; i <= log2N; i++) {
		pfBase = pfInOut;
		for (j = 0; j < nGroups; j++) {
			for (k = iSpan >> 1; k > 0; k--) {
				fC = pfCos[-iWIndex];
				fS = pfSin[iWIndex];
				pfAr = pfBase;
				pfAi = pfBase + 1;
				pfBr = pfBase + (iSpan << 1);
				pfBi = pfBr + 1;
				fR = *pfBr;
				fI = *pfBi;
				fTi = fR * fS + fI * fC;
				fTr = fR * fC - fI * fS;
				fR = *pfAr;
				fI = *pfAi;
				*pfBr = fR - fTr;
				*pfAr = fTr + fR;
				*pfBi = fI - fTi;
				*pfAi = fTi + fI;
				pfBase += 2;
#line 1287 "../../SaseEncVad/source/libtran.c"
				assert(iWIndex <= TWIDDLE_TABLE_SIZE);
				iWIndex += iWStep;
			}
			for (k = iSpan >> 1; k > 0; k--) {
#line 1294 "../../SaseEncVad/source/libtran.c"
				assert(pfBase <= (pfInOut + (N << 1)));
				fC = pfCos[-iWIndex];
				fS = pfSin[iWIndex];
				pfAr = pfBase;
				pfAi = pfBase + 1;
				pfBr = pfBase + (iSpan << 1);
				pfBi = pfBr + 1;
				fR = *pfBr;
				fI = *pfBi;
				fTi = fR * fS - fI * fC;
				fTr = fR * fC + fI * fS;
				fR = *pfAr;
				fI = *pfAi;
				*pfBr = fR + fTr;
				*pfAr = fR - fTr;
				*pfBi = fI - fTi;
				*pfAi = fTi + fI;
				pfBase += 2;
#line 1317 "../../SaseEncVad/source/libtran.c"
				assert(iWIndex <= TWIDDLE_TABLE_SIZE);
				assert(iWIndex >= 0);
				iWIndex -= iWStep;
			}
			pfBase += iSpan << 1;
		}
		iSpan <<= 1;
		iWStep >>= 1;
		nGroups >>= 1;
	}
}

/* the real-input split after the forward passes (GCC round 37a, unwritten before): the N/2-point
 * complex FFT in pfInOut[0..N-1] made the spectrum of N real samples in pfInOut[0..N+1] -- Numerical
 * Recipes' realft unpacking, the twiddles stepping TWIDDLE_PERIOD / N through the quarter-wave table
 * (0x1D5570/4: $gp words), the sine forwards and the cosine backwards from its end; the middle bin
 * conjugated last. Read off the target: the data is indexed (pfInOut[i], pfInOut[N - i]: loop.c's
 * givs, formed in the preheader after the entry test, i kept for the last store), the stores in the
 * source's order (through one pointer they cannot pass each other: [i], [N-i], [i+1], [N-i+1]), the
 * cosine pointer loaded first, and the first pair's sum and difference are the loop's fTi and fTr --
 * global pseudos, which takes them to f5/f6 and the two loads to f1/f0 as the target has them (read
 * into fH1r/fH1i, or stored straight, every prologue register is another).
 * OPEN (23 words: 8 structural, 18 registers alone): the target's sched1 computes the tail C - tr,
 * C + tr, D - ti, ti + D (local-alloc ties C + tr to C and ti + D to ti), so the C chain dies first and
 * the load of pfInOut[i] takes f0, pfInOut[i + 1] f1; this compile schedules ti + D first (the neg's
 * longer path to the last store), C's chain outlives D's and the two take f1/f0 -- 3,840 spellings
 * swept (the 0.5's side, each product's and sum's operand order, tr/ti first, the h statements' and
 * the stores' orders, the last store's form: build/scratch/gcc/r37/p/gen_e408*.py), 21 words at best
 * (the stores out of their order). */
void func_0024E408(float *pfInOut, int log2N)
{
	int N = 1 << log2N;
	int i, iStep;
	const float *pfSin, *pfCos;
	float fRe, fIm, fH1r, fH1i, fH2r, fH2i, fTr, fTi, fC, fS;

#line 1361 "../../SaseEncVad/source/libtran.c"
	assert(pfInOut != NellyNull);
	assert(N >= 8);
	assert(N <= TWIDDLE_PERIOD);
	fRe = pfInOut[0];
	fIm = pfInOut[1];
	fTi = fRe + fIm;
	fTr = fRe - fIm;
	pfInOut[N] = fTr;
	pfInOut[0] = fTi;
	pfInOut[N + 1] = 0.0F;
	pfInOut[1] = 0.0F;
	iStep = TWIDDLE_PERIOD >> log2N;
	pfCos = data_001D5574;
	pfSin = data_001D5570;
	for (i = 2; i < (N >> 1); i += 2) {
		pfCos -= iStep;
		pfSin += iStep;
		fC = *pfCos;
		fS = *pfSin;
		fH1r = 0.5F * (pfInOut[i] + pfInOut[N - i]);
		fH1i = 0.5F * (pfInOut[i + 1] - pfInOut[N - i + 1]);
		fH2r = 0.5F * (pfInOut[i + 1] + pfInOut[N - i + 1]);
		fH2i = 0.5F * (pfInOut[i] - pfInOut[N - i]);
		fTr = fC * fH2r - fS * fH2i;
		fTi = fC * fH2i + fS * fH2r;
		pfInOut[i] = fH1r + fTr;
		pfInOut[N - i] = fH1r - fTr;
		pfInOut[i + 1] = fH1i - fTi;
		pfInOut[N - i + 1] = -(fTi + fH1i);
	}
	pfInOut[i + 1] = -pfInOut[i + 1];
}

/* the real-input merge before the inverse passes (GCC round 37a, unwritten before): the spectrum of
 * N real samples in pfInOut[0..N+1] folded back into the N/2-point complex spectrum the passes
 * invert -- the split's inverse (no 0.5: the 1/N scale is the driver's), the middle bin doubled and
 * conjugated in the prologue (x * 2.0F is fold's x + x, the target's add.s; -2.0F a lui constant),
 * the twiddles as the split steps them, the stores in the target's order.
 * OPEN (36 words, the size and the shape the target's): registers. The target's prologue keeps
 * x1 + x[N+1] in f7, the loop's ti (a global pseudo, as the split's first sum and difference are its
 * ti/tr), and x0 + x[N] tied to x0's f2 (block-local); which variable holds each of the four partial
 * sums -- a local or any of the loop's eight, 3,393 variants (build/scratch/gcc/r37/p/gen_e5b0.py) --
 * reaches 26 words at best, with reuse no vendor would write (fC as a partial sum). */
void func_0024E5B0(float *pfInOut, int log2N)
{
	int N = 1 << log2N;
	int i, iStep;
	const float *pfSin, *pfCos;
	float fH1r, fH1i, fH2r, fH2i, fTr, fTi, fC, fS;

#line 1449 "../../SaseEncVad/source/libtran.c"
	assert(pfInOut != NellyNull);
	assert(N >= 8);
	assert(N <= TWIDDLE_PERIOD);
	assert(log2N >= 3);
	fTr = pfInOut[0] + pfInOut[N];
	fTi = pfInOut[1] + pfInOut[N + 1];
	pfInOut[1] = (pfInOut[0] - pfInOut[N]) + (pfInOut[1] - pfInOut[N + 1]);
	pfInOut[0] = fTr - fTi;
	pfInOut[N >> 1] *= 2.0F;
	pfInOut[(N >> 1) + 1] *= -2.0F;
	iStep = TWIDDLE_PERIOD >> log2N;
	pfCos = data_001D5574;
	pfSin = data_001D5570;
	for (i = 2; i < (N >> 1); i += 2) {
		pfCos -= iStep;
		pfSin += iStep;
		fC = *pfCos;
		fS = *pfSin;
		fH1r = pfInOut[i] + pfInOut[N - i];
		fH1i = pfInOut[i + 1] - pfInOut[N - i + 1];
		fH2r = pfInOut[i + 1] + pfInOut[N - i + 1];
		fH2i = pfInOut[i] - pfInOut[N - i];
		fTr = fC * fH2r + fS * fH2i;
		fTi = fC * fH2i - fS * fH2r;
		pfInOut[i] = fH1r - fTr;
		pfInOut[N - i] = fH1r + fTr;
		pfInOut[i + 1] = fH1i + fTi;
		pfInOut[N - i + 1] = -(fH1i - fTi);
	}
}

/* a DFT of iLength samples about their centre at nOut frequencies, fStep apart (GCC round 36a,
 * unwritten before; no caller in the image): each frequency's phase is rounded to an 8-bit index
 * of the 256-point sine and cosine tables (data_001D5594/8, sase_coder.c's cosine), the index's
 * step that rounding, and the pairs x[c+k], x[c-k] summed outwards -- the last lower sample after
 * the loop and, for an odd length, the last upper one. The scale 256 / 2pi is a const float of
 * MAIN's small data (0x1D559C, used by this function alone), not a literal: loaded so, it is the
 * multiply's first operand (fScale * fPhase) and takes $f8 ahead of the 0.5's $f9, as the target
 * has them -- as a hex-float literal it was the second operand, its register the other one.
 * OPEN (registers only, 51 words): the structure is the target's -- the length's half and its
 * parity hoisted from the loop (each written inside it), the count `k = half; while (--k != 0)`
 * (as `k = half - 1` the count is hoisted too). But local-alloc gives the index's sum a0 and the
 * index follows it by copy preference (ccdump -f: the sum pseudo 117 = a0, ucIdx 15000 behind
 * the two pointers), so every inner-loop register lands one below the target's (index t1, down t2,
 * up t3, count t4, the tables t5/t6, the step t7) and the half takes t9 where the target keeps it
 * in a0 -- an int index masked, the update moved, a char step and seven declaration orders give
 * the same (build/scratch/gcc/dft/). */
void func_0024E788(const float *pfIn, int iLength, int nOut, float *pfRe, float *pfIm, float fStep)
{
	float fPhase = 0.0F;

	while (nOut != 0) {
		const float *pfUp, *pfDown;
		float fRe, fIm, fCos, fSin;
		int iStep, k;
		unsigned char ucIdx;

		fPhase += fStep;
		pfUp = pfIn + (iLength >> 1);
		pfDown = pfUp;
		k = iLength >> 1;
		fRe = *pfUp++;
		fIm = 0.0F;
		pfDown--;
		iStep = (int)(data_001D559C * fPhase + 0.5F);
		ucIdx = iStep;
		while (--k != 0) {
			fCos = data_001D5598[ucIdx];
			fSin = data_001D5594[ucIdx];
			ucIdx += iStep;
			fIm -= *pfUp * fSin;
			fRe += *pfUp * fCos;
			pfUp++;
			fIm += *pfDown * fSin;
			fRe += *pfDown * fCos;
			pfDown--;
		}
		fCos = data_001D5598[ucIdx];
		fSin = data_001D5594[ucIdx];
		fRe += *pfDown * fCos;
		fIm += *pfDown * fSin;
		if (iLength & 1) {
			fIm -= *pfUp * fSin;
			fRe += *pfUp * fCos;
		}
		nOut--;
		*pfRe++ = fRe;
		*pfIm++ = fIm;
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
