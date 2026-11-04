/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-builtin -g */
/* newlib's libm/math/k_sin.c (__kernel_sin, MAIN 0x1B17A0), as newlib 1.8.2 writes it (doubles in soft float:
 * every operation a dp* call), built as the toolchain's runtime is: -g, so GNU as fills none of the
 * delay slots GCC leaves it (README "The GCC front", round 23a), and -fno-builtin. One object, as
 * in the demo. The prototypes are newlib fdlibm.h's. */

#include "fdlibm.h"


#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
static const double 
#else
static double 
#endif
half =  5.00000000000000000000e-01, /* 0x3FE00000, 0x00000000 */
S1  = -1.66666666666666324348e-01, /* 0xBFC55555, 0x55555549 */
S2  =  8.33333333332248946124e-03, /* 0x3F811111, 0x1110F8A6 */
S3  = -1.98412698298579493134e-04, /* 0xBF2A01A0, 0x19C161D5 */
S4  =  2.75573137070700676789e-06, /* 0x3EC71DE3, 0x57B1FE7D */
S5  = -2.50507602534068634195e-08, /* 0xBE5AE5E6, 0x8A2B9CEB */
S6  =  1.58969099521155010221e-10; /* 0x3DE5D93A, 0x5ACFD57C */

#ifdef __STDC__
	double __kernel_sin(double x, double y, int iy)
#else
	double __kernel_sin(x, y, iy)
	double x,y; int iy;		/* iy=0 if y is zero */
#endif
{
	double z,r,v;
	__int32_t ix;
	GET_HIGH_WORD(ix,x);
	ix &= 0x7fffffff;			/* high word of x */
	if(ix<0x3e400000)			/* |x| < 2**-27 */
	   {if((int)x==0) return x;}		/* generate inexact */
	z	=  x*x;
	v	=  z*x;
	r	=  S2+z*(S3+z*(S4+z*(S5+z*S6)));
	if(iy==0) return x+v*(S1+z*r);
	else      return x-((z*(half*y-v*r)-y)-v*S1);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
