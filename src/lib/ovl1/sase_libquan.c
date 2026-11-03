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
