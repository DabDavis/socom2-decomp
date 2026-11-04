/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's libm/math/wf_pow.c (powf, MAIN 0x1B3940) under _IEEE_LIBM: the wrapper is a call of the
 * kernel, 2.96's sibling call under the frame it keeps. One object, as in the demo. */

#include "fdlibm.h"

float __ieee754_powf(float, float);

float powf(float x, float y)	/* wrapper powf */
{
	return  __ieee754_powf(x,y);
}
