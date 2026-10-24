/* <stdarg.h>: the EE's EABI va_list as MW builds it -- a pointer into the register save area,
 * started (8 - named argument registers) slots below its top, each argument 8 bytes. The
 * conditional's sense is the shipped code's (CZAnim::ActivateArg: slti, bnez to the
 * computation, the zero case falling through); with < it inverts, and GCC's args_info(0) form
 * crashes MW. */
#ifndef _SC_STDARG_H
#define _SC_STDARG_H

typedef char *va_list;
#define va_start(ap, last)                                                                        \
    ((ap) = (char *)__builtin_next_arg(last) -                                                    \
            (__builtin_args_info(2) >= 8 ? 0 : (8 - __builtin_args_info(2)) * 8))
#define va_arg(ap, T) (*(T *)(((ap) += 8) - 8))
#define va_end(ap)

#endif
