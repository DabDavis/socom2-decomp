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

/* the SEAL's animation blender (SOCOM 1's CSealAnim: only what the one-anim pass sets -- the names
 * ours) and the one the unit uses (OVL1 bss 0x44CDE0, a pointer) */
struct CSealAnimBlender {
    /* emptied and given a SEAL (an inline: both stores through one load of the global) */
    void Begin(CZSealBody *seal)
    {
        m_count = 0;
        m_seal = seal;
    }

    int m_count;   // 0x0
    CZSealBody *m_seal;   // 0x4
};

extern "C" {
extern CSealAnimBlender *data_0044CDE0;
/* an anim added to the blender at its frame, fraction and weight, and the blend applied (OVL2
 * 0x5777D0, 0x577700, not written yet) */
void func_005777D0(CSealAnimBlender *b, CZSIScriptCommon *anim, float ffrac, float wt, short frame);
void func_00577700(CSealAnimBlender *b);
}

/* a SEAL posed by one anim alone: the blender emptied and given the SEAL, the anim added, the pose
 * applied (OVL2 0x577DF0, new in SOCOM II: seal_cfg.cpp's 0x58C600 calls it; the name unknown).
 * OPEN, 0x50 against 0x4C (1 instruction more, the rest reordered): the two stores through one load
 * of the global needed the inline Begin (written out, MW reloads the global between them); left, MW
 * copies the anim pointer to v1 (`move v1,a1`) and loads its m_anim into a1 second, where the target
 * loads the frame (a2), the two floats, then m_anim into a1 last, off a1 itself. Tried: the floats
 * before or around the frame in 0x5777D0's prototype (the same code), the fields in locals first, the
 * anim through a local. 0x5777D0's parameter order is unproven. */
extern "C" void func_00577DF0(CZSealBody *seal, ACTIVE_ANIM *anim)
{
    data_0044CDE0->Begin(seal);
    func_005777D0(data_0044CDE0, anim->m_anim, anim->m_ffrac, anim->m_wt, anim->m_frame);
    func_00577700(data_0044CDE0);
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
