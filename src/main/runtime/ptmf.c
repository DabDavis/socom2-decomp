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

/* the three words xored pairwise, each widened to a long, or-ed into one accumulator and tested
 * `!= 0` -- nonzero when the two differ; st_mpexit.cpp's `==` on two PTMFs compiles to
 * !__ptmf_cmpr(&a, &b). The accumulator is the shape (round 6): the target keeps the or-chain in the
 * first term's register (a1) -- one expression, in any of 24 orders and associations, puts the
 * first term's load in a2 and the result in v0, 7 words off. */
long __ptmf_cmpr(const __ptmf *ptmf1, const __ptmf *ptmf2)
{
    long r = (long)(ptmf1->this_delta ^ ptmf2->this_delta);
    r |= (long)(ptmf1->v_offset ^ ptmf2->v_offset);
    r |= (long)(ptmf1->f_data ^ ptmf2->f_data);
    return r != 0;
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
