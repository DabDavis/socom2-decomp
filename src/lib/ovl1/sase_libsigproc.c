/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, libsigproc.c (OVL1 0x24C1F0-): the signal-processing helpers, asserting as
 * the codec does (file and line by `#line`, as in sase_encsc.c). Written: the Hamming window
 * (0x24C458: the raised-cosine window 0x24C1F0 with 2/N * 0.54 and 2/N * 0.46 -- the coefficients
 * are the encoder's .lit4 entries through $gp, written as hex floats: this GCC's decimal conversion
 * is not the vendor's, round 12a) and, in the GCC lane's round 18a, the symmetric window's
 * application (0x24C4D8, from both ends at once). The window itself (0x24C1F0: a cosine
 * recurrence its source unrolls by three, the remainder after the loop) is not written. Placeholders:
 * the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

extern void func_0024C1F0(float *pfWindow, int iLength, float fA, float fB);

/* a symmetric window applied from both ends at once: half the window serves both halves */
void func_0024C4D8(const float *pIn, const float *pWin, float *pOut, int iLength)
{
	int iHalf = iLength >> 1;
	const float *pInEnd;
	float *pOutEnd;

#line 62 "../../SaseEncVad/source/libsigproc.c"
	assert((pIn != NellyNull) && (pWin != NellyNull) && (pOut != NellyNull));
	pInEnd = pIn + iLength - 1;
	pOutEnd = pOut + iLength - 1;
	while (iHalf-- > 0) {
		*pOut++ = *pIn++ * *pWin;
		*pOutEnd-- = *pInEnd-- * *pWin++;
	}
	if (iLength & 1)
		*pOut = *pIn * *pWin;
}

void func_0024C458(float *pfWindow, int iLength)
{
	float fScale;

#line 103 "../../SaseEncVad/source/libsigproc.c"
	assert(iLength > 0);
	fScale = 2.0f / (float)iLength;
	func_0024C1F0(pfWindow, iLength, fScale * 0x1.147aep-1f, fScale * 0x1.d70a3cp-2f);
}
