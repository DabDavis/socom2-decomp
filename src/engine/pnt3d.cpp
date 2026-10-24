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

/* VU0: out.xyz = a * s + b, out.w kept (new in SOCOM II: no SOCOM 1 name) */
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

}

void CPnt3D::Add(const CPnt3D *b, CPnt3D *out) const
{
    out->x = x + b->x;
    out->y = y + b->y;
    out->z = z + b->z;
}

void CPnt3D::Sub(const CPnt3D *b, CPnt3D *out) const
{
    out->x = x - b->x;
    out->y = y - b->y;
    out->z = z - b->z;
}

/* the difference in the ground plane: y is left alone */
void CPnt3D::SubXZ(const CPnt3D *b, CPnt3D *out) const
{
    out->x = x - b->x;
    out->z = z - b->z;
}

/* a perpendicular in the ground plane */
void CPnt3D::ToRightXZ(CPnt3D *out) const
{
    out->x = -z;
    out->z = x;
    out->y = 0.0f;
}

void CPnt3D::Scale(float s, CPnt3D *out) const
{
    out->x = x * s;
    out->y = y * s;
    out->z = z * s;
}

/* OPEN (both normalisers): instruction for instruction the target's, but y*y and the
 * 0.0f trade $f0 and $f1 (and inv sits in $f2, the 1.0f in v0). No compiler build,
 * member-function form, comparison form or inline Length/Scale helper changes the
 * numbering; whatever settles one settles the other. */

/* normalise in place; a zero vector is left as it is */
void CPnt3D::Normalize()
{
    float len = sqrtf(x * x + y * y + z * z);

    if (len != 0.0f) {
        float inv = 1.0f / len;
        x = x * inv;
        y = y * inv;
        z = z * inv;
    }
}

/* normalise into out; nothing is written for a zero vector */
void CPnt3D::Normalize(CPnt3D *out) const
{
    float len = sqrtf(x * x + y * y + z * z);

    if (len != 0.0f) {
        float inv = 1.0f / len;
        out->x = x * inv;
        out->y = y * inv;
        out->z = z * inv;
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
