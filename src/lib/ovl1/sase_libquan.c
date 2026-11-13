/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, libquan.c (OVL1 0x24B028-0x24C1F0): the scalar quantiser's nearest
 * codeword (0x24B720) and the 2-D squared distance (0x24C000), asserting as the codec does
 * (file and line by `#line`, as in sase_encsc.c). The set's quantiser 0x24B028 (OPEN) and its decoder 0x24B2F8 (GCC round 44a).
 * Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

float func_0024C000(const float *pfIn, const float *pfRef);

/* the iMBest nearest 2-D codewords, best first (GCC round 34a): an insertion into the candidate
 * list, its distances starting at the largest float the vendor wrote (0x7F7FFFED: a literal over
 * 0x1p+127 GCC puts in .sdata and loads by lui, 8 bytes apart per object as the image holds them);
 * the list cleared index first (the other order swaps the loop's two stores) */
#define MAX_NUMCAND 2

void func_0024B560(const float *pfData, const float *pfCB, int iCBSize, int *piMBestCand, int iMBest)
{
	float afDist[MAX_NUMCAND];
	float fDist;
	int i, j;

#line 709 "../../SaseEncVad/source/libquan.c"
	assert(iMBest <= MAX_NUMCAND);
	assert((pfData != NellyNull) && (pfCB != NellyNull));
	assert(piMBestCand != NellyNull);
	for (i = 0; i < iMBest; i++) {
		piMBestCand[i] = 0;
		afDist[i] = 0x1.ffffdap+127F;
	}
	for (i = 0; i < iCBSize; i++) {
		fDist = func_0024C000(pfData, pfCB);
		pfCB += 2;
		if (fDist < afDist[iMBest - 1]) {
			for (j = iMBest - 1; j > 0 && fDist <= afDist[j - 1]; j--) {
				afDist[j] = afDist[j - 1];
				piMBestCand[j] = piMBestCand[j - 1];
			}
			afDist[j] = fDist;
			piMBestCand[j] = i;
		}
	}
}

int func_0024B720(const float *pfCodeBook, int iLength, float fValue)
{
	float fDist, fMin;
	int i, iBest;

#line 938 "../../SaseEncVad/source/libquan.c"
	assert(pfCodeBook != NellyNull);
	assert(iLength > 0);
	iBest = 0;
	fMin = fValue - pfCodeBook[0];
	fMin *= fMin;
	for (i = 1; i < iLength; i++) {
		fDist = fValue - pfCodeBook[i];
		fDist *= fDist;
		if (fDist < fMin) {
			fMin = fDist;
			iBest = i;
		}
	}
	return iBest;
}

/* the squared distance of two 2-D points (GCC round 77; open since round 34a): one difference
 * variable reused for both axes -- its second assignment must wait for the first square to read it,
 * so sched1 takes the [0] pair's loads and square first, as the target does. With two difference
 * variables both schedulers start with the [1] pair whatever the source order (the last use of
 * each pointer has the smaller register weight). */
float func_0024C000(const float *pfIn, const float *pfRef)
{
	float fDiff, fDist;

#line 892 "../../SaseEncVad/source/libquan.c"
	assert((pfIn != NellyNull) && (pfRef != NellyNull));
	fDiff = pfIn[0] - pfRef[0];
	fDist = fDiff * fDiff;
	fDiff = pfIn[1] - pfRef[1];
	fDist += fDiff * fDiff;
	return fDist;
}

/* the scalar codebook's entry, an index past the end read as entry 0 */
float func_0024B7E8(const float *pfCodeBook, int iLength, int iIndex)
{
#line 978 "../../SaseEncVad/source/libquan.c"
	assert(pfCodeBook != NellyNull);
	assert(iLength>0);
	assert(iIndex<iLength);
	assert(iIndex>=0);
	if (iIndex >= iLength)
		iIndex = 0;
	return pfCodeBook[iIndex];
}

/* the nearest 2-D codeword */
int func_0024B8B0(const float *pfVector, const float *pfCodeBook, int iEntries)
{
	float fD0, fD1, fDist, fMin;
	int i, iBest;

#line 1070 "../../SaseEncVad/source/libquan.c"
	assert(pfVector != NellyNull);
	assert(pfCodeBook != NellyNull);
	iBest = 0;
	fMin = 1e38f;
	for (i = 0; i < iEntries; i++) {
		fD0 = *pfCodeBook++ - pfVector[0];
		fDist = fD0 * fD0;
		fD0 = *pfCodeBook++ - pfVector[1];
		fDist += fD0 * fD0;
		if (fDist < fMin) {
			fMin = fDist;
			iBest = i;
		}
	}
	return iBest;
}

/* the 2-D codeword at an index, one past the table read as entry 0 */
void func_0024B990(int iIndex, const float *pfCodeBook, int iEntries, float *pfVector)
{
#line 1118 "../../SaseEncVad/source/libquan.c"
	assert(pfCodeBook != NellyNull);
	assert(pfVector != NellyNull);
	assert(iIndex < (unsigned int) iEntries);
	if (iIndex >= (unsigned int) iEntries)
		iIndex = 0;
	pfCodeBook += 2 * iIndex;
	pfVector[0] = pfCodeBook[0];
	pfVector[1] = pfCodeBook[1];
}

void func_0024C078(const float *pfIn, const float *pfRSMatrix, int iIndex, float *pfOut);
int func_0024C150(const int *piIndexOfOutCell, int iNumOutCell, int iIndex);
void func_0024BF50(const float *pfIn, const float *pfRSMatrix, int iIndex, float *pfOut);

/* the 2-D vector's two-stage search (GCC round 34a): for each of the iNumCand nearest stage-1
 * codewords, the residual rotated and scaled by the entry's factor, quantised in the stage-2 book
 * its cell takes (func_0024BCF0's choice), rebuilt, and the pair whose rebuilt point lies nearest
 * the input kept -- the input replaced by it (the start distance set before the stage-1 search:
 * after it, the literal's load waits for the loop's entry test, 8 bytes over) */
void func_0024BA48(int *piIndices, float *pfData, int iNumCand, int iStage1Size, const float *pfStage1CB,
		   const float *pfRotScaleCB, int iStage2Size, const float *pfStage2InCellCB,
		   const float *pfStage2OutCellCB, int iNumOutCell, const int *piIndexOfStage2OutCell)
{
	int aiCand[MAX_NUMCAND];
	float afQ1[2];
	float afRes[2];
	float afRot[2];
	float afBest[2];
	const float *pfCB;
	float fDist, fMin;
	int i, i1, i2;

#line 472 "../../SaseEncVad/source/libquan.c"
	assert(iNumCand <= MAX_NUMCAND);
	assert((piIndices != NellyNull) && (pfData != NellyNull));
	assert((pfStage1CB != NellyNull) && (pfRotScaleCB != NellyNull));
	assert((pfStage2InCellCB != NellyNull) && (pfStage2OutCellCB != NellyNull));
	assert(piIndexOfStage2OutCell != NellyNull);
	fMin = 0x1.ffffdap+127F;
	func_0024B560(pfData, pfStage1CB, iStage1Size, aiCand, iNumCand);
	for (i = 0; i < iNumCand; i++) {
		i1 = aiCand[i];
		func_0024B990(i1, pfStage1CB, iStage1Size, afQ1);
		afRes[0] = pfData[0] - afQ1[0];
		afRes[1] = pfData[1] - afQ1[1];
		func_0024BF50(afRes, pfRotScaleCB, i1, afRot);
		if (func_0024C150(piIndexOfStage2OutCell, iNumOutCell, i1) == 1)
			pfCB = pfStage2OutCellCB;
		else
			pfCB = pfStage2InCellCB;
		i2 = func_0024B8B0(afRot, pfCB, iStage2Size);
		func_0024B990(i2, pfCB, iStage2Size, afRot);
		func_0024C078(afRot, pfRotScaleCB, i1, afRes);
		afQ1[0] += afRes[0];
		afQ1[1] += afRes[1];
		fDist = func_0024C000(pfData, afQ1);
		if (fDist < fMin) {
			fMin = fDist;
			piIndices[0] = i1;
			piIndices[1] = i2;
			afBest[0] = afQ1[0];
			afBest[1] = afQ1[1];
		}
	}
	pfData[0] = afBest[0];
	pfData[1] = afBest[1];
}


/* the 2-D vector's two-stage decode (GCC round 34a): the stage-1 codeword, plus the stage-2 one --
 * from the outer-cell book when the stage-1 index is one of the outer cells', else the inner --
 * rotated and scaled back by the stage-1 entry's factor */
void func_0024BCF0(const int *piIndices, float *pfData, int iStage1Size, const float *pfStage1CB,
		   const float *pfRotScaleCB, int iStage2Size, const float *pfStage2InCellCB,
		   const float *pfStage2OutCellCB, int iNumOutCell, const int *piIndexOfStage2OutCell)
{
	float afStage2[2];
	float afRes[2];
	const float *pfCB;

#line 632 "../../SaseEncVad/source/libquan.c"
	assert((piIndices != NellyNull) && (pfData != NellyNull));
	assert((pfStage1CB != NellyNull) && (pfRotScaleCB != NellyNull));
	assert((pfStage2InCellCB != NellyNull) && (pfStage2OutCellCB != NellyNull));
	assert(piIndexOfStage2OutCell != NellyNull);
	func_0024B990(piIndices[0], pfStage1CB, iStage1Size, pfData);
	if (func_0024C150(piIndexOfStage2OutCell, iNumOutCell, piIndices[0]) == 1)
		pfCB = pfStage2OutCellCB;
	else
		pfCB = pfStage2InCellCB;
	func_0024B990(piIndices[1], pfCB, iStage2Size, afStage2);
	func_0024C078(afStage2, pfRotScaleCB, piIndices[0], afRes);
	pfData[0] += afRes[0];
	pfData[1] += afRes[1];
}

/* the prediction: each row of the matrix against the zero-mean coefficients */
void func_0024BE70(const float *pfZMCoef, int iOrder, const float *pfMatrix, float *pfPredCoef,
		   int iRows)
{
	const float *pfRow;
	float fSum;
	int i, j;

#line 362 "../../SaseEncVad/source/libquan.c"
	assert(pfZMCoef != NellyNull && pfPredCoef != NellyNull);
	for (j = 0; j < iRows; j++) {
		pfRow = &pfMatrix[j * iOrder];
		fSum = 0.0F;
		for (i = 0; i < iOrder; i++)
			fSum += pfRow[i] * pfZMCoef[i];
		pfPredCoef[j] = fSum;
	}
}

/* the rotate-and-scale cell: a 2-D point times the entry's complex factor */
void func_0024BF50(const float *pfIn, const float *pfRSMatrix, int iIndex, float *pfOut)
{
	const float *pfRS = pfRSMatrix + 2 * iIndex;

#line 784 "../../SaseEncVad/source/libquan.c"
	assert((pfIn != NellyNull) && (pfRSMatrix != NellyNull) && (pfOut != NellyNull));
	pfOut[0] = pfRS[0] * pfIn[0] + pfRS[1] * pfIn[1];
	pfOut[1] = -pfRS[1] * pfIn[0] + pfRS[0] * pfIn[1];
}

/* and its inverse: divided by the factor's squared norm */
void func_0024C078(const float *pfIn, const float *pfRSMatrix, int iIndex, float *pfOut)
{
	const float *pfRS = pfRSMatrix + 2 * iIndex;
	float fNorm;

#line 818 "../../SaseEncVad/source/libquan.c"
	assert((pfIn != NellyNull) && (pfRSMatrix != NellyNull) && (pfOut != NellyNull));
	fNorm = pfRS[0] * pfRS[0] + pfRS[1] * pfRS[1];
	fNorm = 1.0F / fNorm;
	pfOut[0] = fNorm * (pfIn[0] * pfRS[0] - pfIn[1] * pfRS[1]);
	pfOut[1] = fNorm * (pfIn[1] * pfRS[0] + pfIn[0] * pfRS[1]);
}

/* is the index one of the outer cells' */
int func_0024C150(const int *piIndexOfOutCell, int iNumOutCell, int iIndex)
{
	int i, bFound;

	bFound = 0;
#line 855 "../../SaseEncVad/source/libquan.c"
	assert(piIndexOfOutCell != NellyNull);
	for (i = 0; i < iNumOutCell; i++) {
		if (piIndexOfOutCell[i] == iIndex) {
			bFound = 1;
			break;
		}
	}
	return bFound;
}

extern void func_0024BE70(const float *pfIn, int iOff, const float *pfPred, float *pfOut, int iLen);

typedef struct {
	int iStage1Size;
	const float *pfStage1CB;
	const float *pfRotScaleCB;
	int iStage2Size;
	const float *pfStage2InCellCB;
	const float *pfStage2OutCellCB;
	int iNumOutCell;
	const int *piIndexOfStage2OutCell;
} QuanVQ;

typedef struct {
	int iEntries;
	const float *pfCodeBook;
} QuanCB;

extern const QuanVQ data_003E8928, data_003E8D78, data_003E90D0, data_003E93A8, data_003E9638;
extern const float data_003E82B8[], data_003E82E8[];
extern const float *const data_003DCFC8[], *const data_003DCFE0[];
extern const QuanCB *const data_001D5558[2];

#define SUB_VECTORS 6
#define SUB_DIMENSION 2

/* the coefficient set's vector quantiser: the set less its mean (one of two, by the midpoint), then each of six 2-D
 * subvectors less its prediction quantised, the first four by the two-stage search, the last two by the codebook's
 * nearest word, and the quantised value put back. The last two's codebook pointers are an 8-byte array indexed [i - 4]: its
 * giv's start is one `la` (no temporary), its size known. OPEN (GCC round 44a, 3 words): the walk's `move s1,v1` follows the
 * `la` in the target; here it precedes it, whatever the order of the four initialisers. */
void func_0024B028(float *pfCoef, int *piIndices, int iOrder, float fMid)
{
	float afX[SUB_VECTORS * SUB_DIMENSION];
	float afPred[SUB_DIMENSION];
	const QuanVQ *apVQ[4];
	const float *const *ppPred;
	const float *pfMean;
	const QuanVQ *pVQ;
	float *pfX;
	int i, iIndex;
	int iOff;
	int *piInd;

#line 100 "../../SaseEncVad/source/libquan.c"
	assert(iOrder == (SUB_VECTORS*SUB_DIMENSION));
#line 102 "../../SaseEncVad/source/libquan.c"
	assert((pfCoef != NellyNull) && (piIndices != NellyNull));
	if (0x1.999998p-1F < fMid) {
		apVQ[0] = &data_003E8928;
		pfMean = data_003E82B8;
		ppPred = data_003DCFC8;
	} else {
		apVQ[0] = &data_003E8D78;
		pfMean = data_003E82E8;
		ppPred = data_003DCFE0;
	}
	apVQ[1] = &data_003E90D0;
	apVQ[2] = &data_003E93A8;
	apVQ[3] = &data_003E9638;
	for (i = 0; i < iOrder; i++)
		afX[i] = pfCoef[i] - pfMean[i];
	piInd = piIndices;
	iOff = 0;
	for (i = 0, pfX = afX; i < SUB_VECTORS; i++) {
		func_0024BE70(afX, iOff, ppPred[i], afPred, SUB_DIMENSION);
		iOff += 2;
		pfX[0] -= afPred[0];
		pfX[1] -= afPred[1];
		if (i < 4) {
			pVQ = apVQ[i];
			func_0024BA48(piInd, pfX, SUB_DIMENSION, pVQ->iStage1Size, pVQ->pfStage1CB, pVQ->pfRotScaleCB, pVQ->iStage2Size,
				      pVQ->pfStage2InCellCB, pVQ->pfStage2OutCellCB, pVQ->iNumOutCell, pVQ->piIndexOfStage2OutCell);
		} else {
			iIndex = func_0024B8B0(pfX, data_001D5558[i - 4]->pfCodeBook, data_001D5558[i - 4]->iEntries);
			piInd[1] = 0;
			piInd[0] = iIndex;
			func_0024B990(iIndex, data_001D5558[i - 4]->pfCodeBook, data_001D5558[i - 4]->iEntries, pfX);
		}
		piInd += 2;
		*pfX++ += afPred[0];
		*pfX++ += afPred[1];
	}
	for (i = 0; i < iOrder; i++)
		pfCoef[i] = afX[i] + pfMean[i];
}

/* the set's decoder, the quantiser's inverse: each subvector rebuilt from its indices, its prediction added, the mean added
 * back. OPEN (GCC round 45a, 8 bytes over, every saved register right since the last loop reads afX directly -- its hoisted
 * base `move a1,s7` is post-loop CSE's, one ref less on pfBase): the giv of i * 4 serves apVQ[i] and the table, whose address is
 * `addu v0,sp,s3; lw v1,80(v0)` in the target. Here sp + 80 is a pseudo of its own (`addiu v1,sp,80; addu v0,v1,s3`) because
 * loop.c's second pass hoists it: threshold * savings * life >= the loop's real insns, with life 1 and savings 1, moves it at 63
 * insns and not at 71 or 72 (an added statement proved it: at 71 the target's form comes back, combine then folding the add
 * into the address). The target's loop has more real insns at that pass than this text: index variables, `&afX[2 * i]`
 * and the prediction add's spellings leave the count at 63. */
void func_0024B2F8(const int *piIndices, float *pfCoef, int iOrder, float fMid)
{
	float afX[SUB_VECTORS * SUB_DIMENSION];
	float afPred[SUB_DIMENSION];
	const QuanVQ *apVQ[4];
	const float *const *ppPred;
	const float *pfMean;
	const QuanVQ *pVQ;
	const QuanCB *pCB;
	float *pfX, *pfBase;
	int i, iOff;

#line 245 "../../SaseEncVad/source/libquan.c"
	assert(iOrder == (SUB_VECTORS*SUB_DIMENSION));
#line 247 "../../SaseEncVad/source/libquan.c"
	assert((piIndices != NellyNull) && (pfCoef != NellyNull));
	if (0x1.999998p-1F < fMid) {
		apVQ[0] = &data_003E8928;
		pfMean = data_003E82B8;
		ppPred = data_003DCFC8;
	} else {
		apVQ[0] = &data_003E8D78;
		pfMean = data_003E82E8;
		ppPred = data_003DCFE0;
	}
	apVQ[1] = &data_003E90D0;
	apVQ[2] = &data_003E93A8;
	apVQ[3] = &data_003E9638;
	pfBase = afX;
	iOff = 0;
	for (i = 0, pfX = pfBase; i < SUB_VECTORS; i++) {
		if (i < 4) {
			pVQ = apVQ[i];
			func_0024BCF0(&piIndices[2 * i], &pfBase[2 * i], pVQ->iStage1Size, pVQ->pfStage1CB, pVQ->pfRotScaleCB, pVQ->iStage2Size,
				      pVQ->pfStage2InCellCB, pVQ->pfStage2OutCellCB, pVQ->iNumOutCell, pVQ->piIndexOfStage2OutCell);
		} else {
			pCB = data_001D5558[i - 4];
			func_0024B990(piIndices[2 * i], pCB->pfCodeBook, pCB->iEntries, &pfBase[2 * i]);
		}
		func_0024BE70(afX, iOff, ppPred[i], afPred, SUB_DIMENSION);
		iOff += 2;
		*pfX++ += afPred[0];
		*pfX++ += afPred[1];
	}
	for (i = 0; i < iOrder; i++)
		pfCoef[i] = afX[i] + pfMean[i];
}
