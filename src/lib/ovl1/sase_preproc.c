/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, PreProc.c (OVL1 0x24FE38-0x24FF58): the input's pre-processing -- 16-bit
 * samples to floats through a DC-blocking high-pass (y[n] = x[n] - x[n-1] + c * y[n-1]), its
 * state in a 12-byte block, the feedback flushed to zero when it decays below 1e-5 either way.
 * Asserting as the codec does (file and line by `#line`, as in sase_encsc.c). Written in the GCC
 * lane's round 18a. Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is
 * (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

typedef struct {
	float fPrevIn;
	float fPrevOut;
	float fCoef;
} PreProcMblk;

void func_0024FE38(const short *psIn, float *pfOut, int iLength, PreProcMblk *hPreProcMblk)
{
	float fCoef = hPreProcMblk->fCoef;
	float fPrevIn, fPrevOut, fIn;

#line 95 "../../SaseEncVad/source/PreProc.c"
	assert((psIn != NellyNull) && (pfOut != NellyNull));
	assert(hPreProcMblk != NellyNull);
	fPrevOut = hPreProcMblk->fPrevOut;
	fPrevIn = hPreProcMblk->fPrevIn;
	if ((fPrevOut > 0.0F) ? (fPrevOut < 0x1.4f8b58p-17F) : (fPrevOut > -0x1.4f8b58p-17F))
		fPrevOut = 0.0F;			/* within 1e-5 of zero */
	while (iLength-- > 0) {
		fIn = (float)*psIn++;
		fPrevOut = (fIn - fPrevIn) + fCoef * fPrevOut;
		fPrevIn = fIn;
		*pfOut++ = fPrevOut;
	}
	hPreProcMblk->fPrevIn = fPrevIn;
	hPreProcMblk->fPrevOut = fPrevOut;
}
