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

static inline CPnt3D operator*(const CPnt3D &v, float s)
{
    CPnt3D r;
    v.Scale(s, &r);
    return r;
}

/* the reflection of this vector in the plane whose normal is n. OPEN: the target's chain --
 * the scaled normal, its block copy, an element-wise assignment, the adds -- but it keeps this
 * vector's components in registers from the dot product, where this reloads them. */
CPnt3D CPnt3D::Reflect(const CPnt3D *n)
{
    CPnt3D r;

    r = *n * (-2.0f * Dot(*this, *n));
    r.x += x;
    r.y += y;
    r.z += z;
    return r;
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

/* The distance from this point to the segment [a, b] into *dist: to an endpoint (returns 0)
 * when the point lies beyond it, else to the line (returns 1), through two VU0 cross products. */
int CPnt3D::DistToVector(const CPnt3D *a, const CPnt3D *b, float *dist) const
{
    CPnt3D ab = *b - *a;
    CPnt3D ap = *this - *a;

    if (Dot(ab, ap) <= 0.0f) {
        *dist = sqrtf(Dot(ap, ap));
        return 0;
    }
    ab = *a - *b;
    ap = *this - *b;
    if (Dot(ab, ap) <= 0.0f) {
        *dist = sqrtf(Dot(ap, ap));
        return 0;
    }
    sceVu0FVECTOR n;
    sceVu0FVECTOR v4;
    sceVu0FVECTOR v3;
    sceVu0FVECTOR c;
    sceVu0FVECTOR v1;
    sceVu0FVECTOR v2;
    v1[0] = ap.x;
    v1[1] = ap.y;
    v1[2] = ap.z;
    v1[3] = 1.0f;
    v2[0] = ab.x;
    v2[1] = ab.y;
    v2[2] = ab.z;
    v2[3] = 1.0f;
    sceVu0OuterProduct(c, v1, v2);
    v3[0] = ab.x;
    v3[1] = ab.y;
    v3[2] = ab.z;
    v3[3] = 1.0f;
    v4[0] = c[0];
    v4[1] = c[1];
    v4[2] = c[2];
    v4[3] = 1.0f;
    sceVu0OuterProduct(n, v3, v4);
    sceVu0Normalize(n, n);
    *dist = n[0] * ap.x + n[1] * ap.y + n[2] * ap.z;
    return 1;
}
