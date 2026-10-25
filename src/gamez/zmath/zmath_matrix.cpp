/* zmath_matrix.cpp -- CStack and CMatrix (OVL1 0x307FA0-0x3087F4). The unit name is SOCOM 1's
 * (__sinit_zmath_matrix.cpp builds CMatrix::identity and the stack). Defined in reverse of
 * their image order (-inline deferred). */

#include "zmath.h"
#include <string.h>

const CMatrix CMatrix::identity(1.0f, 0.0f, 0.0f, 0.0f,
                                0.0f, 1.0f, 0.0f, 0.0f,
                                0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f, 0.0f, 0.0f, 1.0f);
CMatrix CStack::m_stack[64];
int CStack::m_pointer;
CMatrix *CStack::m_top;

void CMatrix::SetRows(const CPnt3D *r0, const CPnt3D *r1, const CPnt3D *r2, const CPnt3D *r3)
{
    if (r0)
        memcpy(m_matrix[0], r0, sizeof(CPnt3D));
    if (r1)
        memcpy(m_matrix[1], r1, sizeof(CPnt3D));
    if (r2)
        memcpy(m_matrix[2], r2, sizeof(CPnt3D));
    if (r3)
        memcpy(m_matrix[3], r3, sizeof(CPnt3D));
}

CMatrix::CMatrix(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13,
                 float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33)
{
    m_matrix[0][0] = m00;
    m_matrix[0][1] = m01;
    m_matrix[0][2] = m02;
    m_matrix[0][3] = m03;
    m_matrix[1][0] = m10;
    m_matrix[1][1] = m11;
    m_matrix[1][2] = m12;
    m_matrix[1][3] = m13;
    m_matrix[2][0] = m20;
    m_matrix[2][1] = m21;
    m_matrix[2][2] = m22;
    m_matrix[2][3] = m23;
    m_matrix[3][0] = m30;
    m_matrix[3][1] = m31;
    m_matrix[3][2] = m32;
    m_matrix[3][3] = m33;
}

/* points through the matrix in place, each copied into an aligned vector and back. SOCOM 1's
 * zmath_matrix.h has it inline (these locals, these lines); SOCOM II calls one copy */
void CMatrix::Transform(CPnt3D *inout, int count) const
{
    CPnt4D input;
    CPnt4D output;

    _LoadMatrix(this);
    do {
        input.x = inout->x;
        input.y = inout->y;
        input.z = inout->z;

        _TransformPointXYZ(&input, &output);

        inout->x = output.x;
        inout->y = output.y;
        inout->z = output.z;
        inout++;
    } while (--count);
}

extern "C" {

/* SOCOM II's additions, no SOCOM 1 names: VU0 loops over a point array, hand-written. */

/* out[i] = m * in[i], n CPnt3D points (w = 1): SOCOM 1's Transform, its stack slots swapped */
asm void CMatrix::Transform(const CPnt3D *in, CPnt3D *out, int n) const
{
    addiu sp, sp, -32
    lqc2 vf4, 0(a0)
    lqc2 vf5, 16(a0)
    lqc2 vf6, 32(a0)
    lqc2 vf7, 48(a0)
    addiu a0, sp, 16
    addiu v1, sp, 0
    nop
loop:
    lwc1 f0, 0(a1)
    swc1 f0, 16(sp)
    lwc1 f0, 4(a1)
    swc1 f0, 20(sp)
    lwc1 f0, 8(a1)
    swc1 f0, 24(sp)
    lqc2 vf8, 0(a0)
    vmulax.xyz ACC, vf4, vf8
    vmadday.xyz ACC, vf5, vf8
    vmaddaz.xyz ACC, vf6, vf8
    vmaddw.xyz vf9, vf7, vf0
    sqc2 vf9, 0(v1)
    addiu a3, a3, -1
    addiu a1, a1, 12
    lwc1 f0, 0(sp)
    swc1 f0, 0(a2)
    lwc1 f0, 4(sp)
    swc1 f0, 4(a2)
    lwc1 f0, 8(sp)
    swc1 f0, 8(a2)
    bnez a3, loop
    addiu a2, a2, 12
    jr ra
    addiu sp, sp, 32
}

/* out[i] = m * in[i], n CPnt4D vectors */
asm void CMatrix::Transform(const CPnt4D *in, CPnt4D *out, int n) const
{
    lqc2 vf4, 0(a0)
    lqc2 vf5, 16(a0)
    lqc2 vf6, 32(a0)
    lqc2 vf7, 48(a0)
loop:
    lqc2 vf8, 0(a1)
    vmulax.xyzw ACC, vf4, vf8
    vmadday.xyzw ACC, vf5, vf8
    vmaddaz.xyzw ACC, vf6, vf8
    vmaddw.xyzw vf9, vf7, vf8
    sqc2 vf9, 0(a2)
    addiu a3, a3, -1
    addiu a1, a1, 16
    bnez a3, loop
    addiu a2, a2, 16
    jr ra
    nop
}

/* no translation and a unit diagonal, compared as bits */
bool func_003084F0(const CMatrix *m)
{
    const unsigned int *w = (const unsigned int *)m->m_matrix;
    return w[12] == 0 && w[13] == 0 && w[14] == 0 && w[0] == 0x3F800000 && w[5] == 0x3F800000 &&
           w[10] == 0x3F800000;
}

}

/* The rotation part from Euler angles (x, y, z), the order ToEuler undoes. */
void CMatrix::SetRotate(const CPnt3D *e)
{
    float a;
    float *p, cy, cx, cz, sx, sy, sz;

    a = e->x;
    sx = sinf(a);
    cx = cosf(a);
    a = e->y;
    sy = sinf(a);
    cy = cosf(a);
    a = e->z;
    sz = sinf(a);
    cz = cosf(a);
    p = &m_matrix[0][0];

    *p++ = cz * cy + sz * (sy * sx);
    *p++ = sz * cx;
    *p++ = -cz * sy + cy * sz * sx;
    p++;
    *p++ = -(cy * sz) + cz * (sy * sx);
    *p++ = cz * cx;
    *p++ = sz * sy + sx * (cz * cy);
    p++;
    *p++ = cx * sy;
    *p++ = -sx;
    *p++ = cx * cy;
}

/* The rotation as a quaternion (Shoemake): from the trace when it is positive, else from the
 * largest diagonal element. */
CQuat *CMatrix::ToQuat(CQuat *q) const
{
    float tr = m_matrix[0][0] + m_matrix[1][1] + m_matrix[2][2];

    if (tr > 0.0f) {
        float s = sqrtf(tr + 1.0f);
        q->w = 0.5f * s;
        s = 0.5f / s;
        q->vec.x = s * (m_matrix[1][2] - m_matrix[2][1]);
        q->vec.y = s * (m_matrix[2][0] - m_matrix[0][2]);
        q->vec.z = s * (m_matrix[0][1] - m_matrix[1][0]);
    } else {
        static int next[3] = {1, 2, 0};
        int i = 0;
        if (m_matrix[1][1] > m_matrix[0][0])
            i = 1;
        if (m_matrix[2][2] > m_matrix[i][i])
            i = 2;
        int j = next[i];
        int k = next[j];
        float *v = &q->vec.x;
        float s = sqrtf(m_matrix[i][i] + 1.0f - m_matrix[j][j] - m_matrix[k][k]);
        v[i] = 0.5f * s;
        s = 0.5f / s;
        v[j] = s * (m_matrix[j][i] + m_matrix[i][j]);
        v[k] = s * (m_matrix[k][i] + m_matrix[i][k]);
        q->w = s * (m_matrix[j][k] - m_matrix[k][j]);
    }
    return q;
}

/* A rigid inverse for a matrix whose scale is uniform: the transpose over the squared
 * scale, the translation rotated back. Hand-written VU0 (SOCOM 1 has it byte for byte). */
asm void Vu0MatrixInverseWithUniformScale(CMatrix *dst, CMatrix *src)
{
    lqc2 vf4, 0(a1)
    vmul.xyz vf8, vf4, vf4
    vaddy.x vf8, vf8, vf8
    vaddz.x vf8, vf8, vf8
    vdiv Q, vf0w, vf8x
    addi a1, a1, 16
    lqc2 vf5, 0(a1)
    addi a1, a1, 16
    lqc2 vf6, 0(a1)
    addi a1, a1, 16
    lqc2 vf7, 0(a1)
    vaddy.x vf20, vf0, vf4
    vaddz.x vf21, vf0, vf4
    vaddz.y vf22, vf0, vf5
    vaddx.w vf13, vf0, vf0
    vwaitq
    vmulq.xyz vf10, vf4, Q
    vmulq.xyz vf11, vf5, Q
    vmulq.xyz vf12, vf6, Q
    vaddx.y vf10, vf0, vf11
    vaddx.z vf10, vf0, vf12
    vaddy.z vf11, vf0, vf12
    vmulq.x vf11, vf20, Q
    vmulq.x vf12, vf21, Q
    vmulq.y vf12, vf22, Q
    vmuly.w vf10, vf10, vf0
    vmuly.w vf11, vf11, vf0
    vmuly.w vf12, vf12, vf0
    vaddy.w vf13, vf0, vf0
    vmulax.xyz ACC, vf10, vf7
    vmadday.xyz ACC, vf11, vf7
    vmaddz.xyz vf13, vf12, vf7
    vsub.xyz vf13, vf0, vf13
    sqc2 vf10, 0(a0)
    addi a0, a0, 16
    sqc2 vf11, 0(a0)
    addi a0, a0, 16
    sqc2 vf12, 0(a0)
    addi a0, a0, 16
    jr ra
    sqc2 vf13, 0(a0)
}

void CMatrix::SetZero()
{
    memset(this, 0, sizeof(CMatrix));
}

/* Numerical Recipes' SIGN: |a| with b's sign */
inline float SIGN(float a, float b)
{
    return b >= 0.0f ? fabsf(a) : -fabsf(a);
}

/* The Euler angles of the rotation part; at the poles (|m21| >= 1) z is folded into y. */
void CMatrix::ToEuler(CPnt3D *out) const
{
    if (this == &identity) {
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 0.0f;
        return;
    }
    double s = m_matrix[2][1];
    if (fabs(s) < 1.0) {
        out->x = asinf(-s);
        out->y = atan2f(m_matrix[2][0], m_matrix[2][2]);
        out->z = atan2f(m_matrix[0][1], m_matrix[1][1]);
    } else {
        out->x = -SIGN(1.5707964f, s);
        out->y = atan2f(-m_matrix[0][2], m_matrix[0][0]);
        out->z = 0.0f;
    }
}

CMatrix *CStack::Multiply(const CMatrix *m, bool push)
{
    if (push) {
        m_pointer++;
        m_top++;
        sceVu0MulMatrix(m_top->m_matrix, (m_top - 1)->m_matrix, (float (*)[4])m->m_matrix);
    } else {
        sceVu0MulMatrix(m_top->m_matrix, m_top->m_matrix, (float (*)[4])m->m_matrix);
    }
    return m_top;
}
