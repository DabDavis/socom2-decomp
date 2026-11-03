/* seal_aievent.cpp -- an AI SEAL's events: CSealCtrlAi's EventTick and SetAiEvent, the dodges
 * (SOCOM 1's unit, __sinit_seal_aievent.cpp: CanHearAttenuatedExplosionEvent, EventTick, SetAiEvent,
 * CAiDodge::Destroy and its list's destructor, in that order; this tree keeps the AI states' units
 * under src/ai). OVL2 from after seal_aistate.cpp's tail (0x5F8D40) to 0x5FADEF, before
 * seal_aifollow.cpp's ~CAiSFollow. So far CAiDodge::Destroy: SetAiEvent (0x5FAD00, SOCOM 1's code
 * with the event at 0x628) waits on zseal.h, which cannot declare a member taking CAiEvent::EVENT
 * without ai.h. Defined in reverse of their image order (-inline deferred). */

#include <list>

#include "../gamez/zseal/zseal.h"

/* (OVL2 0x5FAD00; SOCOM 1's, the event 0x1A8 further on: SEAL_AI_PARAMS's bytes at 0x628 read as
 * the CAiEvent they are -- the kind into its 15 bits, the duration, the place copied field by
 * field -- and the kind handed back, `move v0,a1`) */
CAiEvent::EVENT CSealCtrlAi::SetAiEvent(CAiEvent::EVENT e, float duration, const CPnt3D *pos)
{
    CAiEvent *ev = (CAiEvent *)m_event;

    ev->m_type = e;
    ev->m_duration = duration;
    ev->m_pos = *pos;
    return e;
}

/* a dodge the AI makes (SOCOM 1's CAiDodge; no DWARF: only what Destroy touches) and the list of
 * them (SOCOM 1's CAiDodge::CList, whose destructor its unit emits; OVL1 bss 0x44D648) */
class CAiDodge {
public:
    class CList : public std::list<CAiDodge *> {
    };

    ~CAiDodge() {}

    /* every dodge deleted; whether there were any (SOCOM 1's, static) */
    static bool Destroy();

    static CList m_list;
};

/* (OVL2 0x5FAD50; SOCOM 1's: each taken off the front and deleted -- the null test before the
 * delete is an inline destructor's).
 * OPEN, 0x90 against 0x98: the target loads the size twice at entry, the loop's guard first (`lw
 * v1` then `lw v0` for the early return) and tests the guard with `beqz v1` after it; MW here
 * reuses the early return's load for the guard (`beqz v0`). The plain `while` drops the guard
 * (0x80, SOCOM 1's shape); the size taken into a local first gives 0x90 too; a reference local
 * to the list gives 0x98 with its address in v1 (11 instructions) */
bool CAiDodge::Destroy()
{
    if (m_list.size() == 0)
        return false;
    if (m_list.size() != 0)
        do {
            CAiDodge *d = m_list.front();

            m_list.pop_front();
            delete d;
        } while (m_list.size() != 0);
    return true;
}
