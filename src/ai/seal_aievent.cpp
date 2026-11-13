/* seal_aievent.cpp -- an AI SEAL's events: CSealCtrlAi's EventTick and SetAiEvent, the dodges
 * (SOCOM 1's unit, __sinit_seal_aievent.cpp: CanHearAttenuatedExplosionEvent, EventTick, SetAiEvent,
 * CAiDodge::Destroy and its list's destructor, in that order; this tree keeps the AI states' units
 * under src/ai). OVL2 from after seal_aistate.cpp's tail (0x5F8D40) to 0x5FADEF, before
 * seal_aifollow.cpp's ~CAiSFollow. So far SetAiEvent (round 14: zseal.h includes ai.h for
 * CAiEvent::EVENT), CAiDodge::Destroy and EventTick (round 39); 0x5F95D0 (HandleEvent: EventTick's
 * two walks call it, as SOCOM 1's -- an earlier note had it EventTick by size) and 0x5F8D40 round 62;
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
    float scale = m_audiofactor.m_value;
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
static inline int Stale(float t, float limit)
{
    return t > limit;
}

/* whether the AI's target is lost to it: none, or none held and not seen for three seconds (inline: the reaction's
 * tests; the name ours) */
static inline unsigned char TargetGone(CSealCtrlAi *ai, float limit = 3.0f)
{
    unsigned char gone = true;

    if (func_0060A850(ai->m_67c)) {
        bool held = func_0060A850(ai->m_67c) != 0;

        held = held ? (ai->m_6a8 & 0x200) != 0 : held;
        if (!held) {
            gone = Stale(GetTimeSinceLastSighting__9CAiTargetCFv(ai->m_67c), limit);
        } else
            gone = false;
    }
    return gone;
}

/* HandleEvent's helpers (inline; the names ours): SOCOM 1's Rfloat (zkit.h's, a local copy as seal_unit.cpp's
 * UnitRfloat: a draw in [min, min + range) with its zero-range test), a random float between two through it (its min
 * materialised before the range -- the plain `lo + (hi - lo) * r` numbers the range's constant first), a chance taken
 * through it with the odds by value read through a `const float &` (round 74's tell: the target stores the chance before
 * rand() and reloads it from its slot), the event's bit for an entity set and cleared -- an entity's own event has none
 * --, the AI's own event if it has not run out, and the AI's own event made a copy of one (zseal.h's CAiEventFields) */
struct HeRfloat {
    operator float() const
    {
        float f;

        if (m_range == 0.0f)
            f = m_min;
        else {
            float r = 4.656613e-10f * rand();
            f = m_min + m_range * r;
        }
        return f;
    }

    float m_min;
    float m_range;
};

static inline float RandBetween(float lo, float hi)
{
    HeRfloat r;

    r.m_min = lo;
    r.m_range = hi - lo;
    return r;
}

static inline bool Chance(float chance)
{
    const float &c = chance;
    HeRfloat r;

    r.m_min = 0.0f;
    r.m_range = 1.0f;
    return r <= c;
}

static inline int EventSeen(const CAiEvent *ev, unsigned int bit)
{
    return ev->m_type == CAiEvent::AIE_ENTITY ? 0 : ((1 << (bit & 31)) & ev->m_bits.m_vec[bit >> 5]) != 0;
}

static inline int EventOrder(const CAiEvent *ev, const CAiEvent *own)
{
    return own == 0 ? 1 : (signed char)ev->m_type - (signed char)own->m_type;
}

static inline void MarkEvent(const CAiEvent *ev, unsigned int bit)
{
    if (ev->m_type != CAiEvent::AIE_ENTITY)
        ev->m_bits.m_vec[bit >> 5] |= 1 << (bit & 31);
}

static inline void UnmarkEvent(const CAiEvent *ev, unsigned int bit)
{
    if (ev->m_type != CAiEvent::AIE_ENTITY)
        ev->m_bits.m_vec[bit >> 5] &= ~(1 << (bit & 31));
}

static inline const CAiEvent *LiveEvent(CSealCtrlAi *ai)
{
    const CAiEvent *own = 0;
    bool expired = false;

    if (ai->m_ev.m_duration != CAiEvent::NoExpire && ai->m_ev.m_duration <= 0.0f)
        expired = true;
    if (!expired)
        own = (const CAiEvent *)&ai->m_ev;
    return own;
}

static inline void SetOwnEvent(CSealCtrlAi *ai, const CAiEvent *ev, float duration)
{
    duration = duration;
    ai->m_ev = *(const CAiEventFields *)ev;
    ai->m_ev.m_duration = duration;
}

/* the event within the AI's view cone (CSensorCone::Contains on the controller's own fields; seal_sensors.cpp's) */
static inline bool EventInCone(CSealCtrlAi *c, const CAiEvent *ev)
{
    ev->m_pos.Sub((const CPnt3D *)c->m_cone_pos, (CPnt3D *)c->m_cone_vec);
    float dot = c->m_cone_dir[0] * c->m_cone_vec[0] + c->m_cone_dir[1] * c->m_cone_vec[1] + c->m_cone_dir[2] * c->m_cone_vec[2];

    c->m_cone_dot = dot;
    if (dot <= 0.0f || dot > c->m_cone_height)
        return false;
    float r = c->m_cone_point_radius + dot * c->m_cone_inv_height * c->m_cone_radius;

    if (c->m_cone_vec[2] * c->m_cone_vec[2] + (c->m_cone_vec[0] * c->m_cone_vec[0] + c->m_cone_vec[1] * c->m_cone_vec[1]) - dot * dot <= r * r)
        return true;
    return false;
}

/* (the pursuit's range; the state's class is seal_aistate.cpp's) */
class CAiSPursueRange : public CAiState {
public:
    float m_58;   // 0x58
};

class CMission;
extern CMission theMission;
#define data_00436654 (*(float *)((char *)&::theMission + 0x174))   /* theMission+0x174 */
extern float data_00650BF8;
extern float data_00650C00;
extern "C" bool func_00572D30(CSealCtrlAi *ai, CEntity *e);
extern "C" void func_00558AE0(CSealCtrlAi *ai, CZSealBody *seal);
extern "C" bool func_005F8D40(CSealCtrlAi *ai, CSoundEventView *ev, CZSealBody *seal, bool seen, float dist);

/* an event the AI may notice: not its own, not seen by it already, no less than the one it holds; marked as seen unless
 * it lies off the maps; a visible one only through the view cone and a chance by the line to it; the state asked; then
 * by its kind (OVL2 0x5F95D0; SOCOM 1's HandleEvent, 0x1D28 bytes there, the distance new).
 * OPEN, the target's size since round 75 (0x14F8 and 67 structural before; 17 now, 87 register-only). Right: the switch's
 * layout and every arm, the event's copy (an implicit assignment going member by member, through zseal.h's CAiEventFields)
 * and its addresses through m_ev, the union member: through a cast of m_event each address was a common subexpression, s0
 * and s1 held them, `known` was spilled and four more pads came. Round 75: the chance kept in memory and the draws'
 * constant order are SOCOM 1's Rfloat (the helpers' comment -- three words), the event's cell declared before the SEAL's
 * (their frame slots). What differs: bit in a0 for the target's a3 and @type in a3 for its a2 in EventSeen (the shift must colour
 * before the index: a named local, an early return and a mask local do not move it); UnmarkEvent's index in v1 for a0
 * (`mask`, a word pointer and an unsigned local tried); 0.3's `mtc1 v1,$f2` two words early, and the block optimization
 * form at AIE_HELPME's busy test (an `andi` mine keeps after the xori) */
bool CSealCtrlAi::HandleEvent(const CAiEvent *ev, float dist)
{
    CEntity *self = m_entity;

    if (ev->m_owner == self->m_id)
        return false;
    int seen = EventSeen(ev, self->m_index);
    bool vis = ev->m_visible != 0;

    if (seen)
        return false;
    CAiEvent::EVENT type = (CAiEvent::EVENT)ev->m_type;
    const CAiEvent *own = LiveEvent(this);
    int order = EventOrder(ev, own);

    if (order < 0)
        return false;
    CEntity *e = CEntity::m_list.GetEntityById(ev->m_owner);
    CZSealBody *seal = e && e->m_type == (CEntity::TYPE)2 ? (CZSealBody *)e : 0;
    bool comrade = false;

    if (e)
        comrade = !func_00572D30(this, e);
    CAiMapLoc evLoc;
    CAiMapLoc myLoc = self->m_mapLocValid;
    bool known = AiMaps.GetLocation(&ev->m_pos, evLoc);

    if (!known) {
        if (type != CAiEvent::AIE_ENTITY)
            MarkEvent(ev, self->m_index);
        return false;
    }
    if (vis && known) {
        ((CAiEventQ *)m_eventQueue)->Create(ev->m_id);
        bool in = false;

        if (EventInCone(this, ev)) {
            if (Chance(AiMaps.ComputeLineVisibility(myLoc, &ev->m_pos, 0, 15.0f) * data_00650BF8))
                in = true;
        }
        if (!in)
            return false;
    }
    MarkEvent(ev, self->m_index);
    if (!GetAiState()->Virtual34(this, (CAiEvent *)ev, seal, comrade))
        return false;
    bool taken = false;

    switch (type) {
    case CAiEvent::AIE_ENTITY:
        if (!comrade) {
            if (fabsf(self->m_posW.y - ev->m_pos.y) < 15.0f && TargetGone(this, 1.5f) && seal) {
                if (IsNavySeal()) {
                    SetOwnEvent(this, ev, 2.0f);
                    UnmarkEvent(ev, self->m_index);
                } else if (AiMaps.CanSeePoint(&self->m_posW, &ev->m_pos, ev->m_rangeSq * data_00436654)) {
                    if (PushAiState(AI_STATE_EVENT, 0)) {
                        CAiState *s = GetAiState();
                        CAiSEvent *st = s && s->m_state == AI_STATE_EVENT ? (CAiSEvent *)s : 0;

                        st->SetEvent(type, &ev->m_pos, 0);
                        CTarget *t = m_entity->m_targets.FindTarget(seal);

                        if (!t || !(t->m_flags & 0x100))
                            st->SetFlags(0xa, 0);
                        st->SetFlags(0x10, 0);
                        st->m_rnd_min = 1.0f;
                        st->m_rnd_range = 2.0f;
                    } else {
                        SetOwnEvent(this, ev, 2.0f);
                    }
                    if (seal->IsPlayer())
                        UnmarkEvent(ev, self->m_index);
                }
            }
            taken = true;
        }
        break;
    case CAiEvent::AIE_VOICE:
        taken = func_005F8D40(this, (CSoundEventView *)ev, seal, comrade, dist);
        break;
    case CAiEvent::AIE_EXPLOSION:
        if (known && !comrade) {
            AiMaps.GetCell(evLoc);
            if (AiMaps.GetCell(myLoc) && AiMaps.CanSeePoint(&self->m_posW, &ev->m_pos, ev->m_rangeSq * data_00436654)) {
                taken = true;
                SetAiAware((AI_AWARE)3, &ev->m_pos, -1.0f);
                if (TargetGone(this, 5.0f) && PushAiState(AI_STATE_EVENT, 0)) {
                    CAiState *s = GetAiState();
                    CAiSEvent *st = s && s->m_state == AI_STATE_EVENT ? (CAiSEvent *)s : 0;

                    st->SetEvent(type, &ev->m_pos, 0);
                    *(float *)&st->m_88 = 3.0f;
                    st->SetFlags(6, 0);
                    st->m_rnd_min = 3.0f;
                    st->m_rnd_range = 7.0f;
                    st->SetActive(true);
                }
            }
        }
        break;
    case CAiEvent::AIE_ALARM:
        SetAiAware((AI_AWARE)2, 0, -1.0f);
        break;
    case CAiEvent::AIE_GENERIC:
    case CAiEvent::AIE_LIGHTSOFF:
    case CAiEvent::AIE_DOOR:
        if (!IsNavySeal()) {
            if (TargetGone(this, 3.0f)) {
                if (PushAiState(AI_STATE_EVENT, 0)) {
                    CAiState *s = GetAiState();
                    CAiSEvent *st = s && s->m_state == AI_STATE_EVENT ? (CAiSEvent *)s : 0;

                    st->SetEvent(type, &ev->m_pos, 0);
                    st->SetFlags(0xa, 0);
                    st->m_rnd_min = 0.25f;
                    st->m_rnd_range = 0.75f;
                    st->SetActive(true);
                }
                SetAiAware((AI_AWARE)1, 0, -1.0f);
            }
        }
        break;
    case CAiEvent::AIE_WEPHIT:
        if (!comrade && AiMaps.CanSeePoint(&self->m_posW, &ev->m_pos, ev->m_rangeSq * data_00436654)) {
            func_00572240(this, e, 0.0f);
            taken = true;
        }
        break;
    case CAiEvent::AIE_WEPFIRE:
        if (!comrade) {
            unsigned char gone = TargetGone(this, 1.0f);

            if (gone || (unsigned char)GetAiAware() < 3) {
                SetAiAware((AI_AWARE)3, 0, -1.0f);
                SetOwnEvent(this, ev, RandBetween(1.0f, 4.0f));
            }
            CTarget *t = self->m_targets.FindTarget(e);

            if (t)
                t->m_flags |= 0x400;
            taken = true;
        } else {
            if (TargetGone(this, 3.0f)) {
                CZSealBody *c = e && e->m_type == (CEntity::TYPE)2 ? (CZSealBody *)e : 0;
                CSealCtrlAi *cai = c ? c->GetSealCtrlAi() : 0;

                if (cai && func_0060A850(cai->m_67c)) {
                    SetAiAware((AI_AWARE)3, 0, -1.0f);
                    const CPnt3D *p = &(*(CEntity **)cai->m_67c)->m_posW;

                    SetOwnEvent(this, ev, RandBetween(0.3f, 0.5f));
                    if (p) {
                        m_ev.m_pos.x = p->x;
                        m_ev.m_pos.y = p->y;
                        m_ev.m_pos.z = p->z;
                    }
                } else {
                    SetAiAware((AI_AWARE)3, 0, -1.0f);
                    if (c) {
                        const CPnt3D *p = &c->m_1458;

                        SetOwnEvent(this, ev, RandBetween(0.3f, 0.5f));
                        if (p) {
                            m_ev.m_pos.x = p->x;
                            m_ev.m_pos.y = p->y;
                            m_ev.m_pos.z = p->z;
                        }
                    }
                }
            }
            taken = true;
        }
        break;
    case CAiEvent::AIE_CORPSE:
        func_00558AE0(this, seal);
        SetAiAware((AI_AWARE)2, 0, -1.0f);
        SetOwnEvent(this, ev, RandBetween(1.0f, 2.0f));
        taken = true;
        break;
    case CAiEvent::AIE_HELPME:
        if (comrade) {
            int busy = m_aibit16 != 0;
            unsigned int id = ev->m_userdata;

            if (busy)
                busy = !(m_aibit17 != 0);
            if (!busy && TargetGone(this, 3.0f)) {
                GetTimeSinceLastSighting__9CAiTargetCFv(m_67c);
                CEntity *te = CEntity::m_list.GetEntityById(id);
                CTarget *t = self->m_targets.FindTarget(te);

                if (t && self->IsEnemy(te) && t->m_distSq < data_00650C00) {
                    SetTarget(t);
                    if (PushAiState(AI_STATE_PURSUE, 0)) {
                        CAiState *s = GetAiState();
                        CAiSPursueRange *st = s && s->m_state == AI_STATE_PURSUE ? (CAiSPursueRange *)s : 0;

                        st->m_58 = 15.0f;
                        st->SetActive(true);
                    }
                }
            }
            taken = true;
        }
        break;
    default:
        if (!comrade) {
            if (TargetGone(this, 5.0f)) {
                if (PushAiState(AI_STATE_EVENT, 0)) {
                    CAiState *s = GetAiState();
                    CAiSEvent *st = s && s->m_state == AI_STATE_EVENT ? (CAiSEvent *)s : 0;

                    st->SetEvent(type, &ev->m_pos, 0);
                    st->SetActive(true);
                }
                taken = true;
            }
        }
        break;
    }
    return taken;
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
                        ai->AcquireAiAware(other, true);
                        func_00572240(ai, CEntity::m_list.GetEntityById(other->m_730), 0.0f);
                    } else {
                        ai->AcquireAiAware(other, true);
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

