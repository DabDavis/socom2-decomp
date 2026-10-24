/* CPnt3D and CPnt4D (OVL1 0x308FC0-0x309370). Names are SOCOM 1's where it has them;
 * the rest keep their placeholders, as extern "C" free functions taking the object
 * first -- the same code a const member would compile to. Defined in reverse of their
 * image order (-inline deferred). The file name is a placeholder. */

#include "zmath.h"

void CPnt4D::Rotate_AxisX(float a)
{
    float c = cosf(a);
    float s = sinf(a);
    float ns = -s;
    float zz = z;
    float yy = y;

    y = yy * c + zz * ns;
    z = yy * s + zz * c;
}

void CPnt4D::Rotate_AxisY(float a)
{
    float c = cosf(a);
    float s = sinf(a);
    float zz = z;
    float xx = x;

    x = xx * c + zz * s;
    z = zz * c - xx * s;
}

extern "C" {

/* VU0: out.xyz = a * s + b, out.w kept */
void func_00309280(float *out, const float *a, const float *b, float s)
{
    float w = out[3];
    asm __volatile__ (
        "lqc2 $vf4, 0(%0)\n"
        "lqc2 $vf7, 0(%1)\n"
        "mfc1 $8, %2\n"
        "nop\n"
        "qmtc2 $8, $vf5\n"
        "vmulx.xyz $vf6, $vf4, $vf5\n"
        "vadd.xyz $vf6, $vf6, $vf7\n"
        "sqc2 $vf6, 0(%3)\n"
        : : "r"(a), "r"(b), "f"(s), "r"(out) : "$8", "memory");
    out[3] = w;
}

void func_00309240(const CPnt3D *a, const CPnt3D *b, CPnt3D *out)
{
    out->x = a->x + b->x;
    out->y = a->y + b->y;
    out->z = a->z + b->z;
}

void func_00309200(const CPnt3D *a, const CPnt3D *b, CPnt3D *out)
{
    out->x = a->x - b->x;
    out->y = a->y - b->y;
    out->z = a->z - b->z;
}

/* the difference in the ground plane: y is left alone */
void func_003091D0(const CPnt3D *a, const CPnt3D *b, CPnt3D *out)
{
    out->x = a->x - b->x;
    out->z = a->z - b->z;
}

/* a perpendicular in the ground plane */
void func_003091B0(const CPnt3D *a, CPnt3D *out)
{
    out->x = -a->z;
    out->z = a->x;
    out->y = 0.0f;
}

void func_00309180(const CPnt3D *v, CPnt3D *out, float s)
{
    out->x = v->x * s;
    out->y = v->y * s;
    out->z = v->z * s;
}

/* OPEN (both normalisers): instruction for instruction the target's, but y*y and the
 * 0.0f trade $f0 and $f1 (and inv sits in $f2, the 1.0f in v0). No compiler build,
 * member-function form, comparison form or inline Length/Scale helper changes the
 * numbering; whatever settles one settles the other. */

/* normalise in place; a zero vector is left as it is */
void func_00309110(CPnt3D *v)
{
    float len = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);

    if (len != 0.0f) {
        float inv = 1.0f / len;
        v->x = v->x * inv;
        v->y = v->y * inv;
        v->z = v->z * inv;
    }
}

/* normalise into out; nothing is written for a zero vector */
void func_003090A0(const CPnt3D *v, CPnt3D *out)
{
    float len = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);

    if (len != 0.0f) {
        float inv = 1.0f / len;
        out->x = v->x * inv;
        out->y = v->y * inv;
        out->z = v->z * inv;
    }
}

}

/* OPEN, 14 instructions off at the right size: the target loads b's three components
 * before the second loop of updates (b.x/y/z in $f0/$f3/$f2), then interleaves; scalar
 * locals give that load order but not the registers, a struct temporary spills to the
 * stack, and inline CPnt3D operators add copies. */
void CPnt3D::Interp(const CPnt3D *b, float t, CPnt3D *out) const
{
    if (t == 0.0f) {
        out->x = x;
        out->y = y;
        out->z = z;
        return;
    }
    if (t == 1.0f) {
        out->x = b->x;
        out->y = b->y;
        out->z = b->z;
        return;
    }
    float s = 1.0f - t;
    out->x = x * s;
    out->y = y * s;
    out->z = z * s;
    float bx = b->x;
    float by = b->y;
    float bz = b->z;
    out->x += bx * t;
    out->y += by * t;
    out->z += bz * t;
}
