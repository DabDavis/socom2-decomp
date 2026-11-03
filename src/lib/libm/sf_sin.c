/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's libm/common/sf_sin.c (sinf, MAIN 0x1B3720), as newlib wrote it, under the EE's float
 * model (fdlibm.h). One object, as in the demo. */

#include "fdlibm.h"

float __kernel_sinf(float, float, int);
float __kernel_cosf(float, float);
__int32_t __ieee754_rem_pio2f(float, float *);

float sinf(float x)
{
	float y[2],z=0.0;
	__int32_t n,ix;

	GET_FLOAT_WORD(ix,x);

    /* |x| ~< pi/4 */
	ix &= 0x7fffffff;
	if(ix <= 0x3f490fd8) return __kernel_sinf(x,z,0);

    /* sin(Inf or NaN) is NaN */
	else if (!FLT_UWORD_IS_FINITE(ix)) return x-x;

    /* argument reduction needed */
	else {
	    n = __ieee754_rem_pio2f(x,y);
	    switch(n&3) {
		case 0: return  __kernel_sinf(y[0],y[1],1);
		case 1: return  __kernel_cosf(y[0],y[1]);
		case 2: return -__kernel_sinf(y[0],y[1],1);
		default:
			return -__kernel_cosf(y[0],y[1]);
	    }
	}
}
