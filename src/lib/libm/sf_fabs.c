/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's libm/common/sf_fabs.c (fabsf, MAIN 0x1B3620), from the toolchain's own libm: ee-gcc 2.96 -O2 -G0
 * (README "The GCC front"; every 2.9x/2.95.x/3.2 build misses). One object, as in the demo. */

#include "fdlibm.h"

float fabsf(float x)
{
	__uint32_t ix;
	GET_FLOAT_WORD(ix,x);
	SET_FLOAT_WORD(x,ix&0x7fffffff);
        return x;
}
