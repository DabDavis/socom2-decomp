/* seal_anim.cpp -- the SEAL animated: its blends pushed and popped, its root and vertical, its
 * adrenaline (SOCOM 1's unit, __sinit_seal_anim.cpp: Post, GetRootPos, SetVertical, the CSealAnim
 * blender, Animate, AdrenalineIncr and Adrenaline last). OVL2 from seal_ai.cpp's template tail
 * (0x575CF0) to 0x5781BF (Adrenaline), before seal_body.cpp. Defined in reverse of their image order
 * (-inline deferred). */

#include "zseal.h"

extern "C" {
/* AnimSet::BaseAnim: an anim type's base anim, or -1 (SOCOM 1's BaseAnim__7AnimSetF8AnimType, called
 * here where SOCOM 1's IsCurrentBlend calls it; OVL2 0x5E1A90, the kit lane's range) */
AnimType func_005E1A90(AnimSet *set, AnimType type);
}

/* the SEAL's adrenaline (OVL2 0x5781B0, the unit's last; SOCOM 1's, the same load) */
CAdrenaline *CZSealBody::Adrenaline()
{
    return m_adrenaline;
}

/* whether an anim type is the blend on top: itself, or its base anim (OVL2 0x577E40; SOCOM 1's,
 * which compared the base anim only) */
bool CZSealBody::IsCurrentBlend(AnimType type) const
{
    if (type == m_curblend)
        return true;
    return m_curblend == func_005E1A90(m_animset, type);
}
