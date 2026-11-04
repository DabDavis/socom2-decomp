/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, libmath.c (OVL1 0x24AB48-0x24B028): the codec's scalar maths -- the
 * minimal-standard random number generator (Park and Miller's 16807, Carta's two 16-bit halves,
 * no division) and two polynomial approximations scaled decade by decade (0x24AD48, a fifth-degree
 * fit per decade of its argument, and the fourth root 0x24AF50: x scaled up by 10 until it reaches
 * 0.1, the result by 10^-0.25 each time). Asserting as the codec does (file and line by `#line`,
 * as in sase_encsc.c); the constants are hex floats (round 12a: this GCC's decimal conversion is
 * not the vendor's). Written (GCC round 18a): 0x24AEB0 (OPEN), 0x24AF50. Not written: 0x24AB48,
 * 0x24AD48 (its argument is a float/word union on the stack, compared as floats against the
 * decade table at 0x3E81D0 though its assert reads `union_x.l <= uiBoundary[0]`).
 * Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

/* OPEN (12 words, right size, every instruction but the registers of the arithmetic after the
 * assert): the target keeps both halves of the seed in saved registers from before the assert
 * (its *plSeed is read once: the halves are computed into locals first, or the assert's call
 * would reload it), multiplies them into v0 and a0 and folds the sum into v1; here the high
 * product takes a1 and the sum v0 -- every instruction and its order the target's. The
 * declarations' order, a returned seed, the masks spelled `& 0x7FFFFFFF` (one lui/ori shared:
 * 3 words more) or as the shifts the target uses, and the high half's shift written first or
 * last move nothing closer (build/scratch/gcc/v_math.c in the GCC lane's round 18a). */
void func_0024AEB0(int *plSeed)
{
	unsigned int lo, hi;
	unsigned int ulLo = *plSeed & 0xFFFF;
	int lHi = *plSeed >> 16;

#line 78 "../../SaseEncVad/source/libmath.c"
	assert(*plSeed != 0);
	hi = 16807 * lHi;
	lo = ((hi << 17) >> 1) + 16807 * ulLo;
	lo = (hi >> 15) + (lo >> 31) + ((lo << 1) >> 1);
	lo += lo >> 31;
	*plSeed = (lo << 1) >> 1;
}

/* the fourth root of x in [0, 1]: a quadratic fit over [0.1, 1], the argument scaled into it */
float func_0024AF50(float fXval)
{
	float fScale, fY;

#line 636 "../../SaseEncVad/source/libmath.c"
	assert(fXval >= 0.0F);
	assert(fXval <= 1.0F);
	fXval += 0x1.ad7f28p-24F;			/* 1e-7 */
	fScale = 1.0F;
	while (fXval < 0x1.999998p-4F) {		/* 0.1, one ulp short */
		fXval *= 10.0F;
		fScale *= 0x1.1feb32p-1F;		/* 10^-0.25 */
	}
	fY = fXval * -0x1.ebae14p-2F + 0x1.00065ep+0F;
	fY = fXval * fY + 0x1.dba3aap-2F;
	return fY * fScale;
}
