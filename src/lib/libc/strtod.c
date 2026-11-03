/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* newlib's stdlib/strtod.c (MAIN 0x199A98-0x19AA00), the toolchain's libc (README "The GCC front"):
 * the object's two wrappers after _strtod_r (0x199A98, 0xF14 bytes, David Gay's conversion -- not
 * written here). strtod is 2.96's sibling call under the leftover frame; strtof narrows strtod's
 * double through libgcc's dptofp (the EE has no double unit). */

#include "newlib.h"

extern double _strtod_r(struct _reent *ptr, const char *s00, char **se);

double strtod(const char *s00, char **se)
{
	return _strtod_r(_REENT, s00, se);
}

/* OPEN: 3 words. The target leaves both calls' delay slots empty and sets the conversion's argument
 * before its jal (`jal strtod; nop; move a0,v0; jal dptofp; nop`), where 2.96 -O2 moves the move into
 * the second slot -- as if delayed-branch scheduling were off for this function alone (its epilogue's
 * slot is filled, which GCC's epilogue does itself); strtod in the same object is 2.96's sibling call. 
 * Diagnosed (round 4a): both slots' candidates are moves of a double (strtod's result into a0),
 * which the runtime's compiler never put in a delay slot (a DFmode move two instructions long to
 * reorg; libgcc's __fixdfdi and libm's fabs show the same); toolchains/ee-gcc2.96 fills them. */
float strtof(const char *s00, char **se)
{
	return strtod(s00, se);
}
