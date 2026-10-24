/* zMath -- the engine's vector, quaternion and matrix types (OVL1 0x306670-0x30954C), in four
 * units the demo's static initialisers name, in link order: zmath_main.cpp (zMath_Init, CQuat,
 * the box test, CBits), zmath_matrix.cpp (CStack, CMatrix), zmath_pnt3d.cpp, zmath_pnt4d.cpp.
 * Layouts are SOCOM 1's DWARF (SCUS_972.05); member functions are read off the code. The
 * header's own name is ours (the demo's DWARF names a zmath_matrix.h, which this folds in).
 *
 * The constructors are inline: the units' statics (CPnt3D::zero, CQuat::identity, ...) are
 * built by each unit's __sinit, and `CMatrix m;` costs a sceVu0UnitMatrix wherever it is
 * written (CQuat::ToEuler). A function only some code calls, still unnamed, is an extern "C"
 * placeholder taking the object first -- the code a member would compile to. */
#ifndef ZMATH_H
#define ZMATH_H

#include <libvu0.h>

extern "C" float sinf(float);
extern "C" float cosf(float);
extern "C" float sqrtf(float);
extern "C" float fabsf(float);
extern "C" float acosf(float);
extern "C" float asinf(float);
extern "C" float atan2f(float, float);
extern "C" double fabs(double);
extern "C" double exp(double);

/* The C structs the classes extend (SOCOM 1's DWARF). */
struct PNT3D {
    float x, y, z;
};

struct PNT4D {
    float x, y, z, w;
};

class CPnt3D : public PNT3D {
public:
    CPnt3D() {}
    CPnt3D(float ax, float ay, float az)
    {
        x = ax;
        y = ay;
        z = az;
    }

    int DistToVector(const CPnt3D *a, const CPnt3D *b, float *dist) const;
    int DistToVector(const CPnt3D *a, const CPnt3D *b, CPnt3D *nearest, float *dist) const;
    CPnt3D Reflect(const CPnt3D *n);
    void Interp(const CPnt3D *b, float t, CPnt3D *out) const;
    float Normalize(CPnt3D *out) const;
    float Normalize();
    void Scale(float s, CPnt3D *out) const;
    void ToRightXZ(CPnt3D *out) const;
    void SubXZ(const CPnt3D *b, CPnt3D *out) const;
    void Sub(const CPnt3D *b, CPnt3D *out) const;
    void Add(const CPnt3D *b, CPnt3D *out) const;

    static const CPnt3D zero;
    static const CPnt3D unit_x;
    static const CPnt3D unit_y;
    static const CPnt3D unit_z;
    static const CPnt3D unit_nz;
};

/* A VU0 vector, 16-byte aligned for lqc2. */
class CPnt4D : public PNT4D {
public:
    CPnt4D() {}
    CPnt4D(float ax, float ay, float az, float aw)
    {
        x = ax;
        y = ay;
        z = az;
        w = aw;
    }

    void Rotate_AxisX(float a);
    void Rotate_AxisY(float a);

    static const CPnt4D zero;
    static const CPnt4D unit_x;
    static const CPnt4D unit_y;
    static const CPnt4D unit_z;
    static const CPnt4D unit_w;
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

/* two corners, 24 bytes */
class CBBox {
public:
    CPnt3D m_min;
    CPnt3D m_max;

    int GetIntersect(CPnt4D *a, CPnt4D *b, CPnt4D *out, CPnt4D *out2, bool flag);
    /* the centre and the size, each from one read of the box (CCamera::InsideMidPlane reads its
     * node's box once per line; the names are ours) */
    void GetCenter(CPnt3D *out) const { m_min.Interp(&m_max, 0.5f, out); }
    void GetSize(CPnt3D *out) const { m_max.Sub(&m_min, out); }
};

class CQuat;

/* 16-byte aligned, which no DWARF says outright: SOCOM 1's CNode is 0xC0 bytes with its last
 * member ending at 0xBC, every CMatrix member its DWARF lists sits on a 16-byte boundary, and
 * GetDatabaseNode finds CWorld's landmarks where a 0xC0-byte CNode puts them. */
class CMatrix {
public:
    float m_matrix[4][4];

    CMatrix() { sceVu0UnitMatrix(m_matrix); }
    /* an element (SOCOM 1 links an out-of-line copy, __cl__7CMatrixCFUiUi) */
    float &operator()(unsigned int r, unsigned int c) const { return (float &)m_matrix[r][c]; }
    CMatrix(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13,
            float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33);

    void SetRows(const CPnt3D *r0, const CPnt3D *r1, const CPnt3D *r2, const CPnt3D *r3);
    void SetRotate(const CPnt3D *euler);
    CQuat *ToQuat(CQuat *q) const;
    void SetZero();
    void ToEuler(CPnt3D *out) const;
    /* points through the matrix in place, one at a time on VU0 (SOCOM 1's zmath_matrix.h has it
     * inline; SOCOM II calls one copy, 0x308640) */
    void Transform(CPnt3D *inout, int count) const;

    static const CMatrix identity;
} __attribute__((aligned(16)));

/* The matrix stack: m_top points into m_stack, m_pointer counts the pushes. Multiply is a
 * member that never reads `this` (a0 is taken and dropped). */
class CStack {
public:
    CMatrix *Multiply(const CMatrix *m, bool push);

    static CMatrix m_stack[64];
    static int m_pointer;
    static CMatrix *m_top;
};

/* A quaternion is a CPnt3D member plus w: `*out = *this` in Slerp copies x, y, z as one
 * block (loads first) and then w -- the implicit copy of a class whose first member is a
 * struct. Derived from CPnt3D, or as four floats, MW copies all four interleaved. */
class CQuat {
public:
    CPnt3D vec;
    float w;

    CQuat() {}
    CQuat(float x, float y, float z, float aw)      /* w first: __sinit_zmath_main.cpp's order */
    {
        w = aw;
        vec.x = x;
        vec.y = y;
        vec.z = z;
    }

    void Exp(const CPnt3D *axis, float t);
    void Exp(const CPnt3D *axis);
    void Slerp(const CQuat *b, float t, CQuat *out) const;
    int SlerpFaster(const CQuat *b, float t, CQuat *out) const;
    void Align(const CPnt3D &from, const CPnt3D &to);
    void Apply(const CPnt3D *in, CPnt3D *out, int n) const;
    void ToEuler(CPnt3D *out) const;
    void Mul(const CQuat *b, CQuat *out) const;
    void ToMatrix(CMatrix *m) const;
    CQuat *Normalize();
    void Normalize(CQuat *out) const;
    void MakeYXZRot(float ry, float rx, float rz);
    void MakeYXRot(float ry, float rx);
    void MakeZRot(float angle);
    void MakeYRot(float angle);
    void MakeXRot(float angle);

    static const CQuat zero;
    static const CQuat identity;
};

/* the allocator's entry points; a release build passes no file and line (SOCOM 1's passed them) */
void *_malloc(unsigned int size, const char *file, int line);
void *_realloc(void *p, unsigned int size, const char *file, int line);

/* A variable-length bit set (zmath_bits.h in SOCOM 1's asserts). */
class CBits {
public:
    unsigned int m_bits;
    unsigned int *m_vec;

    void Clear(unsigned int i) { m_vec[i >> 5] &= ~(1 << (i & 31)); }
    int Resize(unsigned int bits);
};

/* |a - b| <= eps, out of line wherever it is instantiated (SOCOM 1: _close_enough<f>) */
template <class T>
bool _close_enough(const T &a, const T &b, const T &eps)
{
    T d = a - b;
    return d >= -eps && d <= eps;
}

/* By value through a temporary: CQuat::ToMatrix keeps exactly this copy (ld/sd + lwc1/swc1
 * from the add's result slot into a second one), which a plain assignment would let the
 * optimiser forward away. */
inline CPnt3D operator+(const CPnt3D &a, const CPnt3D &b)
{
    CPnt3D r;
    a.Add(&b, &r);
    return r;
}

inline CPnt3D operator-(const CPnt3D &a, const CPnt3D &b)
{
    CPnt3D r;
    a.Sub(&b, &r);
    return r;
}

inline CPnt3D operator*(const CPnt3D &v, float s)
{
    CPnt3D r;
    v.Scale(s, &r);
    return r;
}

inline bool operator==(const CPnt3D &a, const CPnt3D &b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

/* by value through a temporary, as operator+: OBJECT_AIM's begin keeps both copies */
inline CQuat operator*(const CQuat &a, const CQuat &b)
{
    CQuat r;
    a.Mul(&b, &r);
    return r;
}

inline float Dot(const CPnt3D &a, const CPnt3D &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

void Vu0MatrixInverseWithUniformScale(CMatrix *dst, CMatrix *src);

float exp_tl(float x);
void uninit_trig_table();
void init_trig_table();
void zMath_Init();

#endif
