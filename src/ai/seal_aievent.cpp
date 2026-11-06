/* seal_aievent.cpp -- an AI SEAL's events: CSealCtrlAi's EventTick and SetAiEvent, the dodges
 * (SOCOM 1's unit, __sinit_seal_aievent.cpp: CanHearAttenuatedExplosionEvent, EventTick, SetAiEvent,
 * CAiDodge::Destroy and its list's destructor, in that order; this tree keeps the AI states' units
 * under src/ai). OVL2 from after seal_aistate.cpp's tail (0x5F8D40) to 0x5FADEF, before
 * seal_aifollow.cpp's ~CAiSFollow. So far SetAiEvent (round 14: zseal.h includes ai.h for
 * CAiEvent::EVENT), CAiDodge::Destroy and EventTick (round 39); 0x5F95D0 (HandleEvent: EventTick's
 * two walks call it, as SOCOM 1's -- an earlier note had it EventTick by size) is open; 0x5F8D40 round 62;
 * defined in reverse of their image order (-inline deferred). */

#include <list>

#include "../gamez/zseal/zseal.h"
#include "../gamez/zvid/zvid.h"

/* a table's float (OVL2 0x5750D0, the SEALs' seal_ai.cpp) and the object at 0x67C a state tests
 * (OVL2 0x60A850, unwritten; seal_tasks.cpp's spelling) */
extern "C" float func_005750D0(unsigned char i);
extern "C" int func_0060A850(void *target);

/* the walks' and the look's tests, each a value the target materialises (inline; the names ours):
 * an event the AI did not take, the AI neither asleep nor dozing (bits 8 and 9 at 0x264), its own
 * event run out */
static inline bool Ignored(CSealCtrlAi *ai, CAiEvent *ev, float distSq)
{
    return !ai->HandleEvent(ev, distSq);
}

static inline bool Awake(const CSealCtrlAi *ai)
{
    int r = ai->m_aibit9 != 0;

    if (!r)
        r = ai->m_aibit8 != 0;
    return r ^ 1;
}

static inline bool Expired(const CAiEvent *ev)
{
    return ev->m_duration != CAiEvent::NoExpire && ev->m_duration <= 0.0f;
}

/* the AI's events of the frame: its own event aged; on alternate frames only (by the entity's id,
 * while the switch at OVL2 data 0x650BF0 holds), the events in use and then -- unless one was taken,
 * and while the AI neither sleeps nor dozes -- the persistent ones walked within a range the AI's
 * scale at 0x700 sets, each handed to HandleEvent until one is taken; then, unless the target is
 * held (0x60A850 and its bit 9), a look at the AI's own event while it lasts (OVL2 0x5FAAE0; SOCOM
 * 1's EventTick, which tested its own target and set its switches once). The sleep test is seal_ai.cpp's
 * int-held `||` (Awake: bit 9 as a bool, bit 8 only when clear, a bare xori), the expiry test a plain
 * `!= NoExpire && <= 0` (IsPersistent's materialised bool cost 8 bytes), held defined after the event's
 * Tick and handled and pos assigned after the alternate-frame return (each lands where the target sets
 * it), and each walk's event assigned as a statement of its own and then tested (`move a1,v0; beqz a1`:
 * the assignment inside the test branches on v0). The saved registers (this s0, the walk's hoisted
 * &m_distSq s1, held s2, the entity s3, pos s4, handled s5) want handled and pos numbered right after
 * `this`, ahead of the entity and held: declared uninitialised at the top, in that order (round 42:
 * MW's colouring replayed with the vregs relabelled; pos before handled, or either alone, does not) */
void CSealCtrlAi::EventTick(float dT)
{
    static bool alternate = true;
    bool handled;
    const CPnt3D *pos;
    CEntity *e = m_entity;

    ((CAiEvent *)m_event)->Tick(dT);
    bool held = false;
    if (func_0060A850(m_67c) && (m_6a8 & 0x200))
        held = true;
    if (!((zVid.frameNumber ^ e->m_id) & 1) && alternate)
        return;
    float scale = m_700;
    handled = false;
    pos = &e->m_posW;

    scale *= func_005750D0(6);
    if (m_eventMask) {
        CAiEvent::iterator it = CAiEvent::GetFirstEvent(CAiEvent::EC_INUSE);
        CAiEvent *ev;

        it.m_bit = e->m_index;
        it.m_owner = e->m_id;
        it.m_mask = m_eventMask;
        do {
            ev = CAiEvent::GetNextEvent(pos, it, scale);
            if (!ev)
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
            ev = CAiEvent::GetNextEvent(pos, it, scale);
            if (!ev)
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
 * delete is an inline destructor's). Byte-exact (round 58, open since round 13) under `#pragma
 * opt_common_subs off` in the body (reset after): the target loads the size twice at entry -- the
 * loop's guard first (`lw v1`), then the early return's (`lw v0`), the guard tested after -- which
 * only CSE off keeps apart (with it on MW merges the global's two loads, every spelling: rounds 13-56);
 * the guard's size a local read ahead of the early return; and the front read through one iterator
 * local handed to erase (front() and pop_front() each load the first node with CSE off: 0xA0) */
bool CAiDodge::Destroy()
{
#pragma opt_common_subs off
    unsigned int n = m_list.size();

    if (m_list.size() == 0)
        return false;
    if (n != 0)
        do {
            CList::iterator i = m_list.begin();
            CAiDodge *d = *i;

            m_list.erase(i);
            delete d;
        } while (m_list.size() != 0);
    return true;
}
#pragma opt_common_subs reset

/* what the reaction below reads and calls besides (the event's view, seal_ai.cpp's functions; the names ours) */
struct CSoundEventView {
    CPnt3D m_pos;   // 0x0
    float m_radius;   // 0xc
    unsigned char _10[0x18 - 0x10];
    unsigned int m_18 : 16;   // 0x18
    unsigned int m_1a_0 : 1;   // 0x1a
    unsigned int m_type : 15;
    signed char m_kind;   // 0x1c
};

extern "C" float GetTimeSinceLastSighting__9CAiTargetCFv(const void *target);
extern "C" void func_00560B20(CSealCtrlAi *ai, CSealCtrlAi *other, bool b);
extern "C" void func_00572240(CSealCtrlAi *ai, CEntity *alert, float amount);
extern "C" void func_00605770(CSealCtrlAi *ai, bool order, CHARACTER_SOUND sound, float vol);
extern "C" void func_00570D20(CSealCtrlAi *ai, float v);
extern "C" float func_00570D50(CSealCtrlAi *ai, float scale);
extern float data_00650C08;
extern float data_00650C10;
extern float data_00650C18;
extern float data_00650C20;
extern float data_0064FF70;

/* (an int-returning inline of a float compare: the target's li 1, c.le, bc1f, move zero, the andi at its use) */
static inline int Stale(float t)
{
    return t > 3.0f;
}

/* whether the AI's target is lost to it: none, or none held and not seen for three seconds (inline: the reaction's
 * tests; the name ours) */
static inline unsigned char TargetGone(CSealCtrlAi *ai)
{
    unsigned char gone = true;

    if (func_0060A850(ai->m_67c)) {
        bool held = func_0060A850(ai->m_67c) != 0;

        held = held ? (ai->m_6a8 & 0x200) != 0 : held;
        if (!held) {
            gone = Stale(GetTimeSinceLastSighting__9CAiTargetCFv(ai->m_67c));
        } else
            gone = false;
    }
    return gone;
}

/* a comrade's or an enemy's noise the AI can reach: by its kind -- a call for help, taken up when the AI's target is
 * gone and the caller has one: the alarm shared, an event state run to the place at a speed set by the distance and
 * the alert; a comrade's alarm remembered; a shot's noise: the Navy SEAL's answer, or the alert raised by how near and
 * how clear it was (OVL2 0x5F8D40, new in SOCOM II: HandleEvent calls it; the name ours) */
extern "C" bool func_005F8D40(CSealCtrlAi *ai, CSoundEventView *ev, CZSealBody *seal, bool seen, float dist)
{
    bool taken = false;
    signed char kind;
    CEntity *e = ai->m_entity;

    kind = ev->m_kind;
    signed char type = ev->m_type;
    CAiMapLoc loc;

    AiMaps.GetLocation(&ev->m_pos, loc);
    bool can = AiMaps.CanSeePoint(&e->m_posW, &ev->m_pos, ev->m_radius);

    if (!can)
        return false;
    switch (kind) {
    case 3:
    case 4:
    case 2:
        if (seal) {
            unsigned char gone = TargetGone(ai);

            if (gone && seen) {
                CSealCtrlAi *other = seal->GetSealCtrlAi();

                if (other && other->m_730 != CEntity::invalidId) {
                    if ((unsigned char)ai->GetAiAware() >= 2) {
                        func_00560B20(ai, other, true);
                        func_00572240(ai, CEntity::m_list.GetEntityById(other->m_730), 0.0f);
                    } else {
                        func_00560B20(ai, other, true);
                        if (TargetGone(ai) && ai->PushAiState(AI_STATE_EVENT, 0)) {
                            CAiState *s = ai->GetAiState();
                            CAiSEvent *st = s && s->m_state == AI_STATE_EVENT ? (CAiSEvent *)s : 0;

                            if (TargetGone(other)) {
                                st->SetFlags(2, 0);
                                st->SetEvent(type, &ev->m_pos, 0);
                            } else {
                                st->SetFlags(3, 0);
                                CEntity *t = *(CEntity **)other->m_67c;
                                const CPnt3D *q = t ? &t->m_posW : 0;

                                st->SetEvent(type, &ev->m_pos, q);
                            }
                            st->SetFlags(0x14, 0);
                            float r = ev->m_radius;
                            float near;

                            if (!(r <= 0.0f)) {
                                CPnt3D d;

                                ev->m_pos.Sub(&e->m_posW, &d);
                                near = (d.x * d.x + d.y * d.y + d.z * d.z) / r;
                            } else
                                near = 1.0f;
                            float speed = near * (2.0f - (float)other->GetAiAware());

                            speed = ai::_min<float>(data_00650C08, speed);
                            st->m_rnd_min = speed;
                            st->m_rnd_range = 0.0f;
                            st->m_80 = data_00650C08;
                            st->m_time = 2.0f + 2.0f * (4.656613e-10f * (float)rand());
                        }
                    }
                    taken = true;
                }
            }
        }
        break;
    case 0:
        if (seen) {
            taken = true;
            CAiEvent *own = (CAiEvent *)ai->m_event;

            own->m_type = (CAiEvent::EVENT)2;
            own->m_duration = 1.5f;
            own->m_pos = ev->m_pos;
        }
        break;
    case 1:
        if (seen) {
            if (ai->IsNavySeal() && seal->IsPlayer())
                ai->PlayAiSound((CHARACTER_SOUND)5, 1.0f, (CAiEvent::EVENT)kind);
        } else {
            float raise = data_00650C10 * (1.0f - ai->m_aware.m_value);
            float radius = ev->m_radius;

            if (!(radius <= 0.0f))
            {
                float f = dist / radius;

                raise += (1.0f - f) * data_00650C18;
            }
            if (!AiMaps.CanSeeLine(e->m_mapLocValid, loc, 0))
                raise *= data_00650C20;
            bool call = false;

            if (TargetGone(ai) || (unsigned char)ai->GetAiAware() < 3)
                call = true;
            CAiEvent *own = (CAiEvent *)ai->m_event;

            own->m_type = (CAiEvent::EVENT)2;
            own->m_duration = 1.5f;
            own->m_pos = ev->m_pos;
            if (call)
                func_00605770(ai, true, (CHARACTER_SOUND)70, 0.5f);
            func_00570D20(ai, func_00570D50(ai, 1.0f) - raise);
            ai->m_404 += data_0064FF70;
        }
        break;
    }
    return taken;
}

