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

extern double pow(double, double);

/* the gain step for a level (GCC round 33a): the level's distance from the history's (176 when the
 * level is not positive), less a dead band of 16 either side, as 10^(d / 56.67) - 1 clamped to
 * +-0.0592537 and scaled by 4000 -- on the cut side 1 - 10^(-d / 56.67). The constants are the
 * vendor's truncated conversions (round 12a). */
short func_00252BF0(VadHist *p, short sLevel)
{
	int iLevel = (sLevel > 0) ? sLevel : 176;
	float fD = (float)iLevel - p->fLevel;
	float fE, fG;

	if (fD <= 16.0F && fD >= -16.0F)
		fE = 0.0F;
	else if (fD < -16.0F)
		fE = fD + 16.0F;
	else
		fE = fD - 16.0F;
	if (fE == 0.0F)
		return 0;
	if (fE < 0.0F) {
		fG = -pow(10.0, -fE * 0x1.21212p-6F);
		if (fG < -0x1.0f2b4p+0F)
			fG = -0x1.0f2b4p+0F;
		return (short)((fG + 1.0F) * 4000.0F);
	}
	fG = pow(10.0, fE * 0x1.21212p-6F);
	if (fG > 0x1.0f2b4p+0F)
		fG = 0x1.0f2b4p+0F;
	return (short)((fG - 1.0F) * 4000.0F);
}

/* a frame's band struck (ucX >> 3): every band decayed by 0.96875, the struck one toward 1, and the
 * level made the bands' centroid, times 8; the frame count restarted (GCC round 28a).
 * OPEN (0x110 against 0x120, every word after the first loop the target's): the target loads the
 * bands into v1 and copies them into a2 on BOTH paths of the first loop's entry test (its
 * preheader and the skip path), the second loop's `i = iBand + 1` duplicated after the loop (in the
 * `b`'s slot) and on the skip path; here the load goes straight to a2 and the paths join first. Ten
 * spellings alike (build/scratch/gcc/gen_2d68.py: the bands as a local or as p->pfBand in each loop,
 * assigned before or after the first statement, an `if` around the first loop -- that one doubles
 * the test, 0x118) */
void func_00252D68(unsigned char ucX, VadHist *p)
{
	float *pf = p->pfBand;
	int iBand = ucX >> 3;
	float fSum;
	int i;

	pf[iBand] = (pf[iBand] - 1.0F) * 0.96875F + 1.0F;
	fSum = pf[iBand];
	for (i = 0; i < iBand; i++) {
		pf[i] *= 0.96875F;
		fSum += pf[i];
	}
	for (i = iBand + 1; i < 33; i++) {
		pf[i] *= 0.96875F;
		fSum += pf[i];
	}
	p->fLevel = 0.0F;
	for (i = 0; i < 33; i++)
		p->fLevel += (float)i * pf[i] / fSum;
	p->sFrames = 0;
	p->fLevel *= 8.0F;
}

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

/* the noise floor (GCC round 33a): allowed to rise by 1.125 or 0.547 % below 160000, 0.137 % below
 * 500000, 0.068 % above; a level below the floor pulls it down -- harder the lower the level, from the
 * first frames (cInit not yet positive) through a 0.1/0.9 blend -- and a level above it, while the
 * detector is not above, lifts it a quarter of the way; capped at 5059644, cInit counted up to 1.
 * The constants are the vendor's truncated conversions (round 12a); the minima and maxima are
 * conditional expressions (round 19a). Its float comes first: the driver's call says so.
 * OPEN (right size, 26 words; GCC round 33a): the floor loaded into the result (the target's f3, the
 * old value copied to f4) and the first-frames blend as one expression put those in place (with one
 * fT, 0x1B8); left: the first rise computes into the result register at once where the target
 * computes both candidates into temporaries and moves one, and the low-level pair's arms come out
 * swapped -- reorg reverses the `fLevel < 1` branch into a bc1tl holding the then-arm's add, where
 * the target's bc1f keeps a nop (build/scratch/gcc/v_2fe0.py). */
void func_00252FE0(float fLevel, VadState *p)
{
	float fNew = p->fNoise;
	float fNoise = fNew;
	float fT;

	if (fNoise < 160000.0F) {
		fNew = fNoise + 1.125F;
		if (fNew <= fNoise * 0x1.01667ap+0F)
			fNew = fNoise * 0x1.01667ap+0F;
	} else if (fNoise < 500000.0F)
		fNew = fNoise * 0x1.005996p+0F;
	else
		fNew = fNoise * 0x1.002cd2p+0F;
	if (fLevel < fNoise) {
		if (p->cInit <= 0) {
			fNew = (fNoise * 0x1.999998p-4F + (fLevel + 1.0F) * 0x1.ccccccp-1F <= fNew) ?
				fNoise * 0x1.999998p-4F + (fLevel + 1.0F) * 0x1.ccccccp-1F : fNew;
		}
		if (fLevel <= fNoise * 0x1.0624dcp-10F) {
			if (fLevel < 1.0F)
				fT = fNoise * 0x1.4cccccp-1F + (fLevel + 1.0F) * 0x1.666666p-2F;
			else
				fT = fNoise * 0x1.999998p-2F + fLevel * 0x1.333332p-1F;
		} else if (fLevel <= fNoise * 0x1.47ae14p-7F)
			fT = fNoise * 0x1.333332p-2F + fLevel * 0x1.666666p-1F;
		else
			fT = fNoise * 0x1.999998p-3F + fLevel * 0x1.999998p-1F;
		fNew = (fT <= fNew) ? fT : fNew;
	} else if (p->bAbove == 0) {
		fT = fNoise * 0.75F + fLevel * 0.25F;
		fNew = (fNew <= fT) ? fT : fNew;
	}
	if (fNew > 0x1.34d0fp+22F)
		fNew = 0x1.34d0fp+22F;
	if (p->cInit <= 0)
		p->cInit++;
	p->fNoise = fNew;
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
void func_002531F8(float fLevel, float fMax, short sPeak, VadState *p)
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

/* active by hang-over (GCC round 32a): while speech, the run restarts and the detector turns active
 * once the level (at most 255) reaches the threshold; otherwise the run counts on, and once it
 * passes the frames' hang time (rounded, through a double) the detector turns inactive, the run held
 * at 32700.
 * Byte-exact (GCC round 33a; open since round 32a at 0x11C) by two readings of the target:
 * - the flag is tested from its own signed copy (lb), read BEFORE the result's zero-extended load
 *   (lbu): read after it, CSE takes the test from the result's register (`lbu; sll 24; bnez`); the
 *   lbu then stands before the branch, so reorg moves it into the bnez's slot, and the then-arm's
 *   `ret = 1` keeps its `li v1,1` -- with the result loaded only on the else path, v1 still holds
 *   the speech test's 1 there and reload_cse deletes it (round 32a's "two words short");
 * - the turn-inactive path's two `ret = 0`s and the else's reload reach one exit by jumps: the
 *   capped run and the else jump to it, the uncapped run falls into the last `ret = 0` (laid out
 *   after the else). Structured, that `ret = 0` is laid out before the else; as returns, each
 *   return sign-extends on its own (F1-F3, build/scratch/gcc/v_3288*.py, GCC round 33a). */
signed char func_00253288(float fThresh, short sLevel, unsigned short usFrames, VadState *p)
{
	int iLevel;
	float fT;
	short sT;
	unsigned char ret;
	int iActive;

	iLevel = (sLevel < 255) ? sLevel : 255;
	if (p->bSpeech == 1) {
		p->usRun = 0;
		iActive = p->bActive;
		ret = p->bActive;
		if (iActive == 0 && (float)iLevel <= fThresh) {
			p->bActive = 1;
			ret = 1;
		}
	} else {
		fT = usFrames * p->fRate + 0.5;
		sT = (unsigned int)fT;
		p->usRun++;
		if (sT < (short)p->usRun) {
			p->bActive = 0;
			if ((short)p->usRun > 32700) {
				p->usRun = 32700;
				ret = 0;
				goto out;
			}
		} else {
			ret = p->bActive;
			goto out;
		}
		ret = 0;
	}
out:
	return ret;
}

/* active: speech, its flag and the caller's agreeing, at a level of 0.45 or more. Byte-exact in GCC round 45a: the store on the else
 * path, `if (!(x < k)) ret = c;`, falls after the compare's two double calls, so ret never crosses them (a2) and the flag alone
 * is saved; the first thirteen spellings set it first. The 0.45 is a double, an ulp below the exact decimal as 2.96's parser lands
 * it (round 11c). */
signed char func_002533B0(VadState *p, short sFlag, float fLevel)
{
	int ret = 0;
	int c;

	if (p->bSpeech == 1 && sFlag == p->bSpeech) {
		c = p->bActive;
		if (c == sFlag) {
			if (!(fLevel < 0x1.cccccccccccccp-2))
				ret = c;
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
