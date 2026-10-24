/* CQuat -- quaternions (OVL1 0x3066D0-0x307B40). Names are SOCOM 1's (SCUS_972.05's
 * symbols, carried by body fingerprint) where it has them; the file name is a
 * placeholder. Functions are emitted in REVERSE definition order (-inline deferred),
 * so this file defines them bottom-up: the image's last function comes first. */

#include "zmath.h"

void CQuat::MakeXRot(float angle)
{
    float half = 0.5f * angle;
    x = sinf(half);
    w = cosf(half);
    y = 0.0f;
    z = 0.0f;
}

void CQuat::MakeYRot(float angle)
{
    float half = 0.5f * angle;
    y = sinf(half);
    w = cosf(half);
    x = 0.0f;
    z = 0.0f;
}

void CQuat::MakeZRot(float angle)
{
    float half = 0.5f * angle;
    z = sinf(half);
    w = cosf(half);
    x = 0.0f;
    y = 0.0f;
}

void func_00307470(CQuat *q, float a, float b)
{
    float h = 0.5f * a;
    float sa = sinf(h);
    float ca = cosf(h);
    float sb, cb;

    h = 0.5f * b;
    sb = sinf(h);
    cb = cosf(h);
    q->w = ca * cb;
    q->x = ca * sb;
    q->y = sa * cb;
    q->z = -sa * sb;
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
    x = cysx * cz + sycx * sz;
    y = sycx * cz - cysx * sz;
    z = -sysx * cz + cycx * sz;
}

void func_003072E0(const CQuat *q, CQuat *out)
{
    float ww = q->w;
    sceVu0ScaleVector((float *)out, (float *)q,
                      1.0f / sqrtf(ww * ww + sceVu0InnerProduct((float *)q, (float *)q)));
}

CQuat *CQuat::Normalize()
{
    float ww = w;
    sceVu0ScaleVector((float *)this, (float *)this,
                      1.0f / sqrtf(ww * ww + sceVu0InnerProduct((float *)this, (float *)this)));
    return this;
}

/* The rotation matrix of q, 3x3 part only (callers start from a unit matrix). The
 * products' order is one the permuter found (tools/perm.py); others compile differently. */
void func_00307170(const CQuat *q, CMatrix *mat)
{
    CPnt3D v2 = *(const CPnt3D *)q + *(const CPnt3D *)q;
    float *p = &mat->m[0][0];
    float xx, zz, yy, xy, yz, xz, wx, wz, wy;

    xx = q->x * v2.x;
    yy = q->y * v2.y;
    xy = q->x * v2.y;
    zz = q->z * v2.z;
    wx = q->w * v2.x;
    yz = q->y * v2.z;
    xz = q->z * v2.x;
    wy = q->w * v2.y;
    wz = q->w * v2.z;
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

    sceVu0UnitMatrix(m.m);
    func_00307170(this, &m);
    m.ToEuler(out);
}

void CQuat::Exp(const CPnt3D *v)
{
    float len2 = v->x * v->x + v->y * v->y + v->z * v->z;

    if (!len2) {
        w = 1.0f;
        x = data_003F64C0.x;
        y = data_003F64C0.y;
        z = data_003F64C0.z;
    } else {
        float len = sqrtf(len2);
        float s = sinf(len);
        w = cosf(len);
        func_00309180(v, (CPnt3D *)this, s / len);
    }
}

void func_003066D0(CQuat *q, const CPnt3D *v, float t)
{
    float len2 = v->x * v->x + v->y * v->y + v->z * v->z;

    if (!len2) {
        q->w = 1.0f;
        q->x = data_003F64C0.x;
        q->y = data_003F64C0.y;
        q->z = data_003F64C0.z;
    } else {
        float len = sqrtf(len2);
        float a = t * len;
        float s = sinf(a);
        q->w = cosf(a);
        func_00309180(v, (CPnt3D *)q, s / len);
    }
}
