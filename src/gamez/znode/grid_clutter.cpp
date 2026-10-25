/* grid_clutter.cpp -- the grid's clutter: the props scattered on the cells (CClutter, CClutterList)
 * and their animation by effectors (CClutterAnimManager, CClutterAnim) (OVL1 0x2D7EA0-0x2D9CBF).
 * The unit is SOCOM 1's (its static initialiser, __sinit_grid_clutter.cpp, registers CStack's
 * destructor for its own matrix stack, after grid_di.cpp's); SOCOM 1's order runs from
 * CClutterAnimManager::UnRegisterEffector to CClutterList::Add, CInput's unit after it. Defined in
 * reverse of their image order (-inline deferred).
 * Written so far: GetTranslate, the manager's list walks and its destructor. The pointer-list
 * walks run on znode.h's ListBegin/ListEnd/ListClear: SOCOM II's begin() builds the first node from
 * a reference to the sentinel's link, so &end_.next_ is computed once and shared by the walk and
 * the clear after it (the target keeps it in the lowest saved register). */

#include <algorithm>

#include "../zanim/zanim.h"
#include "znode.h"

extern CSched_Manager zTaskScheduler;

namespace zdb {

/* a point set from three values (the arguments evaluated right to left: z, y and x loaded before
 * the stores; the name is ours) */
inline void ClutterPntSet(float *p, float x, float y, float z)
{
    p[0] = x;
    p[1] = y;
    p[2] = z;
}

/* a prop let go of by a list: its holder count dropped, the prop deleted at none (the function
 * object ours) */
struct CClutterRelease {
    void operator()(CClutter *c) const
    {
        c->m_count--;
        if (c->m_count == 0)
            delete c;
    }
};

/* every prop let go of (an MSL for_each, its class iterators in stack slots), the list emptied
 * (SOCOM 1's; SOCOM II clears its word at 0x14 too) */
void CClutterList::Free()
{
    std::for_each(begin(), end(), CClutterRelease());
    size_ = 0;
    m_socom2_14 = 0;
}

/* the unit matrix, the flags cleared each through its setter, no animation, the node kept (SOCOM
 * 1's; SOCOM II's flag word has three fields more) */
CClutter::CClutter(CNode *node)
{
    SetBit2(false);
    SetBit0(false);
    SetBit12(false);
    SetQuat(false);
    SetBit3(false);
    SetBits4(0);
    SetBit13(false);
    SetCount(0);
    SetBits18(0);
    m_node = 0;
    m_anim = 0;
    m_5c = -1;
    m_node = node;
}

bool CClutter::Read(zar::CZAR *zar)
{
    if (m_node) {
        m_node->SetDynamic(true, true);
        m_node->SetApplyMultipassEffects(false);
        m_node->SetApplyDecals(false);
    }
    zar->Fetch("params", this, 0x60);
    SetCount(0);
    bool ok = zar->Fetch("scale_inverse", &m_scale_inverse, 4);

    return ok;
}

/* the prop's translation: its matrix's last row, or the translation row of its rotation form
 * (SOCOM 1's, the same eight words) */
CPnt3D *CClutter::GetTranslate()
{
    if (m_quat)
        return (CPnt3D *)m_matrix.m_matrix[2];
    return (CPnt3D *)m_matrix.m_matrix[3];
}

/* the prop's matrix: its own, or -- in the rotation form -- built in the clutter manager's matrix
 * from the product of its two rotations (normalised), its scale (row 2's w) and its translation
 * (SOCOM 1's; SOCOM II normalises the product and skips a unit scale).
 * OPEN, 19 instructions, registers only (right size): the target copies `this` into s1 after the
 * world's load and keeps the matrix in s0; here `this` takes s0 in the test's delay slot. Tried:
 * the matrix declared first, after the quaternion, at function scope; the world, the manager or
 * `this` through a local; the rotation path as an inline helper; the translation through a
 * pointer */
CMatrix *CClutter::GetMatrix()
{
    if (m_quat) {
        CMatrix *m = &CWorld::m_world->m_grid.m_ClutterAnimManager->m_matrix;
        CQuat q;

        m->SetIdentity();
        ((CQuat *)m_matrix.m_matrix[1])->Mul((CQuat *)m_matrix.m_matrix[0], &q);
        q.Normalize();
        q.ToMatrix(m);
        if (m_matrix.m_matrix[2][3] != 1.0f) {
            CMatrix s;
            float k = m_matrix.m_matrix[2][3];

            s.m_matrix[0][0] = k;
            s.m_matrix[1][1] = k;
            s.m_matrix[2][2] = k;
            sceVu0MulMatrix(m->m_matrix, s.m_matrix, m->m_matrix);
        }
        ClutterPntSet(m->m_matrix[3], m_matrix.m_matrix[2][0], m_matrix.m_matrix[2][1],
                      m_matrix.m_matrix[2][2]);
        return m;
    }
    return &m_matrix;
}

/* the effectors let go of (SOCOM 1's) */
void CClutterAnimManager::ClearEffectors()
{
    PtrListBase::iterator it = ListBegin(m_ClutterEffectors), e = ListEnd(m_ClutterEffectors);

    for (; it != e; ++it)
        delete (CClutterEffector *)*it;
    ListClear(m_ClutterEffectors);
}

/* the clutter task taken off the scheduler, the animations and the effectors let go of (SOCOM
 * 1's).
 * OPEN, 1 instruction (right size): the effector list's implicit destructor reads its first node
 * through the inlined ClearEffectors' &end_.next_ (lw v0,0(s2)), where include/list's
 * ~__list_deleter builds it by value from &end_ (lw v0,4(s3)) -- SOCOM II's deleter destructor
 * binds the link by reference as ListBegin does (waits on include/list) */
CClutterAnimManager::~CClutterAnimManager()
{
    zTaskScheduler.RemoveTask("CClutterAnimManager", false);
    ClearAnimations(false);
    ClearEffectors();
}

/* every animation deleted -- its prop's rotation reset and its node deleted -- and the list
 * emptied; with detach, each prop forgets its animation first (SOCOM 1's) */
void CClutterAnimManager::ClearAnimations(bool detach)
{
    PtrListBase::iterator it = ListBegin(m_ClutterAnims), e = ListEnd(m_ClutterAnims);

    for (; it != e; ++it) {
        if (detach && ((CClutterAnim *)*it)->m_clutter)
            ((CClutterAnim *)*it)->m_clutter->m_anim = 0;
        delete (CClutterAnim *)*it;
    }
    ListClear(m_ClutterAnims);
}

/* an owner's effector deleted and taken off the list (SOCOM 1's) */
void CClutterAnimManager::UnRegisterEffector(const void *owner)
{
    PtrListBase::iterator it = ListBegin(m_ClutterEffectors), e = ListEnd(m_ClutterEffectors);

    for (; it != e; ++it) {
        if (((CClutterEffector *)*it)->m_owner == owner) {
            delete (CClutterEffector *)*it;
            m_ClutterEffectors.erase(it);
            return;
        }
    }
}

}
