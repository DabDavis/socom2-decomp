/* grid_clutter.cpp -- the grid's clutter: the props scattered on the cells (CClutter, CClutterList)
 * and their animation by effectors (CClutterAnimManager, CClutterAnim) (OVL1 0x2D7EA0-0x2D9CBF).
 * The unit is SOCOM 1's (its static initialiser, __sinit_grid_clutter.cpp, registers CStack's
 * destructor for its own matrix stack, after grid_di.cpp's); SOCOM 1's order runs from
 * CClutterAnimManager::UnRegisterEffector to CClutterList::Add, CInput's unit after it. Defined in
 * reverse of their image order (-inline deferred).
 * Written so far: GetTranslate, ClearEffectors. */

#include "znode.h"

namespace zdb {

/* the effectors let go of (SOCOM 1's).
 * OPEN (0x7C of 0x8C): the target keeps the list, the sentinel and &sentinel.next in three saved
 * registers and reads begin() for the clear through the last; here the clear re-reads it off this.
 * Not a reference to the list, nor the end taken first (both longer) */
void CClutterAnimManager::ClearEffectors()
{
    std::list<CClutterEffector *>::iterator it = m_ClutterEffectors.begin(),
                                            e = m_ClutterEffectors.end();

    for (; it != e; ++it)
        delete *it;
    m_ClutterEffectors.clear();
}

/* the prop's translation: its matrix's last row, or the translation row of its rotation form
 * (SOCOM 1's, the same eight words) */
CPnt3D *CClutter::GetTranslate()
{
    if (m_quat)
        return (CPnt3D *)m_matrix.m_matrix[2];
    return (CPnt3D *)m_matrix.m_matrix[3];
}

}
