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
#include <string.h>

extern "C" float sinf(float);
extern "C" float cosf(float);
extern "C" float sqrtf(float);
extern "C" float fabsf(float);
extern "C" float acosf(float);
extern "C" float asinf(float);
extern "C" float atan2f(float, float);
extern "C" double fabs(double);
extern "C" double exp(double);

/* MIN and MAX, as macros: CCamera::GetFogVisibility clamps with a MAX then a MIN, the two
 * ternaries a pair of ifs does not compile to */
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* The C structs the classes extend (SOCOM 1's DWARF). */
struct PNT2D {
    float x, y;
};

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
    /* inline (GetFogVisibility's distance arrives in a register of its own; the name is ours) */
    float Length() const { return sqrtf(x * x + y * y + z * z); }
    /* scaled, a point added, in place (the AI lane: CAiMap::GetCellCenter's out-of-line copies,
     * a tail call each -- OVL1 0x2105F0/0x2105E0, OVL2 0x5168F0/0x5168E0) */
    void Scale(float s) { Scale(s, this); }
    void Add(const CPnt3D *b) { Add(b, this); }
    /* into itself (CZAnimFromToAcc::Tick adds a product's temporary straight onto the point) */
    CPnt3D &operator+=(const CPnt3D &b)
    {
        Add(&b, this);
        return *this;
    }
    /* by value through a temporary, const members as SOCOM II's own copies are named
     * (__pl__6CPnt3DCFRC6CPnt3D 0x557DC0, __ml__6CPnt3DCFf 0x557E10, the minus 0x557E90): CQuat::
     * ToMatrix keeps exactly this copy (ld/sd + lwc1/swc1 from the add's result slot into a second
     * one), which a plain assignment would let the optimiser forward away */
    CPnt3D operator+(const CPnt3D &b) const;
    CPnt3D operator-(const CPnt3D &b) const;
    CPnt3D operator*(float s) const;
    /* the cross product (0x592400, OVL2: SOCOM II's, not defined here) */
    void Cross(const CPnt3D *b, CPnt3D *out, bool normalize) const;

    static const CPnt3D zero;
    static const CPnt3D unit_x;
    static const CPnt3D unit_y;
    static const CPnt3D unit_z;
    static const CPnt3D unit_nz;
};

/* A VU0 vector, 16-byte aligned for lqc2. */
/* SOCOM II's CPnt4D has a written copy constructor through PNT4D's (its copy 0x328D60;
 * zparticle.h's stand-in declares it), but not here: declared, it loses SeeFarther (thezoom.cpp,
 * whose 128-bit copies need the implicit one) and gains nothing in any hand-written unit
 * (re-measured by the MSL lane, 2026-10-25: 3005 -> 3004) */
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
    /* by value through VU0, each result copied out of its temporary (CCamera::Update's near
     * plane point) */
    CPnt4D operator*(float s) const
    {
        CPnt4D r;
        sceVu0ScaleVector((float *)&r, (float *)this, s);
        return r;
    }
    CPnt4D operator+(const CPnt4D &b) const
    {
        CPnt4D r;
        sceVu0AddVector((float *)&r, (float *)this, (float *)&b);
        return r;
    }

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
    /* each axis's corners swapped where the minimum exceeds the maximum (SOCOM 1's; SOCOM II's
     * is OVL2 0x56AC70, defined in seal_ai.cpp) */
    void Normalize();
};

class CQuat;

/* 16-byte aligned, which no DWARF says outright: SOCOM 1's CNode is 0xC0 bytes with its last
 * member ending at 0xBC, every CMatrix member its DWARF lists sits on a 16-byte boundary, and
 * GetDatabaseNode finds CWorld's landmarks where a 0xC0-byte CNode puts them. */
class CMatrix {
public:
    float m_matrix[4][4];

    CMatrix() { sceVu0UnitMatrix(m_matrix); }
    /* the elements as the array VU0 takes (SOCOM 1 links the const one, __opRA4_A4_Cf__7CMatrixCFv;
     * CNode::BuildMTW's inner copies call both) */
    typedef float Rows[4][4];
    typedef const float CRows[4][4];
    operator Rows &() { return m_matrix; }
    operator CRows &() const { return m_matrix; }
    /* the unit matrix (BuildMTW's innermost copy calls it; the name is ours) */
    void SetIdentity() { sceVu0UnitMatrix(m_matrix); }
    /* out = b * this (SOCOM 1's Multiply, out of line wherever an inlining runs out) */
    void Multiply(const CMatrix *b, CMatrix *out) const
    {
        sceVu0MulMatrix((Rows &)*out, (float (*)[4])(CRows &)*b, (float (*)[4])(CRows &)*this);
    }
    /* copied on VU0 (CNode::ComputeFacadeMatrix: a product's return copied out, then assigned) */
    CMatrix(const CMatrix &m) { sceVu0CopyMatrix(m_matrix, (float (*)[4])m.m_matrix); }
    CMatrix &operator=(const CMatrix &m)
    {
        sceVu0CopyMatrix(m_matrix, (float (*)[4])m.m_matrix);
        return *this;
    }
    /* by value, the product kept in a named local (so copied out) */
    CMatrix operator*(const CMatrix &b) const
    {
        CMatrix r;

        sceVu0MulMatrix(r.m_matrix, (float (*)[4])m_matrix, (float (*)[4])b.m_matrix);
        return r;
    }
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
    /* four-element vectors through the matrix, n of them (VU0; SOCOM II's 0x308580, new since
     * SOCOM 1 inlined its one use; the name is ours) */
    void Transform(const CPnt4D *in, CPnt4D *out, int n) const;
    void Transform(const CPnt3D *in, CPnt3D *out, int n) const;   /* w = 1 (SOCOM 1's) */

    static const CMatrix identity;
} __attribute__((aligned(16)));

/* The matrix stack: m_top points into m_stack, m_pointer counts the pushes. Multiply is a
 * member that never reads `this` (a0 is taken and dropped). */
class CStack {
public:
    /* an owner pushes the unit matrix and pops it again (the destructor's out-of-line copy is
     * 0x2D4FE0; the member's name is ours) */
    CStack() : m_own(true) { Push(&CMatrix::identity); }
    /* a stack that pushes nothing (grid_di.cpp's file static: its initialiser clears the owner byte
     * alone, as SOCOM 1's) */
    CStack(bool own) : m_own(own) {}
    ~CStack()
    {
        if (m_own)
            Pop();
    }
    void Push(const CMatrix *m)
    {
        m_pointer++;
        m_top++;
        sceVu0CopyMatrix(m_top->m_matrix, (float (*)[4])m->m_matrix);
    }
    void Pop()
    {
        m_pointer--;
        m_top--;
    }
    CMatrix *Multiply(const CMatrix *m, bool push);

    bool m_own;

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

/* SOCOM 1's zmath_matrix.h VU0 helpers, inline: a matrix's rows into vf4-vf7, then a point
 * through them (w = 1) or a vector through the rotation alone, as often as wanted. SOCOM 1's
 * line table puts each helper's code on its caller's one line. GCC-style (operands bound left
 * to right: Transform loads &input before &output; an asm block takes them in the arguments'
 * right-to-left order) */
inline void _LoadMatrix(const CMatrix *m)
{
    asm __volatile__(
        "lqc2 $vf4, 0x0(%0)\n"
        "lqc2 $vf5, 0x10(%0)\n"
        "lqc2 $vf6, 0x20(%0)\n"
        "lqc2 $vf7, 0x30(%0)\n"
        : : "r"(m) : "memory");
}

inline void _TransformPointXYZ(const CPnt4D *in, CPnt4D *out)
{
    asm __volatile__(
        "lqc2 $vf8, 0(%0)\n"
        "vmulax.xyz $ACC, $vf4, $vf8\n"
        "vmadday.xyz $ACC, $vf5, $vf8\n"
        "vmaddaz.xyz $ACC, $vf6, $vf8\n"
        "vmaddw.xyz $vf9, $vf7, $vf0\n"
        "sqc2 $vf9, 0(%1)\n"
        : : "r"(in), "r"(out) : "memory");
}

inline void _TransformVectorXYZ(const CPnt4D *in, CPnt4D *out)
{
    asm __volatile__(
        "lqc2 $vf8, 0(%0)\n"
        "vmulax.xyz $ACC, $vf4, $vf8\n"
        "vmadday.xyz $ACC, $vf5, $vf8\n"
        "vmaddz.xyz $vf9, $vf6, $vf8\n"
        "sqc2 $vf9, 0(%1)\n"
        : : "r"(in), "r"(out) : "memory");
}

/* vectors through the matrix's rotation (no translation), each copied into an aligned vector and
 * back: inline (ZAnim's SCROLL_MOTION tick; the name is ours) */
inline void TransformVectors(const CMatrix *m, const CPnt3D *in, CPnt3D *out, int count)
{
    CPnt4D input;
    CPnt4D output;

    _LoadMatrix(m);
    do {
        input.x = in->x;
        input.y = in->y;
        input.z = in->z;

        _TransformVectorXYZ(&input, &output);

        out->x = output.x;
        out->y = output.y;
        out->z = output.z;
        in++;
        out++;
    } while (--count);
}

/* the allocator's entry points; a release build passes no file and line (SOCOM 1's passed them) */
void *_malloc(unsigned int size, const char *file, int line);
void *_realloc(void *p, unsigned int size, const char *file, int line);
/* free and strdup with a file and line, both 0 here (SOCOM 1's _free__FPvPCci and
 * _strdup__FPCcPCci: CNode::SetName and SetModelname call them where SOCOM II's call these) */
void _free(void *p, const char *file, int line);
char *_strdup(const char *s, const char *file, int line);

/* A variable-length bit set (zmath_bits.h in SOCOM 1's asserts). */
class CBits {
public:
    /* empty, then sized (CAiEvent's constructor inlines it: both words zeroed, Resize called) */
    CBits(unsigned int bits) : m_bits(0), m_vec(0) { Resize(bits); }
    /* the words freed with no file or line (SOCOM 1's out of line, 0x17F440 in the demo; SOCOM
     * II's inlines, its this-test kept: CAiEvent's deletes) */
    ~CBits()
    {
        _free(m_vec, 0, 0);
        m_vec = 0;
    }
    /* the words that hold so many bits, the count signed (SOCOM 1's GetWordCount__5CBitsCFi:
     * sra) */
    int GetWordCount(int bits) const { return (bits >> 5) + 1; }
    /* every bit cleared (CAiEvent::Initalize; the name is ours) */
    void ClearAll()
    {
        if (m_vec)
            memset(m_vec, 0, GetWordCount(m_bits) * 4);
        else
            m_bits = 0;
    }
    bool Test(unsigned int i) const { return (m_vec[i >> 5] & (1 << (i & 31))) != 0; }

    unsigned int m_bits;
    unsigned int *m_vec;

    void Clear(unsigned int i) { m_vec[i >> 5] &= ~(1 << (i & 31)); }
    int Resize(unsigned int bits);
};

/* a value held within [lo, hi], and whether it had to be: out of line wherever it is
 * instantiated (SOCOM 1's _saturate<f>, void there; SOCOM II's returns the bool -- its float
 * instance 0x267A60, which zanim_cmd_tick.cpp links; the light's tick and CPad::Dilate call it) */
template <class T>
bool _saturate(T *v, const T &lo, const T &hi)
{
    if (*v > hi) {
        *v = hi;
        return true;
    }
    if (*v < lo) {
        *v = lo;
        return true;
    }
    return false;
}

/* |a - b| <= eps, out of line wherever it is instantiated (SOCOM 1: _close_enough<f>) */
template <class T>
bool _close_enough(const T &a, const T &b, const T &eps)
{
    T d = a - b;
    return d >= -eps && d <= eps;
}

inline CPnt3D CPnt3D::operator+(const CPnt3D &b) const
{
    CPnt3D r;
    Add(&b, &r);
    return r;
}

inline CPnt3D CPnt3D::operator-(const CPnt3D &b) const
{
    CPnt3D r;
    Sub(&b, &r);
    return r;
}

inline CPnt3D CPnt3D::operator*(float s) const
{
    CPnt3D r;
    Scale(s, &r);
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
