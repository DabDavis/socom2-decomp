/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's libm/common/s_copysign.c (copysign, MAIN 0x198010), from the toolchain's own libm: ee-gcc 2.96 -O2 -G0
 * (README "The GCC front"; every 2.9x/2.95.x/3.2 build misses). One object, as in the demo. */

#include "fdlibm.h"

double copysign(double x, double y)
{
	__uint32_t hx,hy;
	GET_HIGH_WORD(hx,x);
	GET_HIGH_WORD(hy,y);
	SET_HIGH_WORD(x,(hx&0x7fffffff)|(hy&0x80000000));
        return x;
}
