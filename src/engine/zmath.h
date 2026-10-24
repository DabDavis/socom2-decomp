/* zMath -- the engine's vector, quaternion and matrix types (OVL1 0x306670-0x3099xx).
 * "zMath" is Zipper's own name (zMath_Init, SOCOM 1's symbols); the FILE names here are
 * placeholders until SOCOM 1's debug info names the translation units.
 *
 * Layouts are read off the code: CPnt3D is 12 bytes (CQuat::ToMatrix copies one with
 * ld/sd + lwc1/swc1), CQuat is x, y, z, w, and CMatrix is a 4x4 float matrix the
 * libvu0 calls take directly. Functions whose names are not yet known keep their
 * placeholder func_<VA> as free functions: a non-virtual const member passes `this`
 * in a0 exactly as a first pointer argument, so the code is the same either way. */
#ifndef ZMATH_H
#define ZMATH_H

#include <libvu0.h>

extern "C" float sinf(float);
extern "C" float cosf(float);
extern "C" float sqrtf(float);
extern "C" float fabsf(float);
extern "C" float acosf(float);

class CPnt3D {
public:
    float x, y, z;
};

/* A VU0 vector, 16-byte aligned for lqc2 (SOCOM 1 names CPnt4D::Rotate_AxisX/Y). */
class CPnt4D {
public:
    float x, y, z, w;
} __attribute__((aligned(16)));

/* Fill a VU0 vector from four floats. MW evaluates the arguments right to left, so a
 * copy through this loads z, y, x and stores x, y, z with w among them, through the
 * same element addresses later code reuses -- the projector code's pattern, which an
 * element-by-element fill (interleaved load/store) does not give. An inline leaves no
 * symbol: the name is ours. */
inline void VecSet(float *v, float x, float y, float z, float w)
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = w;
}

class CMatrix {
public:
    float m[4][4];

    void ToEuler(CPnt3D *out) const;
};

/* A quaternion is a CPnt3D member plus w: `*out = *this` in Slerp copies x, y, z as one
 * block (loads first) and then w -- the implicit copy of a class whose first member is a
 * struct. Derived from CPnt3D, or as four floats, MW copies all four interleaved. */
class CQuat {
public:
    CPnt3D v;
    float w;

    void Exp(const CPnt3D *axis);
    void MakeYXZRot(float ry, float rx, float rz);
    void MakeXRot(float angle);
    void MakeYRot(float angle);
    void MakeZRot(float angle);
    CQuat *Normalize();
    void Mul(const CQuat *b, CQuat *out) const;
    void Apply(const CPnt3D *in, CPnt3D *out, int n) const;
    void Slerp(const CQuat *b, float t, CQuat *out) const;
    int SlerpFaster(const CQuat *b, float t, CQuat *out) const;
    void ToEuler(CPnt3D *out) const;
};

/* Placeholders are extern "C" so they keep the map's name: C++ would mangle them. */
extern "C" {
/* CPnt3D helpers in the CPnt3D translation unit (not yet named) */
void func_00309180(const CPnt3D *v, CPnt3D *out, float s);    /* out = v * s */

void func_00309240(const CPnt3D *a, const CPnt3D *b, CPnt3D *out);   /* out = a + b */

/* CQuat's own, not yet named */
void func_003066D0(CQuat *q, const CPnt3D *axis, float t);       /* Exp of v scaled by t */
void func_00307170(const CQuat *q, CMatrix *m);               /* the rotation matrix of q */
void func_003072E0(const CQuat *q, CQuat *out);               /* out = q normalised */
void func_00307470(CQuat *q, float a, float b);               /* q = Y(a) * X(b) */

extern const CPnt3D data_003F64C0;                             /* a zero vector */
extern const float data_003DF320;                              /* 1.0f */
extern const float data_003DF328;                              /* 0.004f */

/* |*a - *b| <= *eps, as two comparisons (OVL1, in another translation unit) */
int func_0028F1E0(const float *a, const float *b, const float *eps);
}

/* By value through a temporary: CQuat::ToMatrix keeps exactly this copy (ld/sd + lwc1/swc1
 * from the add's result slot into a second one), which a plain assignment would let the
 * optimiser forward away. */
inline CPnt3D operator+(const CPnt3D &a, const CPnt3D &b)
{
    CPnt3D r;
    func_00309240(&a, &b, &r);
    return r;
}

#endif
