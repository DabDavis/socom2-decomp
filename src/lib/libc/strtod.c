/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
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

/* Byte-exact under the runtime's -g (GCC round 23a; open since round 3a): the target leaves both
 * calls' delay slots empty (`jal strtod; nop; move a0,v0; jal dptofp; nop`), both slots' candidates
 * moves of a double. GCC gives a DFmode register move a length of two instructions, so reorg leaves
 * a slot whose only candidate is one to GNU as, which fills it -- but not under -g, which GNU as
 * reads as -O0 on MIPS; the runtime's objects were built with GCC's CFLAGS_FOR_TARGET, -g -O2
 * (SCE's patch appends -G0). cc1's output is the same with and without -g. */
float strtof(const char *s00, char **se)
{
	return strtod(s00, se);
}
