/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* newlib's libm/common/s_isinf.c (isinf, MAIN 0x198058), from the toolchain's own libm: ee-gcc 2.96 -O2 -G0
 * (README "The GCC front"; every 2.9x/2.95.x/3.2 build misses). One object, as in the demo. */

#include "fdlibm.h"

int isinf(double x)
{
	__int32_t hx,lx;
	EXTRACT_WORDS(hx,lx,x);
	hx &= 0x7fffffff;
	hx |= (__uint32_t)(lx|(-lx))>>31;
	hx = 0x7ff00000 - hx;
	return 1 - (int)((__uint32_t)(hx|(-hx))>>31);
}
