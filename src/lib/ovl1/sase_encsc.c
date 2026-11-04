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
