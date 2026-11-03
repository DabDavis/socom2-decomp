/* sc: compiler=mwcps2-3.0b52-030722; flags=-O4,p -sdatathreshold 0 -inline auto,deferred,bottomup,level=5 -enum min -Cpp_exceptions on */
/* ptmf.c -- Metrowerks' C++ runtime: pointers to member functions (MAIN 0x1828A0-0x18292B). The
 * file name is the PowerPC runtime's, which holds __ptmf_test, __ptmf_cmpr and __ptmf_scall in
 * that order; this image emits them reversed (deferred), __ptmf_test stripped. A PTMF is three
 * words: the `this` adjustment, the vtable offset (negative: not virtual), the function or the
 * vtable slot.
 *
 * __ptmf_scall is assembly on both targets: the call through a PTMF arrives with the PTMF in t9
 * and the object in a0, adjusts a0 and jumps to the function. Its row was 0x40 in the seeded map:
 * three words of zero padding counted in (SOCOM 1's symbol is 0x34, its padding the same). */

typedef struct __ptmf {
    int this_delta;
    int v_offset;
    int f_data;
} __ptmf;

/* OPEN, 19 instructions (7 differ): the three words xored pairwise, each widened to a long, or-ed,
 * tested `!= 0` -- nonzero when the two differ; st_mpexit.cpp's `==` on two PTMFs compiles to
 * !__ptmf_cmpr(&a, &b). The target loads a's 0 and 4 into t0/a2 and or-s (x0 | x4) | x8; every
 * order and association of the three terms (24 spellings, both operand orders) gives this
 * compiler's other allocation (x4 in t0), 7 words off at best; `!=` chains branch (16-19 words). */
long __ptmf_cmpr(const __ptmf *ptmf1, const __ptmf *ptmf2)
{
    return ((long)(ptmf1->this_delta ^ ptmf2->this_delta) | (long)(ptmf1->v_offset ^ ptmf2->v_offset)
            | (long)(ptmf1->f_data ^ ptmf2->f_data)) != 0;
}

asm void __ptmf_scall(void)
{
    .set noreorder
    lw at, 0(t9)
    lw t8, 4(t9)
    lw t9, 8(t9)
    bltz t8, L1
    addu a0, a0, at
    addu t9, a0, t9
    lw t9, 0(t9)
    nop
    addu t9, t9, t8
    lw t9, 0(t9)
    nop
L1:
    jr t9
    nop
}
