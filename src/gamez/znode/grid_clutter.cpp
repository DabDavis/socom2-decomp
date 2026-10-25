/* grid_clutter.cpp -- the grid's clutter: the props scattered on the cells (CClutter, CClutterList)
 * and their animation by effectors (CClutterAnimManager, CClutterAnim) (OVL1 0x2D7EA0-0x2D9CBF).
 * The unit is SOCOM 1's (its static initialiser, __sinit_grid_clutter.cpp, registers CStack's
 * destructor for its own matrix stack, after grid_di.cpp's); SOCOM 1's order runs from
 * CClutterAnimManager::UnRegisterEffector to CClutterList::Add, CInput's unit after it. Defined in
 * reverse of their image order (-inline deferred).
 * Written so far: GetTranslate, the manager's list walks and its destructor. The pointer-list
 * walks run on include/list's begin() and clear(): SOCOM II's begin() reads the first node through
 * a reference to the sentinel, so &end_.next_ is computed once and shared by the walk and the clear
 * after it (the target keeps it in the lowest saved register) -- znode.h's stand-ins for it
 * (ListBegin, ListEnd, ListClear) retired, every walk alike through the header. */

#include <algorithm>

#include "../zanim/zanim.h"
#include "znode.h"

extern CSched_Manager zTaskScheduler;

namespace zdb {

/* two props traded (MSL's swap through references, written here: MW inlines no template
 * function; the name is ours) */
inline void ClutterSwap(CClutter *&a, CClutter *&b)
{
    CClutter *t = a;

    a = b;
    b = t;
}

/* one prop inserted before position, through the pointer vector's pod insert (MSL's
 * insert(iterator, const T &), which include/vector lacks: the position a class iterator passed by
 * value, so copied, then its pointer handed on; the name is ours) */
inline void ClutterInsert(CClutterList *list, CClutterList::iterator pos, CClutter *const &x)
{
    list->std::__vector_pod<unsigned int, std::allocator<unsigned int> >::insert(
        (unsigned int *)pos.m_p, 1, (const unsigned int &)x);
}

/* a prop listed: the list's repel flag and farthest fade kept up, room made ten at a time, the prop
 * held once more (refused at fifteen holders), put at the end when its node has the flag, else
 * after the props put in front so far; then moved down past every fading prop that fades nearer
 * (SOCOM 1's; the holders and the front insertion SOCOM II's).
 * OPEN, 13 instructions (the right size), registers only: in the front insertion the target
 * loads the index into a1 and its increment into t0, and in the sort keeps &data[i] in a1 and the
 * prop in a2 -- here each pair the other way round (both the first-made temporary in the lower
 * register there). The swap traded as words (a typed view, or a volatile read) is what makes MW
 * reload the element as the target does; a swap of the CClutter pointers reuses the tested one.
 * Tried: the insert as a member, through begin(), the index taken first; the counter's types and
 * declaration places; a local or a reference for the tested prop */
bool CClutterList::Add(CClutter *clutter)
{
    if (size_ == 0) {
        m_ContainsRepelClutter = 0;
        m_max_farFadeSq = 0.0f;
    }
    if (clutter->m_quat)
        m_ContainsRepelClutter = 1;
    if (clutter->m_bit0 && clutter->m_farFadeSq > m_max_farFadeSq)
        m_max_farFadeSq = clutter->m_farFadeSq;
    if (size_ == capacity_)
        reserve(size_ + 10);
    if (!clutter->AddHolder())
        return false;
    if (clutter->m_node->m_reflective)
        push_back(clutter);
    else
        ClutterInsert(this, iterator(&(*this)[m_socom2_14++]), clutter);
    if (size() > 1) {
        for (int i = size() - 2; i >= 0; i--) {
            if ((*this)[i]->m_bit0) {
                if (!clutter->m_bit0 || clutter->m_farFadeSq > (*this)[i]->m_farFadeSq)
                    std::swap((unsigned int &)(*this)[i], (unsigned int &)(*this)[i + 1]);
                else
                    return true;
            }
        }
    }
    return true;
}

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
    std::list<CClutterEffector *>::iterator it = m_ClutterEffectors.begin(), e = m_ClutterEffectors.end();

    for (; it != e; ++it)
        delete *it;
    m_ClutterEffectors.clear();
}

/* the clutter task taken off the scheduler, the animations and the effectors let go of (SOCOM
 * 1's). The effector list's implicit destructor reads its first node through the inlined
 * ClearEffectors' &end_.next_ (lw v0,0(s2)): byte-exact since include/list's ~__list_deleter binds
 * the sentinel by reference, as SOCOM II's begin() does */
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
    std::list<CClutterAnim *>::iterator it = m_ClutterAnims.begin(), e = m_ClutterAnims.end();

    for (; it != e; ++it) {
        if (detach && (*it)->m_clutter)
            (*it)->m_clutter->m_anim = 0;
        delete *it;
    }
    m_ClutterAnims.clear();
}

/* an owner's effector deleted and taken off the list (SOCOM 1's) */
void CClutterAnimManager::UnRegisterEffector(const void *owner)
{
    std::list<CClutterEffector *>::iterator it = m_ClutterEffectors.begin(), e = m_ClutterEffectors.end();

    for (; it != e; ++it) {
        if ((*it)->m_owner == owner) {
            delete *it;
            m_ClutterEffectors.erase(it);
            return;
        }
    }
}

}
