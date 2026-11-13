/* GCC 2.96's own <stdarg.h> (ginclude/stdarg.h), what newlib's stdio was built against: va_list is
 * __builtin_va_list (a pointer into the EABI register save area, 4 bytes) and va_arg the builtin,
 * which reads at the pointer and then steps it by 8 (`move v0,s4; lw s8,0(v0); addiu s4,s4,8` in
 * _vfiprintf_r) -- the repo's include/stdarg.h steps first and reads at -8, which compiles the
 * same wherever a va_list is only passed on (printf, sprintf ...), not where va_arg reads one. */
#ifndef SC_GCCSTDARG_H
#define SC_GCCSTDARG_H

#include <zport.h>

typedef __builtin_va_list va_list;
#if Z_PORT
#define va_start(v, l)	__builtin_va_start(v, l)
#else
#define va_start(v, l)	__builtin_stdarg_start((v), l)
#endif
#define va_end		__builtin_va_end
#define va_arg		__builtin_va_arg

#endif
