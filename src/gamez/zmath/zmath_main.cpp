/* zmath_main.cpp -- zMath_Init, CQuat, the box test and CBits (OVL1 0x306670-0x307F9C). The
 * file name is the press beta's (SCUS_971.34 asserts in zmath_main.cpp from CBits::Resize) and
 * SOCOM 1's (__sinit_zmath_main.cpp builds CQuat::zero and identity), the directory the demo's
 * (Z:\dev\gamez\zmath). The unit keeps SOCOM 1's function order, with SOCOM II's additions
 * (the camera and projector builders) in place. MW emits a unit's functions in REVERSE
 * definition order (-inline deferred), so this file runs bottom-up. */

#include "zmath.h"
#include "../zsys/zsys.h"

const CQuat CQuat::zero(0.0f, 0.0f, 0.0f, 0.0f);
const CQuat CQuat::identity(0.0f, 0.0f, 0.0f, 1.0f);

/* Grow the set to `bits`, clearing the new ones; it never shrinks the storage. */
int CBits::Resize(unsigned int bits)
{
    int words = ((int)bits >> 5) + 1;
    int had = ((int)m_bits >> 5) + 1;

    if (m_vec == 0) {
        m_vec = (unsigned int *)_malloc(words * sizeof(unsigned int), 0, 0);
        for (unsigned int i = m_bits; i < bits; i++)
            Clear(i);
    } else if (words != had && had < words) {
        m_vec = (unsigned int *)_realloc(m_vec, words * sizeof(unsigned int), 0, 0);
        for (unsigned int i = m_bits; i < bits; i++)
            Clear(i);
    }
    m_bits = bits;
    return 1;
}

extern "C" void func_003079C0(const CPnt3D *eye, const CPnt3D *at, const CPnt3D *dir, float a, float b,
                              sceVu0FMATRIX out)
{
    sceVu0FVECTOR xc;
    sceVu0FVECTOR p;
    sceVu0FVECTOR yd;
    sceVu0FVECTOR zd;
    sceVu0FMATRIX cam;
    sceVu0FMATRIX proj;
    sceVu0FVECTOR x;
    sceVu0FVECTOR v1;
    sceVu0FVECTOR v2;

    sceVu0UnitMatrix(cam);
    sceVu0UnitMatrix(proj);
    v1[0] = dir->x; v1[1] = dir->y; v1[2] = dir->z; v1[3] = 1.0f;
    v2[0] = at->x; v2[1] = at->y; v2[2] = at->z; v2[3] = 1.0f;
    sceVu0OuterProduct(x, v1, v2);
    sceVu0Normalize(x, x);
    xc[0] = x[0]; xc[1] = x[1]; xc[2] = x[2];
    VecSet(zd, at->x, at->y, at->z, 1.0f);
    VecSet(p, eye->x, eye->y, eye->z, 1.0f);
    sceVu0OuterProduct(yd, xc, zd);
    sceVu0CameraMatrix(cam, p, zd, yd);
    sceVu0UnitMatrix(proj);
    proj[0][0] = proj[1][1] = (a - 1.0f) / (b * a);
    proj[3][0] = proj[3][1] = 0.5f;
    sceVu0MulMatrix(out, proj, cam);
}

/* The same projector, its up axis the one dir leans on least -- the source's two tests
 * run straight-line (the last one updates no minimum, and fabsf is no pure function to
 * MW, so that was never a dead call), with `up` otherwise uninitialised. */
extern "C" void func_00307810(const CPnt3D *eye, const CPnt3D *dir, float a, float b, sceVu0FMATRIX out)
{
    float min;
    sceVu0FVECTOR up;
    int axis;
    sceVu0FVECTOR d;
    sceVu0FVECTOR p;
    sceVu0FVECTOR yd;
    sceVu0FVECTOR zd;
    sceVu0FMATRIX cam;
    sceVu0FMATRIX proj;

    sceVu0UnitMatrix(cam);
    sceVu0UnitMatrix(proj);
    VecSet(d, dir->x, dir->y, dir->z, 0.0f);
    min = fabsf(d[0]);
    axis = 0;
    if (fabsf(d[1]) < min) {
        axis = 1;
        min = fabsf(d[1]);
    }
    if (fabsf(d[2]) < min)
        axis = 2;
    up[axis] = 1.0f;
    sceVu0OuterProduct(yd, d, up);
    sceVu0Normalize(yd, yd);
    VecSet(zd, dir->x, dir->y, dir->z, 1.0f);
    VecSet(p, eye->x, eye->y, eye->z, 1.0f);
    sceVu0CameraMatrix(cam, p, zd, yd);
    sceVu0UnitMatrix(proj);
    proj[0][0] = proj[1][1] = (a - 1.0f) / (b * a);
    proj[3][0] = proj[3][1] = 0.5f;
    sceVu0MulMatrix(out, proj, cam);
}

/* A camera looking along zd from p: the up axis is x or z, whichever zd leans on less.
 * Two unit matrices are set up and never used, and `up` keeps whatever the stack held in
 * its other components -- both as the original has them. */
extern "C" void func_00307700(sceVu0FVECTOR zd, sceVu0FVECTOR p, sceVu0FMATRIX out)
{
    sceVu0FVECTOR up;
    sceVu0FVECTOR yd;
    sceVu0FMATRIX m0;
    sceVu0FMATRIX m1;
    float z;

    sceVu0UnitMatrix(m0);
    sceVu0UnitMatrix(m1);
    z = zd[2];
    if (fabsf(zd[0]) < fabsf(z))
        up[0] = 1.0f;
    else
        up[2] = 1.0f;
    up[3] = 1.0f;
    sceVu0OuterProduct(yd, zd, up);
    sceVu0Normalize(yd, yd);
    if (yd[1] < 0.0f) {
        yd[0] = -yd[0];
        yd[1] = -yd[1];
        yd[2] = -yd[2];
    }
    sceVu0CameraMatrix(out, p, zd, yd);
}

/* OPEN, 2 instructions off: a segment (a, b) against box through the VU0 microprogram at
 * 0x30; out[3] comes back 1.0 on a hit. The target loads out[3] before materialising the
 * 1.0f; MW (which schedules inline asm instruction by instruction) hoists the constant
 * here, and no statement order (tools/perm.py, 30,800 variants), comparison form or asm
 * qualifier has moved it. */
int CBBox::GetIntersect(CPnt4D *pa, CPnt4D *pb, CPnt4D *pout, CPnt4D *pout2, bool flag)
{
    sceVu0FVECTOR vmin;
    sceVu0FVECTOR vmax;
    float *pmin = vmin;
    float *pmax = vmax;
    int *w = (int *)&pmax[3];

    float *a = &pa->x;
    float *b = &pb->x;
    float *out = &pout->x;
    float *out2 = &pout2->x;

    vmin[0] = m_min.x;
    vmin[1] = m_min.y;
    vmin[2] = m_min.z;
    vmax[0] = m_max.x;
    vmax[1] = m_max.y;
    vmax[2] = m_max.z;
    if (!flag)
        *w = 0xE0;
    else
        *w = 0;
    out[3] = 0.0f;
    asm __volatile__ (
        "qmtc2.i %0, $vf0\n"
        "lqc2 $vf1, 0(%1)\n"
        "lqc2 $vf2, 0(%2)\n"
        "lqc2 $vf3, 0(%3)\n"
        "lqc2 $vf4, 0(%4)\n"
        "vcallms 0x30\n"
        "qmtc2.i %0, $vf0\n"
        "sqc2 $vf10, 0(%5)\n"
        "sqc2 $vf30, 0(%6)\n"
        "qmtc2.i %0, $vf0\n"
        : : "r"(this), "r"(a), "r"(b), "r"(pmin), "r"(pmax), "r"(out), "r"(out2) : "memory");
    if (out[3] == 1.0f)
        return 1;
    return 0;
}

void CQuat::MakeXRot(float angle)
{
    float half = 0.5f * angle;
    vec.x = sinf(half);
    w = cosf(half);
    vec.y = 0.0f;
    vec.z = 0.0f;
}

void CQuat::MakeYRot(float angle)
{
    float half = 0.5f * angle;
    vec.y = sinf(half);
    w = cosf(half);
    vec.x = 0.0f;
    vec.z = 0.0f;
}

void CQuat::MakeZRot(float angle)
{
    float half = 0.5f * angle;
    vec.z = sinf(half);
    w = cosf(half);
    vec.x = 0.0f;
    vec.y = 0.0f;
}

void CQuat::MakeYXRot(float a, float b)
{
    float h = 0.5f * a;
    float sa = sinf(h);
    float ca = cosf(h);
    float sb, cb;

    h = 0.5f * b;
    sb = sinf(h);
    cb = cosf(h);
    w = ca * cb;
    vec.x = ca * sb;
    vec.y = sa * cb;
    vec.z = -sa * sb;
}

void CQuat::MakeYXZRot(float ry, float rx, float rz)
{
    float h = 0.5f * ry;
    float sy = sinf(h);
    float cy = cosf(h);
    float sx, cx, sz, cz;
    float cycx, cysx, sycx, sysx;

    h = 0.5f * rx;
    sx = sinf(h);
    cx = cosf(h);
    cysx = cy * sx;
    cycx = cy * cx;
    sycx = sy * cx;
    sysx = sy * sx;
    h = 0.5f * rz;
    sz = sinf(h);
    cz = cosf(h);
    w = cycx * cz + sysx * sz;
    vec.x = cysx * cz + sycx * sz;
    vec.y = sycx * cz - cysx * sz;
    vec.z = -sysx * cz + cycx * sz;
}

void CQuat::Normalize(CQuat *out) const
{
    float ww = w;
    sceVu0ScaleVector((float *)out, (float *)this,
                      1.0f / sqrtf(ww * ww + sceVu0InnerProduct((float *)this, (float *)this)));
}

CQuat *CQuat::Normalize()
{
    float ww = w;
    sceVu0ScaleVector((float *)this, (float *)this,
                      1.0f / sqrtf(ww * ww + sceVu0InnerProduct((float *)this, (float *)this)));
    return this;
}

/* The rotation matrix, 3x3 part only (callers start from a unit matrix). The
 * products' order is one the permuter found (tools/perm.py); others compile differently. */
void CQuat::ToMatrix(CMatrix *mat) const
{
    CPnt3D v2 = vec + vec;
    float *p = &mat->m_matrix[0][0];
    float xx, zz, yy, xy, yz, xz, wx, wz, wy;

    xx = vec.x * v2.x;
    yy = vec.y * v2.y;
    xy = vec.x * v2.y;
    zz = vec.z * v2.z;
    wx = w * v2.x;
    yz = vec.y * v2.z;
    xz = vec.z * v2.x;
    wy = w * v2.y;
    wz = w * v2.z;
    *p++ = 1.0f - yy - zz;
    *p++ = xy + wz;
    *p++ = xz - wy;
    p++;
    *p++ = xy - wz;
    *p++ = 1.0f - zz - xx;
    *p++ = yz + wx;
    p++;
    *p++ = xz + wy;
    *p++ = yz - wx;
    *p++ = 1.0f - xx - yy;
}

void CQuat::Mul(const CQuat *b, CQuat *out) const
{
    sceVu0FVECTOR tmp;

    sceVu0OuterProduct((float *)out, (float *)this, (float *)b);
    sceVu0ScaleVector(tmp, (float *)this, b->w);
    sceVu0AddVector((float *)out, (float *)out, tmp);
    sceVu0ScaleVector(tmp, (float *)b, w);
    sceVu0AddVector((float *)out, (float *)out, tmp);
    out->w -= tmp[3] + sceVu0InnerProduct((float *)this, (float *)b);
}

void CQuat::ToEuler(CPnt3D *out) const
{
    CMatrix m;

    ToMatrix(&m);
    m.ToEuler(out);
}

/* Rotate n points by q through the VU0 microprogram at 0x50 (vf1 = q, vf2 = point ->
 * vf3), written with GCC-style asm, which MW accepts. The point is widened by VecSet,
 * copied element-wise into an aligned buffer (the loads-first copy), and read back
 * through a pointer declared in the loop -- an inline helper returning that pointer
 * allocates the temporaries the other way round. */
void CQuat::Apply(const CPnt3D *in, CPnt3D *out, int n) const
{
    sceVu0FVECTOR vin;
    sceVu0FVECTOR vout;
    float v[4];

    while (n--) {
        VecSet(v, in->x, in->y, in->z, 0.0f);
        vin[0] = v[0];
        vin[1] = v[1];
        vin[2] = v[2];
        vin[3] = v[3];
        asm __volatile__ (
            "qmtc2.i %0, $vf0\n"
            "lqc2 $vf1, 0(%0)\n"
            "lqc2 $vf2, 0(%1)\n"
            "vcallms 0x50\n"
            "qmtc2.i %0, $vf0\n"
            "sqc2 $vf3, 0(%2)\n"
            "qmtc2.i %0, $vf0\n"
            : : "r"(this), "r"(vin), "r"(vout) : "memory");
        float *r = vout;
        out->x = r[0];
        out->y = r[1];
        out->z = r[2];
        in++;
        out++;
    }
}

/* The rotation taking `from` onto `to` (both unit): about their cross product by twice the
 * half angle -- the half-way vector does it in one step. Opposite vectors have no half-way
 * vector or cross product; any axis perpendicular to `from` in the ground plane serves. */
void CQuat::Align(const CPnt3D &from, const CPnt3D &to)
{
    CPnt3D half;

    half = from + to;

    if (!(half == CPnt3D::zero)) {
        sceVu0FVECTOR c;
        sceVu0FVECTOR a;
        sceVu0FVECTOR b;

        half.Normalize();
        a[0] = from.x;
        a[1] = from.y;
        a[2] = from.z;
        a[3] = 1.0f;
        b[0] = half.x;
        b[1] = half.y;
        b[2] = half.z;
        b[3] = 1.0f;
        sceVu0OuterProduct(c, a, b);
        CPnt3D axis;
        axis.x = c[0];
        axis.y = c[1];
        axis.z = c[2];
        if (!(axis == CPnt3D::zero) || !(Dot(from, to) < 0.0f)) {
            w = Dot(from, half);
            vec = axis;
            return;
        }
    }
    w = 0.0f;
    vec.z = 0.0f;
    if (from.x == 0.0f) {
        vec.x = 1.0f;
        vec.y = 0.0f;
    } else {
        float inv = 1.0f / sqrtf(from.x * from.x + from.y * from.y);
        vec.x = from.y * inv;
        vec.y = -from.x * inv;
    }
}

/* Spherical interpolation: the endpoints outside (0, 1), b flipped onto this one's
 * hemisphere, then a normalised lerp above cos 0.95 (Normalize inlined: -inline auto) or
 * the sine-weighted blend. b->w is read once, into a local, before the dot product. */
void CQuat::Slerp(const CQuat *b, float t, CQuat *out) const
{
    CQuat to;
    CQuat from;
    float cosom;
    float bw;

    if (t <= 0.0f) {
        *out = *this;
        return;
    }
    if (t >= 1.0f) {
        *out = *b;
        return;
    }
    bw = b->w;
    cosom = bw * w + sceVu0InnerProduct((float *)this, (float *)b);
    if (cosom < 0.0f) {
        cosom = -cosom;
        to.w = -bw;
        to.vec.x = -b->vec.x;
        to.vec.y = -b->vec.y;
        to.vec.z = -b->vec.z;
    } else {
        to = *b;
    }
    if (cosom > 0.95f) {
        sceVu0ScaleVector((float *)&from, (float *)this, 1.0f - t);
        sceVu0ScaleVector((float *)&to, (float *)&to, t);
        sceVu0AddVector((float *)out, (float *)&from, (float *)&to);
        out->Normalize();
    } else {
        float omega = acosf(cosom);
        float sinom = 1.0f / sinf(omega);
        sceVu0ScaleVector((float *)&from, (float *)this, sinom * sinf((1.0f - t) * omega));
        sceVu0ScaleVector((float *)&to, (float *)&to, sinom * sinf(t * omega));
        sceVu0AddVector((float *)out, (float *)&from, (float *)&to);
    }
}

/* A cheaper slerp: a plain lerp when the two are within cos 0.87, accepted only if the
 * result is unit length to 0.004 (_close_enough); otherwise Slerp. Returns 1 when the
 * cheap path held. The sign flag is set by if, not by assignment: `neg = cosom < 0` lets
 * MW fold it into the branch, where the target materialises it. */
int CQuat::SlerpFaster(const CQuat *b, float t, CQuat *out) const
{
    float cosom;
    float s;
    float len2;
    bool neg;

    if (t <= 0.0f) {
        *out = *this;
        return 1;
    }
    if (t >= 1.0f) {
        *out = *b;
        return 1;
    }
    cosom = b->w * w + sceVu0InnerProduct((float *)this, (float *)b);
    if (fabsf(cosom) < 0.87f) {
        Slerp(b, t, out);
        return 0;
    }
    neg = false;
    if (cosom < 0.0f)
        neg = true;
    s = 1.0f - t;
    if (neg) {
        out->vec.x = vec.x * s - b->vec.x * t;
        out->vec.y = vec.y * s - b->vec.y * t;
        out->vec.z = vec.z * s - b->vec.z * t;
        out->w = w * s - b->w * t;
    } else {
        out->vec.x = vec.x * s + b->vec.x * t;
        out->vec.y = vec.y * s + b->vec.y * t;
        out->vec.z = vec.z * s + b->vec.z * t;
        out->w = w * s + b->w * t;
    }
    float ww = out->w;
    len2 = ww * ww + sceVu0InnerProduct((float *)out, (float *)out);
    if (!_close_enough(len2, 1.0f, 0.004f)) {
        Slerp(b, t, out);
        return 0;
    }
    return 1;
}

void CQuat::Exp(const CPnt3D *axis)
{
    float len2 = axis->x * axis->x + axis->y * axis->y + axis->z * axis->z;

    if (!len2) {
        w = 1.0f;
        vec.x = CPnt3D::zero.x;
        vec.y = CPnt3D::zero.y;
        vec.z = CPnt3D::zero.z;
    } else {
        float len = sqrtf(len2);
        float s = sinf(len);
        w = cosf(len);
        axis->Scale(s / len, &vec);
    }
}

void CQuat::Exp(const CPnt3D *axis, float t)
{
    float len2 = axis->x * axis->x + axis->y * axis->y + axis->z * axis->z;

    if (!len2) {
        w = 1.0f;
        vec.x = CPnt3D::zero.x;
        vec.y = CPnt3D::zero.y;
        vec.z = CPnt3D::zero.z;
    } else {
        float len = sqrtf(len2);
        float a = t * len;
        float s = sinf(a);
        w = cosf(a);
        axis->Scale(s / len, &vec);
    }
}

extern u_long128 vu0_CodeSegment;      /* the start of the VU0 microcode (CQuat::Apply's 0x50, the box test's 0x30) */

/* Build the trig tables and load VU0's microprograms, by physical address. */
void zMath_Init()
{
    init_trig_table();
    CVU vu(0);
    FlushCache(0);
    vu.Code((u_long128 *)((u_int)&vu0_CodeSegment & 0x0FFFFFFF));
}
