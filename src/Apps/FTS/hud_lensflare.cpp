/* hud_lensflare.cpp -- the HUD's lens flares (OVL1 0x1FF060-0x20157F): SOCOM 1's unit (its DWARF names
 * it, Z:\dev\Apps\FTS\hud_lensflare.cpp), between the letterbox and the pause menu as there. SOCOM
 * II replaced CLensFlareSaturate with a manager of flare entries; only the manager's Tick so far,
 * the unit's first function in the image. Defined in reverse of their image order (-inline
 * deferred). */

#include "hud_lensflare.h"

/* while active, every flare ticked. The iterator is assigned from begin(), so it lives in memory
 * (the copy is bitwise), as CZNewHudMap's finders'; the end is taken once into a local (the target
 * keeps it in a saved register, where end() in the test recomputes it from `this`) */
void CLensFlareManager::Tick(float dT)
{
    if (m_active && size()) {
        std::list<CLensFlareEntry *>::iterator i, e = end();

        for (i = begin(); i != e; ++i)
            (*i)->Tick(dT);
    }
}
