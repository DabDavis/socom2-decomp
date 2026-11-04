/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* Nellymoser's decoder, AskToEnvInline.c (OVL1 0x253510-0x2539C0): the decoder's spectral envelope
 * from the received reflection coefficients -- the envelope itself (0x253510, its asserts this
 * file's), its cepstrum 0x253760 (GCC round 28a), the step-up recursion (0x253888: libtran.c's 0x24EA68 at the
 * decoder's fixed order 12, the odd-order swap folded away, no assert) and the envelope's
 * resampling by a pitch ratio (0x253920, 65 points); and after them the decoder's bit reader
 * (0x2539C0, called by the frame unpackers 0x253DF8 and 0x254020) -- no assert names its file, so
 * it may be the next object's: it is kept here until one does. Written in the GCC lane's round 26a:
 * 0x253888, 0x253920, 0x2539C0. Placeholders: the codec's names are not in SOCOM 1. Built as
 * sase_decapi.c is (ee-gcc 2.96, -O2 -G0: the decoder reaches nothing through $gp). */

#define DEC_LPC_ORDER 12
#define ENV_POINTS 65
#define NUM_CEPS 44

/* the cepstrum's lifter: 1 / (2 n ln 2) after a leading 1 (OVL1 0x3EC1C8) */
static const float afLifter[NUM_CEPS] = {
	0x1.0p+0F, 0x1.715476p-1F, 0x1.715486p-2F, 0x1.ec7086p-3F, 0x1.715486p-3F, 0x1.27768p-3F,
	0x1.ec710cp-4F, 0x1.a617cp-4F, 0x1.7154p-4F, 0x1.484b5cp-4F, 0x1.277706p-4F, 0x1.0c9a76p-4F,
	0x1.ec7p-5F, 0x1.c68ec4p-5F, 0x1.a617cp-5F, 0x1.89f40ap-5F, 0x1.7154p-5F, 0x1.5b9a5ap-5F,
	0x1.484b5cp-5F, 0x1.370478p-5F, 0x1.2775fap-5F, 0x1.19652ap-5F, 0x1.0c9b84p-5F, 0x1.00ecfap-5F,
	0x1.ec7p-6F, 0x1.d8be72p-6F, 0x1.c68ec4p-6F, 0x1.b5bb38p-6F, 0x1.a615a8p-6F, 0x1.97891ep-6F,
	0x1.89f40ap-6F, 0x1.7d3d42p-6F, 0x1.7154p-6F, 0x1.66234ap-6F, 0x1.5b9a5ap-6F, 0x1.51ac9ap-6F,
	0x1.484944p-6F, 0x1.3f6c26p-6F, 0x1.370478p-6F, 0x1.2f09d8p-6F, 0x1.277814p-6F, 0x1.204294p-6F,
	0x1.19652ap-6F, 0x1.12dba4p-6F,
};

/* the predictor's cepstrum, 44 terms, by the LPC recursion (n c[n] form: -n a[n] less the sum of
 * a[k] c[n - k]; past the order the sum alone), then liftered (GCC round 28a). The first loop's
 * bound is a variable: its exit test is `li t2,12; slt v1,t2,t0; beqz` -- MIPS's LE of two
 * registers (operands reversed, the branch inverted), the 12 found by CSE after expansion; the
 * literal bound is `slti v1,t0,13` (2 words short, the loop head's alignment nop with them) */
void func_00253760(const float *pfA, float *pfC, float fC0)
{
	float fSum;
	int n, k, iOrder = DEC_LPC_ORDER;

	pfC[0] = fC0;
	for (n = 1; n <= iOrder; n++) {
		fSum = -(float)n * pfA[n];
		for (k = 1; k < n; k++)
			fSum -= pfA[k] * pfC[n - k];
		pfC[n] = fSum;
	}
	for (; n < NUM_CEPS; n++) {
		fSum = 0.0F;
		for (k = 1; k <= DEC_LPC_ORDER; k++)
			fSum -= pfA[k] * pfC[n - k];
		pfC[n] = fSum;
	}
	for (n = 1; n < NUM_CEPS; n++)
		pfC[n] = afLifter[n] * pfC[n];
}

/* the predictor polynomial from the reflection coefficients, by the step-up recursion */
void func_00253888(const float *pfk, float *pfA)
{
	float afTmp[DEC_LPC_ORDER + 1];
	float *pfPrev, *pfT;
	float fK;
	int i, j;

	pfA[0] = 1.0F;
	pfPrev = afTmp;
	for (i = 0; i < DEC_LPC_ORDER; i++) {
		pfT = pfA;
		pfA = pfPrev;
		pfPrev = pfT;
		fK = pfk[i];
		pfA[i + 1] = -fK;
		for (j = 1; j <= i; j++)
			pfA[j] = pfPrev[j] - fK * pfPrev[i + 1 - j];
	}
}

/* the envelope's 65 points resampled by fRatio through the scratch array, the last point held.
 * The point is stored once before the range test and again in each arm -- the vendor's redundant
 * store is the target's `swc1` in the test's delay slot and the in-range arm's reload of pfEnv[k]
 * (the store may alias it); a plain if/else, a conditional expression, a clamped index or a
 * temporary are 23-38 words off (build/scratch/gcc/v_resamp.py, GCC round 26a). */
void func_00253920(float *pfEnv, float *pfTmp, float fRatio)
{
	int i, k;

	if (fRatio == 1.0F)
		return;
	for (i = 0; i < ENV_POINTS; i++) {
		k = (int)((float)i * fRatio);
		pfTmp[i] = pfEnv[k];
		if (k < ENV_POINTS)
			pfTmp[i] = pfEnv[k];
		else
			pfTmp[i] = pfEnv[ENV_POINTS - 1];
	}
	for (i = 0; i < ENV_POINTS; i++)
		pfEnv[i] = pfTmp[i];
}

/* the decoder's bit reader: the packet ring's base and length, its byte position and the bit
 * offset in that byte (both kept by the caller) */
typedef struct {
	unsigned char *pucBuf;		/* 0x00 */
	unsigned short usLen;		/* 0x04: bytes in the ring */
	unsigned short *pusPos;		/* 0x08 */
	unsigned char *pucBit;		/* 0x0C */
} BIT_READER;

/* iBits (up to 8) bits into *pucOut, low bits first, across a byte boundary and the ring's end.
 * The reader's pointers and length are locals (every store through pucOut, a char pointer, would
 * reload them; the target re-reads only the base), and the second byte's shift is a variable of
 * its own: through iBit, iBit outranks pusPos in global-alloc and the two trade a3/t0 (15 words,
 * every declaration order alike). */
void func_002539C0(BIT_READER *p, unsigned char *pucOut, int iBits)
{
	unsigned char *pucBit = p->pucBit;
	unsigned short *pusPos = p->pusPos;
	int iMask = (1 << iBits) - 1;
	int iBit = *pucBit;
	unsigned char *puc = p->pucBuf + *pusPos;
	int iEnd = iBit + iBits;
	unsigned short usLen = p->usLen;
	int iShift;

	if (iEnd >> 3) {
		*pucOut = *puc++ >> iBit;
		iShift = 8 - iBit;
		iMask >>= iShift;
		if (++*pusPos >= usLen) {
			*pusPos = 0;
			puc = p->pucBuf;
		}
		*pucOut |= (*puc & iMask) << iShift;
		iEnd -= 8;
	} else {
		*pucOut = (*puc >> iBit) & iMask;
		if (iEnd == 8) {
			iEnd = 0;
			if (++*pusPos >= usLen)
				*pusPos = 0;
		}
	}
	*pucBit = iEnd;
}

/* the decoder's memory block (80 bytes): the synthesis frame sase_swsynth.c reads -- its buffers,
 * gains, rate, position in the period, hold flag and state -- and a level at 72 */
typedef struct {
	void *pv0;
	float *pfHist;                  /* 644 */
	float *pf8;                     /* 65 */
	float *pf12;                    /* 65 */
	float *pf16;                    /* 102 */
	float *pf20;                    /* 102 */
	int i24;
	float afGain[4];
	float fRate;
	float fPos;
	int i52;
	int i56;
	int i60;
	short sHold;
	short s66;
	int iState;
	float fLevel;
	int i76;
} DecMblk;

extern void *(*data_001D55A8)(int n, int size);  /* the host's allocator (calloc's arguments) */
extern int func_00256970(DecMblk *p);

/* the decoder's block made, its buffers allocated (1 on any failure) and its state set (GCC round
 * 28a; no assert names its file: kept beside the bit reader). The state's stores in this order, one
 * of 5,040 (tools/ccperm.py) */
int func_00253A98(DecMblk **pp)
{
	DecMblk *p;

	p = data_001D55A8(1, sizeof(DecMblk));
	*pp = p;
	if (p == 0)
		return 1;
	if ((p->pfHist = data_001D55A8(644, 4)) == 0)
		return 1;
	if (func_00256970(p) != 0)
		return 1;
	p->sHold = 1;
	p->afGain[0] = 1.0F;
	p->afGain[1] = 1.0F;
	p->afGain[2] = 1.0F;
	p->afGain[3] = 1.0F;
	p->fRate = 1.0F;
	p->fPos = 80.0F;
	p->iState = 1;
	p->i56 = 0;
	if ((p->pf8 = data_001D55A8(65, 4)) == 0)
		return 1;
	if ((p->pf12 = data_001D55A8(65, 4)) == 0)
		return 1;
	if ((p->pf16 = data_001D55A8(102, 4)) == 0)
		return 1;
	if ((p->pf20 = data_001D55A8(102, 4)) == 0)
		return 1;
	p->fLevel = 100.0F;
	return 0;
}
