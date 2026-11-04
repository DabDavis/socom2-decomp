/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-builtin -g */
/* newlib's libm/math/s_tan.c (tan, MAIN 0x1B3138), as newlib wrote it (doubles in soft float). One
 * object, as in the demo.
 * Byte-exact under the runtime's -g (GCC round 23a; open since round 16a: the NaN path's `jal
 * dpsub; nop`). GCC gives a DFmode register move a length of two instructions, so reorg leaves a
 * slot whose only candidate is one to GNU as, which fills it -- but not under -g, which GNU as
 * reads as -O0 on MIPS; the runtime's objects were built with GCC's CFLAGS_FOR_TARGET, -g -O2
 * (SCE's patch appends -G0). cc1's output is the same with and without -g. */

#include "fdlibm.h"

double __kernel_tan(double, double, int);
__int32_t __ieee754_rem_pio2(double, double *);

double tan(double x)
{
	double y[2],z=0.0;
	__int32_t n,ix;

    /* High word of x. */
	GET_HIGH_WORD(ix,x);

    /* |x| ~< pi/4 */
	ix &= 0x7fffffff;
	if(ix <= 0x3fe921fb) return __kernel_tan(x,z,1);

    /* tan(Inf or NaN) is NaN */
	else if (ix>=0x7ff00000) return x-x;		/* NaN */

    /* argument reduction needed */
	else {
	    n = __ieee754_rem_pio2(x,y);
	    return __kernel_tan(y[0],y[1],1-((n&1)<<1)); /*   1 -- n even
							-1 -- n odd */
	}
}
