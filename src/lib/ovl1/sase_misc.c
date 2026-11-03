/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder: QP0SC3.c's pitch-index dequantiser (OVL1 0x250C68: exp of a line in the
 * index, its two constants .lit4 floats through $gp, the call a sibling `j expf`), asserting as
 * its own file and line (by `#line`, as in sase_encsc.c). libsnd.c's circular-buffer release,
 * here since round 12a, is in sase_libsnd.c with the rest of its object (GCC round 18a).
 * Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8).
 *
 * A float constant is written as a decimal that converts to the target's bits under this GCC, which
 * does not round to nearest: 0.011747969314455986f, the target's 0x3C407A8E exactly, comes out one
 * ulp low (0x3C407A8D); a decimal between it and the next float up gives it. */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

extern float expf(float x);

#define BITS_PITCH_SC3 8

float func_00250C68(int iIndex)
{
#line 134 "../../SaseEncVad/source/QP0SC3.c"
	assert((iIndex >= 0) && (iIndex < (1<<BITS_PITCH_SC3)));
	return expf(5.0751733779907227f - (float)iIndex * 0.0117479697f);
}
