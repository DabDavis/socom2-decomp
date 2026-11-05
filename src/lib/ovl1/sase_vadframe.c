/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder (SaseEncVad), the voice-activity detector's per-frame driver (OVL1
 * 0x2525C0-0x252748: an object of its own between the real FFT, sase_rfft2g.c, and the parameter
 * API, sase_parapi.c; no assert names its file). The frame's energy and pitch are recorded, the
 * history's level read, the energy taken to a dB scale (log10f, 85/3 a decade) and the detector's
 * state stepped (sase_vad.c's functions, in order); an active frame strikes its band in the
 * history and, when asked, takes the gain step, an inactive one clears the history. GCC round 33a:
 * the calls' argument order says three callees take their floats first -- 0x2531F8 (fLevel, fMax,
 * sPeak, p), 0x252FE0 (fLevel, p), 0x253288 (fThresh, sLevel, usFrames, p): GCC evaluates the
 * arguments right to left, and the EABI's per-class registers hide the order otherwise (round
 * 26a). As declared before, three calls' moves came in another order (7 words).
 * Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

typedef struct VadHist VadHist;
typedef struct VadState VadState;

typedef struct {
	int i0;
	VadState *pState;		/* 4 */
	VadHist *pHist;			/* 8 */
	float fEnergy;			/* 0xC */
	float fPitch;			/* 0x10 */
	short sGain;			/* 0x14: the gain step, when asked */
	short sLevel;			/* 0x16 */
	unsigned short usPeak;		/* 0x18: 1 when unset */
	unsigned short usFrames;	/* 0x1A: 90 when unset */
	short sActive;			/* 0x1C */
	unsigned char bGain;		/* 0x1E */
} VadFrame;

extern float log10f(float);
extern float func_00252EF0(VadHist *p);
extern void func_00253190(VadState *p, float fLevel);
extern void func_002531F8(float fLevel, float fMax, short sPeak, VadState *p);
extern void func_00252FE0(float fLevel, VadState *p);
extern signed char func_00253288(float fThresh, short sLevel, unsigned short usFrames, VadState *p);
extern signed char func_002533B0(VadState *p, short sFlag, float fLevel);
extern void func_00252D68(unsigned char ucX, VadHist *p);
extern short func_00252BF0(VadHist *p, short sLevel);
extern void func_00252E88(VadHist *p);

int func_002525C0(VadFrame *p, float fEnergy, float fPitch)
{
	float fHist, fDb;
	unsigned char ucDb;
	int bLevel;

	p->fEnergy = fEnergy;
	p->fPitch = fPitch;
	fHist = func_00252EF0(p->pHist);
	bLevel = p->sLevel >= 0;
	if (p->usPeak == 0)
		p->usPeak = 1;
	if (p->usFrames == 0)
		p->usFrames = 90;
	fDb = log10f(fEnergy + 1.0F) * 0x1.c55554p+4F;
	ucDb = fDb + 0.5F;
	func_00253190(p->pState, fEnergy);
	func_002531F8(fDb, fHist, p->usPeak, p->pState);
	func_00252FE0(fEnergy, p->pState);
	p->sActive = func_00253288(fDb, p->usPeak, p->usFrames, p->pState);
	if (func_002533B0(p->pState, bLevel, fPitch)) {
		func_00252D68(ucDb, p->pHist);
		if (p->bGain)
			p->sGain = func_00252BF0(p->pHist, p->sLevel);
	} else
		func_00252E88(p->pHist);
	return 0;
}
