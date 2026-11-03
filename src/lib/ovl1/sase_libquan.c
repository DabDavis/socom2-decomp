/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, libquan.c (OVL1 0x24B028-0x24C1F0): the scalar quantiser's nearest
 * codeword (0x24B720), asserting as the codec does (file and line by `#line`, as in
 * sase_encsc.c). The vector quantisers and the codebook readers around it are not written.
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
