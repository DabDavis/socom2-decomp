/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, libvect.c (OVL1 0x24ECF8-0x24F010): vector helpers -- the power of a
 * complex vector held as two arrays or interleaved, and a scaled copy. Each asserts its length (file
 * and line by `#line`, as in sase_encsc.c) and walks its pointers in a while loop: `while (n > 0)`
 * with the count stepped last, so the asserted n > 0 enters the body without a second test, and
 * `while (n-- > 0)` for the scaled copy. 0x24ECF8 and 0x24EF30 are not written. Placeholders: the
 * codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

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

void func_0024EEA0(const float *pfIn, int iLength, float *pfOut, float fScale)
{
#line 264 "../../SaseEncVad/source/libvect.c"
	assert(iLength >= 0);
	while (iLength-- > 0)
		*pfOut++ = *pfIn++ * fScale;
}
