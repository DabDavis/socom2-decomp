/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's libm/common/s_fabs.c (fabs, MAIN 0x1B2DB0), from the toolchain's own libm: ee-gcc 2.96 -O2 -G0
 * (README "The GCC front"; every 2.9x/2.95.x/3.2 build misses). One object, as in the demo. */

#include "fdlibm.h"

/* OPEN: one instruction placed differently -- the target copies the result to v0 before the
 * return and leaves the delay slot empty (`move v0,a0; jr ra; nop`), where this compiles the copy
 * into the slot (`jr ra; move v0,a0`); every 2.9x/2.95.x compiler misses by 10 words, 3.2 by 13,
 * and the union, signed-word, temporary and pointer spellings, -O1/-O3/-Os and -fno-builtin do not
 * leave the slot empty either. Diagnosed (round 4a, libgcc): the move is of a double, and the
 * compiler that built the runtime gave a DFmode register move a length of two instructions, which
 * reorg never puts in a delay slot (libgcc's __fixdfdi/__floatdidf: every such slot empty, an int
 * move filling the same kind of slot); toolchains/ee-gcc2.96 gives it one. */
double fabs(double x)
{
	__uint32_t high;
	GET_HIGH_WORD(high,x);
	SET_HIGH_WORD(x,high&0x7fffffff);
        return x;
}
