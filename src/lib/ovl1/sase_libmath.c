/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, libmath.c (OVL1 0x24AB48-0x24B028): the codec's scalar maths -- the
 * minimal-standard random number generator (Park and Miller's 16807, Carta's two 16-bit halves,
 * no division) and two polynomial approximations scaled decade by decade (0x24AD48, a fifth-degree
 * fit per decade of its argument, and the fourth root 0x24AF50: x scaled up by 10 until it reaches
 * 0.1, the result by 10^-0.25 each time). Asserting as the codec does (file and line by `#line`,
 * as in sase_encsc.c); the constants are hex floats (round 12a: this GCC's decimal conversion is
 * not the vendor's). Written: 0x24AF50 (GCC round 18a), 0x24AEB0 (round 19a). Not written: 0x24AB48,
 * 0x24AD48 (its argument is a float/word union on the stack, compared as floats against the
 * decade table at 0x3E81D0 though its assert reads `union_x.l <= uiBoundary[0]`).
 * Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

/* the minimal-standard generator, Carta's form: the seed's halves multiplied apart, the high
 * product's low fifteen bits folded into the low product, the carries out of bit 31 added back;
 * the halves are read at the declarations (the assert's call would reload *plSeed otherwise).
 * The high half is the accumulator, updated in place -- `hi >>= 15; hi += lo >> 31; lo = (lo << 1)
 * >> 1; hi += lo;`: one variable through the chain gives the target's v0 from the product to the
 * store, the low sum its v1 (open since round 18a, 12 words, on the sum written as one expression
 * into `lo`: then the low half held the chain and took v0) -- GCC round 19a, measured over 101
 * spellings (build/scratch/gcc/v_rng*.c): this and a temporary `t` for the accumulator are the two
 * that match; the decoder's copy 0x256380 (sase_swsynth.c) is the same text. */
void func_0024AEB0(int *plSeed)
{
	unsigned int lo, hi;
	unsigned int ulLo = *plSeed & 0xFFFF;
	int lHi = *plSeed >> 16;

#line 78 "../../SaseEncVad/source/libmath.c"
	assert(*plSeed != 0);
	hi = 16807 * lHi;
	lo = ((hi << 17) >> 1) + 16807 * ulLo;
	hi >>= 15;
	hi += lo >> 31;
	lo = (lo << 1) >> 1;
	hi += lo;
	hi += hi >> 31;
	*plSeed = (hi << 1) >> 1;
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
