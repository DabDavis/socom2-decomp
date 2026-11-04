/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder (SaseEncVad), the voice-activity detector's objects (OVL1 0x252E88-0x253510,
 * after the parameter API, sase_parapi.c, and before the decoder): no assert names their file, so
 * the unit is named for what they do. Two records, each made and freed through the host's
 * callbacks (reached by $gp here, -G8): a 33-band history (0x252E88-0x252F70: the bands decayed by
 * 0.992 a frame for the first 84 frames, a level read at +4) and the detector's state (0x252FE0-
 * 0x2534D0: a tracked noise level and threshold, the 0.001-scaled rate, hang and run counters and
 * three flags). Its constants are the encoder's .lit4 entries (round 12a: hex floats where a
 * decimal need not convert as the vendor's did). Written in the GCC lane's round 19a. Placeholders:
 * the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

extern void *(*data_001D55A8)(int n, int size);  /* the host's allocator (calloc's arguments) */
extern void (*data_001D55AC)(void *);           /* the host's free callback */

typedef struct {
	float *pfBand;          /* 33 bands */
	float fLevel;
	short sFrames;
} VadHist;

typedef struct {
	float fNoise;           /* 0: the tracked noise level */
	float fThresh;          /* 4 */
	float fRate;            /* 8: the frame rate's 0.001-scaled step */
	short s12;
	short sHang;            /* 0xE */
	unsigned short usRun;   /* 0x10 */
	signed char cInit;      /* 0x12: -2 until the level settles */
	signed char bAbove;     /* 0x13 */
	signed char bSpeech;    /* 0x14 */
	signed char bActive;    /* 0x15 */
} VadState;

/* the history's first 84 frames: every band decayed */
void func_00252E88(VadHist *p)
{
	short i;

	if (p->sFrames < 85) {
		for (i = 0; i < 33; i++)
			p->pfBand[i] *= 0x1.fbe76cp-1F;		/* 0.992 */
	} else
		p->sFrames = 84;
	p->sFrames++;
}

float func_00252EF0(VadHist *p)
{
	return p->fLevel;
}

int func_00252EF8(VadHist **pp)
{
	VadHist *p;

	p = data_001D55A8(1, 12);
	*pp = p;
	if (p == 0)
		return 1;
	p->pfBand = data_001D55A8(33, 4);
	if (p->pfBand == 0)
		return 1;
	p->fLevel = 0;
	p->sFrames = 0;
	return 0;
}

int func_00252F70(VadHist **pp)
{
	VadHist *p = *pp;

	if (p == 0)
		return 1;
	if (p->pfBand != 0) {
		data_001D55AC(p->pfBand);
		p->pfBand = 0;
	}
	if (*pp != 0) {
		data_001D55AC(*pp);
		*pp = 0;
	}
	return 0;
}

/* the threshold: a quadratic in the noise level, one fit above 160000 and one below */
void func_00253190(VadState *p, float fLevel)
{
	float fNoise = p->fNoise;
	float fT;

	if (fNoise > 160000.0F)
		fT = (fNoise * -0x1.846f3ap-24F + 0x1.c49362p+1F) * fNoise + 62071.0F;
	else
		fT = (fNoise * -0x1.7417b8p-18F + 0x1.030488p+2F) * fNoise + 362.0F;
	p->fThresh = fT;
	p->bAbove = fT < fLevel;
}

/* speech once the level has stood above the larger of two floors for two frames running */
void func_002531F8(short sPeak, VadState *p, float fLevel, float fMax)
{
	float fFloor, fT, fU;

	if (p->bAbove) {
		fT = (float)sPeak - 42.5F;
		fU = fMax - 0x1.c55554p+5F;		/* 56.67 */
		fFloor = (fT <= fU) ? fU : fT;
		if (fFloor <= fLevel) {
			if (++p->sHang >= 2) {
				p->bSpeech = 1;
				p->sHang = 1;
			} else
				p->bSpeech = 0;
			return;
		}
	}
	p->sHang = 0;
	p->bSpeech = 0;
}

/* active: speech, its flag and the caller's agreeing, at a level of 0.45 or more.
 * OPEN (17 words, one short): the target holds the active flag's own lb in s0 across the two
 * double calls and selects it or 0 with movn into a2 -- the result never in a saved register; here
 * CSE follows the three equalities (the flag == sFlag == bSpeech == 1) and the result is a known 1
 * kept in s0 from the entry. Thirteen spellings (one result or several returns, an int or a char
 * result, the tests nested or one &&, the comparison inverted; build/scratch/gcc/v_vad3.c, v_vad4.c in
 * the GCC lane's round 19a) give 17-31 words; with the result a char the flag is lbu'd and re-extended
 * (22). The 0.45 is a double, an ulp below the exact decimal as 2.96's parser lands it (round 11c). */
signed char func_002533B0(VadState *p, short sFlag, float fLevel)
{
	int ret = 0;
	int c;

	if (p->bSpeech == 1 && sFlag == p->bSpeech) {
		c = p->bActive;
		if (c == sFlag) {
			ret = c;
			if (fLevel < 0x1.cccccccccccccp-2)
				ret = 0;
		}
	}
	return ret;
}

int func_00253420(VadState **pp, short sRate, float fScale)
{
	VadState *p;

	p = data_001D55A8(1, 24);
	*pp = p;
	if (p == 0)
		return 1;
	p->sHang = 0;
	p->fNoise = 5059644.0F;
	p->usRun = 0;
	p->fThresh = 15636450.0F;
	p->bAbove = 0;
	p->bSpeech = 0;
	p->bActive = 0;
	p->cInit = -2;
	p->fRate = fScale * 0x1.0624dcp-10F / (float)sRate;	/* 0.001 */
	return 0;
}

int func_002534D0(VadState **pp)
{
	if (*pp != 0) {
		data_001D55AC(*pp);
		*pp = 0;
	}
	return 0;
}
