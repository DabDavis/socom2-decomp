/* hud_lensflare.cpp -- the HUD's lens flares (OVL1 0x1FF060-0x20157F): SOCOM 1's unit (its DWARF names
 * it, Z:\dev\Apps\FTS\hud_lensflare.cpp), between the letterbox and the pause menu as there. SOCOM
 * II replaced CLensFlareSaturate with a manager of flare entries. Written: the manager's Tick,
 * PreDITick, Init, ReInit and Reset (the unit's first functions in the image); Clear open. Defined
 * in reverse of their image order (-inline deferred). */

#include "../../gamez/znode/znode.h"
#include "hud_lensflare.h"

/* the flare system (OVL1 bss 0x488F10; no rows yet): a flare made at a point with a size (its
 * handle back), moved, moved and sized, asked whether it shows, and released */
struct CFlareSystem;
extern CFlareSystem data_00488F10;
extern "C" int func_00336650(CFlareSystem *system, CPnt3D *pos, float size);
extern "C" void func_00336580(CFlareSystem *system, int handle, CPnt3D *pos);
extern "C" void func_003365E0(CFlareSystem *system, int handle, CPnt3D *pos, float size);
extern "C" bool func_00336520(CFlareSystem *system, int handle, void *query);
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

/* every flare on a node placed at the node's origin and handed to the flare system: made if it has
 * no handle yet, else moved (CHUD::ReInit_Init's last call on the manager; the name ours) */
void CLensFlareManager::ReInit()
{
    std::list<CLensFlareEntry *>::iterator i, e = end();

    for (i = begin(); i != e; ++i) {
        CLensFlareEntry *flare = *i;

        if (flare->m_node) {
            flare->m_pos = CPnt3D::zero;
            flare->m_node->TransformToWorld(flare->m_pos);
            if (flare->m_handle < 0)
                flare->m_handle = func_00336650(&data_00488F10, &flare->m_pos, flare->m_size);
            else
                func_003365E0(&data_00488F10, flare->m_handle, &flare->m_pos, flare->m_size);
        }
    }
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

/* while active, before the world draws: each flare that is on asked whether it shows, and a
 * tracking one moved to its node's origin */
void CLensFlareManager::PreDITick(float dT)
{
    if (m_active && size()) {
        std::list<CLensFlareEntry *>::iterator i, e = end();

        for (i = begin(); i != e; ++i) {
            CLensFlareEntry *flare = *i;

            if (flare->m_on && flare->m_handle >= 0) {
                if (func_00336520(&data_00488F10, flare->m_handle, flare->m_90))
                    flare->m_visible = true;
                else
                    flare->m_visible = false;
                if (flare->m_tracking && flare->m_node) {
                    flare->m_pos = CPnt3D::zero;
                    flare->m_node->TransformToWorld(flare->m_pos);
                    func_00336580(&data_00488F10, flare->m_handle, &flare->m_pos);
                }
            }
        }
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
