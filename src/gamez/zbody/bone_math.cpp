/* bone_math.cpp -- the body's geometric helpers: CSequence's envelope, the plane and box clips,
 * the rotation helpers and CBody::PartInSpace (OVL1 0x2869D0-0x2875C4). SOCOM 1 has the same
 * functions in the same order (0x1DC520-0x1DD0F0), its unit closed by RTList<ACTIVE_ANIM>'s
 * instances, which SOCOM II's body.cpp emits instead (PushAnim and RenormAnims moved there); no
 * initialiser and no assert names the file, so its name is ours. Defined in reverse of their image
 * order (-inline deferred). */

#include "zbody.h"

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
