/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* libgcc2's L_floatdidf object (MAIN 0x1A03A0): libgcc2.c's __floatdidf, built by the toolchain's own
 * 2.96 (libgcc2.h holds the shared text).
 *
 * OPEN: every instruction is right but the call delay slots: the target leaves each slot whose
 * only candidate is a move of a double (DFmode, a GPR on the EE: soft float) empty -- `move s0,a0;
 * jal dpcmp; nop` -- and fills one with an int move (__floatdidf's second litodp). The compiler
 * that built libgcc gave a DFmode register move a length of two instructions, which reorg never
 * puts in a delay slot; toolchains/ee-gcc2.96 gives it one and fills them. No flag changes it
 * (-msingle-float, -mhard-float, -mfp32, -mips3, -mlong64, -mabi=o64; -fno-delayed-branch hands
 * the slots to GNU as, which fills them too). newlib's strtof and libm's fabs are open on the same
 * cause (README "The GCC front").
 */
#define L_floatdidf
#include "libgcc2.h"
