/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, libvect.c (OVL1 0x24ECF8-0x24F010): vector helpers -- the power of a
 * complex vector held as two arrays or interleaved, and a scaled copy. Each asserts its length (file
 * and line by `#line`, as in sase_encsc.c) and walks its pointers in a while loop: `while (n > 0)`
 * with the count stepped last, so the asserted n > 0 enters the body without a second test, and
 * `while (n-- > 0)` for the scaled copy. GCC round 18a: the maximum 0x24ECF8 and the normalised
 * fourth roots 0x24EF30, the object whole. The scaled copy is an inline function -- 0x24EF30
 * inlines it, assert and all, and its out-of-line copy is the same code -- and the image holds the
 * maximum (line 397) first: GCC defers an inline function's output to the end of the object, so
 * the helpers before it in the image are most likely inline too (not tested: each scores alone). 0x24EF30's `if (fNorm == 0.0F) fScale = 1.0F; else ...` sets 1.0 before the test
 * and loads 1.0 again for the division (if-conversion after CSE); `fScale = 1.0F; if (fNorm !=
 * 0.0F) ...` divides f20 by itself, 2 words short. Placeholders: the codec's names are not in
 * SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

void func_0024EDD0(const float *pfRe, const float *pfIm, int iLength, float *pfOut)
{
#line 192 "../../SaseEncVad/source/libvect.c"
	assert(iLength > 0);
	while (iLength > 0) {
		*pfOut++ = *pfRe * *pfRe + *pfIm * *pfIm;
		pfRe++;
		pfIm++;
		iLength--;
	}
}

void func_0024EE38(const float *pfIn, int iLength, float *pfOut)
{
#line 220 "../../SaseEncVad/source/libvect.c"
	assert(iLength > 0);
	while (iLength > 0) {
		*pfOut++ = pfIn[0] * pfIn[0] + pfIn[1] * pfIn[1];
		pfIn += 2;
		iLength--;
	}
}

__inline__ void func_0024EEA0(const float *pfIn, int iLength, float *pfOut, float fScale)
{
#line 264 "../../SaseEncVad/source/libvect.c"
	assert(iLength >= 0);
	while (iLength-- > 0)
		*pfOut++ = *pfIn++ * fScale;
}

extern float func_0024AF50(float fXval);

/* the vector normalised by fNorm (unscaled when it is zero), each element's fourth root taken:
 * the scaled copy above is inlined here, its assert with it */
void func_0024EF30(const float *pfIn, int iLength, float *pfOut, float fNorm)
{
	float fScale;
	int i;

	if (fNorm == 0.0F)
		fScale = 1.0F;
	else
		fScale = 1.0F / fNorm;
	func_0024EEA0(pfIn, iLength, pfOut, fScale);
	for (i = 0; i < iLength; i++)
		pfOut[i] = func_0024AF50(pfOut[i]);
}

/* the largest element and its index */
void func_0024ECF8(const float *pfX, int iLength, int *piMaxIndex, float *pfXmax)
{
	float fMax;
	int i, iMax;

#line 397 "../../SaseEncVad/source/libvect.c"
	assert(pfX != NellyNull && piMaxIndex != NellyNull && pfXmax != NellyNull);
	assert(iLength >= 1);
	fMax = *pfX++;
	iMax = 0;
	for (i = 1; i < iLength; i++) {
		if (fMax < *pfX) {
			fMax = *pfX;
			iMax = i;
		}
		pfX++;
	}
	*piMaxIndex = iMax;
	*pfXmax = fMax;
}
