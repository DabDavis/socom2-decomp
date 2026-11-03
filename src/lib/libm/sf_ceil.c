/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's libm/common/sf_ceil.c (ceilf, MAIN 0x1B3470), as newlib wrote it, under the EE's float
 * model (fdlibm.h: no infinity, no denormals). One object, as in the demo. */

#include "fdlibm.h"

static const float huge = 1.0e30;

float ceilf(float x)
{
	__int32_t i0,j0;
	__uint32_t i,ix;
	GET_FLOAT_WORD(i0,x);
	ix = (i0&0x7fffffff);
	j0 = (ix>>23)-0x7f;
	if(j0<23) {
	    if(j0<0) { 	/* raise inexact if x != 0 */
		if(huge+x>(float)0.0) {/* return 0*sign(x) if |x|<1 */
		    if(i0<0) {i0=0x80000000;}
		    else if(!FLT_UWORD_IS_ZERO(ix))
			{ i0=0x3f800000;}
		}
	    } else {
		i = (0x007fffff)>>j0;
		if((i0&i)==0) return x; /* x is integral */
		if(huge+x>(float)0.0) {	/* raise inexact flag */
		    if(i0>0) i0 += (0x00800000)>>j0;
		    i0 &= (~i);
		}
	    }
	} else {
	    if(!FLT_UWORD_IS_FINITE(ix)) return x+x; /* inf or NaN */
	    else return x;		/* x is integral */
	}
	SET_FLOAT_WORD(x,i0);
	return x;
}
