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

#include "../zanim/zanim.h"
#include "znode.h"

extern CSched_Manager zTaskScheduler;

namespace zdb {

/* the prop's translation: its matrix's last row, or the translation row of its rotation form
 * (SOCOM 1's, the same eight words) */
CPnt3D *CClutter::GetTranslate()
{
    if (m_quat)
        return (CPnt3D *)m_matrix.m_matrix[2];
    return (CPnt3D *)m_matrix.m_matrix[3];
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
