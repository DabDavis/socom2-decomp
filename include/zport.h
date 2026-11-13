/* <zport.h>: the switch between the matching build (Metrowerks 3.0b52 and ee-gcc, scored by
 * sccheck) and a native port (-DZ_PORT=1; docs/native-port-scope.md section 9). With Z_PORT 0 --
 * the PS2 build, the only one this tree scores -- every macro here is the tokens the source spelt
 * before it existed, so it changes no object: tools/objcmp.py and the whole-tree sccheck prove that
 * for each change that routes a site through it. C and C++, and old enough for ee-gcc 2.9 (no
 * variadic macros on the PS2 side). Reached through <eetypes.h> and <stdarg.h>.
 *
 * Inline assembly is NOT wrapped here: MW's `asm { }` blocks cannot pass through a macro argument
 * reliably and an `asm T f()` function cannot be wrapped at all. A port gives each asm site
 * `#if Z_PORT <C, op by op> #else <the asm, untouched> #endif` (MW game units expand no __LINE__,
 * so a unit may grow). */
#ifndef _SC_ZPORT_H
#define _SC_ZPORT_H

#ifndef Z_PORT
#define Z_PORT 0
#endif

#if Z_PORT

/* a 16-byte quadword. clang (-m32, wasm32) has a real 128-bit integer behind mode(TI): keep it.
 * gcc -m32 has no TI mode: a 16-byte aligned struct that zero-extends a 64-bit integer, as the
 * EE's dsll/por packers do -- never a vector_size type, whose conversion from a scalar broadcasts
 * it into every lane. */
#ifdef __clang__
#define Z_QWORD_TYPEDEF(base, name) typedef base name __attribute__((mode(TI)))
#define Z_Q128(hi, lo) (((u_long128)0x##hi##ULL << 64) | (u_long128)0x##lo##ULL)
#else
/* 64 bits spelt without `long`, which a port's abi.h makes a macro (EQOA's `#define long long long`) */
typedef unsigned int z_u64 __attribute__((mode(DI)));
#ifdef __cplusplus
struct z_qword {
    unsigned int w[4];
    z_qword() = default;
    z_qword(const z_qword &) = default;
    z_qword &operator=(const z_qword &) = default;
    z_qword(z_u64 v) { w[0] = (unsigned int)v; w[1] = (unsigned int)(v >> 32); w[2] = w[3] = 0; }
    z_qword(z_u64 hi, z_u64 lo)
    {
        w[0] = (unsigned int)lo; w[1] = (unsigned int)(lo >> 32);
        w[2] = (unsigned int)hi; w[3] = (unsigned int)(hi >> 32);
    }
    operator z_u64() const { return w[0] | ((z_u64)w[1] << 32); }
} __attribute__((aligned(16)));
#define Z_Q128(hi, lo) z_qword(0x##hi##ULL, 0x##lo##ULL)
#else
struct z_qword { unsigned int w[4]; } __attribute__((aligned(16)));
#endif
#define Z_QWORD_TYPEDEF(base, name) typedef struct z_qword name
#endif

/* the EABI va_list (include/stdarg.h) becomes the host's */
#define Z_VA_LIST __builtin_va_list
#define Z_VA_START(ap, last) __builtin_va_start(ap, last)
#define Z_VA_ARG(ap, T) __builtin_va_arg(ap, T)
#define Z_VA_END(ap) __builtin_va_end(ap)

/* a layout check `typedef char x[c ? 1 : -1]`: the EE's offsets are not the host's */
#define Z_LAYOUT_OK(c) 1

/* Metrowerks' intrinsics */
#define Z_RSQRT(a, x) ((a) / __builtin_sqrtf(x))
#define Z_MEMCPY __builtin_memcpy
#define Z_I_BREAK(code) __builtin_trap()
#define Z_I_NOP() ((void)0)

#else /* the PS2: the original tokens */

#define Z_QWORD_TYPEDEF(base, name) typedef base name __attribute__((mode(TI)))
#define Z_Q128(hi, lo) 0x##hi##lo

#define Z_VA_LIST char *
#define Z_VA_START(ap, last)                                                                      \
    ((ap) = (char *)__builtin_next_arg(last) -                                                    \
            (__builtin_args_info(2) >= 8 ? 0 : (8 - __builtin_args_info(2)) * 8))
#define Z_VA_ARG(ap, T) (*(T *)(((ap) += 8) - 8))
#define Z_VA_END(ap)

#define Z_LAYOUT_OK(c) (c)

#define Z_RSQRT(a, x) __rsqrt(a, x)
#define Z_MEMCPY __memcpy
#define Z_I_BREAK(code) __I_break(code)
#define Z_I_NOP() __I_nop()

#endif

#endif
