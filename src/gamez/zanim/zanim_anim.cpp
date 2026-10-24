/* zanim_anim.cpp -- CZAnim, one anim: its nodes and references, sequences and state (OVL1
 * 0x26D370-...). The unit name is SOCOM 1's (__sinit_zanim_anim.cpp, which follows
 * anim_main.cpp's). Defined in reverse of their image order (-inline deferred). */

#include "zanim.h"

/* the anim and its chain started again, unless it activates on demand (3) */
void CZAnim::RestartClones()
{
    if (m_activation == 3)
        return;
    for (CZAnim *anim = this; anim; anim = anim->m_sibling_anim)
        anim->Start();
}

void CZAnim::Priority(float priority)
{
    m_priority = priority;
}
