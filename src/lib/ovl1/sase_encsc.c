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
	char pad36[4];
	short f40;
	short f42;
	short f44;
	char pad46[2];
	float f48;
	float f52;
	float f56;
} EncParams;

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
	int pad36;
	int i40;
	int pad44;
	int i48;
	float f52;
	float f56;
	float f60;
	int i64;
	int f68;
	EncParams *params;
	int i76;
	char pad80[8];
} EncState;

extern void *(*data_001D55A8)(int n, int size);  /* the host's allocator (calloc's arguments) */
extern int func_0024C5D0(int iSize, void **pvCircMblk);
extern int func_0024FF58(void **pp, float x);
extern int func_0024A840(void **hLD_PDA_mblk);
extern int func_00248D40(void **ppTrack);
extern int func_00249EE0(void **pp);

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
	p->i48 = 0;
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

typedef struct CircMblk CircMblk;
extern int func_00251748(void);
extern int func_00249C88(void *pvState, CircMblk *pvCircMblk, int iDelay, int iLength, float *pfWork);
extern void func_0024C7D0(CircMblk *pvCircMblk, int iOffset, int iLength, float *pfOutBuff);
extern void func_0024EEA0(const float *pfIn, int iLength, float *pfOut, float fScale);
extern void func_0024C458(float *pfWindow, int iLength);
extern void func_002518E8(const float *pfSig, int iLength, float *pfSpec, int iLog2N);
extern void func_0024E8D0(const float *pfIn, float *pfRe, float *pfIm, int iLength);

/* the pitch's conversion (GCC round 26a): its two mode flags, and the three estimators' scales
 * (0x249C50 0x38, 0x249C60 0x30/0x34, 0x249C78 0x1C) */
typedef struct {
	char ac00[0x2A];
	unsigned short usLinear;	/* 0x2A */
	unsigned short usDirect;	/* 0x2C */
} PITCH_CONV;

extern float func_00249C50(PITCH_CONV *p, float *pf);
extern float func_00249C60(PITCH_CONV *p, float *pf);
extern float func_00249C78(PITCH_CONV *p, float *pf);

/* the period in samples to a pitch at 8 kHz, re-estimated by the block's mode, clamped to 50-1000
 * Hz and turned back into a period (the constants locals initialised at the top: the target loads
 * all three into f20-f22 at the entry; the two estimators' results meet in one store after the
 * join -- a store in each arm is 29s).
 * OPEN (GCC round 26a, 7 words, registers only): fFs and fMin trade f22/f21 -- global-alloc ranks
 * fFs (four references) first here; all six declaration orders tried (7-12 words,
 * build/scratch/gcc/v_pconv.py). */
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
		fP = func_00249C60(p, pfPeriod);
	} else {
		*pfPeriod = fFs / *pfPeriod;
		fP = func_00249C78(p, pfPeriod);
	}
	*pfPeriod = fP;
	if (*pfPeriod < fMin)
		*pfPeriod = fMin;
	if (*pfPeriod > fMax)
		*pfPeriod = fMax;
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

void func_002499A8(CircMblk *pvCircMblk, void *pvLdpda, float *pfEnergy)
{
	float afFrame[512 + 2];

	func_0024C7D0(pvCircMblk, ENC_DELAY_SIZE, ENC_LONG_FRAME, afFrame);
	func_0024C4D8(afFrame, data_003E7C00, afFrame, ENC_LONG_FRAME);
	*pfEnergy = func_00248F60(afFrame, afFrame, ENC_LONG_FRAME) * 0x1.c26b52p-9F;
	func_002518E8(afFrame, ENC_LONG_FRAME, afFrame, 9);
	func_00249FA8(afFrame, pvLdpda);
}

void func_00249A38(void *pvState, CircMblk *pvCircMblk, float *pfRe, float *pfIm, int *piLength,
		   int *piWinShift)
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

