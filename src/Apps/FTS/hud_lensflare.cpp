/* hud_lensflare.cpp -- the HUD's lens flares (OVL1 0x1FF060-0x20157F): SOCOM 1's unit (its DWARF names
 * it, Z:\dev\Apps\FTS\hud_lensflare.cpp), between the letterbox and the pause menu as there. SOCOM
 * II replaced CLensFlareSaturate with a manager of flare entries; only the manager's Tick so far,
 * the unit's first function in the image. Defined in reverse of their image order (-inline
 * deferred). */

#include "hud_lensflare.h"

/* the flare system (OVL1 bss 0x488F10; no rows yet) and its handle release */
struct CFlareSystem;
extern CFlareSystem data_00488F10;
extern "C" void func_003364D0(CFlareSystem *system, int handle);

/* every flare deleted, the list emptied, inactive.
 * OPEN (4 bytes short): the target keeps three saved registers -- this, the end (this + 4) and
 * this + 8, the address begin() reads, taken before the loop for the final clear(), which
 * recomputes the end from this. MW here keeps two; the end taken after begin(), erase(begin(), e)
 * and end() in the test all miss too */
void CLensFlareManager::Clear()
{
    if (size()) {
        std::list<CLensFlareEntry *>::iterator i, e = end();

        for (i = begin(); i != e; ++i)
            delete *i;
        clear();
    }
    m_active = false;
}

/* the flares read from lensflare.rdr, active if there were any */
void CLensFlareManager::Init()
{
    m_active = false;
    LoadRdr("lensflare.rdr");
    if (size())
        m_active = true;
}

/* every flare's handle given back to the flare system */
void CLensFlareManager::Reset()
{
    std::list<CLensFlareEntry *>::iterator i, e = end();

    for (i = begin(); i != e; ++i) {
        CLensFlareEntry *flare = *i;

        if (flare->m_handle >= 0)
            func_003364D0(&data_00488F10, flare->m_handle);
        flare->m_handle = -1;
    }
}

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
