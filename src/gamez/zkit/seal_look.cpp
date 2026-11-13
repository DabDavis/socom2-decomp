/* seal_look.cpp -- where an AI SEAL looks: CSealCtrl's look and scan state (SOCOM 1 keeps these
 * functions -- ScanTick, ClearLook, SetLookDirR/W, SetLookPosW, SetDefaultLookParams and the scan
 * setters -- right after the hand signals, before seal_airesponse.cpp's states: its unit
 * seal_aiscan.cpp; this unit's name is ours). OVL2 0x600550-0x60187F: ScanTick (0x600550,
 * through a view local: there the parameter's register is the view's), a new look-direction helper (0x600CF0, a draft), ClearLook ... SetLookRate. zseal.h's
 * CSealCtrl (the SEAL lane's) declares the functions but names only m_look_mode, so the members are
 * read through a view of the controller, cast through an inline (a cast local would be a variable
 * of its own, numbered after the parameters: the target's registers are the parameters'). Defined
 * in reverse of their image order (-inline deferred). */

#include "zkit.h"

/* what the look's point is (SOCOM 1's DWARF, a byte) */
enum LOOK_DATA_TYPE { LOOK_GARBAGE = 0, LOOK_POSW = 1, LOOK_DIRW = 2, LOOK_DIRR = 3 };

/* the controller's look and scan members (SOCOM 1's DWARF: CSealCtrl's m_look_timer at 0x14 on;
 * SOCOM II's look members sit 4 further on, its scan timer where SOCOM 1's was -- the scan angle,
 * SOCOM 1's Rfloat, is one float now) */
struct CSealLook {
    unsigned char _0[0x18];
    float m_look_timer;   // 0x18
    Rfloat m_look_rate;   // 0x1c
    AI_LOOK m_look_mode;   // 0x24
    LOOK_DATA_TYPE m_look_type;   // 0x25
    unsigned char _26[0x28 - 0x26];
    CPnt3D m_look_data;   // 0x28
    CPnt3D m_look_data_mdir;   // 0x34
    CPnt3D m_look_bias;   // 0x40
    unsigned int m_look_flags;   // 0x4c
    float m_scan_angle;   // 0x50
    CAiTimer m_scan_timer;   // 0x54
    float m_scan_limits;   // 0x60 (SOCOM 1's, the same place)
};

static inline CSealLook *Look(CSealCtrl *c)
{
    return (CSealLook *)c;
}

/* the look's turn rate, fixed (OVL2 0x601870; SOCOM 1's, the members 4 bytes on) */
void CSealCtrl::SetLookRate(float rate)
{
    Look(this)->m_look_rate = rate;
}

/* (OVL2 0x601860; SOCOM 1's) */
void CSealCtrl::SetLookTimer(float t)
{
    Look(this)->m_look_timer = t;
}

/* (OVL2 0x601850; SOCOM 1's) */
void CSealCtrl::SetLookFlags(unsigned int flags)
{
    Look(this)->m_look_flags = flags;
}

/* `clear` cleared, then `set` set (OVL2 0x601830; SOCOM 1's) */
void CSealCtrl::ModifyLookFlags(unsigned int set, unsigned int clear)
{
    Look(this)->m_look_flags &= ~clear;
    Look(this)->m_look_flags |= set;
}

/* (OVL2 0x601820: SOCOM 1's SetScanAngle by its place and its callers -- CAiSPursue::Tick calls
 * SetLookDirR, it, ModifyLookFlags and SetLookTimer in both builds -- where SOCOM 1's set an Rfloat
 * from -angle over twice the angle, SOCOM II's keeps the angle alone) */
void CSealCtrl::SetScanAngle(float angle)
{
    Look(this)->m_scan_angle = angle;
}

/* the scan's period, its countdown started at it (OVL2 0x601810; SOCOM 1's) */
void CSealCtrl::SetScanRate(float rate)
{
    CAiTimer *t = &Look(this)->m_scan_timer;

    t->m_timer = rate;
    t->m_min = rate;
    t->m_range = 0.0f;
}

/* the scan's timer given a random period, its countdown drawn from it (inline; the name ours: the
 * range compared and the minimum read back as CAiTimer's Period does) */
static inline void SetScanTimer(CAiTimer *t, const Rfloat &r)
{
    t->m_min = r.m_min;
    t->m_range = r.m_range;
    t->m_timer = t->Period();
}

/* the look's defaults for its kind: a first wait of one to three seconds, the flags 0x80 and no bias,
 * then by kind the turn rate, the wait and the flags -- and, when the point's kind changed, the scan
 * reset to the controller's limits (OVL2 0x601400: SOCOM 1's SetDefaultLookParams(AI_LOOK), its rate
 * a function-local static there, with SOCOM II's bool and kinds 9-11). The kind kept unless it is the
 * same and nothing changed */
void CSealCtrl::SetDefaultLookParams(AI_LOOK look, bool changed)
{
    CSealLook *l = Look(this);

    l->m_look_timer = Rfloat(1.0f, 3.0f);
    l->m_look_flags = 0x80;
    l->m_look_bias = CPnt3D::zero;
    switch (look) {
    case AIL_NORMAL:
        if (changed) {
            l->m_scan_angle = l->m_scan_limits;
            SetScanTimer(&l->m_scan_timer, Rfloat(4.0f, 7.0f));
        }
        l->m_look_rate = Rfloat(0.1f, 0.3f);
        l->m_look_timer = CAiTimer::infinite;
        l->m_look_flags |= 6;
        break;
    case AIL_EVENT:
        l->m_look_rate = Rfloat(0.5f, 0.8f);
        l->m_look_timer = CAiTimer::infinite;
        break;
    case AIL_FRANTIC:
        if (changed) {
            l->m_scan_angle = l->m_scan_limits;
            SetScanTimer(&l->m_scan_timer, Rfloat(0.5f, 1.2f));
        }
        l->m_look_rate = 0.8f;
        l->m_look_flags |= 0xa;
        break;
    case AIL_BUDDY:
        l->m_look_timer = Rfloat(1.0f, 2.5f);
        l->m_look_rate = 1.0f;
        break;
    case AIL_CORPSE:
    case AIL_TASK:
        if (changed) {
            l->m_scan_angle = l->m_scan_limits;
            SetScanTimer(&l->m_scan_timer, Rfloat(0.5f, 2.5f));
        }
        l->m_look_flags |= 5;
        l->m_look_rate = 0.8f;
        break;
    case AIL_TURNING:
        l->m_look_rate = 0.8f;
        l->m_look_timer = 0.25f;
        break;
    case AIL_TARGET:
        l->m_look_flags |= 1;
        l->m_look_rate = 1.0f;
        l->m_look_timer = CAiTimer::infinite;
        break;
    case AIL_DEPLOY:
        l->m_look_flags |= 5;
        l->m_look_rate = 1.0f;
        l->m_look_timer = 3.0f;
        if (changed) {
            SetScanRate(0.25f);
            SetScanAngle(0.2617994f);
        }
        break;
    case AIL_10:
        l->m_look_rate = 1.0f;
        l->m_look_timer = CAiTimer::infinite;
        break;
    case AIL_11:
        l->m_look_flags |= 1;
        l->m_look_rate = 1.0f;
        l->m_look_timer = CAiTimer::infinite;
        break;
    }
    if (l->m_look_mode != look || changed)
        l->m_look_mode = look;
}

/* a difference in the ground plane, by value, and a squared length taken through a reference: the
 * temporary bound to the reference stays in the frame, its y stored as a word zero and y and z read
 * back off the stack, as the target has it (a named point, a pointer parameter or the sum written
 * out scalarise it; the names ours) */
static inline CPnt3D SubXZ(const CPnt3D &a, const CPnt3D &b)
{
    return CPnt3D(a.x - b.x, 0.0f, a.z - b.z);
}

static inline float LenSq(const CPnt3D &v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

/* the closest a point looked at may be, squared, in the ground plane (OVL2 data: a variable) */
extern float data_00650DA0;

/* a point looked at, unless it is the SEAL's own spot (SOCOM 1's; OVL2 0x601330, inlined into the two
 * overloads before it: SOCOM II's refuses a point closer than the limit in the ground plane) */
bool CSealCtrl::SetLookPosW(AI_LOOK look, const CPnt3D *pos)
{
    bool ok = look >= Look(this)->m_look_mode;

    if (ok) {
        if (LenSq(SubXZ(m_entity->m_posW, *pos)) < data_00650DA0)
            return false;
        Look(this)->m_look_data = *pos;
        bool changed = Look(this)->m_look_type != LOOK_POSW;

        Look(this)->m_look_type = LOOK_POSW;
        SetDefaultLookParams(look, changed);
    }
    return ok;
}

/* an entity looked at, by its target point (OVL2 0x601200, new in SOCOM II: SOCOM 1's CTLookAt::Tick
 * asked the entity itself and called SetLookPosW; the overload ours). The kind is tested through
 * zseal.h's member here, the view's in the inlined overload: one spelling for both and MW keeps the
 * member's address in a saved register across the entity's call */
bool CSealCtrl::SetLookPosW(AI_LOOK look, CEntity *e)
{
    bool ok = look >= m_look_mode;

    if (ok && e) {
        CPnt3D p;

        ok = SetLookPosW(look, e->GetTargetPosW(&p));
    }
    return ok;
}

/* a cell looked at, by its centre -- nothing done for the SEAL's own cell (ai.h's materialised
 * operator!=) (OVL2 0x601070, new in SOCOM II; the overload ours), the kind tested as in the entity's
 * overload */
bool CSealCtrl::SetLookPosW(AI_LOOK look, const CAiMapLoc &loc)
{
    bool ok = look >= m_look_mode;

    if (ok) {
        if (loc != m_entity->m_mapLoc) {
            CPnt3D c;

            if (AiMaps.GetCellCenter(loc, &c))
                ok = SetLookPosW(look, &c);
        }
    }
    return ok;
}

/* a world direction looked along (SOCOM 1's; OVL2 0x600F70): taken as it is, or normalised -- a
 * direction too short to normalise refused, the old one kept (put back through an offset of its
 * own: spelled as the member, MW keeps the member's address in a saved register from the call's
 * argument to these stores, where the target adds the offset to `this` at each) */
bool CSealCtrl::SetLookDirW(AI_LOOK look, const CPnt3D *dir, bool exact)
{
    bool ok = look >= Look(this)->m_look_mode;

    if (ok) {
        if (exact)
            Look(this)->m_look_data = *dir;
        else {
            CPnt3D old = Look(this)->m_look_data;

            if (dir->Normalize(&Look(this)->m_look_data) <= 0.1f) {
                *(CPnt3D *)((char *)this + 0x28) = old;
                return false;
            }
        }
        bool changed = Look(this)->m_look_type != LOOK_DIRW;

        Look(this)->m_look_type = LOOK_DIRW;
        SetDefaultLookParams(look, changed);
    }
    return ok;
}

/* a map direction looked along (SOCOM 1's; OVL2 0x600ED0) */
bool CSealCtrl::SetLookDirW(AI_LOOK look, CAiMap::DIR dir)
{
    CPnt3D v;

    CAiMap::GetDirVec(dir, v);
    bool ok = look >= Look(this)->m_look_mode;

    if (ok) {
        Look(this)->m_look_data = v;
        bool changed = Look(this)->m_look_type != LOOK_DIRW;

        Look(this)->m_look_type = LOOK_DIRW;
        SetDefaultLookParams(look, changed);
    }
    return ok;
}

/* a direction relative to the SEAL looked along (SOCOM 1's; OVL2 0x600DD0), as SetLookDirW */
bool CSealCtrl::SetLookDirR(AI_LOOK look, const CPnt3D *dir, bool exact)
{
    bool ok = look >= Look(this)->m_look_mode;

    if (ok) {
        if (exact)
            Look(this)->m_look_data = *dir;
        else {
            CPnt3D old = Look(this)->m_look_data;

            if (dir->Normalize(&Look(this)->m_look_data) <= 0.1f) {
                *(CPnt3D *)((char *)this + 0x28) = old;
                return false;
            }
        }
        bool changed = Look(this)->m_look_type != LOOK_DIRR;

        Look(this)->m_look_type = LOOK_DIRR;
        SetDefaultLookParams(look, changed);
    }
    return ok;
}

/* a look dropped -- this kind's, or any (AIL_ALL): straight ahead, no timer, no flags, the
 * defaults back (OVL2 0x600D40; SOCOM 1's CSealCtrl::ClearLook(AI_LOOK), the same stores 4
 * bytes on) */
void CSealCtrl::ClearLook(AI_LOOK look)
{
    CSealLook *l = (CSealLook *)this;

    if (look == AIL_ALL || look == l->m_look_mode) {
        bool changed = l->m_look_type != LOOK_DIRR;

        l->m_look_data = CPnt3D::unit_nz;
        l->m_look_type = LOOK_DIRR;
        l->m_look_mode = AIL_NORMAL;
        l->m_look_timer = CAiTimer::infinite;
        l->m_look_flags = 0;
        SetDefaultLookParams(AIL_NORMAL, changed);
    }
}

/* the scan's constants (OVL2 data, read as variables): the closest a head-relative point may be, the
 * cosine under which the head turns, the turn rate's ends, the bias's sign and the sweep's side that
 * flips each scan */
extern float data_00650DA8;
extern float data_00650DB0;
extern float data_00650DB8;
extern float data_00650DC0;
extern float data_00650DC8;
extern float data_00650DD0;
/* the scan's vertical reach (OVL2 data) */
extern float data_0066BB48;

/* the scan's countdown run down by dT, drawn again once it runs out -- an infinite one never runs --
 * and whether it did (the name ours: in place, a reference or the member, MW keeps the timer's address
 * in a saved register; through an inline it folds to offsets from `this` as the target's) */
static inline bool ScanTimerTick(CAiTimer &t, float dT)
{
    bool r = false;

    if (t.m_timer != CAiTimer::infinite) {
        t.m_timer -= dT;
        r = t.m_timer != CAiTimer::infinite && t.m_timer <= 0.0f;
        if (r)
            t.m_timer = t.Period();
    }
    return r;
}

/* the look turned to its point and the scan swept (OVL2 0x600550; SOCOM 1's ScanTick -- SOCOM II's runs
 * only for a SEAL whose frame slice it is, or with look flags set): the point as a direction in the
 * SEAL's frame -- a world point from the head, a world direction, or a relative one as it is -- the bias
 * added once each time it changes (bit 7), the head turned there when it is off by more than the
 * cosine, and the body told to face it while the head is still turning; the look's timer run down to a
 * plain look; the scan's timer drawing a new bias each period (bits 2 and 3: across the scan angle, or
 * to one side and the other). Through a view local (`l`): Look(this) at each use keeps every member's
 * address in a saved register. The dot product through Dot keeps &out.y/&out.z in s3/s1 across the
 * head's call, as the target (written out, they fold to sp offsets: a 0x14-byte shorter body). The
 * locals declared have, seal, view (s1, s0, this's s2) */
void CSealCtrl::ScanTick(float dT)
{
    CPnt3D out;
    CPnt3D dir;
    CPnt3D head;
    CPnt3D hdir;
    bool have;
    CZSealBody *seal = (CZSealBody *)m_entity;
    CSealLook *l = Look(this);

    if (!ai_slice0.Has(seal->m_index) && !l->m_look_flags)
        return;
    have = false;
    ai_slice0.m_count++;
    seal->GetLookDir();
    switch (l->m_look_type) {
    case LOOK_POSW:
        seal->GetHeadParams(&head, &hdir);
        l->m_look_data.Sub(&head, &dir);
        if (dir.Normalize() < data_00650DA8)
            break;
        {
            CQuat inv = seal->m_quat;

            inv.vec.x = -inv.vec.x;
            inv.vec.y = -inv.vec.y;
            inv.vec.z = -inv.vec.z;
            inv.Apply(&dir, &out, 1);
        }
        have = true;
        ModifyLookFlags(0x80, 0);
        break;
    case LOOK_DIRW:
        dir = l->m_look_data;
        {
            CQuat inv = seal->m_quat;

            inv.vec.x = -inv.vec.x;
            inv.vec.y = -inv.vec.y;
            inv.vec.z = -inv.vec.z;
            inv.Apply(&dir, &out, 1);
        }
        have = true;
        ModifyLookFlags(0x80, 0);
        break;
    case LOOK_DIRR:
        have = true;
        out = l->m_look_data;
        break;
    }
    if (have && (l->m_look_flags & 0x80)) {
        bool settled;

        l->m_look_flags &= ~0x80;
        out.Add(&l->m_look_bias, &out);
        out.Normalize();
        settled = true;
        if (Dot(out, l->m_look_data_mdir) < data_00650DB0) {
            float r = l->m_look_rate;

            settled = seal->SetLookDir(&out, data_00650DB8 * (1.0f - r) + data_00650DC0 * r);
            l->m_look_data_mdir = out;
        }
        if (l->m_look_type != LOOK_DIRR
            && ((!settled && !(l->m_look_flags & 2)) || (l->m_look_flags & 1)))
            SetGoalDir((GOAL_TYPE)1, &dir, false);
    }
    if (l->m_look_timer != CAiTimer::infinite) {
        l->m_look_timer -= dT;
        if (l->m_look_timer <= 0.0f)
            ClearLook(AIL_ALL);
    }
    if (l->m_look_flags & 0xc) {
        bool fired = ScanTimerTick(l->m_scan_timer, dT);
        if (fired) {
            float angle;
            CPnt3D d = CPnt3D::zero;

            if (data_0066BB48 != 0.0f)
                d.y = Rfloat(-data_0066BB48, data_0066BB48);
            if (l->m_look_flags & 8)
                angle = Rfloat(-l->m_scan_angle, l->m_scan_angle);
            else {
                float k = Rfloat(0.8f, 1.1f);

                if (l->m_look_bias.x == 0.0f) {
                    angle = data_00650DD0 * (l->m_scan_angle * k);
                    data_00650DD0 = -data_00650DD0;
                } else if (l->m_look_bias.x >= 0.0f)
                    angle = data_00650DC8 * (-l->m_scan_angle * k);
                else
                    angle = data_00650DC8 * (l->m_scan_angle * k);
            }
            d.x = sinf(angle);
            d.z = cosf(angle);
            d.z = -d.z;
            if (d.y != 0.0f)
                d.Normalize();
            l->m_look_bias = d - CPnt3D::unit_nz;
            ModifyLookFlags(0x80, 0);
        }
    }
}
