/* zbody.h -- the animated body: CBody (a base of CZSealBody), its parts (CZBodyPart), the motion
 * scripts it plays (CZSIScript and its objects, CZBodyAnim), the archives they load from
 * (CScriptArchive) and the small helpers between them (CSequence, the plane and quaternion
 * functions). OVL1 0x2869D0-0x290360, five units in SOCOM 1's link order: bone_math.cpp (ours: the
 * helpers), bone_archive.cpp and bone_init.cpp (the press beta's asserts name both), si_object.cpp
 * (the same), si_script.cpp and body.cpp (SOCOM 1's static initialisers name them).
 *
 * The layouts are SOCOM 1's DWARF moved to where SOCOM II's code puts each member; only what a
 * written function touches is named. Self-contained over zmath.h, znode.h and zar.h, so zseal.h
 * can include it for CZSealBody's CBody base. The header's name is ours. */
#ifndef ZBODY_H
#define ZBODY_H

#include <list>
#include <vector>

#include "../zmath/zmath.h"
#include "../znode/znode.h"
#include "../zar/zar.h"

using zdb::CNode;

/* an up-hold-down envelope: its weight rises over m_upT, holds, and falls over m_downT while
 * m_currT counts down (SOCOM 1's DWARF, 0x10 bytes) */
class CSequence {
public:
    float Weight();
    float LinearWeight();
    float Tick(float dT);
    void Clear();
    void Start(float up, float hold, float down);

    float m_upT;   // 0x0
    float m_holdT;   // 0x4
    float m_downT;   // 0x8
    float m_currT;   // 0xc
};

/* a bone of the body: its node and its pose relative to its parent (SOCOM 1's DWARF, 0x50 bytes
 * there; SOCOM II's is 0x44 -- the parent moved ahead of the rotation, the id and two bits last) */
class CZBodyPart {
public:
    /* the node placed where the part is (SOCOM 1's) */
    void Orient();

    CPnt3D m_pos;   // 0x0
    CNode *m_node;   // 0xc
    CPnt3D m_saved_pos;   // 0x10
    CZBodyPart *m_parent;   // 0x1c
    CQuat m_rot;   // 0x20
    CQuat m_saved_rot;   // 0x30
    short m_id;   // 0x40
    unsigned char m_bit0 : 1;   // 0x42
    unsigned char m_bit1 : 1;
};

/* the animated body (SOCOM 1's DWARF: 0x48 bytes, its table first; SOCOM II's is 0x90 -- its
 * constructor, 0x28EDD0, zeroes through 0x8C). SOCOM II's table (0x406100) has two slots, the
 * destructor and NullNode, an inline returning -1. */
class CBody {
public:
    virtual ~CBody();
    virtual int NullNode() const { return -1; }

    /* a part's position and rotation in the body's space: up its chain of parents (SOCOM 1's) */
    void PartInSpace(CZBodyPart *part, const CPnt3D *offset, CPnt3D *pos, CQuat *rot) const;

    short m_state;   // 0x4: the SEAL's state (SOCOM 1's char at 0xd)
    unsigned char _6[0x90 - 0x6];
};

/* the plane and quaternion helpers (bone_math.cpp, SOCOM 1's free functions) */
float LineIntersectPlane(float d, const CPnt3D &n, const CPnt3D &a, const CPnt3D &b);
float PlaneConstant(const CPnt3D &n, const CPnt3D &p);
void ClipToBox(const CPnt3D &a, const CPnt3D &b, CPnt3D &out, float x0, float x1, float z0, float z1);
void GetFractionalRotations(const CPnt3D *v, float *a, float *b);
float quadW(float t);
CQuat *QFromRot(CQuat *q, const CPnt3D *rot);

#endif
