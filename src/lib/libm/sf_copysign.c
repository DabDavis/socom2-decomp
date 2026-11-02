/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's libm/common/sf_copysign.c (copysignf, MAIN 0x1983E0), from the toolchain's own libm: ee-gcc 2.96 -O2 -G0
 * (README "The GCC front"; every 2.9x/2.95.x/3.2 build misses). One object, as in the demo. */

#include "fdlibm.h"

float copysignf(float x, float y)
{
	__uint32_t ix,iy;
	GET_FLOAT_WORD(ix,x);
	GET_FLOAT_WORD(iy,y);
	SET_FLOAT_WORD(x,(ix&0x7fffffff)|(iy&0x80000000));
        return x;
}
