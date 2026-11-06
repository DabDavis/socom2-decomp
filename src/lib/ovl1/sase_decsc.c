/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* Nellymoser's decoder, DecSC.c (OVL1 0x253C00-0x254510): a frame's parameters from its bit fields
 * (0x253C00) and the two frame unpackers that read the fields and synthesise the frame's 80 samples
 * (0x253DF8, the decoder API's; 0x254020, with a second synthesis and the samples' output optional),
 * the block's release (0x254250, over its draft), the parameter setters (0x254318-0x254458: each asserts
 * its block -- `assert(pMemBlkD != NellyNull)`, file and line by `#line` as in sase_encsc.c -- and
 * stores one gain) and the pitch candidates' merge 0x254458. The object is DecSC.c whole by its
 * .rodata (GCC round 38a): the voicing, energy and field-width tables 0x3EC2B8, 0x3EC2D8, 0x3EC3D8 end
 * four bytes before the setters' file name and expression -- 0x3EC420 "../../SaseDec/source/DecSC.c",
 * "pMemBlkD != NellyNull" -- so the tables are this file's statics, defined first. 0x253C00 makes the
 * merge's test inline (the same code as 0x254458's, which follows the setters: an inline helper's
 * out-of-line copy, deferred to the object's end). Built at -G0 as the decoder's objects are (the
 * host's callbacks, MAIN 0x1D55A8-0x1D55C0, by lui and an offset). Placeholders: the codec's names
 * are not in SOCOM 1.
 *
 * What the target showed (round 38a): a table read once and stored twice is the stored copy
 * (pPrm->fVoice = pPair->fVoice: the address in lui/addu/lwc1 %lo, not built apart); each
 * clamp-then-index its own variable; the voicing loaded before the pitch test fills the first bc1f's
 * slot unannulled and leaves the second's a nop (loaded in the else arm, both bc1fl); the PCM loop's
 * index a variable of its own (the field loop's, live across the reader's calls, took a saved
 * register for the conversion's counter); the second history copy's source computed before the first
 * memcpy (it lives across the call in s0, the first's built at the call); the reader's length stored
 * first (its andi's pseudo s0, the frame's s1); and the envelope's energy argument loaded before the
 * history (a float parameter before the history pointer in its prototype). The frame has 16 bytes
 * between the reader and the field byte the source never names (an unused local here). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

/* the decoder's memory block (sase_asktoenv.c's DecMblk, the fields this object reads named) */
typedef struct {
	void *pv0;
	float *pfHist;			/* 0x04: 644 floats */
	float *pf8;			/* 65 */
	float *pf12;			/* 65 */
	float *pf16;			/* 102 */
	float *pf20;			/* 102 */
	int i24;
	float afGain[4];		/* 0x1C: the four setters' */
	float fRate;
	float fPos;
	int i52;
	int i56;
	int i60;
	short sHold;			/* 0x40: 1 while a frame's parameters are held for the next */
	short s66;
	int iState;
	float fLevel;			/* 0x48: the held gain */
	float fVoice;			/* 0x4C: the held voicing */
} DecMblk;

typedef struct {
	float fGain;			/* 0x00 (of the pair at 0x08) */
	float fVoice;			/* 0x04 */
} DecPair;

/* a frame's decoded parameters (the unpackers' frame at sp) */
typedef struct {
	float fPitch;			/* 0x00 */
	float fVoice;			/* 0x04 */
	DecPair pair;			/* 0x08: copied to the block's level and voicing when held */
	float fEnergy;			/* 0x10 */
	float afEnv[12];		/* 0x14 */
} DecPrm;

typedef struct {
	unsigned char *pucBuf;		/* 0x00 */
	unsigned short usLen;		/* 0x04: bytes in the ring */
	unsigned short *pusPos;		/* 0x08 */
	unsigned char *pucBit;		/* 0x0C */
} BIT_READER;

extern float expf(float x);
extern void func_0024B2F8(int *piFields, float *pfOut, int n, float *pfHist, float fVoice);
extern void func_002539C0(BIT_READER *p, unsigned char *pucOut, int iBits);
extern int func_00251A28(void);
extern void (*data_001D55B0)(void *p, int c, int n);			/* the host's memset */
extern void (*data_001D55B4)(void *pDst, const void *pSrc, int n);	/* the host's memcpy */
extern void func_00253510(float *pfEnv, float fEnergy, float *pfHist, float *pfB, float *pfA, float fGain);
extern void func_00256190(DecMblk *pMemBlkD, DecPrm *pPrm, short *psPcm, int *piPos);
extern void func_00256280(DecMblk *pMemBlkD, DecPrm *pPrm, short *psPcm, int *piPos, int iX, int iY);

static const float afVoice[8] = {
	0.0F, 0x1.24dd2ep-3F, 0x1.24dd2ep-2F, 0x1.b74bc6p-2F,
	0x1.245a1cp-1F, 0x1.6d9168p-1F, 0x1.b6c8b4p-1F, 0x1.000000p+0F,
};

static const float afEnergy[64] = {
	-0x1.56d7f0p+1F, -0x1.d29d62p+0F, -0x1.62393ep+0F, -0x1.12d9cep+0F,
	-0x1.84e7eep-1F, -0x1.e06d92p-2F, -0x1.8b3e56p-3F, 0x1.8a6330p-4F,
	0x1.596bfcp-2F, 0x1.19d648p-1F, 0x1.7f531cp-1F, 0x1.ed4046p-1F,
	0x1.30156ep+0F, 0x1.693ab4p+0F, 0x1.a9d5e4p+0F, 0x1.ed2598p+0F,
	0x1.15fedap+1F, 0x1.2d077cp+1F, 0x1.422b50p+1F, 0x1.57ed9cp+1F,
	0x1.6cb274p+1F, 0x1.821c3ep+1F, 0x1.98f4c6p+1F, 0x1.b147f0p+1F,
	0x1.ca6a2ap+1F, 0x1.e2cfa6p+1F, 0x1.f8cf94p+1F, 0x1.07985ep+2F,
	0x1.12fbf4p+2F, 0x1.1dce30p+2F, 0x1.289110p+2F, 0x1.34aa2ap+2F,
	0x1.3f1da2p+2F, 0x1.4a538ep+2F, 0x1.54eb06p+2F, 0x1.605dbep+2F,
	0x1.6af112p+2F, 0x1.756dacp+2F, 0x1.804e92p+2F, 0x1.8a56f6p+2F,
	0x1.950634p+2F, 0x1.9f6fb8p+2F, 0x1.a9f1bep+2F, 0x1.b5355cp+2F,
	0x1.c06cb4p+2F, 0x1.cc0a24p+2F, 0x1.d74c54p+2F, 0x1.e33d28p+2F,
	0x1.eea4f8p+2F, 0x1.fa2156p+2F, 0x1.02c068p+3F, 0x1.08967ep+3F,
	0x1.0ea7e8p+3F, 0x1.14df4cp+3F, 0x1.1be6dcp+3F, 0x1.23528ap+3F,
	0x1.2ae138p+3F, 0x1.32d9c2p+3F, 0x1.3b718ap+3F, 0x1.44d29ap+3F,
	0x1.4e8556p+3F, 0x1.5a01b8p+3F, 0x1.674e62p+3F, 0x1.773b2ep+3F,
};

/* each field's width in bits: the energy, the voicing (and its hold flag), the gain, the twelve
 * envelope stages */
static const int aiBits[17] = {6, 4, 8, 5, 5, 5, 4, 5, 3, 5, 2, 5, 0, 5, 0, 1, 1};

/* a frame's parameters from its 17 fields (GCC round 38a): the voicing level from the second (its
 * bit 3 set, the level stands for the held one too), the gain from the third (expf of a line in it,
 * scaled by the block's second gain and clamped to [8, 160]), the pitch merged with the block's level
 * as 0x254458 merges two candidates, the first field's energy, and the twelve envelope coefficients
 * through libquan.c's dequantiser 0x24B2F8 (a tail call) */
void func_00253C00(DecMblk *pMemBlkD, int *piFields, float *pfHist, DecPrm *pPrm)
{
	DecPair *pPair = &pPrm->pair;
	float fHeld = pMemBlkD->fVoice;
	float fLevel = pMemBlkD->fLevel;
	int iFlag, iIdx, iEnergy;
	float fGain, fVoice, fPitch;

	iFlag = piFields[1] & 8;
	iIdx = piFields[1] - iFlag;
	if (iIdx >= 8)
		iIdx = 0;
	pPair->fVoice = afVoice[iIdx];
	if (!iFlag)
		pPrm->fVoice = fHeld;
	else
		pPrm->fVoice = pPair->fVoice;
	pPair->fGain = expf(0x1.44cfa4p+2F - (float)piFields[2] * 0x1.80f51cp-7F);
	pPair->fGain *= pMemBlkD->afGain[1];
	if (pPair->fGain < 8.0F)
		pPair->fGain = 8.0F;
	if (pPair->fGain > 160.0F)
		pPair->fGain = 160.0F;
	fGain = pPair->fGain;
	fVoice = pPair->fVoice;
	if (fGain <= fLevel * 1.25F && fLevel * 0x1.999998p-1F <= fGain)
		fPitch = (fLevel + fGain) * 0.5F;
	else {
		if (fVoice < fHeld) {
			fVoice = fHeld;
			fPitch = fLevel;
		} else
			fPitch = fGain;
		if (fPitch < 30.0F && fVoice * fPitch * 0.5F < 1.0F) {
			fPitch = fGain;
			if (!(fLevel < fPitch))
				fPitch = fLevel;
		}
	}
	pPrm->fPitch = fPitch;
	iEnergy = piFields[0];
	if (iEnergy >= 64)
		iEnergy = 0;
	pPrm->fEnergy = afEnergy[iEnergy];
	func_0024B2F8(piFields + 3, pPrm->afEnv, 12, pfHist, pPair->fVoice);
}

/* a frame unpacked and synthesised (the decoder API's, sase_decapi.c 0x254EC8): the transform mode
 * made sure of, the samples cleared, the 17 fields read through a reader on the caller's ring, the
 * parameters made, the history's first 80 converted to samples (rounded), the envelope rebuilt when a
 * held frame completes, the synthesis 0x256190, the sample count and energy returned; a frame held
 * keeps its pair in the block, the next one rewinds the ring to read the same bits again; the
 * history's two 65-point tails copied out */
void func_00253DF8(DecMblk *pMemBlkD, short *psPcm, unsigned short *pusN, unsigned char *pucFrame,
		   unsigned short usLen, unsigned short *pusPos, unsigned char *pucBit, float *pfB, float *pfA,
		   float *pfGain)
{
	DecPrm prm;
	int aiFields[17];
	BIT_READER r;
	int aiPad[4];
	unsigned char uc;
	int iPos;
	float *pfHist, *pf2;
	unsigned short usPos;
	unsigned char ucBit;
	int i, j;

	iPos = 0;
	func_00251A28();
	pfHist = pMemBlkD->pfHist;
	(*data_001D55B0)(psPcm, 0, 160);
	usPos = *pusPos;
	r.usLen = usLen;
	r.pucBuf = pucFrame;
	r.pusPos = pusPos;
	r.pucBit = pucBit;
	ucBit = *pucBit;
	for (i = 0; i < 17; i++) {
		func_002539C0(&r, &uc, aiBits[i]);
		aiFields[i] = uc;
	}
	func_00253C00(pMemBlkD, aiFields, pfHist, &prm);
	for (j = 0; j < 80; j++)
		psPcm[j] = pfHist[j] + 0.5F;
	if (pMemBlkD->sHold == 1)
		func_00253510(prm.afEnv, prm.fEnergy, pMemBlkD->pfHist, pfB, pfA, pMemBlkD->afGain[2]);
	func_00256190(pMemBlkD, &prm, psPcm, &iPos);
	*pusN = 80;
	*pfGain = prm.fEnergy;
	if (pMemBlkD->sHold == 0) {
		pMemBlkD->sHold = 1;
		(*data_001D55B4)(&pMemBlkD->fLevel, &prm.pair, 8);
	} else {
		pMemBlkD->sHold = 0;
		*pusPos = usPos;
		*pucBit = ucBit;
	}
	pf2 = pfHist + 579;
	(*data_001D55B4)(pMemBlkD->pf8, pfHist + 514, 260);
	(*data_001D55B4)(pMemBlkD->pf12, pf2, 260);
}

/* the other unpacker (the decoder's 0x255028 and 0x255490 call it): no mode check, the samples'
 * conversion only when asked, the second synthesis 0x256280 with two more of the caller's values */
void func_00254020(DecMblk *pMemBlkD, short *psPcm, unsigned short *pusN, unsigned char *pucFrame,
		   unsigned short usLen, unsigned short *pusPos, unsigned char *pucBit, int iX, int iY,
		   float *pfB, float *pfA, float *pfGain, int iPcm)
{
	DecPrm prm;
	int aiFields[17];
	BIT_READER r;
	int aiPad[4];
	unsigned char uc;
	int iPos;
	float *pfHist, *pf2;
	unsigned short usPos;
	unsigned char ucBit;
	int i, j;

	iPos = 0;
	pfHist = pMemBlkD->pfHist;
	(*data_001D55B0)(psPcm, 0, 160);
	usPos = *pusPos;
	r.usLen = usLen;
	r.pucBuf = pucFrame;
	r.pusPos = pusPos;
	r.pucBit = pucBit;
	ucBit = *pucBit;
	for (i = 0; i < 17; i++) {
		func_002539C0(&r, &uc, aiBits[i]);
		aiFields[i] = uc;
	}
	func_00253C00(pMemBlkD, aiFields, pfHist, &prm);
	if (iPcm)
		for (j = 0; j < 80; j++)
			psPcm[j] = pfHist[j] + 0.5F;
	if (pMemBlkD->sHold == 1)
		func_00253510(prm.afEnv, prm.fEnergy, pMemBlkD->pfHist, pfB, pfA, pMemBlkD->afGain[2]);
	func_00256280(pMemBlkD, &prm, psPcm, &iPos, iX, iY);
	*pusN = 80;
	*pfGain = prm.fEnergy;
	if (pMemBlkD->sHold == 0) {
		pMemBlkD->sHold = 1;
		(*data_001D55B4)(&pMemBlkD->fLevel, &prm.pair, 8);
	} else {
		pMemBlkD->sHold = 0;
		*pusPos = usPos;
		*pucBit = ucBit;
	}
	pf2 = pfHist + 579;
	(*data_001D55B4)(pMemBlkD->pf8, pfHist + 514, 260);
	(*data_001D55B4)(pMemBlkD->pf12, pf2, 260);
}

extern void (*data_001D55AC)(void *p);					/* the host's free */
extern void func_002569D8(DecMblk *pMemBlkD);

/* the block released (the decoder API's destroy, sase_decapi.c): the history, then the synthesis'
 * own state (0x2569D8), the four tails and the block itself -- each pointer cleared once freed, the
 * caller's left as it was */
int func_00254250(DecMblk **ppMemBlkD)
{
	DecMblk *pMemBlkD = *ppMemBlkD;

	if (pMemBlkD != NellyNull) {
		if (pMemBlkD->pfHist != NellyNull) {
			(*data_001D55AC)(pMemBlkD->pfHist);
			pMemBlkD->pfHist = NellyNull;
		}
		func_002569D8(pMemBlkD);
		if (pMemBlkD->pf8 != NellyNull) {
			(*data_001D55AC)(pMemBlkD->pf8);
			pMemBlkD->pf8 = NellyNull;
		}
		if (pMemBlkD->pf12 != NellyNull) {
			(*data_001D55AC)(pMemBlkD->pf12);
			pMemBlkD->pf12 = NellyNull;
		}
		if (pMemBlkD->pf16 != NellyNull) {
			(*data_001D55AC)(pMemBlkD->pf16);
			pMemBlkD->pf16 = NellyNull;
		}
		if (pMemBlkD->pf20 != NellyNull) {
			(*data_001D55AC)(pMemBlkD->pf20);
			pMemBlkD->pf20 = NellyNull;
		}
		(*data_001D55AC)(pMemBlkD);
	}
	return 0;
}

void func_00254318(DecMblk *pMemBlkD, float x)
{
#line 526 "../../SaseDec/source/DecSC.c"
	assert(pMemBlkD != NellyNull);
	pMemBlkD->afGain[0] = x;
}

void func_00254368(DecMblk *pMemBlkD, float x)
{
#line 540 "../../SaseDec/source/DecSC.c"
	assert(pMemBlkD != NellyNull);
	pMemBlkD->afGain[1] = x;
}

void func_002543B8(DecMblk *pMemBlkD, float x)
{
#line 554 "../../SaseDec/source/DecSC.c"
	assert(pMemBlkD != NellyNull);
	pMemBlkD->afGain[2] = x;
}

void func_00254408(DecMblk *pMemBlkD, float x)
{
#line 568 "../../SaseDec/source/DecSC.c"
	assert(pMemBlkD != NellyNull);
	pMemBlkD->afGain[3] = x;
}

/* the two pitch candidates merged (GCC round 26a; after the setters, no assert naming its file):
 * within 25 % of each other they are averaged, else the better-voiced one is taken -- and a low
 * pitch whose voicing is weak gives way to the higher candidate. 0.8 is the vendor's truncated
 * conversion (0x3F4CCCCC), a hex float. */
float func_00254458(float fP1, float fP2, float fV1, float fV2)
{
	float fP, fV;

	if (fP2 <= fP1 * 1.25F && fP1 * 0x1.999998p-1F <= fP2)
		fP = (fP1 + fP2) * 0.5F;
	else {
		if (fV2 < fV1) {
			fV = fV1;
			fP = fP1;
		} else {
			fV = fV2;
			fP = fP2;
		}
		if (fP < 30.0F && fV * fP * 0.5F < 1.0F)
			fP = (fP1 < fP2) ? fP2 : fP1;
	}
	return fP;
}

extern const float *data_003DCFFC, *data_003DCFF8;	/* the cosine and sine tables, 256 entries */
extern void func_00251AF8(float *pfIn, int n, float *pfRe, float *pfIm);
extern int func_00251C00(float *pfRe, float *pfIm, int n);
extern void func_00251B38(float *pfRe, float *pfIm, int n, float *pfOut);

/* the frame's voiced synthesis: each harmonic's amplitude and phase put at its bin as a complex spectrum (the phase by a 256-entry
 * table, 256 / 2 pi = 40.74), the rest cleared, the inverse FFT, and the first half cross-faded into the previous frame's tail */
void func_00254510(float fPitch, int iPeaks, int iLength, short *psOut, float *pfSpec, const float *pfPhase, const float *pfAmp)
{
	float afRe[260];
	float afIm[260];
	float fBin = fPitch;
	int iHalf = iLength >> 1;
	int i, j = 0;
	float fStep, fUp, fDown;

	for (i = 0; i < iPeaks; i++) {
		int k;
		float fA;

		fA = pfAmp[i] * 0.5F;
		k = (int)(pfPhase[i] * 0x1.45f306p+5F + 0.5F) & 0xFF;
		j = (int)(fBin + 0.5F);
		j *= 2;
		fBin += fPitch;
		pfSpec[j] = fA * data_003DCFFC[k];
		pfSpec[j + 1] = fA * data_003DCFF8[k];
	}
	for (i = j + 2; i < 514; i++)
		pfSpec[i] = 0.0F;
	func_00251AF8(pfSpec, 512, afRe, afIm);
	func_00251C00(afRe, afIm, 512);
	func_00251B38(afRe, afIm, 512, pfSpec);
	if (iHalf > 0)
		fStep = 1.0F / iHalf;
	else
		fStep = 0.0F;
	fUp = 0.0F;
	fDown = 1.0F;
	{
		float *pf = pfSpec + (512 - iHalf);
		int n;

		for (n = iHalf; n > 0; n--) {
			float fS = *psOut * fDown, fT = *pf++ * fUp, fR;

			fR = fS + fT;
			fR = fR + 32768.5F - 32768.0F;
			*psOut = (short)fR;
			fUp += fStep;
			fDown -= fStep;
			psOut++;
		}
	}
}

typedef struct {
	int i0;
	float *pfSpec;		/* 4 */
	int i8, i12;
	const float *pfPhase;	/* 16 */
	const float *pfAmp;	/* 20 */
} HarmSet;

extern void func_0024DA70(float *pfSpec, int iLog, int iShift);

/* the same synthesis over three sets of harmonics summed into the first's spectrum, each by its own pitch; the first set stores, the
 * others add */
void func_00254730(float fPitch1, int iPeaks1, float fPitch2, int iPeaks2, float fPitch3, int iPeaks3, int iLength, short *psOut,
		   HarmSet *p1, HarmSet *p2, HarmSet *p3)
{
	float fBin = fPitch1;
	int iHalf = iLength >> 1;
	float *pfSpec = p1->pfSpec;
	const float *pfPhase1 = p1->pfPhase;
	const float *pfAmp1 = p1->pfAmp;
	const float *pfPhase2 = p2->pfPhase;
	const float *pfAmp2 = p2->pfAmp;
	const float *pfPhase3 = p3->pfPhase;
	const float *pfAmp3 = p3->pfAmp;
	int i, j, k;
	float fA;
	float fStep, fUp, fDown;

	for (i = 0; i < iPeaks1; i++) {
		fA = pfAmp1[i] * 0.5F;
		k = (int)(pfPhase1[i] * 0x1.45f306p+5F + 0.5F);
		k &= 0xFF;
		j = (int)(fBin + 0.5F);
		fBin += fPitch1;
		j *= 2;
		pfSpec[j] = fA * data_003DCFFC[k];
		pfSpec[j + 1] = fA * data_003DCFF8[k];
	}
	fBin = fPitch2;
	for (i = 0; i < iPeaks2; i++) {
		fA = pfAmp2[i] * 0.5F;
		k = (int)(pfPhase2[i] * 0x1.45f306p+5F + 0.5F);
		k &= 0xFF;
		j = (int)(fBin + 0.5F);
		fBin += fPitch2;
		j *= 2;
		pfSpec[j] += fA * data_003DCFFC[k];
		pfSpec[j + 1] += fA * data_003DCFF8[k];
	}
	fBin = fPitch3;
	for (i = 0; i < iPeaks3; i++) {
		fA = pfAmp3[i] * 0.5F;
		k = (int)(pfPhase3[i] * 0x1.45f306p+5F + 0.5F);
		k &= 0xFF;
		j = (int)(fBin + 0.5F);
		fBin += fPitch3;
		j *= 2;
		pfSpec[j] += fA * data_003DCFFC[k];
		pfSpec[j + 1] += fA * data_003DCFF8[k];
	}
	func_0024DA70(pfSpec, 9, 8);
	if (iHalf > 0)
		fStep = 1.0F / iHalf;
	else
		fStep = 0.0F;
	fUp = 0.0F;
	fDown = 1.0F;
	{
		float *pf = pfSpec + (512 - iHalf);
		int n;

		for (n = iHalf; n > 0; n--) {
			float fS = *psOut * fDown, fT = *pf++ * fUp, fR;

			fR = fS + fT;
			fR = fR + 32768.5F - 32768.0F;
			*psOut = (short)fR;
			fUp += fStep;
			fDown -= fStep;
			psOut++;
		}
	}
}

void func_00254A88(float fPitch1, int iPeaks1, float fPitch2, int iPeaks2, float fPitch3, int iPeaks3, float fPitch4, int iPeaks4,
		   int iLength, short *psOut, HarmSet *p1, HarmSet *p2, HarmSet *p3, HarmSet *p4)
{
	float fBin = fPitch1;
	int iHalf = iLength >> 1;
	float *pfSpec = p1->pfSpec;
	const float *pfPhase1 = p1->pfPhase;
	const float *pfAmp1 = p1->pfAmp;
	const float *pfPhase2 = p2->pfPhase;
	const float *pfAmp2 = p2->pfAmp;
	const float *pfPhase3 = p3->pfPhase;
	const float *pfAmp3 = p3->pfAmp;
	const float *pfPhase4 = p4->pfPhase;
	const float *pfAmp4 = p4->pfAmp;
	int i, j, k;
	float fA;
	float fStep, fUp, fDown;

	for (i = 0; i < iPeaks1; i++) {
		fA = pfAmp1[i] * 0.5F;
		k = (int)(pfPhase1[i] * 0x1.45f306p+5F + 0.5F);
		k &= 0xFF;
		j = (int)(fBin + 0.5F);
		fBin += fPitch1;
		j *= 2;
		pfSpec[j] = fA * data_003DCFFC[k];
		pfSpec[j + 1] = fA * data_003DCFF8[k];
	}
	fBin = fPitch2;
	for (i = 0; i < iPeaks2; i++) {
		fA = pfAmp2[i] * 0.5F;
		k = (int)(pfPhase2[i] * 0x1.45f306p+5F + 0.5F);
		k &= 0xFF;
		j = (int)(fBin + 0.5F);
		fBin += fPitch2;
		j *= 2;
		pfSpec[j] += fA * data_003DCFFC[k];
		pfSpec[j + 1] += fA * data_003DCFF8[k];
	}
	fBin = fPitch3;
	for (i = 0; i < iPeaks3; i++) {
		fA = pfAmp3[i] * 0.5F;
		k = (int)(pfPhase3[i] * 0x1.45f306p+5F + 0.5F);
		k &= 0xFF;
		j = (int)(fBin + 0.5F);
		fBin += fPitch3;
		j *= 2;
		pfSpec[j] += fA * data_003DCFFC[k];
		pfSpec[j + 1] += fA * data_003DCFF8[k];
	}
	fBin = fPitch4;
	for (i = 0; i < iPeaks4; i++) {
		fA = pfAmp4[i] * 0.5F;
		k = (int)(pfPhase4[i] * 0x1.45f306p+5F + 0.5F);
		k &= 0xFF;
		j = (int)(fBin + 0.5F);
		fBin += fPitch4;
		j *= 2;
		pfSpec[j] += fA * data_003DCFFC[k];
		pfSpec[j + 1] += fA * data_003DCFF8[k];
	}
	func_0024DA70(pfSpec, 9, 8);
	if (iHalf > 0)
		fStep = 1.0F / iHalf;
	else
		fStep = 0.0F;
	fUp = 0.0F;
	fDown = 1.0F;
	{
		float *pf = pfSpec + (512 - iHalf);
		int n;

		for (n = iHalf; n > 0; n--) {
			float fS = *psOut * fDown, fT = *pf++ * fUp, fR;

			fR = fS + fT;
			fR = fR + 32768.5F - 32768.0F;
			*psOut = (short)fR;
			fUp += fStep;
			fDown -= fStep;
			psOut++;
		}
	}
}

