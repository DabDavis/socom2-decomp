/* <zport_ee.h>: what the port's C stands on where the PS2 source has inline assembly (the port's
 * M1, docs/native-port-scope.md sections 1.3, 9, 10). Reached through <zport.h> only when Z_PORT
 * is set: the PS2 build never reads this file. Each asm site in the tree is
 * `#if Z_PORT <C> #else <the asm, untouched> #endif`, and the C uses what is declared here:
 *
 *  - fences (`sync`, `sync.l`, `sync.p`) and an interrupt handler's closing `ei`: no hardware to
 *    order on the host, so a compiler barrier and nothing;
 *  - the EE's hardware the asm reads directly (COP0's Count): a hook the platform implements;
 *  - VU0. Macro-mode maths (lqc2/sqc2, v* ops, the ACC) is written as C lane by lane in the asm's
 *    operation order, each op a float rounding of its own (the port builds with -ffp-contract=off
 *    and SSE floats), its registers locals of the function -- except where the state reaches a
 *    VU0 MICRO program (vcallms) or the next function: there the registers are z_vu0's, the port's
 *    VU0 register file, which the platform defines and whose micro-mode runs z_vu0_callms. The
 *    helpers below give the VU's own answers where its arithmetic is not IEEE's (ftoi's
 *    saturation, the clip flags' compare, div by zero, the zero/sign flags): PCSX2's VUops.cpp
 *    and VUflags.cpp, the reference this port follows for VU0.
 *
 * Every hook's name starts z_; the platform's list is socom-port's README (M1). */
#ifndef _SC_ZPORT_EE_H
#define _SC_ZPORT_EE_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- fences ---------------------------------------------------------------------------------- */

/* sync / sync.l / sync.p: the EE orders its memory and register accesses. The host's memory-mapped
 * registers are the platform's, called in program order: only the compiler must not move accesses
 * across the fence */
#define Z_EE_SYNC() __atomic_signal_fence(__ATOMIC_SEQ_CST)

/* `ei` as the last instruction of an interrupt handler (Sony's convention: the handler re-enables
 * what the dispatcher disabled). The platform's dispatcher calls the handler and restores its own
 * state; nothing to do here */
#define Z_EE_EI() ((void)0)

/* ---- the EE's hardware ---------------------------------------------------------------------- */

/* mfc0 $9 (COP0 Count): the EE's free-running cycle counter, 294.912 MHz, wrapping at 2^32 (the
 * Medius clock, rt_audio.c and rt_time_platform.c, divides it into milliseconds) */
unsigned int z_ee_cop0_count(void);

/* ---- VU0 ------------------------------------------------------------------------------------ */

/* VU0's register file: what macro-mode code leaves for a micro program or for the next function,
 * and what a micro program hands back. vf[0] is (0, 0, 0, 1) and never written; vi[0] is 0. The
 * platform defines z_vu0 (with vf[0] set) */
typedef struct z_vu0_regs {
    float vf[32][4];        /* vf0-vf31, lanes x y z w */
    float acc[4];           /* ACC */
    float q, p, i, r;       /* Q (vdiv/vsqrt/vrsqrt), P (unused by VU0), I, R */
    unsigned int vi[16];    /* vi0-vi15, 16 bits used */
    unsigned int status;    /* vi16 */
    unsigned int mac;       /* vi17 */
    unsigned int clip;      /* vi18: the last four vclip results, six bits each, the newest low */
} z_vu0_regs;

extern z_vu0_regs z_vu0;

/* vcallms ADDR (ADDR a byte address in VU0's micro memory) followed by the waits the EE code does
 * (`qmtc2.i`, `vnop`, `cfc2.i`): run the micro program loaded there on z_vu0 to its [E] bit and
 * return. The microcode is what the game DMAs to VIF0 (CVU::Code: zMath_Init's vu0_CodeSegment,
 * OVL1 0x3D7540 in r0001) or copies into VU0's micro memory (Nellymoser's FFT, 0x3D5980); a C
 * translation per entry point or the VU1 interpreter EQOA's port has (VU0 is the same ISA) */
void z_vu0_callms(unsigned int addr);

/* lqc2 vfN, 0(p) and sqc2 vfN, 0(p) on z_vu0: a quadword copied whole (a write to vf0 is lost, as
 * on VU0) */
static inline void z_vu0_lqc2(int n, const void *p)
{
    const float *s = (const float *)p;

    if (n) {
        z_vu0.vf[n][0] = s[0];
        z_vu0.vf[n][1] = s[1];
        z_vu0.vf[n][2] = s[2];
        z_vu0.vf[n][3] = s[3];
    }
}

static inline void z_vu0_sqc2(int n, void *p)
{
    float *d = (float *)p;

    d[0] = z_vu0.vf[n][0];
    d[1] = z_vu0.vf[n][1];
    d[2] = z_vu0.vf[n][2];
    d[3] = z_vu0.vf[n][3];
}

/* one lane through vuDouble (PCSX2): a denormal is a signed zero; VU0 has no inf/NaN (an all-ones
 * exponent is clamped to the largest finite value) */
static inline float z_vu0_lane(float f)
{
    union { float f; unsigned int u; } v;

    v.f = f;
    if ((v.u & 0x7F800000) == 0)
        v.u &= 0x80000000;
    else if ((v.u & 0x7F800000) == 0x7F800000)
        v.u = (v.u & 0x80000000) | 0x7F7FFFFF;
    return v.f;
}

/* vftoi0/4/12/15: f * 2^frac truncated toward zero; |f| >= 2^31 saturates to 0x7FFFFFFF or
 * 0x80000000 by the sign (PCSX2's floatToInt) where C's conversion is undefined */
static inline int z_vu0_ftoi(float f, int frac)
{
    union { float f; unsigned int u; } v;

    v.f = f;
    if (frac) {
        union { float f; unsigned int u; } s;

        s.u = 0x3F800000 + ((unsigned int)frac << 23);
        v.f = f * s.f;
    }
    if ((v.u & 0x7F800000) >= 0x4F000000)
        return (v.u & 0x80000000) ? (int)0x80000000 : 0x7FFFFFFF;
    return (int)v.f;
}

/* vitof0/4/12/15: the integer as a float, times 2^-frac (PCSX2's intToFloat: the conversion rounds,
 * the scale is exact) */
static inline float z_vu0_itof(int i, int frac)
{
    float f = (float)i;

    if (frac) {
        union { float f; unsigned int u; } s;

        s.u = 0x3F800000 - ((unsigned int)frac << 23);
        f = f * s.f;
    }
    return f;
}

/* vdiv Q, fs, ft: fs / ft, and by zero the largest finite value signed as the two signs' XOR
 * (PCSX2's _vuDIV) */
static inline float z_vu0_div(float fs, float ft)
{
    union { float f; unsigned int u; } a, b, q;

    a.f = z_vu0_lane(fs);
    b.f = z_vu0_lane(ft);
    if (b.f == 0.0f) {
        q.u = ((a.u ^ b.u) & 0x80000000) ? 0xFF7FFFFF : 0x7F7FFFFF;
        return q.f;
    }
    return z_vu0_lane(a.f / b.f);
}

/* vclip.xyz fs, ftw: the six bits of one vclip -- x > +|w|, x < -|w|, y, z likewise -- compared as
 * PCSX2 does, as integers on the bit patterns (a denormal |w| as the largest denormal, so only a
 * normal value compares above it). The caller shifts them into z_vu0.clip */
static inline unsigned int z_vu0_clipbits(const float *fs, float w)
{
    union { float f; int i; } x, y, z, ww;
    int value;
    unsigned int c = 0;

    x.f = fs[0];
    y.f = fs[1];
    z.f = fs[2];
    ww.f = w;
    value = (ww.i & 0x7F800000) ? (ww.i & 0x7FFFFFFF) : 0x007FFFFF;
    if (x.i > value)
        c |= 0x01;
    if ((int)((unsigned int)x.i ^ 0x80000000u) > value)
        c |= 0x02;
    if (y.i > value)
        c |= 0x04;
    if ((int)((unsigned int)y.i ^ 0x80000000u) > value)
        c |= 0x08;
    if (z.i > value)
        c |= 0x10;
    if ((int)((unsigned int)z.i ^ 0x80000000u) > value)
        c |= 0x20;
    return c;
}

/* vclip into the clip flag register: the older results shifted up, 24 bits kept */
static inline unsigned int z_vu0_clip(const float *fs, float w)
{
    z_vu0.clip = ((z_vu0.clip << 6) | z_vu0_clipbits(fs, w)) & 0xFFFFFF;
    return z_vu0.clip;
}

/* the status flag's sticky zero and sign bits (ZS 0x40, SS 0x80) one upper op's lane result sets
 * (PCSX2's VU_MAC_UPDATE: sign from the sign bit, -0 included; zero for 0 and for a denormal,
 * which VU0 flushes) */
static inline unsigned int z_vu0_zs(float r)
{
    union { float f; unsigned int u; } v;
    unsigned int s = 0;

    v.f = r;
    if (v.u & 0x80000000)
        s |= 0x80;
    if ((v.u & 0x7F800000) == 0)
        s |= 0x40;
    return s;
}

#ifdef __cplusplus
}
#endif

#endif
