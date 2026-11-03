/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder: single functions out of two objects the target holds apart (each asserts
 * as its own file and line, by `#line` as in sase_encsc.c) -- libsnd.c's circular-buffer release
 * (OVL1 0x24C648) and QP0SC3.c's pitch-index dequantiser (0x250C68: exp of a line in the index, its
 * two constants .lit4 floats through $gp, the call a sibling `j expf`). Placeholders: the codec's
 * names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8).
 *
 * A float constant is written as a decimal that converts to the target's bits under this GCC, which
 * does not round to nearest: 0.011747969314455986f, the target's 0x3C407A8E exactly, comes out one
 * ulp low (0x3C407A8D); a decimal between it and the next float up gives it. */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

extern void (*data_001D55AC)(void *);           /* the host's free callback */
extern float expf(float x);

typedef struct {
	int f0;
	int f4;
	void *pBuf;
} CircMblk;

#define FREE(p) if ((p) != NellyNull) { data_001D55AC(p); (p) = NellyNull; }

void func_0024C648(CircMblk **pvCircMblk)
{
#line 119 "../../SaseEncVad/source/libsnd.c"
	assert(pvCircMblk != NellyNull);
	if (*pvCircMblk != NellyNull) {
		FREE((*pvCircMblk)->pBuf);
		FREE(*pvCircMblk);
	}
}

#define BITS_PITCH_SC3 8

float func_00250C68(int iIndex)
{
#line 134 "../../SaseEncVad/source/QP0SC3.c"
	assert((iIndex >= 0) && (iIndex < (1<<BITS_PITCH_SC3)));
	return expf(5.0751733779907227f - (float)iIndex * 0.0117479697f);
}
