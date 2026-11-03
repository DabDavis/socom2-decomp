/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-builtin */
/* newlib's libm/math/s_tan.c (tan, MAIN 0x1B3138), as newlib wrote it (doubles in soft float). One
 * object, as in the demo.
 * OPEN, 0xA0 against 0xA8: the runtime compiler's DFmode moves again (round 4a) -- the target's NaN
 * path is `jal dpsub; nop`, this 2.96 puts the double's `move a1,t0` in the call's slot, and the
 * kernel call's argument setup schedules apart. The double routines of the object (exp, pow,
 * rem_pio2, sqrt, the three kernels, cos, sin, floor) wait on the same toolchain difference. */

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
