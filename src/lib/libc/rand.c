/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdlib/rand.c (MAIN 0x197728-0x1977A4), the toolchain's libc: srand then rand, one
 * object as in the demo -- the 64-bit LCG in the reentrancy block (Knuth's multiplier). */

#include "newlib.h"

void srand(unsigned int seed)
{
	_REENT->_rand_next = seed;
}

int rand(void)
{
	_REENT->_rand_next = _REENT->_rand_next * __extension__ 6364136223846793005LL + 1;
	return (int)((_REENT->_rand_next >> 32) & 0x7fffffff);
}
