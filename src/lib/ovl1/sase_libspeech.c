/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, libspeech.c (OVL1 0x24C8E8-0x24D8B8): the speech analysis helpers -- the
 * LPC recursion over the autocorrelation, the pitch and peak pickers, the parabolic peak
 * interpolation, the STFT phase. Asserting as the codec does (file and line by `#line`, as in
 * sase_encsc.c). Written in the GCC lane's round 18a: the parabolic interpolation 0x24D498 (its
 * guarded division `if (fA != 0.0F) fX = ...; else fX = 0.0F;`: the else's zero copied from the
 * compare's in the branch's slot; `fX = 0.0F; if (...)` 4 words off, the test inverted 27). The
 * rest (0x24C8E8, 0x24CAE8, 0x24CD28, 0x24CF10, 0x24D190, 0x24D370, 0x24D560, 0x24D770) is not
 * written. Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96,
 * -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

/* the vertex of the parabola through three samples about a peak: its offset from the middle
 * sample returned, its height stored */
float func_0024D498(float *pfYpeak, float fY_1, float fY0, float fY1)
{
	float fA, fB, fX;

#line 256 "../../SaseEncVad/source/libspeech.c"
	assert((fY0 >= fY_1) && (fY0 >= fY1));
	fB = (fY1 - fY_1) * 0.5F;
	fA = (fY_1 + fY1) * 0.5F - fY0;
	if (fA != 0.0F)
		fX = -fB / (2.0F * fA);
	else
		fX = 0.0F;
	*pfYpeak = (fA * fX + fB) * fX + fY0;
	return fX;
}
