/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's libm/math/wf_log10.c (log10f, MAIN 0x1B3928) under _IEEE_LIBM: the wrapper is a call of
 * the kernel, 2.96's sibling call under the frame it keeps. One object, as in the demo. */

#include "fdlibm.h"

float __ieee754_log10f(float);

float log10f(float x)		/* wrapper log10f */
{
	return __ieee754_log10f(x);
}
