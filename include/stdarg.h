/* <stdarg.h>: the EE's EABI va_list as MW builds it -- a pointer into the register save area,
 * started (8 - named argument registers) slots below its top, each argument 8 bytes. The
 * conditional's sense is the shipped code's (CZAnim::ActivateArg: slti, bnez to the
 * computation, the zero case falling through); with < it inverts, and GCC's args_info(0) form
 * crashes MW. The forms themselves are <zport.h>'s Z_VA_*: these on the PS2, the host's builtins
 * in a port. */
#ifndef _SC_STDARG_H
#define _SC_STDARG_H

#include <zport.h>

typedef Z_VA_LIST va_list;
#define va_start(ap, last) Z_VA_START(ap, last)
#define va_arg(ap, T) Z_VA_ARG(ap, T)
#define va_end(ap) Z_VA_END(ap)

#endif
