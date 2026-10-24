/* fov.cpp -- the camera's field-of-view tests, hand-written VU0 code (OVL1 0x294A30-...). The
 * unit name and both functions are SOCOM 1's (its DWARF puts _OutsideGuardBand and
 * sceVu0ClipBBox in fov.cpp, a line per instruction: assembly). Defined in reverse of their image
 * order (-inline deferred). */

#include "../zmath/zmath.h"

/* the clip flags of eight corners (v) against the clip matrix (lc): the planes every corner is
 * outside of in the low byte, the planes any corner is outside of in the next (CCamera::TestFOV) */
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

/* whether any of n points (vm, through the matrix ms) falls outside the guard band [minv, maxv]
 * scaled by its w: the clip flags' sign bits after each subtraction (0 when none does) */
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
