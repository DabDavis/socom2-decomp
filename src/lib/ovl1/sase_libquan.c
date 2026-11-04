/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, libquan.c (OVL1 0x24B028-0x24C1F0): the scalar quantiser's nearest
 * codeword (0x24B720) and the 2-D squared distance (0x24C000, OPEN), asserting as the codec does
 * (file and line by `#line`, as in sase_encsc.c). The vector quantisers and the codebook readers
 * around them are not written.
 * Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

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

/* OPEN (11 words, right size): the squared distance of two 2-D points. The target loads pfIn[0],
 * pfRef[0], pfIn[1], then subtracts and loads pfRef[1]; here both schedulers start with the [1]
 * pair whatever the source order -- five spellings (either difference first, one expression, the
 * squares apart, stepped pointers) give the same 11 words and a loop 33s: a priority, not the
 * order (tools/ccdump.py's .sched/.sched2 ready lists are the next thing to read). */
float func_0024C000(const float *pfIn, const float *pfRef)
{
	float fD0, fD1;

#line 892 "../../SaseEncVad/source/libquan.c"
	assert((pfIn != NellyNull) && (pfRef != NellyNull));
	fD0 = pfIn[0] - pfRef[0];
	fD1 = pfIn[1] - pfRef[1];
	return fD0 * fD0 + fD1 * fD1;
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
