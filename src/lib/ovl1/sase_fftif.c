/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, the FFT interface after fftIf.c's asserting 0x2518E8 (OVL1 0x251A28-
 * 0x251C00): the transform mode's query and setter (the codec's mode, data_001D55A0 in MAIN's
 * $gp area: 0 and 1 the two transforms, 2 a set-up through 0x252150), the split of an
 * interleaved complex vector into its two halves and back, and the inverse pass by the mode
 * (round 5a's nelly.c holds its forward twin, 0x251C00). Placeholders: the codec's names are
 * not in SOCOM 1, and these assert nothing. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). The
 * split and merge count i = n >> 1 down to 0 (n/2 + 1 pairs), as the target does.
 *
 * OPEN: func_00251A98, the setter (7 words: the target stores modes 0 and 1 through a copy in v0
 * from a bnezl's slot and reaches mode 2's set-up by a bnel; this if-chain and a switch storing
 * first both come to 7, a switch per case to 20s -- build/scratch/gcc/gen_fft.py). */

extern int data_001D55A0;
extern void func_00252150(void);
extern int func_00252098(float *a, float *b, int n);
extern int func_00251C80(float *a, float *b, int n);
extern int func_00252338(float *a, float *b, int n);

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))
extern void (*data_001D55B4)(void *pDst, const void *pSrc, int n);	/* the host's memcpy */
extern void (*data_001D55B0)(void *p, int c, int n);			/* the host's memset */
void func_00251AF8(float *in, int n, float *re, float *im);
void func_00251B38(float *re, float *im, int n, float *out);
int func_00251B78(float *a, float *b, int n);

/* the forward transform of a frame (GCC round 32a; fftIf.c by its asserts, lines 25 and 27): the
 * signal's second half first and its first half last into the transform's buffer (in place when
 * pfSig is pfSW, whose room must then hold the half twice), the rest zeroed, split, transformed and
 * merged back -- the encoder's long and short frames call it (EncSC.c 0x2499A8, 0x249A38). */
void func_002518E8(const float *pfSig, int iLength, float *pfSW, int iLog2Nfft)
{
	float afRe[260];
	float afIm[260];
	int iHalfLen = iLength >> 1;
	int iNfft = 1 << iLog2Nfft;

#line 25 "../../shared/fftIf.c"
	assert(pfSig != NellyNull && pfSW != NellyNull);
#line 27 "../../shared/fftIf.c"
	assert(((pfSig == pfSW)&&(iNfft>=(iLength+iHalfLen))) ||((pfSig != pfSW)&&(iNfft>=iLength)));
	data_001D55B4(pfSW + (iNfft - iHalfLen), pfSig, iHalfLen * sizeof(float));
	data_001D55B4(pfSW, pfSig + iHalfLen, (iLength - iHalfLen) * sizeof(float));
	data_001D55B0(pfSW + (iLength - iHalfLen), 0, (iNfft - iLength) * sizeof(float));
	func_00251AF8(pfSW, iNfft, afRe, afIm);
	func_00251B78(afRe, afIm, iNfft);
	func_00251B38(afRe, afIm, iNfft, pfSW);
}

int func_00251A28(void)
{
	int ret;

	ret = 0;
	switch (data_001D55A0) {
	case 1:
		func_00252150();
		break;
	case 0:
	case 2:
		break;
	default:
		data_001D55A0 = 0;
		ret = -1;
		break;
	}
	return ret;
}

int func_00251A98(int mode)
{
	int ret;

	ret = 0;
	if (mode < 0) {
		data_001D55A0 = 0;
		ret = -1;
	} else if (mode < 2)
		data_001D55A0 = mode;
	else if (mode == 2) {
		data_001D55A0 = mode;
		func_00252150();
	} else {
		data_001D55A0 = 0;
		ret = -1;
	}
	return ret;
}

void func_00251AF8(float *in, int n, float *re, float *im)
{
	int i;

	for (i = n >> 1; i >= 0; i--) {
		*re++ = *in++;
		*im++ = *in++;
	}
}

void func_00251B38(float *re, float *im, int n, float *out)
{
	int i;

	for (i = n >> 1; i >= 0; i--) {
		*out++ = *re++;
		*out++ = *im++;
	}
}

int func_00251B78(float *a, float *b, int n)
{
	if (data_001D55A0 != 0)
		func_00252098(a, b, n / 2);
	else
		func_00251C80(a, b, n / 2);
	return func_00252338(a, b, n / 2);
}
