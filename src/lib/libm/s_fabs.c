/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's libm/common/s_fabs.c (fabs, MAIN 0x1B2DB0), from the toolchain's own libm: ee-gcc 2.96 -O2 -G0
 * (README "The GCC front"; every 2.9x/2.95.x/3.2 build misses). One object, as in the demo. */

#include "fdlibm.h"

/* Byte-exact under the runtime's -g (GCC round 23a; open since round 2a): the target copies the
 * result to v0 before the return and leaves the slot empty (`move v0,a0; jr ra; nop`) -- the copy
 * is a double's. GCC gives a DFmode register move a length of two instructions, so reorg leaves a
 * slot whose only candidate is one to GNU as, which fills it -- but not under -g, which GNU as
 * reads as -O0 on MIPS; the runtime's objects were built with GCC's CFLAGS_FOR_TARGET, -g -O2
 * (SCE's patch appends -G0). cc1's output is the same with and without -g. */
double fabs(double x)
{
	__uint32_t high;
	GET_HIGH_WORD(high,x);
	SET_HIGH_WORD(x,high&0x7fffffff);
        return x;
}
