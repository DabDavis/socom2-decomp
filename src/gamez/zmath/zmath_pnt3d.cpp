/* zmath_pnt3d.cpp -- CPnt3D (OVL1 0x308800-0x309270). The unit name is SOCOM 1's
 * (__sinit_zmath_pnt3d.cpp, which builds the statics below). Defined in reverse of their
 * image order (-inline deferred). */

#include "zmath.h"

const CPnt3D CPnt3D::zero(0.0f, 0.0f, 0.0f);
const CPnt3D CPnt3D::unit_x(1.0f, 0.0f, 0.0f);
const CPnt3D CPnt3D::unit_y(0.0f, 1.0f, 0.0f);
const CPnt3D CPnt3D::unit_z(0.0f, 0.0f, 1.0f);
const CPnt3D CPnt3D::unit_nz(0.0f, 0.0f, -1.0f);

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

/* Both normalisers return the length: kept in $f0, the return register, it is what fixes
 * every other register (as a void function, MW numbers them the other way round). */

/* normalise in place and return the length; a zero vector is left as it is */
float CPnt3D::Normalize()
{
    float len = sqrtf(x * x + y * y + z * z);

    if (len != 0.0f) {
        float inv = 1.0f / len;
        x = x * inv;
        y = y * inv;
        z = z * inv;
    }
    return len;
}

/* normalise into out and return the length; nothing is written for a zero vector */
float CPnt3D::Normalize(CPnt3D *out) const
{
    float len = sqrtf(x * x + y * y + z * z);

    if (len != 0.0f) {
        float inv = 1.0f / len;
        out->x = x * inv;
        out->y = y * inv;
        out->z = z * inv;
    }
    return len;
}

/* the reflection of this vector in the plane whose normal is n */
CPnt3D CPnt3D::Reflect(const CPnt3D *n)
{
    float d = x * n->x + y * n->y + z * n->z;
    CPnt3D t;
    n->Scale(-2.0f * d, &t);
    return t + *this;
}

/* A lerp through the unit's own Scale and Add, which MW inlines here (-inline auto, the same
 * unit) and cannot inline into CQuat's code in zmath_main.cpp: the image keeps that split. */
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
    CPnt3D tmp;
    Scale(1.0f - t, out);
    b->Scale(t, &tmp);
    out->Add(&tmp, out);
}
