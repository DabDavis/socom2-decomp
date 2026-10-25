/* bone_math.cpp -- the body's geometric helpers: CSequence's envelope, the plane and box clips,
 * the rotation helpers and CBody::PartInSpace (OVL1 0x2869D0-0x2875C4). SOCOM 1 has the same
 * functions in the same order (0x1DC520-0x1DD0F0), its unit closed by RTList<ACTIVE_ANIM>'s
 * instances, which SOCOM II's body.cpp emits instead (PushAnim and RenormAnims moved there); no
 * initialiser and no assert names the file, so its name is ours. Defined in reverse of their image
 * order (-inline deferred). */

#include "zbody.h"

extern "C" {
float acosf(float);
float fabsf(float);
}

/* the quaternion of three Euler angles, z then y then x (SOCOM 1's; returns its first argument) */
CQuat *QFromRot(CQuat *q, const CPnt3D *rot)
{
    CQuat t;
    CQuat qx, qy, qz;

    qx.MakeXRot(rot->x);
    qy.MakeYRot(rot->y);
    qz.MakeZRot(rot->z);
    qz.Mul(&qy, &t);
    t.Mul(&qx, q);
    return q;
}

/* acosf held to its domain: 0 at or past 1, pi at or below -1 (SOCOM II's, inline; SOCOM 1 called
 * acosf straight; the name ours) */
static inline float SafeAcos(float x)
{
    float r;

    if (x >= 1.0f)
        r = 0.0f;
    else if (x <= -1.0f)
        r = 3.1415927f;
    else
        r = acosf(x);
    return r;
}

/* a direction's yaw and pitch as fractions of a right angle and of 80 degrees, signed by its x and
 * its y (SOCOM 1's; the yaw left 0 when it points straight up or down) */
void GetFractionalRotations(const CPnt3D *v, float *yaw, float *pitch)
{
    float p = 0.0f;
    float y;

    if (v->y == 0.0f) {
        y = SafeAcos(-v->z);
    } else {
        float len = sqrtf(v->x * v->x + v->z * v->z);

        if (len < 0.01f)
            p = 1.5707964f;
        else
            p = SafeAcos(len);
        if (fabsf(v->x * CPnt3D::unit_y.x + v->y * CPnt3D::unit_y.y + v->z * CPnt3D::unit_y.z) > 0.999f)
            y = 0.0f;
        else
            y = SafeAcos(-v->z / len);
    }
    float fy = y / 1.5707964f;
    float fp = p / 1.3962635f;

    if (fy > 1.0f)
        fy = 1.0f;
    if (fp > 1.0f)
        fp = 1.0f;
    if (v->x < 0.0f)
        fy = -fy;
    if (v->y < 0.0f)
        fp = -fp;
    *yaw = fy;
    *pitch = fp;
}

/* where the line from a to b crosses a bound on one axis, read on the other: the bound itself when
 * the line runs along it (SOCOM II's, inline; the name ours) */
static inline float Intercept(float bound, const float &a1, float b1, const float &a2, float b2)
{
    float r;

    if (a1 == b1) {
        r = bound;
    } else {
        float t = (bound - a1) / (b1 - a1);

        r = a2 + t * (b2 - a2);
    }
    return r;
}

/* b kept when inside the box y1..y0, x1..x0 (true), else the segment from a clipped to its edge,
 * x and y only (SOCOM 1's) */
bool ClipToBox(const CPnt3D &a, const CPnt3D &b, CPnt3D &out, float y0, float y1, float x0, float x1)
{
    float xAtY0;
    float xAtY1;
    float yAtX0;
    float yAtX1;
    bool inY0 = true;
    bool inY1 = true;
    bool inX0 = true;
    bool inX1 = true;
    float by = b.y;

    if (!(by <= y0))
        inY0 = false;
    if (by < y1)
        inY1 = false;
    float bx = b.x;

    if (!(bx <= x0))
        inX0 = false;
    if (bx < x1)
        inX1 = false;
    if (inY0 && inY1 && inX0 && inX1) {
        out = b;
        return true;
    }
    if (!inY0)
        xAtY0 = Intercept(y0, a.y, by, a.x, bx);
    else
        xAtY0 = bx;
    if (!inY1)
        xAtY1 = Intercept(y1, a.y, by, a.x, bx);
    else
        xAtY1 = bx;
    if (!inX0)
        yAtX0 = Intercept(x0, a.x, bx, a.y, by);
    else
        yAtX0 = by;
    if (!inX1)
        yAtX1 = Intercept(x1, a.x, bx, a.y, by);
    else
        yAtX1 = by;

    if (!inY0) {
        if (!inX0) {
            if (!(xAtY0 <= x0) && !(yAtX0 <= y0)) {
                out.x = x0;
                out.y = y0;
                return false;
            }
            if (!(xAtY0 <= x0)) {
                out.x = x0;
                out.y = yAtX0;
                return false;
            }
            out.x = xAtY0;
            out.y = y0;
            return false;
        }
        if (!inX1) {
            if (xAtY0 < x1 && !(yAtX1 <= y0)) {
                out.x = x1;
                out.y = y0;
                return false;
            }
            if (xAtY0 < x1) {
                out.x = x1;
                out.y = yAtX1;
                return false;
            }
            out.x = xAtY0;
            out.y = y0;
            return false;
        }
        out.x = xAtY0;
        out.y = y0;
        return false;
    }
    if (!inY1) {
        if (!inX0) {
            if (!(xAtY1 <= x0) && yAtX0 < y1) {
                out.x = x0;
                out.y = y1;
                return false;
            }
            if (!(xAtY1 <= x0)) {
                out.x = x0;
                out.y = yAtX0;
                return false;
            }
            out.x = xAtY1;
            out.y = y1;
            return false;
        }
        if (!inX1) {
            if (xAtY1 < x1 && yAtX0 < y1) {
                out.x = x1;
                out.y = y1;
                return false;
            }
            if (xAtY1 < x1) {
                out.x = x1;
                out.y = yAtX1;
                return false;
            }
            out.x = xAtY1;
            out.y = y1;
            return false;
        }
        out.x = xAtY1;
        out.y = y1;
        return false;
    }
    if (!inX0) {
        out.x = x0;
        out.y = yAtX0;
        return false;
    }
    if (!inX1) {
        out.x = x1;
        out.y = yAtX1;
        return false;
    }
    return false;
}

/* a 0..1 ramp eased in and out: two parabolas meeting at one half (SOCOM 1's) */
float quadW(float t)
{
    if (t <= 0.5f)
        return 2.0f * t * t;
    float s = 2.0f * (t - 1.0f);
    return 1.0f - 0.5f * s * s;
}

/* the plane through p with normal n, as its constant (SOCOM 1's) */
float PlaneConstant(const CPnt3D &n, const CPnt3D &p)
{
    return -(n.x * p.x + n.y * p.y + n.z * p.z);
}

/* where the line from a to b meets the plane (n, d), as a fraction of the way (SOCOM 1's) */
float LineIntersectPlane(float d, const CPnt3D &n, const CPnt3D &a, const CPnt3D &b)
{
    return (-d - (n.x * a.x + n.y * a.y + n.z * a.z)) /
           (n.x * (b.x - a.x) + n.y * (b.y - a.y) + n.z * (b.z - a.z));
}

void CSequence::Start(float up, float hold, float down)
{
    m_upT = up;
    m_holdT = hold;
    m_downT = down;
    m_currT = down + (up + hold);
}

void CSequence::Clear()
{
    m_upT = 0.0f;
    m_holdT = 0.0f;
    m_downT = 0.0f;
    m_currT = 0.0f;
}

/* the time left, run down by dT and held at zero */
float CSequence::Tick(float dT)
{
    m_currT -= dT;
    if (m_currT < 0.0f)
        m_currT = 0.0f;
    return m_currT;
}

/* the envelope's weight now: rising, held at 1, falling */
float CSequence::LinearWeight()
{
    float total = m_upT + m_holdT + m_downT;
    float w;

    m_currT = m_currT < total ? m_currT : total;
    if (m_currT <= 0.0f)
        w = 0.0f;
    else if (m_currT > m_holdT + m_downT)
        w = (total - m_currT) / m_upT;
    else if (m_currT >= m_downT)
        w = 1.0f;
    else
        w = m_currT / m_downT;
    return w;
}

/* the same, eased (SOCOM 1's) */
float CSequence::Weight()
{
    return quadW(LinearWeight());
}

void CBody::PartInSpace(CZBodyPart *part, const CPnt3D *offset, CPnt3D *pos, CQuat *rot) const
{
    CPnt3D t;
    CPnt3D p;
    CQuat q;

    if (!part)
        return;
    if (offset)
        part->m_rot.Apply(offset, &p, 1);
    else
        p = CPnt3D::zero;
    if (rot)
        q = part->m_rot;
    else
        q = CQuat::identity;
    while (part->m_parent) {
        p.Add(&part->m_pos, &t);
        const CQuat *prot = &part->m_parent->m_rot;

        prot->Apply(&t, &p, 1);
        if (rot) {
            CQuat r = q;

            prot->Mul(&r, &q);
        }
        part = part->m_parent;
    }
    if (pos)
        p.Add(&part->m_pos, pos);
    if (rot)
        q.Normalize(rot);
}
