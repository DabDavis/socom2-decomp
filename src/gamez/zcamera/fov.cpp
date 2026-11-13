/* fov.cpp -- the camera's field-of-view tests, hand-written VU0 code (OVL1 0x294A30-...). The
 * unit name and both functions are SOCOM 1's (its DWARF puts _OutsideGuardBand and
 * sceVu0ClipBBox in fov.cpp, a line per instruction: assembly). Defined in reverse of their image
 * order (-inline deferred). */

#include "../zmath/zmath.h"

/* the clip flags of eight corners (v) against the clip matrix (lc): the planes every corner is
 * outside of in the low byte, the planes any corner is outside of in the next (CCamera::TestFOV) */
#if Z_PORT
/* the port: each corner through the rows (ACC = r0*x; += r1*y; += r2*z; o = ACC + r3*w), vclipw
 * into VU0's clip register, which cfc2 reads WHOLE: the AND keeps the six newest bits (63 to
 * start), the OR takes the register's older results too -- this call's earlier corners and, for the
 * first three, whatever vclip ran before the call -- shifted up by 8 with it, as the asm does */
unsigned int sceVu0ClipBBox(float lc[4][4], PNT4D *v)
{
    float r[16];
    float p[4], o[4];
    unsigned int all = 63, any = 0, c;
    int t = 8, k;

    for (k = 0; k < 16; k++)
        r[k] = (&lc[0][0])[k];
    do {
        for (k = 0; k < 4; k++)
            p[k] = ((const float *)v)[k];
        v++;
        for (k = 0; k < 4; k++) {
            float acc = r[k] * p[0];
            acc = acc + r[4 + k] * p[1];
            acc = acc + r[8 + k] * p[2];
            o[k] = acc + r[12 + k] * p[3];
        }
        c = z_vu0_clip(o, o[3]);
        all &= c;
        any |= c;
    } while (--t);
    return all | (any << 8);
}
#else
asm unsigned int sceVu0ClipBBox(float lc[4][4], PNT4D *v)
{
    lqc2 vf4, 0(a0)
    lqc2 vf5, 16(a0)
    lqc2 vf6, 32(a0)
    lqc2 vf7, 48(a0)
    addi v0, zero, 63
    addi a0, zero, 0
    addi t0, zero, 8
    nop
loop:
    lqc2 vf8, 0(a1)
    addi a1, a1, 16
    vmulax.xyzw ACC, vf4, vf8
    vmadday.xyzw ACC, vf5, vf8
    vmaddaz.xyzw ACC, vf6, vf8
    vmaddw.xyzw vf9, vf7, vf8
    vclipw.xyz vf9, vf9
    vnop
    vnop
    vnop
    vnop
    vnop
    cfc2 v1, vi18
    and v0, v0, v1
    or a0, a0, v1
    addi t0, t0, -1
    bne zero, t0, loop
    nop
    sll a0, a0, 8
    jr ra
    or v0, v0, a0
}
#endif

/* whether any of n points (vm, through the matrix ms) falls outside the guard band [minv, maxv]
 * scaled by its w: the clip flags' sign bits after each subtraction (0 when none does) */
#if Z_PORT
/* the port: each point through the rows into p (vf8), the band's corners scaled by p.w (vmulw.xyz
 * vf11/vf12), the status flag cleared, then p - min*w and max*w - p in lanes x, y and w -- the w
 * lanes against min.w and max.w themselves (vmove.w puts them back after each pass) -- and the
 * sticky zero/sign bits those six results set (status & 0xC0) returned at the first point that
 * sets any: a result <= 0, -0 or a denormal counts. n 0 loops as the asm does */
int _OutsideGuardBand(float *minv, float *maxv, float ms[4][4], float vm[][4], int n)
{
    float r[16];
    float lo[4], hi[4], p[4], o[4], a[3], b[3];
    unsigned int st;
    int k;

    for (k = 0; k < 16; k++)
        r[k] = (&ms[0][0])[k];
    for (k = 0; k < 4; k++) {
        lo[k] = minv[k];
        hi[k] = maxv[k];
        p[k] = vm[0][k];
    }
    do {
        for (k = 0; k < 4; k++) {
            float acc = r[k] * p[0];
            acc = acc + r[4 + k] * p[1];
            acc = acc + r[8 + k] * p[2];
            o[k] = acc + r[12 + k] * p[3];
        }
        for (k = 0; k < 2; k++) {
            a[k] = lo[k] * o[3];                /* vmulw.xyz vf11, vf9, vf8 (z unused) */
            b[k] = hi[k] * o[3];                /* vmulw.xyz vf12, vf10, vf8 */
        }
        st = 0;                                 /* ctc2 zero, vi16 */
        st |= z_vu0_zs(o[0] - a[0]);            /* vsub.xyw vf11, vf8, vf11 */
        st |= z_vu0_zs(o[1] - a[1]);
        st |= z_vu0_zs(o[3] - lo[3]);
        st |= z_vu0_zs(b[0] - o[0]);            /* vsub.xyw vf12, vf12, vf8 */
        st |= z_vu0_zs(b[1] - o[1]);
        st |= z_vu0_zs(hi[3] - o[3]);
        if (st & 0xC0)
            return st & 0xC0;
        if (--n == 0)
            return 0;
        vm++;                                   /* (the asm loads the next point first, the one */
        for (k = 0; k < 4; k++)                 /* past the end too; unused there) */
            p[k] = vm[0][k];
    } while (1);
}
#else
asm int _OutsideGuardBand(float *minv, float *maxv, float ms[4][4], float vm[][4], int n)
{
    lqc2 vf8, 0(a3)
    lqc2 vf4, 0(a2)
    lqc2 vf5, 16(a2)
    lqc2 vf6, 32(a2)
    lqc2 vf7, 48(a2)
    lqc2 vf9, 0(a0)
    lqc2 vf10, 0(a1)
    lqc2 vf11, 0(a0)
    lqc2 vf12, 0(a1)
    nop
loop:
    vmulax.xyzw ACC, vf4, vf8
    vmadday.xyzw ACC, vf5, vf8
    vmaddaz.xyzw ACC, vf6, vf8
    vmaddw.xyzw vf8, vf7, vf8
    vmulw.xyz vf11, vf9, vf8
    vmulw.xyz vf12, vf10, vf8
    vnop
    vnop
    ctc2 zero, vi16
    vsub.xyw vf11, vf8, vf11
    vsub.xyw vf12, vf12, vf8
    vmove.w vf11, vf9
    vmove.w vf12, vf10
    vnop
    addi a3, a3, 16
    lqc2 vf8, 0(a3)
    addi t0, t0, -1
    cfc2 v0, vi16
    andi v0, v0, 0xc0
    bnez v0, out
    nop
    bne zero, t0, loop
    nop
    addi v0, zero, 0
out:
    jr ra
    nop
}
#endif
