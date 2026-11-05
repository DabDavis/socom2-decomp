/* seal_aievent.cpp -- an AI SEAL's events: CSealCtrlAi's EventTick and SetAiEvent, the dodges
 * (SOCOM 1's unit, __sinit_seal_aievent.cpp: CanHearAttenuatedExplosionEvent, EventTick, SetAiEvent,
 * CAiDodge::Destroy and its list's destructor, in that order; this tree keeps the AI states' units
 * under src/ai). OVL2 from after seal_aistate.cpp's tail (0x5F8D40) to 0x5FADEF, before
 * seal_aifollow.cpp's ~CAiSFollow. So far SetAiEvent (round 14: zseal.h includes ai.h for
 * CAiEvent::EVENT), CAiDodge::Destroy and EventTick (round 39); 0x5F95D0 (HandleEvent: EventTick's
 * two walks call it, as SOCOM 1's -- an earlier note had it EventTick by size) and 0x5F8D40 are
 * open. Defined in reverse of their image order (-inline deferred). */

#include <list>

#include "../gamez/zseal/zseal.h"
#include "../gamez/zvid/zvid.h"

/* a table's float (OVL2 0x5750D0, the SEALs' seal_ai.cpp) and the object at 0x67C a state tests
 * (OVL2 0x60A850, unwritten; seal_tasks.cpp's spelling) */
extern "C" float func_005750D0(unsigned char i);
extern "C" bool func_0060A850(void *target);

/* the walks' and the look's tests, each a value the target materialises (inline; the names ours):
 * an event the AI did not take, the AI neither asleep nor dozing (bits 8 and 9 at 0x264), its own
 * event run out */
static inline bool Ignored(CSealCtrlAi *ai, CAiEvent *ev, float distSq)
{
    return !ai->HandleEvent(ev, distSq);
}

static inline bool Awake(const CSealCtrlAi *ai)
{
    return !(ai->m_aibit9 || ai->m_aibit8);
}

static inline bool Expired(const CAiEvent *ev)
{
    return !ev->IsPersistent() && ev->m_duration <= 0.0f;
}

/* the AI's events of the frame: its own event aged; on alternate frames only (by the entity's id,
 * while the switch at OVL2 data 0x650BF0 holds), the events in use and then -- unless one was taken,
 * and while the AI neither sleeps nor dozes -- the persistent ones walked within a range the AI's
 * scale at 0x700 sets, each handed to HandleEvent until one is taken; then, unless the target is
 * held (0x60A850 and its bit 9), a look at the AI's own event while it lasts (OVL2 0x5FAAE0; SOCOM
 * 1's EventTick, which tested its own target and set its switches once).
 * OPEN, 0x230 against 0x218 (written round 39, the first draft): the target's walks call
 * GetNextEvent once at the loop's top (no rotated copy), their back edge a materialised
 * `!HandleEvent` (sltu/xori, bnez); the sleep and expiry tests are values too (inline bools here,
 * Ignored/Awake/Expired); the scale's field loads before func_005750D0's call into $f20; the saved
 * registers are this s0, the entity s3, held s2, handled s5, the position s4 and the walk's
 * &m_distSq s1 (here this s5, the entity s4) -- the do/while shapes still leave six instructions
 * over; next: the walks as one inline over the event class (the two are alike), the iterator's
 * fields set through it */
void CSealCtrlAi::EventTick(float dT)
{
    static bool alternate = true;
    CEntity *e = m_entity;
    bool held = false;
    bool handled = false;

    ((CAiEvent *)m_event)->Tick(dT);
    if (func_0060A850(m_67c) && (m_6a8 & 0x200))
        held = true;
    if (!((zVid.frameNumber ^ e->m_id) & 1) && alternate)
        return;
    float scale = m_700;
    const CPnt3D *pos = &e->m_posW;

    scale *= func_005750D0(6);
    if (m_eventMask) {
        CAiEvent::iterator it = CAiEvent::GetFirstEvent(CAiEvent::EC_INUSE);
        CAiEvent *ev;

        it.m_bit = e->m_index;
        it.m_owner = e->m_id;
        it.m_mask = m_eventMask;
        do {
            if ((ev = CAiEvent::GetNextEvent(pos, it, scale)) == 0)
                goto inuse_done;
        } while (Ignored(this, ev, it.m_distSq));
        handled = true;
    inuse_done:;
    }
    if (!Awake(this))
        return;
    if (!handled) {
        CAiEvent::iterator it = CAiEvent::GetFirstEvent(CAiEvent::EC_PERSIST);
        CAiEvent *ev;

        it.m_bit = e->m_index;
        it.m_owner = e->m_id;
        it.m_mask = m_eventMask;
        do {
            if ((ev = CAiEvent::GetNextEvent(pos, it, scale)) == 0)
                break;
        } while (Ignored(this, ev, it.m_distSq));
    }
    if (held)
        return;
    if (!Expired((CAiEvent *)m_event))
        SetLookPosW((AI_LOOK)5, &((CAiEvent *)m_event)->m_pos);
}

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
