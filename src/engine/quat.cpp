/* CQuat -- quaternions (OVL1 0x3066D0-0x307B40). Names are SOCOM 1's (SCUS_972.05's
 * symbols, carried by body fingerprint) where it has them; the file name is a
 * placeholder. Functions are emitted in REVERSE definition order (-inline deferred),
 * so this file defines them bottom-up: the image's last function comes first. */

#include "zmath.h"

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

    sceVu0UnitMatrix(m.m_matrix);
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
 * result is unit length to 0.004 (func_0028F1E0); otherwise Slerp. Returns 1 when the
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
    if (!func_0028F1E0(&len2, &data_003DF320, &data_003DF328)) {
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
        vec.x = data_003F64C0.x;
        vec.y = data_003F64C0.y;
        vec.z = data_003F64C0.z;
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
        vec.x = data_003F64C0.x;
        vec.y = data_003F64C0.y;
        vec.z = data_003F64C0.z;
    } else {
        float len = sqrtf(len2);
        float a = t * len;
        float s = sinf(a);
        w = cosf(a);
        axis->Scale(s / len, &vec);
    }
}
