/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-builtin -g */
/* newlib's libm/math/s_sin.c (sin, MAIN 0x1B3000), as newlib 1.8.2 writes it (doubles in soft float:
 * every operation a dp* call), built as the toolchain's runtime is: -g, so GNU as fills none of the
 * delay slots GCC leaves it (README "The GCC front", round 23a), and -fno-builtin. One object, as
 * in the demo. The prototypes are newlib fdlibm.h's. */

#include "fdlibm.h"

__int32_t __ieee754_rem_pio2(double, double *);
double __kernel_sin(double, double, int);
double __kernel_cos(double, double);


#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double sin(double x)
#else
	double sin(x)
	double x;
#endif
{
	double y[2],z=0.0;
	__int32_t n,ix;

    /* High word of x. */
	GET_HIGH_WORD(ix,x);

    /* |x| ~< pi/4 */
	ix &= 0x7fffffff;
	if(ix <= 0x3fe921fb) return __kernel_sin(x,z,0);

    /* sin(Inf or NaN) is NaN */
	else if (ix>=0x7ff00000) return x-x;

    /* argument reduction needed */
	else {
	    n = __ieee754_rem_pio2(x,y);
	    switch(n&3) {
		case 0: return  __kernel_sin(y[0],y[1],1);
		case 1: return  __kernel_cos(y[0],y[1]);
		case 2: return -__kernel_sin(y[0],y[1],1);
		default:
			return -__kernel_cos(y[0],y[1]);
	    }
	}
}

#endif /* _DOUBLE_IS_32BITS */
