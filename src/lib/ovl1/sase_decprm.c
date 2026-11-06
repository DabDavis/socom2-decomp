/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* Nellymoser's decoder, a frame's parameters from its bit fields (OVL1 0x253C00, GCC round 38a): the
 * frame unpackers 0x253DF8 and 0x254020 read 17 fields with the bit reader (sase_asktoenv.c's
 * 0x2539C0) and hand them here with the decoder's block, its history and the parameters to fill --
 * a voicing level from the second field (its bit 3 a flag: set, the level stands for the held one
 * too), the gain from the third (expf of a line in it, scaled by the block's second gain and clamped
 * to [8, 160]), the pitch merged with the block's level (their mean within -20 %/+25 %, else the
 * level when the frame's voicing is weaker than the held one, a pitch under 30 with little voicing
 * taken whole or lifted to the level), the first field's energy, and the twelve envelope
 * coefficients through libquan.c's dequantiser 0x24B2F8 (its tail call). Its two tables are this
 * object's (0x3EC2B8: 8 voicing levels, 0x3EC2D8: 64 energies). Built as the decoder's objects
 * (-G0). Placeholders and our names: the codec is not in SOCOM 1, and nothing here asserts. */

typedef struct {
	void *pv0;
	float *pfHist;			/* 0x04 */
	float *pf8;
	float *pf12;
	float *pf16;
	float *pf20;
	int i24;
	float afGain[4];		/* 0x1C */
	float fRate;
	float fPos;
	int i52;
	int i56;
	int i60;
	short sHold;			/* 0x40 */
	short s66;
	int iState;
	float fLevel;			/* 0x48 */
	float fVoice;			/* 0x4C: the held voicing */
} DecMblk;

typedef struct {
	float fGain;			/* 0x00 (of the pair at 0x08) */
	float fVoice;			/* 0x04 */
} DecPair;

typedef struct {
	float fPitch;			/* 0x00 */
	float fVoice;			/* 0x04 */
	DecPair pair;			/* 0x08 */
	float fEnergy;			/* 0x10 */
	float afEnv[12];		/* 0x14 */
} DecPrm;

extern float expf(float x);
extern void func_0024B2F8(int *piFields, float *pfOut, int n, float *pfHist, float fVoice);

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
