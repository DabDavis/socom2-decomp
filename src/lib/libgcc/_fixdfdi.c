/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -g */
/* libgcc2's L_fixdfdi object (MAIN 0x1A0560): libgcc2.c's __fixdfdi, built by the toolchain's own
 * 2.96 (libgcc2.h holds the shared text).
 *
 * Byte-exact under the runtime's -g (GCC round 23a; open since round 4a): the target leaves each
 * call slot whose only candidate is a move of a double (DFmode, a GPR on the EE: soft float) empty
 * -- `move s0,a0; jal dpcmp; nop` -- and fills one with an int move (__floatdidf's second litodp).
 * GCC gives a DFmode register move a length of two instructions, so reorg leaves a slot whose only
 * candidate is one to GNU as, which fills it -- but not under -g, which GNU as reads as -O0 on
 * MIPS; the runtime's objects were built with GCC's CFLAGS_FOR_TARGET, -g -O2 (SCE's patch appends
 * -G0). cc1's output is the same with and without -g (README "The GCC front").
 */
#define L_fixdfdi
#include "libgcc2.h"
