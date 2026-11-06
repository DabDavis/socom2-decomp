/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, libsigproc.c (OVL1 0x24C1F0-): the signal-processing helpers, asserting as
 * the codec does (file and line by `#line`, as in sase_encsc.c). Written: the Hamming window
 * (0x24C458: the raised-cosine window 0x24C1F0 with 2/N * 0.54 and 2/N * 0.46 -- the coefficients
 * are the encoder's .lit4 entries through $gp, written as hex floats: this GCC's decimal conversion
 * is not the vendor's, round 12a) and, in the GCC lane's round 18a, the symmetric window's
 * application (0x24C4D8, from both ends at once). GCC round 44a: the window itself (0x24C1F0: a cosine
 * recurrence its source unrolls by three, the remainder after the loop). Placeholders:
 * the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

extern void func_0024C1F0(float *x, int iLength, float fA, float fB);


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

extern float cosf(float);

/* the raised-cosine window multiplied into pfWindow from both ends at once: fA - fB cos(2 pi i / (N - 1)), the cosine by
 * its recurrence (c[n+1] = 2 cos(w) c[n] - c[n-1]), the loop unrolled by three, the remainder after it */
void func_0024C1F0(float *x, int iLength, float fA, float fB)
{
	float *p = x;
	float *q = x + iLength - 1;
	float fC0 = fB, fC1, fC2, fTwoCos, f;
	int half_length = iLength >> 1;
	int i;

	fTwoCos = cosf(0x1.921fb4p+2F / (float)(iLength - 1));
#line 159 "../../SaseEncVad/source/libsigproc.c"
	assert(x != NellyNull);
	fC1 = fTwoCos * fB;
	fTwoCos += fTwoCos;
#line 164 "../../SaseEncVad/source/libsigproc.c"
	assert(half_length > 2);
	f = fA - fB;
	*p++ *= f;
	*q-- *= f;
	f = fA - fC1;
	*p++ *= f;
	*q-- *= f;
	for (i = 2; i + 2 < half_length; i += 3) {
		fC2 = fTwoCos * fC1;
		fC2 -= fC0;
		f = fA - fC2;
		fC0 = fTwoCos * fC2;
		fC0 -= fC1;
		*p++ *= f;
		fC1 = fTwoCos * fC0;
		fC1 -= fC2;
		*q-- *= f;
		f = fA - fC0;
		*p++ *= f;
		*q-- *= f;
		f = fA - fC1;
		*p++ *= f;
		*q-- *= f;
	}
	if (i < half_length) {
		fC2 = fTwoCos * fC1;
		fC2 -= fC0;
		f = fA - fC2;
		*p++ *= f;
		*q-- *= f;
		i++;
		if (i < half_length) {
			fC0 = fTwoCos * fC2;
		fC0 -= fC1;
			f = fA - fC0;
			*p++ *= f;
			*q-- *= f;
		}
	}
	if (iLength & 1)
		*p *= fA + fB;
}
