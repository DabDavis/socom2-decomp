/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, libmath.c (OVL1 0x24AB48-0x24B028): the codec's scalar maths -- the
 * minimal-standard random number generator (Park and Miller's 16807, Carta's two 16-bit halves,
 * no division) and two polynomial approximations scaled decade by decade (0x24AD48, a fifth-degree
 * fit per decade of its argument, and the fourth root 0x24AF50: x scaled up by 10 until it reaches
 * 0.1, the result by 10^-0.25 each time). Asserting as the codec does (file and line by `#line`,
 * as in sase_encsc.c); the constants are hex floats (round 12a: this GCC's decimal conversion is
 * not the vendor's). Written: 0x24AF50 (GCC round 18a), 0x24AEB0 (round 19a), 0x24AD48 (round 36a:
 * its argument a float/int union in the frame, compared as floats against the decade table at
 * 0x3E81D0 as its assert's `union_x.l <= uiBoundary[0]` reads). GCC round 44a: the resampler 0x24AB48: the object whole.
 * Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

/* the decades of 0x24AD48's argument (1 down to 1e-6; a zero after them), the scale that brings
 * each into [0.1, 1], and each decade's fifth-degree fit (0x3E81D0, 0x3E81F0, 0x3E8208) */
static const float uiBoundary[8] = {
	1.0F, 0x1.999998p-4F, 0x1.47ae14p-7F, 0x1.0624dcp-10F,
	0x1.a36e2ep-14F, 0x1.4f8b58p-17F, 0x1.0c6f7ap-20F, 0.0F,
};
static const float afDecadeScale[6] = { 1.0F, 10.0F, 100.0F, 1000.0F, 10000.0F, 100000.0F };
static const float afDecadePoly[6][6] = {
	{ 0x1.8113dap-1F, -0x1.578764p+1F, 0x1.e63836p+1F, -0x1.63dd78p+1F, 0x1.4374e2p+0F, 0x1.4c7b08p-1F },
	{ 0x1.20c476p-1F, -0x1.019c3ep+1F, 0x1.6c9cfep+1F, -0x1.0adc76p+1F, 0x1.e51dcep-1F, 0x1.f2a68cp-2F },
	{ 0x1.b1170ep-2F, -0x1.825c68p+0F, 0x1.116bdep+1F, -0x1.903c3ep+0F, 0x1.6bc936p-1F, 0x1.75ef66p-2F },
	{ 0x1.44c58ep-2F, -0x1.21bad8p+0F, 0x1.9a12fep+0F, -0x1.2c2258p+0F, 0x1.10cd0ep-1F, 0x1.18696cp-2F },
	{ 0x1.e716c0p-3F, -0x1.b28892p-1F, 0x1.338324p+0F, -0x1.c22342p-1F, 0x1.9924d0p-2F, 0x1.a48ef2p-3F },
	{ 0x1.6d43dep-3F, -0x1.45daaap-1F, 0x1.cd340ep-1F, -0x1.518e40p-1F, 0x1.32d088p-2F, 0x1.3b5fd2p-3F },
};

/* a fit of x in [1e-6, 1] decade by decade (GCC round 36a, unwritten before): x clamped at 1e-6,
 * its decade found by a binary search written out (above 1e-3, then 1e-2 and 1e-1; else at most
 * 1e-4 and 1e-5), scaled into [0.1, 1] and the decade's polynomial evaluated by Horner's rule over
 * its six coefficients. The argument lives in a float/int union, which GCC keeps in the frame (a
 * float/long union -- round 26a's kind -- sits in a register, its halves masked and ORed: 0x1C8
 * bytes against 0x164; `float a[1]` beside the float matches too); the assert's text names the
 * float `union_x.l`, compared as a float against the table (c.le.s), so `l` is the float here. */
float func_0024AD48(float fXval)
{
	union {
		float l;
		int i;
	} union_x;
	int iDecade;
	const float *pfC;
	float fY;

	union_x.l = fXval;
	if (fXval <= uiBoundary[6])
		union_x.l = uiBoundary[6];
#line 730 "../../SaseEncVad/source/libmath.c"
	assert(union_x.l <= uiBoundary[0]);
	if (union_x.l > uiBoundary[3]) {
		if (union_x.l > uiBoundary[2]) {
			if (union_x.l > uiBoundary[1])
				iDecade = 0;
			else
				iDecade = 1;
		} else
			iDecade = 2;
	} else if (union_x.l <= uiBoundary[4]) {
		if (union_x.l <= uiBoundary[5])
			iDecade = 5;
		else
			iDecade = 4;
	} else
		iDecade = 3;
	union_x.l *= afDecadeScale[iDecade];
	pfC = afDecadePoly[iDecade];
	fY = *pfC++ * union_x.l;
	fY += *pfC++;
	fY *= union_x.l;
	fY += *pfC++;
	fY *= union_x.l;
	fY += *pfC++;
	fY *= union_x.l;
	fY += *pfC++;
	fY *= union_x.l;
	fY += *pfC++;
	return fY;
}

/* the minimal-standard generator, Carta's form: the seed's halves multiplied apart, the high
 * product's low fifteen bits folded into the low product, the carries out of bit 31 added back;
 * the halves are read at the declarations (the assert's call would reload *plSeed otherwise).
 * The high half is the accumulator, updated in place -- `hi >>= 15; hi += lo >> 31; lo = (lo << 1)
 * >> 1; hi += lo;`: one variable through the chain gives the target's v0 from the product to the
 * store, the low sum its v1 (open since round 18a, 12 words, on the sum written as one expression
 * into `lo`: then the low half held the chain and took v0) -- GCC round 19a, measured over 101
 * spellings (build/scratch/gcc/v_rng*.c): this and a temporary `t` for the accumulator are the two
 * that match; the decoder's copy 0x256380 (sase_swsynth.c) is the same text. */
void func_0024AEB0(int *plSeed)
{
	unsigned int lo, hi;
	unsigned int ulLo = *plSeed & 0xFFFF;
	int lHi = *plSeed >> 16;

#line 78 "../../SaseEncVad/source/libmath.c"
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

/* the fourth root of x in [0, 1]: a quadratic fit over [0.1, 1], the argument scaled into it */
float func_0024AF50(float fXval)
{
	float fScale, fY;

#line 636 "../../SaseEncVad/source/libmath.c"
	assert(fXval >= 0.0F);
	assert(fXval <= 1.0F);
	fXval += 0x1.ad7f28p-24F;			/* 1e-7 */
	fScale = 1.0F;
	while (fXval < 0x1.999998p-4F) {		/* 0.1, one ulp short */
		fXval *= 10.0F;
		fScale *= 0x1.1feb32p-1F;		/* 10^-0.25 */
	}
	fY = fXval * -0x1.ebae14p-2F + 0x1.00065ep+0F;
	fY = fXval * fY + 0x1.dba3aap-2F;
	return fY * fScale;
}

extern float func_002517D8(float fX);		/* the encoder's fast log2 (shared) */

/* the spectrum resampled: its log magnitude taken every fStep bins, at most 80 points and the last, and each stretch between
 * two points interpolated linearly back into pfSpec. iLength - 1 written out twice: GCSE (one pass) reuses the integer, spilled
 * at 1312(sp) with every saved register taken, and not its conversion to float, reloaded by lwc1 (one variable: the float
 * kept in $f23, the frame 16 smaller; GCC round 44a). */
void func_0024AB48(float *pfSpec, int iLength, float fStep)
{
	float afBuf[328];
	float *pfLog, *pfFreq;
	int *piIdx;
	int n, i, j, k;
	float fFreq;

	n = (int)((float)(iLength - 1) / fStep);
	n++;
	if (n > 80)
		n = 80;
	fFreq = 0.0F;
	pfLog = afBuf;
	pfFreq = &afBuf[n + 4];
	piIdx = (int *)&afBuf[2 * n + 8];
	for (i = 0; i < n; i++) {
		k = (int)(fFreq + 0.5F);
		pfLog[i] = func_002517D8(pfSpec[k]);
		piIdx[i] = k;
		pfFreq[i] = fFreq;
		fFreq += fStep;
	}
	pfLog[n] = func_002517D8(pfSpec[iLength - 1]);
	piIdx[n] = iLength;
	pfFreq[n] = (float)(iLength - 1);
	for (i = 1; i < n + 1; i++) {
		for (j = piIdx[i - 1]; j < piIdx[i]; j++)
			pfSpec[j] = pfLog[i - 1] + (pfLog[i] - pfLog[i - 1]) * ((float)j - pfFreq[i - 1]) / (pfFreq[i] - pfFreq[i - 1]);
	}
}
