/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, the pitch decision's object after libvect.c (OVL1 0x24F010-0x24F518): the
 * frame's decision 0x24F010 (called by the frame analysis 0x248FB0) over its helpers -- the two
 * candidates' merge 0x24F468, 0x24F108, the voicing ratio 0x24F170 and 0x24F250. No assert names
 * the object's file (it links between libvect.c and the encoder API's), so the unit is named by
 * what it does, as sase_vad.c is. Written in the GCC lane's round 26a: 0x24F170, 0x24F468; round 44a: 0x24F250 (the
 * decoder's 0x254458 is the same text, at -G0). The constants are the vendor's truncated
 * conversions, as hex floats (round 12a). Placeholders: the codec's names are not in SOCOM 1.
 * Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

extern float fabsf(float);

/* the decision's state (its other fields not read here) */
typedef struct {
	void *pvSpec;			/* 0x00: handed to 0x24F108 */
	char ac04[0x1C - 0x04];
	float fPrevMax;			/* 0x1C: 0x24F170's previous maximum */
	char ac20[0x30 - 0x20];
	float fVoic;			/* 0x30 */
	float fPitch;			/* 0x34 */
} PITCH_STATE;

extern void func_0024F108(void *pvSpec, float *pfBuf, int *piLength, void *pv, float fPitch);
float func_0024F250(float *pfBuf, int iLength, float *pfV, float fPitch, float fMin);
float func_0024F468(float fP1, float fP2, float fV1, float fV2);
float func_0024F170(float fA, float fB, float fCur, float fV, float *pfPrev);

/* the frame's pitch and voicing (pfFrame[0] and [1]) from the tracked pitch and the frame's own
 * candidate (pfFrame[10] and [11]); mode 1 works at twice the pitch. The voicing ratio's
 * parameters are in the vendor's order -- the floats first, the pointer after fCur: GCC evaluates
 * a call's arguments right to left, and the target's moves come f14, then a0, then f13 and f12;
 * the EABI gives each class its registers whatever the order, so only the schedule tells
 * (the pointer first: 4 words; two of the 120 orders match, build/scratch/gcc/v_pdec2.py). */
void func_0024F010(PITCH_STATE *p, float *pfFrame, void *pv, void *pvUnused, int iMode, float fMax)
{
	float afBuf[264];
	int iLength;
	float fV, fPitch, fR;
	float *pfCand = pfFrame + 10;

	fPitch = func_0024F468(p->fPitch, pfCand[0], p->fVoic, pfCand[1]);
	if (iMode == 1)
		fPitch += fPitch;
	func_0024F108(p->pvSpec, afBuf, &iLength, pv, fPitch);
	fR = func_0024F250(afBuf, iLength, &fV, fPitch, 0x1.999998p-4F);
	pfFrame[0] = fR;
	if (iMode == 1)
		pfFrame[0] = fR * 0.5F;
	pfFrame[1] = func_0024F170(p->fVoic, pfCand[1], fMax, fV, &p->fPrevMax);
}

/* the voicing ratio: the smaller estimate over the previous maximum when weakly voiced, the larger
 * over the current one when not (each maximum floored at 0.1); close estimates averaged; clamped
 * to [0, 1], the current maximum kept for the next frame */
float func_0024F170(float fA, float fB, float fCur, float fV, float *pfPrev)
{
	float fR;

	if (*pfPrev < 0x1.999998p-4F)
		*pfPrev = 0x1.999998p-4F;
	if (fCur < 0x1.999998p-4F)
		fCur = 0x1.999998p-4F;
	if (fabsf(fB - fA) < 0x1.333332p-3F)
		fR = (fB + fA) * 0.5F;
	else if (fV < 0x1.666666p-2F) {
		if (fA < fB)
			fR = fA * fV / *pfPrev;
		else
			fR = fB * fV / fCur;
	} else {
		if (fA < fB)
			fR = fB * fV / fCur;
		else
			fR = fA * fV / *pfPrev;
	}
	if (fR < 0.0F)
		fR = 0.0F;
	if (1.0F < fR)
		fR = 1.0F;
	*pfPrev = fCur;
	return fR;
}

/* the two pitch candidates merged: within 25 % of each other averaged, else the better-voiced
 * one, and a low pitch whose voicing is weak gives way to the higher candidate (one exit) */
float func_0024F468(float fP1, float fP2, float fV1, float fV2)
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

extern float sqrtf(float);
extern float func_00248F60(const float *pfA, const float *pfB, int iLength);
extern float func_0024D498(float *pfYpeak, float fY_1, float fY0, float fY1);
extern const float data_001D5590[];		/* -MAXFLOAT, addressed whole (no $gp): another object's */

/* the pitch refined by the autocorrelation around it: the lag of the largest product in pitch +- 4 (or the fraction fMin of
 * it), interpolated when it is inside the search, and its value normalised by the two parts' energies */
float func_0024F250(float *pfBuf, int iLength, float *pfV, float fPitch, float fMin)
{
	float afY[3];
	float fMax, fDelta, fP, fDot, fE;
	int iDelta, iLo, iHi, i, iBest, j, k, n;

	afY[0] = afY[1] = afY[2] = 0.0F;
	fMax = data_001D5590[0];
	fMin *= fPitch;
	iDelta = (int)(fMin + 0.5F);
	if (iDelta > 4)
		iDelta = 4;
	fDelta = (float)iDelta;
	fMin = fPitch - fDelta;
	iLo = (int)(fMin + 0.5F);
	fMin = fPitch + fDelta;
	iHi = (int)(fMin + 0.5F);
	if (iLo < 8)
		iLo = 8;
	if (iHi > 160)
		iHi = 160;
	iBest = 0;
	j = 0;
	i = iLo;
	do {
		fDot = func_00248F60(pfBuf, pfBuf + i, iLength - i);
		i++;
		if (fMax < fDot) {
			iBest = j;
			fMax = fDot;
			afY[0] = afY[1];
		} else if (iBest == j - 1)
			afY[2] = fDot;
		afY[1] = fDot;
		j++;
	} while (i <= iHi);
	k = iBest + iLo;
	if (iLo < k && k < iHi)
		fP = (float)k + func_0024D498(pfV, afY[0], fMax, afY[2]);
	else {
		fP = (float)k;
		*pfV = fMax;
	}
	n = iLength - k;
	fE = func_00248F60(pfBuf, pfBuf, n);
	fE *= func_00248F60(pfBuf + k, pfBuf + k, n);
	*pfV /= sqrtf(fE + 0x1.0c6f7ap-20F);
	return fP;
}
