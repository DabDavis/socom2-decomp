/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, PtchCand.c (OVL1 0x24FFF8-0x250BE0): the pitch candidates -- the frame's
 * candidate search (0x24FFF8: a harmonic cost per lag of the pitch table, its peak and the
 * candidates around it, each refined), the peaks of the cost function walked down an octave at a
 * time (0x250320), a candidate at two and three times the last one's pitch tested for its
 * subharmonics (0x2504F0), a new lag inserted into the descending candidate list (0x250708), the
 * peak inside [0.75, 1.333] of a pitch (0x2508F0) and a lag's two half-step neighbours costed
 * (0x250A38). The asserts carry the codec's own expressions, file and line by `#line` (as in
 * sase_encsc.c); the lags index LDPDA.c's pitch table (89 entries, 0x3E7EB0).
 *
 * The object's order is not its lines': 0x2508F0 (line 314) and 0x250A38 (line 780) follow
 * 0x250708 (line 657). GCC 2.96 compiles an inlinable function only at the end of the unit
 * (rest_of_compilation defers it; wrapup_global_declarations emits the ones still needed, in
 * declaration order), and the frame search calls both before their bodies -- so they are `inline`
 * here, the rest are not. The constants are the vendor's conversions (0.72 as 0x3F3851EB: round
 * 12a's truncation), hex floats. Written in the GCC lane's round 27a: 0x2508F0, 0x2504F0 (its second-
 * and third-harmonic targets one variable assigned twice -- an expression each takes f4 and f1 for
 * the target's f2 twice), 0x250708 (the equality before the break, `iNew == piIndCand[i]`; the slot
 * kept in its own variable and the shift counting down in the scan's -- the target's `move a2,a1` --
 * and the new lag's cost loaded before the two thresholds: one order of six) and 0x250A38 (the
 * candidate in a variable, kept in f20 across the call; the lag stepped in place); 0x250320 OPEN
 * (below); the frame search 0x24FFF8 (five parameters spilled, 0x230 of frame) is not written.
 * Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

#define NUM_PITCH 89
extern const float data_003E7EB0[NUM_PITCH];    /* LDPDA.c's pitch table, lags in sixteenths */
#define fPitchTable data_003E7EB0

extern void func_0024ECF8(const float *pfX, int iLength, int *piMaxIndex, float *pfXmax);
extern float func_002485D0(const int *piPeakFreq, int iNPeaks, float fPitch, const float *pfMag,
                           int iNumBins, float fScale);

/* the cost function's largest value over the lags whose pitch lies in [0.75, 1.333] of fPitch
 * (the lag itself returned), averaged with fBackCost when it falls short of that */
inline int func_002508F0(const float *pfCostFunc, float *pfNewBackCost, float fPitch, float fBackCost)
{
	float fLo, fHi, fMax;
	int i, iMax;

#line 314 "../../SaseEncVad/source/PtchCand.c"
	assert(pfCostFunc != NellyNull);
	assert(pfNewBackCost != NellyNull);
	fLo = fPitch * 0.75F;
	fHi = fPitch * 0x1.553f7cp+0F;			/* 1.333 */
	i = 0;
	while (fPitchTable[i] < fLo)
		i++;
	fMax = pfCostFunc[i];
	iMax = i;
	do {
		if (fMax < pfCostFunc[i]) {
			fMax = pfCostFunc[i];
			iMax = i;
		}
		i++;
	} while (i < NUM_PITCH && fPitchTable[i] <= fHi);
	if (fMax < fBackCost)
		*pfNewBackCost = (fBackCost + fMax) * 0.5F;
	else
		*pfNewBackCost = fMax;
	return iMax;
}

/* up to five candidates, each the cost's peak below the last one's pitch (an octave's 0.8333 down,
 * then down the slope), while it holds 0.78 of the last; the last within 0.84 is the high pitch.
 * OPEN (0x1D8 against 0x1CC): loop.c hoists the 0.78 literal's load out of the loop into a saved
 * f21 (threshold * savings * lifetime >= insn_count: at the edge, a call in the loop), where the
 * target loads it in the loop after the call, as it does 0.84 and 0.8333 -- every register but the
 * float temporaries is the target's (the candidates indexed by the count: strength reduction's
 * `move s1,s0`; the top test unfolded, a do-while). Tried: the loop as for(;;), while (iNum < 5)
 * with and without `&& iLen >= 0`, the initialisers at the declarations, the walk as for loops and
 * breaks, the stores and the threshold tests reordered and reversed (26 variants). */
int func_00250320(const float *pfCostFunc, int *piIndCand, int *piHPIndex)
{
	float fMax, fPrev, fLim;
	int iNum, iHP, iLen, iInd;

#line 393 "../../SaseEncVad/source/PtchCand.c"
	assert(pfCostFunc != NellyNull);
	assert(piIndCand != NellyNull);
	assert(piHPIndex != NellyNull);
	fPrev = -1.0F;
	iNum = 0;
	iHP = 0;
	iLen = NUM_PITCH - 1;
	do {
		if (iNum >= 5)
			break;
		func_0024ECF8(pfCostFunc, iLen + 1, &iInd, &fMax);
		if (fMax < fPrev * 0x1.8f5c28p-1F)	/* 0.78 */
			break;
		iLen = iInd;
		piIndCand[iNum] = iLen;
		if (fPrev * 0x1.ae147ap-1F <= fMax)	/* 0.84 */
			iHP = iNum;
		fPrev = fMax;
		fLim = fPitchTable[iLen] * 0x1.aaaaacp-1F;	/* 0.8333 */
		iNum++;
		if (fLim < fPitchTable[0])
			break;
		while (fLim < fPitchTable[iLen])
			iLen--;
		while (iLen >= 0 && pfCostFunc[iLen] < pfCostFunc[iLen + 1])
			iLen--;
	} while (iLen >= 0);
	*piHPIndex = iHP;
	return iNum;
}

/* the last candidate, when its pitch is 30 or under: a dip below 0.2 of its cost on either side of
 * its second or third harmonic's lag drops it (or, alone, moves it to the peak above the dip) */
void func_002504F0(const float *pfCostFunc, int *piIndCand, int *piNumCand, int *piHPIndex)
{
	float fPitch, fThr, fMax, fHarm;
	int iNum, iHP, k, iFlag, iInd;

#line 519 "../../SaseEncVad/source/PtchCand.c"
	assert(pfCostFunc != NellyNull);
	assert(piIndCand != NellyNull);
	assert(piNumCand != NellyNull);
	assert(piHPIndex != NellyNull);
	iNum = *piNumCand;
	iHP = *piHPIndex;
	k = piIndCand[iNum - 1];
	fPitch = fPitchTable[k];
	if (fPitch <= 30.0F) {
		fThr = pfCostFunc[k] * 0x1.999998p-3F;	/* 0.2 */
		iFlag = 0;
		fHarm = fPitch * 2.0F;
		do
			k++;
		while (fPitchTable[k] < fHarm);
		if (pfCostFunc[k - 1] < fThr && pfCostFunc[k] < fThr)
			iFlag = 1;
		fHarm = fPitch * 3.0F;
		do
			k++;
		while (fPitchTable[k] < fHarm);
		if (pfCostFunc[k - 1] < fThr && pfCostFunc[k] < fThr)
			iFlag = 1;
		if (iFlag == 1) {
			if (iNum == 1) {
				func_0024ECF8(&pfCostFunc[k], NUM_PITCH - k, &iInd, &fMax);
				piIndCand[0] = iInd + k;
			} else {
				iNum--;
				if (iHP == iNum)
					iHP = 0;
			}
		}
	}
	*piHPIndex = iHP;
	*piNumCand = iNum;
}

/* iNew into the candidates (in descending lag order) unless it is there: after the last when it
 * holds 0.72 of the first's cost, or above the high pitch's when it holds 0.84 of that one's (and
 * then it is the high pitch) */
void func_00250708(const float *pfCostFunc, int iNew, int *piIndCand, int *piNumCand, int *piHPIndex)
{
	float fNew, fThr1, fThr2;
	int i, iPos, iNum, iHP, iFound;

#line 657 "../../SaseEncVad/source/PtchCand.c"
	assert(pfCostFunc != NellyNull);
	assert(piIndCand != NellyNull);
	assert(piNumCand != NellyNull);
	assert(piHPIndex != NellyNull);
	iNum = *piNumCand;
	iHP = *piHPIndex;
	iFound = 0;
	for (i = 0; i < iNum; i++) {
		if (iNew == piIndCand[i])
			iFound = 1;
		if (piIndCand[i] < iNew)
			break;
	}
	iPos = i;
	if (!iFound) {
		fNew = pfCostFunc[iNew];
		fThr1 = pfCostFunc[piIndCand[0]] * 0x1.70a3d6p-1F;	/* 0.72 */
		fThr2 = pfCostFunc[piIndCand[iHP]] * 0x1.ae147ap-1F;	/* 0.84 */
		if ((iPos == iNum && fThr1 <= fNew) || (iHP < iPos && fThr2 <= fNew)) {
			for (i = iNum; i > iPos; i--)
				piIndCand[i] = piIndCand[i - 1];
			piIndCand[iPos] = iNew;
			if (fThr2 <= fNew)
				iHP = iPos;
			iNum++;
		}
	}
	*piHPIndex = iHP;
	*piNumCand = iNum;
}

/* the lag's pitch against its two neighbours' midpoints: the one whose harmonic cost beats
 * *pfMaxCost (which it then holds) */
inline float func_00250A38(int iInd, const int *piPeakFreq, int iNPeaks, const float *pfMag, int iNumBins,
                           float *pfMaxCost, float fScale)
{
	float afPitch[2];
	float fPitch, fCand, fCost;
	int i, n;

#line 780 "../../SaseEncVad/source/PtchCand.c"
	assert(piPeakFreq != NellyNull);
	assert(pfMag != NellyNull);
	assert(pfMaxCost != NellyNull);
	fPitch = fPitchTable[iInd];
	n = 0;
	if (iInd > 0)
		afPitch[n++] = (fPitch + fPitchTable[iInd - 1]) * 0.5F;
	iInd++;
	if (iInd < NUM_PITCH)
		afPitch[n++] = (fPitch + fPitchTable[iInd]) * 0.5F;
	for (i = 0; i < n; i++) {
		fCand = afPitch[i];
		fCost = func_002485D0(piPeakFreq, iNPeaks, fCand, pfMag, iNumBins, fScale);
		if (*pfMaxCost < fCost) {
			*pfMaxCost = fCost;
			fPitch = fCand;
		}
	}
	return fPitch;
}
