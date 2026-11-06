/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, EncSC.c's parameter setters (OVL1 0x249608-0x2499A8): each asserts its
 * block (the codec's own `assert(pEncMemBlk != NellyNull)`, file and line as its __assert call
 * passes them: the `#line`s give __FILE__ and __LINE__ the text the target's strings hold) and sets
 * one encoder parameter, marking it changed (the flag's store first: the target's registers). The
 * four flag setters read the parameter block before the assert, as a declaration's initialiser. Called one each by the encoder API's setters
 * (sase_encapi.c). GCC round 18a: the frame's spectrum 0x249A38 (line 586), byte-exact as first
 * written (its 1,056-byte frame a float[264]). Placeholders: the codec's names are not in SOCOM 1.
 * Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

typedef struct {
	char pad0[28];
	float f28;
	short f32;
	short f34;
	float f36;
	short f40;
	short f42;
	short f44;
	char pad46[2];
	float f48;
	float f52;
	float f56;
} __attribute__((aligned(8))) EncParams;   /* 8-aligned: 0x248FB0 tests f40-f44 with one ld */

typedef struct {
	char pad0[68];
	int f68;
	EncParams *params;
} EncMemBlk;

/* the encoder's 88-byte block as its creation fills it (GCC round 32a: 0x249190, the block whose
 * +68 and +72 the setters below read -- EncSC.c's, by its fields; its eight closing stores in one
 * order of the few measured, tools/try.py) */
typedef struct {
	void *pCirc;            /* 0: the input ring (libsnd.c, 371 samples) */
	void *pF;               /* 4: nelly.c's float block */
	int i8;
	int pad12;
	void *pLdPda;           /* 16 */
	void *pTrack;           /* 20 */
	void *pBlk48;           /* 24 */
	int i28;
	int i32;
	int i36;
	int i40;
	float f44;              /* the long frame's energy */
	float f48;              /* the pitch pair kept (0x248FB0) */
	float f52;
	float f56;
	float f60;
	int i64;
	int f68;
	EncParams *params;
	int i76;
	char pad80[8];
} EncState;

/* the pitch's conversion (GCC round 26a): its two mode flags, and the three estimators' scales
 * (0x249C50 0x38, 0x249C60 0x30/0x34, 0x249C78 0x1C) */
typedef struct {
	char ac00[0x2A];
	unsigned short usLinear;	/* 0x2A */
	unsigned short usDirect;	/* 0x2C */
} PITCH_CONV;
typedef struct CircMblk CircMblk;

/* the frame the encoder fills: the pitch pair at 40 (0x248FB0) */
typedef struct {
	char pad0[40];
	float afPitch[2];
	char pad48[32];
	float f80;
} EncFrame;

extern int func_00251A28(void);
extern void func_0024FE38(const short *psIn, float *pfOut, int iLength, void *hPreProcMblk);
extern void func_0024C6C0(void *pvCircMblk, int iOffset, int iLength, const float *pfInBuff);
float func_002499A8(CircMblk *pvCircMblk, void *pvLdpda, float *pfEnergy);
void func_00249A38(void *pvState, CircMblk *pvCircMblk, float *pfRe, float *pfIm, int *piLength,
		   int *piWinShift, float fEnergy);
int func_002492A0(void *pvCircMblk, float *pfRe, float *pfIm, EncFrame *pFrame, void *pEnc,
		  int iLength, int iWinShift, int *piMode, float *pfMax);
extern void func_0024F010(void *p, EncFrame *pFrame, int n, short sFlag, int iMode, float fMax);
extern void func_00248C80(const EncFrame *pfIn, void *p);
extern void func_002525C0(int i, float fEnergy, float fPitch);
void func_00249B58(PITCH_CONV *p, float *pfOut, float *pfPeriod);

/* the encoder's frame (GCC round 32a): 160 samples preprocessed into the input ring, the long and
 * short frames analysed (the energy 0x2499A8 leaves in $f0 is the frame's first pitch word and the
 * short frame's unused float argument -- 0x2499A8 is a float function that falls off its end), the
 * pitch merged, the voicing tracked, and the pitch pair converted when a flag of the parameter
 * block asks (one doubleword test of the three: EncParams is 8-aligned). Byte-exact once the two
 * zeroed slots are assigned in the body (as initialisers they store in declaration order) and the
 * conversion's test is one condition with the block's flag (a `return 0` of its own sets v0 before
 * the flag's load, which then takes v1). */
extern void *(*data_001D55A8)(int n, int size);  /* the host's allocator (calloc's arguments) */
extern int func_0024C5D0(int iSize, void **pvCircMblk);
extern int func_0024FF58(void **pp, float x);
extern int func_0024A840(void **hLD_PDA_mblk);
extern int func_00248D40(void **ppTrack);
extern int func_00249EE0(void **pp);

int func_00248FB0(const short *psIn, EncState *pEnc, EncFrame *pFrame)
{
	float afIn[160];
	float afRe[516];
	float afIm[256];
	int iLength;
	int iWinShift;
	float fMax;
	int iMode;
	float fOut, fPeriod;
	float fEnergy;
	float *pfEnergy = &pEnc->f44;
	float *pfPitch = pFrame->afPitch;
	CircMblk *pCirc = pEnc->pCirc;
	EncParams *q = pEnc->params;

	iMode = 0;
	iWinShift = 0;
	func_00251A28();
	if ((pEnc->i40 & pEnc->i32) == 0)
		return 1;
	func_0024FE38(psIn, afIn, 160, pEnc->pF);
	func_0024C6C0(pCirc, 0, 160, afIn);
	fEnergy = func_002499A8(pCirc, pEnc->pLdPda, pfEnergy);
	pfPitch[0] = fEnergy;
	func_00249A38(pEnc->pBlk48, pCirc, afRe, afIm, &iLength, &iWinShift, fEnergy);
	func_002492A0(pCirc, afRe, afIm, pFrame, pEnc, iLength, iWinShift, &iMode, &fMax);
	func_0024F010(pEnc, pFrame, 225, pEnc->i40, iMode, fMax);
	func_00248C80(pFrame, pEnc->pTrack);
	pEnc->f52 = pfPitch[0];
	pEnc->i36 = pEnc->i40;
	pEnc->f48 = pfPitch[1];
	if (pEnc->i76)
		func_002525C0(pEnc->i76, *pfEnergy, pfPitch[1]);
	if (pEnc->f68 != 0 && (q->f40 || q->f42 || q->f44)) {
		fOut = pfPitch[1];
		fPeriod = pfPitch[0];
		func_00249B58((PITCH_CONV *)q, &fOut, &fPeriod);
		pfPitch[0] = fPeriod;
		pfPitch[1] = fOut;
		pFrame->f80 *= q->f36;
	}
	return 0;
}

int func_00249190(EncState **ppEncMemBlk, int iMode)
{
	EncState *p;
	EncParams *q;

	p = data_001D55A8(1, sizeof(EncState));
	*ppEncMemBlk = p;
	if (p == NellyNull)
		return 1;
	p->i64 = iMode;
	p->f68 = 1;
	q = data_001D55A8(1, 64);
	p->params = q;
	if (q == NellyNull)
		return 1;
	if (func_0024C5D0(371, &p->pCirc))
		return 1;
	if (func_0024FF58(&p->pF, 0x1.fae146p-1f))
		return 1;
	p->i8 = 0;
	if (func_0024A840(&p->pLdPda))
		return 1;
	if (func_00248D40(&p->pTrack))
		return 1;
	if (func_00249EE0(&p->pBlk48))
		return 1;
	p->i32 = 1;
	p->i28 = 0;
	p->i40 = 1;
	p->f48 = 0.0F;
	p->f52 = 100.0F;
	p->f56 = 0.0F;
	p->f60 = 100.0F;
	p->i76 = 0;
	return 0;
}

void func_00249608(EncMemBlk *pEncMemBlk, float x)
{
#line 876 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	pEncMemBlk->params->f40 = 1;
	pEncMemBlk->params->f28 = x;
}

void func_00249660(EncMemBlk *pEncMemBlk, float x)
{
#line 895 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	pEncMemBlk->params->f32 = x / 15.625f;
	pEncMemBlk->params->f40 = 1;
}

void func_002496D8(EncMemBlk *pEncMemBlk, float x)
{
#line 913 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	pEncMemBlk->params->f34 = x / 15.625f;
	pEncMemBlk->params->f40 = 1;
}

void func_00249750(EncMemBlk *pEncMemBlk, float x)
{
#line 932 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	pEncMemBlk->params->f44 = 1;
	pEncMemBlk->params->f56 = x;
}

void func_002497A8(EncMemBlk *pEncMemBlk, float x, float y)
{
#line 950 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	pEncMemBlk->params->f42 = 1;
	pEncMemBlk->params->f48 = x;
	pEncMemBlk->params->f52 = y;
}

void func_00249810(EncMemBlk *pEncMemBlk)
{
#line 960 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	pEncMemBlk->f68 = 1;
}

void func_00249858(EncMemBlk *pEncMemBlk)
{
#line 967 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	pEncMemBlk->f68 = 0;
}

void func_00249898(EncMemBlk *pEncMemBlk)
{
	EncParams *p = pEncMemBlk->params;

#line 976 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	p->f42 = 1;
}

void func_002498E0(EncMemBlk *pEncMemBlk)
{
	EncParams *p = pEncMemBlk->params;

#line 985 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	p->f42 = 0;
}

void func_00249920(EncMemBlk *pEncMemBlk)
{
	EncParams *p = pEncMemBlk->params;

#line 994 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	p->f44 = 1;
}

void func_00249968(EncMemBlk *pEncMemBlk)
{
	EncParams *p = pEncMemBlk->params;

#line 1003 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	p->f44 = 0;
}

/* the frame's spectrum (GCC round 18a): the window's length and shift, the frame read centred
 * out of the input ring, doubled, Hamming-windowed, transformed and split into its two halves */
#define ENC_DELAY_SIZE 145

extern int func_00251748(void);
extern int func_00249C88(void *pvState, CircMblk *pvCircMblk, int iDelay, int iLength, float *pfWork);
extern void func_0024C7D0(CircMblk *pvCircMblk, int iOffset, int iLength, float *pfOutBuff);
extern void func_0024EEA0(const float *pfIn, int iLength, float *pfOut, float fScale);
extern void func_0024C458(float *pfWindow, int iLength);
extern void func_002518E8(const float *pfSig, int iLength, float *pfSpec, int iLog2N);
extern void func_0024E8D0(const float *pfIn, float *pfRe, float *pfIm, int iLength);


extern float func_00249C50(PITCH_CONV *p, float *pf);
extern float func_00249C60(PITCH_CONV *p, float *pf);
extern float func_00249C78(PITCH_CONV *p, float *pf);

/* the period in samples to a pitch at 8 kHz, re-estimated by the block's mode, clamped to 50-1000
 * Hz and turned back into a period (the constants locals initialised at the top: the target loads
 * all three into f20-f22 at the entry). Each arm stores its estimate and clamps it itself -- the
 * two tails are identical, and cross-jumping merges them after reload into the one store and
 * clamp after the join the target has; but through allocation there were two, and fMin's four
 * references (as one tail: three, 652 against fFs's 1333 in global-alloc's priority, so fFs took
 * f21 -- open since round 26a, 7 words) rank it ahead of fFs: f21, the target's (GCC round 34a;
 * the clamps alone in the arms, or the store and the low clamp, stay 7 off). */
void func_00249B58(PITCH_CONV *p, float *pfOut, float *pfPeriod)
{
	float fFs = 8000.0F, fMin = 50.0F, fMax = 1000.0F;
	float fP;

	if (p->usDirect) {
		*pfOut = func_00249C50(p, pfOut);
		return;
	}
	if (p->usLinear) {
		*pfPeriod = fFs / *pfPeriod;
		*pfPeriod = func_00249C60(p, pfPeriod);
		if (*pfPeriod < fMin)
			*pfPeriod = fMin;
		if (*pfPeriod > fMax)
			*pfPeriod = fMax;
	} else {
		*pfPeriod = fFs / *pfPeriod;
		*pfPeriod = func_00249C78(p, pfPeriod);
		if (*pfPeriod < fMin)
			*pfPeriod = fMin;
		if (*pfPeriod > fMax)
			*pfPeriod = fMax;
	}
	*pfPeriod = fFs / *pfPeriod;
}

/* the long frame's analysis (GCC round 26a): 291 samples out of the input ring, windowed by the
 * 291-point table at 0x3E7C00, its mean energy kept, transformed (512 points: the 514-float
 * buffer holds the packed spectrum) and handed to the pitch analysis (LDPDA.c's 0x249FA8) */
#define ENC_LONG_FRAME 291
extern const float data_003E7C00[ENC_LONG_FRAME];
extern void func_0024C4D8(const float *pIn, const float *pWin, float *pOut, int iLength);
extern float func_00248F60(const float *pfA, const float *pfB, int iLength);
extern void func_00249FA8(float *pfSpec, void *pvLdpda);

float func_002499A8(CircMblk *pvCircMblk, void *pvLdpda, float *pfEnergy)
{
	float afFrame[512 + 2];

	func_0024C7D0(pvCircMblk, ENC_DELAY_SIZE, ENC_LONG_FRAME, afFrame);
	func_0024C4D8(afFrame, data_003E7C00, afFrame, ENC_LONG_FRAME);
	*pfEnergy = func_00248F60(afFrame, afFrame, ENC_LONG_FRAME) * 0x1.c26b52p-9F;
	func_002518E8(afFrame, ENC_LONG_FRAME, afFrame, 9);
	func_00249FA8(afFrame, pvLdpda);
}

void func_00249A38(void *pvState, CircMblk *pvCircMblk, float *pfRe, float *pfIm, int *piLength,
		   int *piWinShift, float fEnergy)
{
	float afFrame[264];
	int iLength;

	iLength = func_00251748();
	*piWinShift = func_00249C88(pvState, pvCircMblk, ENC_DELAY_SIZE, iLength, afFrame);
#line 586 "../../SaseEncVad/source/EncSC.c"
	assert((iLength>>1) <= (ENC_DELAY_SIZE+*piWinShift));
	func_0024C7D0(pvCircMblk, ENC_DELAY_SIZE + *piWinShift, iLength, afFrame);
	func_0024EEA0(afFrame, iLength, afFrame, 2.0F);
	func_0024C458(afFrame, iLength);
	func_002518E8(afFrame, iLength, pfRe, 9);
	func_0024E8D0(pfRe, pfRe, pfIm, 256);
	*piLength = iLength;
}

extern float fabsf(float);
extern void func_0024C7D0(CircMblk *pvCircMblk, int iDelay, int iLen, float *pfOut);

typedef struct {
	float afRe[4];			/* 0x00 */
	float afIm[4];			/* 0x10 */
	float fRe;			/* 0x20 */
	float fIm;			/* 0x24 */
	int iPos;			/* 0x28 */
	int iBest;			/* 0x2C */
} TransientState;

/* the window's shift from the block's transient: a sliding sum of four smoothed bins, the largest step in phase against its
 * power over the block, the position scaled to +-40 from the centre (GCC round 44a; one float, fA, is the gain and the step's
 * size: global, so f4, and the multiplies tie to the bins' locals; the two rings through plain float pointers) */
int func_00249C88(void *pvState, CircMblk *pvCircMblk, int iDelay, int iLength, float *pfWork)
{
	TransientState *p = pvState;
	float *pfRe = pvState;
	float *pfIm = p->afIm;
	int iBest = 0;
	int iPos, iStart, i, iShift;
	float fRe, fIm, fSumRe, fSumIm, fBestP, fBestM, fT;

	iPos = p->iPos;
	fSumRe = p->fRe;
	iStart = p->iBest - 150;
	fRe = pfRe[iPos];
	fIm = pfIm[iPos];
	fSumIm = p->fIm;
	func_0024C7D0(pvCircMblk, iDelay, 161, pfWork);
	fBestP = 1.0F;
	fBestM = 0x1.47ae14p-3F;
	for (i = 0; i < 160; i += 2) {
		float fX = pfWork[0];
		float fY = pfWork[1];
		float fOldRe, fOldIm, fM, fP, fA;

		fA = fX * 0x1.fcp-6F;
		pfWork += 2;
		fRe = fA * fX + fRe * 0x1.f01ffep-1F;
		fIm = fA * fY + fIm * 0x1.f01ffep-1F;
		iPos = (iPos + 1) % 4;
		fOldRe = pfRe[iPos];
		fOldIm = pfIm[iPos];
		pfRe[iPos] = fRe;
		pfIm[iPos] = fIm;
		fM = fIm * fSumRe - fRe * fSumIm;
		fP = fSumRe * fRe;
		fSumRe = fSumRe + fRe - fOldRe;
		fSumIm = fSumIm + fIm - fOldIm;
		fA = fabsf(fM);
		if (i >= iStart && fA * fBestP > fP * fBestM) {
			fBestP = fP;
			fBestM = fA;
			iBest = i;
		}
	}
	p->iPos = iPos;
	p->fRe = fSumRe;
	p->fIm = fSumIm;
	p->iBest = iBest;
	if (iBest > 80) {
		fT = (float)(160 - iBest) * (float)(iDelay - (iLength >> 1)) * 0x1.999998p-7F;
		iShift = (int)(fT + 0.5F);
		if (iShift > 40)
			iShift = 40;
	} else {
		fT = (float)(iBest * (iDelay - (iLength >> 1))) * 0x1.999998p-7F;
		iShift = (int)(fT + 0.5F);
		iShift = -iShift;
		if (iShift < -40)
			iShift = -40;
	}
	return iShift;
}

typedef struct {
	char pad00[0x20];
	unsigned short sAcc;		/* 0x20 */
	short sStart;			/* 0x22 */
	float fRatio;			/* 0x24 */
	unsigned short usFlag;		/* 0x28 */
} EncWarp;

typedef struct {
	char pad00[0x08];
	float afVoic[3];		/* 0x08 */
	void *pvTrack;			/* 0x14 */
	char pad18[0x18];
	float fPrevVoic;		/* 0x30 */
	float fPrevPitch;		/* 0x34 */
	char pad38[0x0C];
	int iWarp;			/* 0x44 */
	EncWarp *pWarp;			/* 0x48 */
} EncAnalysis;

extern void (*data_001D55B0)(void *p, int c, int n);	/* the host's memset */
extern void (*data_001D55B4)(void *pDst, const void *pSrc, int n);	/* the host's memcpy */
extern void func_0024CF10(const float *pfRe, const float *pfIm, float *pfMag, float *pfAmp, float *pfFreq, int *piNPeaks);
extern void func_0024CAE8(const float *pfEnv, const float *pfMag, const float *pfFreq, int iLength, float *pfOut, float fPitchDFT);
extern void func_00250F60(float *pfV, const float *pfMag, float *pfPitch, float *pfOut, float *pfAmp, const float *pfFreq, int iNPeaks);
extern void func_0024AB48(float fStep, float *pfSpec, int iLength);
extern void func_0024D770(float *pfLogEnv, int iLpcOrder, float *pfGain, float *pfLPC, float *pfRC);
extern void func_0024E918(int iLPCorder, const float *pfk, float *pfASK);
extern void func_0024EEA0(const float *pfIn, int iLength, float *pfOut, float fScale);
extern void func_0024C458(float *pfWindow, int iLength);
extern float func_0024F250(float *pfBuf, int iLength, float *pfV, float fPitch, float fMin);
extern float func_00251270(float fCorr, float fVoic, float fPrev, float fPitch, float fPrevPitch);
extern int func_00248AD0(void *hParamMblk, void *pTrack, const float *pfLPC, int iOrder);

/* a frame's analysis: its spectral peaks, envelope and voicing, the envelope resampled and (when the warp is on) shifted by the
 * warp's offset, its LPC and their arcsines; the window's pitch refined and the voicing tracked; the mode from the coder's
 * choice, the pitch halved in the mode that doubled it. OPEN (GCC round 45a; every saved register right, 14 words, the warp loop's
 * registers alone): the target gives the loop's output pointer a2, i a3, the walker t0, the shift t1 (here i a2, the shift a3, the
 * walker t0, the pointer t1) and i + 1 v1 (here v0). What the first draft lacked (0x2E8 against 0x2E4, 103 words of registers):
 * pfRe and pfIm are BLOCK-LOCAL pseudos -- local-alloc gives them s1 and s0 before global-alloc runs, so pfW takes s0 back (it too
 * is local to the last block) and the pitch pointer can take s1 only if its set falls after `$4 = pfRe`; the scheduler's list runs
 * two to a cycle, and one more first-block insn (`pfAmp = afAmp`, an equivalence reload deletes) pushes it past the call. The shift
 * is an expression of the biv (`short k = i + usAcc`: lhu, a giv); k's range test is `k >= 0 && k < 256` folded, the arms in the
 * order the cross-jump keeps. The float comes first in 0x24AB48's prototype (its definition keeps byte-exact). For the loop's
 * registers: a pseudo whose FIRST set has a constant REG_EQUAL (the copy of a frame address, `i = 0`) has its live length doubled
 * by update_equiv_regs, even when a later set drops the equivalence -- here i 56, the walker 60 -- so the target's order
 * (pointer > i > walker > shift) needs the pointer at 14/29: undoubled, or a preference for a2. */
int func_002492A0(void *pvCircMblk, float *pfRe, float *pfIm, EncFrame *pFrame, void *pEnc,
		  int iLength, int iWinShift, int *piMode, float *pfMax)
{
	float afAmp[100];
	float afFreq[100];
	float afEnv[256];
	float afWin[292];
	float afTmp[256];
	int iPeaks;
	float *pfEnv = afEnv;
	float *pfF = afFreq;
	float fSpacing = 512.0F;
	EncAnalysis *p = pEnc;
	EncWarp *pWarp = p->pWarp;
	float *pfPitch, *pfW;
	float *pfAmp = afAmp;
	float fFirst, fX;
	float *pfIn, *pfOut;
	short i, sStart;
	int iMode;

	data_001D55B0(pfAmp, 0, sizeof(afAmp));
	data_001D55B0(pfF, 0, sizeof(afFreq));
	func_0024CF10(pfRe, pfIm, pfEnv, pfAmp, pfF, &iPeaks);
	pfPitch = pFrame->afPitch;
	fSpacing /= pfPitch[0];
	func_0024CAE8(pfEnv, pfAmp, pfF, iPeaks, pfEnv, fSpacing);
	func_00250F60(p->afVoic, pfEnv, pfPitch, &pFrame->afPitch[1], pfAmp, pfF, iPeaks);
	func_0024AB48(fSpacing, pfEnv, 256);
	if (p->iWarp != 0 && pWarp->usFlag != 0) {
		fFirst = pfEnv[0];
		sStart = pWarp->sStart;
		for (i = 0, pfIn = pfEnv, pfOut = afTmp; i < 256; i++, pfIn++, pfOut++) {
			short k = i + pWarp->sAcc;

			if (i < sStart)
				fX = *pfIn;
			else if (k >= 0 && k < 256)
				fX = pfEnv[k];
			else if (k >= 256)
				fX = pfEnv[255];
			else
				fX = *pfIn;
			*pfOut = fX;
		}
		data_001D55B4(pfEnv, afTmp, 256);
	}
	func_0024D770(pfEnv, 12, &pFrame->f80, pfAmp, pfF);
	if (p->iWarp != 0)
		pWarp->fRatio = pfEnv[0] / fFirst;
	func_0024E918(12, pfF, (float *)&pFrame->f80 + 1);
	pfW = afWin;
	func_0024C7D0(pvCircMblk, iWinShift + 145, iLength, pfW);
	func_0024EEA0(pfW, iLength, pfW, 2.0F);
	func_0024C458(pfW, iLength);
	func_0024F250(pfW, iLength, pfMax, pfPitch[0], 0.0F);
	pfPitch[1] = func_00251270(*pfMax, pfPitch[1], p->fPrevVoic, pfPitch[0], p->fPrevPitch);
	iMode = func_00248AD0(pFrame, p->pvTrack, pfAmp, 12);
	*piMode = iMode;
	if (iMode == 1)
		pfPitch[0] *= 0.5F;
}
