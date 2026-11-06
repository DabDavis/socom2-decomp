/* seal_throw.h -- CDynGrenade::Initalize, the one body the three functions that throw inline (zseal.h has its
 * class). Apart from zseal.h because of its statics: seal_control.cpp and the kit's 0x5B8920, 0x5C1970 include
 * it; no other unit should see them. */
#ifndef SEAL_THROW_H
#define SEAL_THROW_H

#include "zseal.h"

/* the throw solved: the most speed the distance asks at the point's height, the aim's elevation plus
 * the power's offset, the flight's reach, and the direction -- toward the point the reach leaves, or
 * straight -- turned into the world by the matrix. An inline in three functions (CSealCtrl::PostTick and
 * the kit's 0x5B8920, 0x5C1970: SOCOM 1's PostTick and TickExplosives have it expanded whole; the name
 * from data.txt's old note, unproved). Its expanded body passes inline_max_size, so it is inlined one
 * level: everything it calls is called, even CPnt3D() -- and its locals come last in the frame, q below
 * every other inline's temporaries. Its statics are one each across the units that inline it: the
 * bounds in OVL2 data 0x65E630 (`const`: FLASHBANG_DOT's section), from_range 0x6505C8; the zero of
 * speed's bound is a literal, each unit's own (0x6505D0 here, 0x650930 for the kit) */
inline void CDynGrenade::Initalize(const CMatrix *m, const CPnt3D &aim, float dist, float power, CPnt3D &p)
{
    static const float elev_min = 0.0f;
    static const float elev_max = 0.9599311f;
    static bool from_range = true;
    float height = p.y;
    float unused = 0.0f;
    float maxvel = ComputeMaxVel(dist, height, &unused);
    float elev = asinf(aim.y);

    _saturate(&elev, elev_min, elev_max);
    elev += ComputeElevOfs(power);

    float s, c;

    SinCos(elev, s, c);

    float speed = Lerp(0.05f * maxvel, maxvel, power);
    float t = m_18 = ComputeTimeToImpact(speed * c, speed * s, height);

    if (from_range) {
        speed = sqrtf(t * t + LengthSqXZ(p) + 2.0f * t * p.z);
        _saturate(&speed, 0.0f, maxvel);
        m_vel.x = -p.x;
        m_vel.z = -(t + p.z);
        m_vel.y = 0.0f;
        m_vel.Normalize();
        ai::elevate(m_vel, s, c);
    } else {
        m_vel.x = 0.0f;
        m_vel.z = -c;
        m_vel.y = s;
    }

    CPnt3D q;

    RotateVectors(m, &m_vel, 1);
    m->Transform(&p, &q, 1);
    Set(q, (const CPnt3D &)m_vel, speed);
}

#endif
