/* veh_turret.cpp -- CTurret, a mounted gun (SOCOM 1's veh_turret.cpp, by its static initialiser:
 * SOCOM II's 0x404240 builds the definitions (0x4A4038) and the turret list (0x4A4058) and sets
 * turret_debug_mask's bits, as SOCOM 1's __sinit_veh_turret.cpp does). OVL1 0x3A71A0-0x3ACA5F,
 * opening with the copies of its inline virtual overrides and closing with the two vectors'
 * destructors; CVehicle's unit follows. Defined in reverse of their image order (-inline
 * deferred). */

#include <algorithm>
#include <string.h>

#include "zveh.h"

using namespace zdb;

/* the network platform's state (fts_net_platform.cpp's class): only the send a gunner's turret
 * makes */
class CZPlatformState {
public:
    static void UpdateLocalPlatform(CTurret *turret, bool now);
};

/* not yet written here (placeholders): the turret list's remove and the turret's halves of the
 * tick */
extern "C" {
/* the yaw joint and the pitch joint turned by a rate over a step into a matrix, the angle reached
 * written out; whether the joint moved (not written: 0x3A76D0, 0x3A7490 -- the pitch's limited,
 * its third float 0x3C0). The floats come first: Rotate's set-up loads the rate ahead of the node
 * and moves the step to $f12 before the call (the pointers first, the floats last: 2 off) */
bool func_003A76D0(CTurret *t, float dT, float rate, CNode *node, CMatrix *m, float *angle);
bool func_003A7490(CTurret *t, float dT, float rate, float extra, CNode *node, CMatrix *m,
                   float *angle);
void func_003A78E0(CTurret *turret, float dT);
void func_003A82C0(CTurret *turret);
void func_003A84E0(CTurret *turret);
void func_003A88C0(CTurret *turret, float dT);
/* the turrets made, and the most there may be (-1: no limit) -- the unit's statics */
extern int data_004A4020;
extern int data_003E0800;
}

CZSealBody *ftsGetPlayer();

/* SOCOM 1's file-static turret_debug_mask (0x4A4068: seven 1-bit flags, SOCOM II's 1, 0, 1, 1, 0,
 * 0, 0 where SOCOM 1's were 0, 0, 1, 1, 0, 0, 0; the class and the bits' names ours). Set in the
 * mem-initializer list, each is a word read-mask-write, `true` folded and `false` left `(0 & 1) << n`,
 * as both builds' initialisers have it */
class CTurretDebugMask {
public:
    CTurretDebugMask() : m_0(true), m_1(false), m_2(true), m_3(true), m_4(false), m_5(false), m_6(false) {}

    unsigned int m_0 : 1;
    unsigned int m_1 : 1;
    unsigned int m_2 : 1;
    unsigned int m_3 : 1;
    unsigned int m_4 : 1;
    unsigned int m_5 : 1;
    unsigned int m_6 : 1;
};

/* the initialiser (__sinit_veh_turret.cpp, 0x404240) builds these in order: the definitions and the
 * turret list (each vector's words zeroed in place, its destructor registered), then the switches */
CTurretDefVector CTurret::m_turret_def;
CTurretList CTurret::m_turrets;
static CTurretDebugMask turret_debug_mask;

/* the limits' defaults, in degrees (the unit's statics, 60 and 30; not named) */
extern "C" float data_003E0808;
extern "C" float data_003E0810;

/* a definition's defaults: no weapon or ammo, the strings empty, the trigger 1-2 up and 2-4 down,
 * the AI looking down -z, yaw within 60 degrees and pitch within 30, the rates 1, flag 2 set and
 * the AI's accuracy, range and discipline the turret's (SOCOM 1's constructor, the members SOCOM
 * II's; out of line, as SOCOM 1's: right after Parse in the image, ahead of the template tail) */
CTurretDef::CTurretDef() : m_weapon(EQUIP_NONE), m_ammo(AMMO_NONE), m_flags(0)
{
    m_name[0] = 0;
    m_20[0] = 0;
    m_destroyAnim[0] = 0;
    m_effect[0] = 0;
    m_parts[0][0] = 0;
    m_parts[1][0] = 0;
    m_parts[2][0] = 0;
    m_health = 0.0f;
    m_effectValue = 0.0f;
    m_team = 0;
    m_trigger.SetTimes(Rfloat(2.0f, 4.0f), Rfloat(1.0f, 2.0f));
    m_lookDir = CPnt3D::unit_nz;
    m_yaw.m_min = 0.017453292f * -data_003E0808;
    m_yaw.m_max = 0.017453292f * data_003E0808;
    m_pitch.m_min = 0.017453292f * -data_003E0810;
    m_pitch.m_max = 0.017453292f * data_003E0810;
    m_yawRate = 1.0f;
    m_pitchRate = 1.0f;
    SetFlag(2, true);
    m_aiParams.accuracy = 0.8f;
    m_aiParams.visual_range = 1000.0f;
    m_aiParams.target_discipline = 0.996195f;
    m_thermal = false;
    m_compass = false;
}

/* the definition whose name starts the node's (SOCOM 1's; called, never inlined) */
#pragma auto_inline off
const CTurretDef *CTurretDefVector::PrefixMatch(const char *name)
{
    if (name) {
        unsigned int i;
        unsigned int n = size();

        if (strlen(name)) {
            for (i = 0; i < n; i++) {
                CTurretDef *def = &(*this)[i];
                unsigned int len = def ? strlen(def->m_name) : 0;

                if (def && len && !strncmp(name, def->m_name, len))
                    return def;
            }
        }
    }
    return 0;
}
#pragma auto_inline reset

/* the definitions read: the vector sized to the "TURRET" list's pairs, each parsed from its name
 * and value. The count goes into a local ahead of the counter's zero, which then fills the resize
 * call's delay slot as the target's does (computed in the call's argument, MW put the this-move
 * there and sank the zero to the loop's first test). The resize is include/vector's over
 * CTurretDef's constructor: the unit's template tail (0x3AB5F0-0x3AC6DF) */
int CTurretDefVector::Init(const char *name, const char *path)
{
    _zrdr *rdr = zrdr_read(name, path, 0);

    if (rdr) {
        _zrdr *list = zrdr_findtag(rdr, "TURRET");
        unsigned int i;

        unsigned int n = (list ? list->Count() : 0) / 2;

        i = 0;
        resize(n);
        for (iterator it = begin(); it != end(); ++it, i += 2)
            (*it).Parse(list->Item(i)->String(), list->Item(i + 1));
    }
    zrdr_free((CRdrFile *)rdr);
    return size();
}

int CTurret::Init(const char *name, const char *path)
{
    return m_turret_def.Init(name, path);
}

void CTurret::ResetAmmo()
{
    for (CTurretList::iterator it = m_turrets.begin(); it != m_turrets.end(); ++it) {
        CTurret *t = *it;
        CZWeapon *w = t->m_weapon;

        t->m_ammo = w ? w->GetAmmoCapacity() * w->GetNumMags() : 0;
        t->m_3c0 = 0;
    }
}

void CTurret::LoadTurrets()
{
    CWorld *world = CWorld::m_world;
    CNodeVector &landmarks = world->m_landmarks;

    for (CNodeVector::iterator it = landmarks.begin(); it != landmarks.end(); ++it)
        ScanForTurretDefinitionHookups(*it, true);
    ScanForTurretDefinitionHookups(world, true);
}

CTurret::~CTurret()
{
    m_turrets.Remove(this);
    if (m_turrets.size() == 0)
        FreeVector<std::vector<CTurret *> >(m_turrets);
}

/* a turret for the node, made and listed as an entity, unless the limit is reached */
static inline CTurret *NewTurret(CNode *node, const CTurretDef *def)
{
    CTurret *t = 0;

    if (!node)
        return 0;
    if (data_003E0800 > -1 && data_004A4020 >= data_003E0800)
        return 0;
    data_004A4020++;
    t = new CTurret(node, *def);
    CEntity::CreateEntity(t, true);
    t->UpdateAiMapLoc();
    return t;
}

CTurret *CTurret::Create(int index)
{
    CTurret *t = 0;
    CNode *node = NodeUniverse->GetElement(index);

    if (node) {
        const CTurretDef *def = m_turret_def.PrefixMatch(node->m_name);

        if (def)
            t = NewTurret(node, def);
    }
    return t;
}

void CTurret::SetThermal(CNode *node, bool thermal)
{
    CNode *scope = m_scope;
    CNode *thermalScope = m_thermalScope;

    if (node) {
        if (!scope) {
            scope = node->FindChild("scope", true);
            m_scope = scope;
        }
        if (!thermalScope) {
            thermalScope = node->FindChild("thermal_scope", true);
            m_thermalScope = thermalScope;
        }
    }
    thermal = thermalScope && m_thermal ? m_thermal : thermal;
    if (thermalScope)
        thermalScope->SetActive(thermal);
    if (scope)
        scope->SetActive(!thermal);
}

void CTurret::OnMissionStart()
{
    if (GetFlag(2)) {
        m_240 = 0;
        if (GetFlag(0x40))
            CreateCompassAnchor(11, "Turret");
    }
}

void CTurret::UpdateCompassAnchor()
{
    if (m_anchor && !m_anchor->m_locked) {
        m_anchor->m_posW = m_posW;
        m_anchor->SetEnabled(IsOperational());
    }
}

void CTurret::SetActive(bool on)
{
    SetFlag(0x20, on);
    m_isAlive = on;
}

int CTurret::ScanForTurretDefinitionHookups(CNode *node, bool recurse)
{
    int n = 0;

    if (!node || m_turret_def.size() == 0)
        return 0;
    if (node->GetType() == 10)
        return 0;
    if (!node->m_name)
        return 0;
    const CTurretDef *def = m_turret_def.PrefixMatch(node->m_name);

    if (def) {
        NewTurret(node, def);
        n++;
    } else if (recurse) {
        CNodeVector &children = node->GetChildren();

        for (CNodeVector::iterator it = children.begin(); it != children.end(); ++it)
            n += ScanForTurretDefinitionHookups(*it, recurse);
    }
    return n;
}

void CTurret::UpdateAiMapLoc()
{
    unsigned int loc;

    if (AiMaps.GetLocation(&m_posW, *(CAiMapLoc *)&loc)) {
        *(unsigned int *)&m_mapLoc = loc;
        *(unsigned int *)&m_mapLocValid = loc;
    }
}

void CTurret::PreTick(float dT)
{
    CNode *root = m_root;

    if (root)
        SetFlag(1, GetFlag(0x20) && root->m_active);
    else
        SetFlag(1, false);
    if (GetFlag(1) && !GetFlag(2) && IsControlled() == false)
        SetFlag(1, false);
    if (GetFlag(1)) {
        SampleDynamics(dT);
        float r = m_220;

        m_254 = r;
        m_260 = 1.0f / r;
        m_258 = r * m_def->m_tanHalfAngle;
        m_25c = 25.0f;
        m_264 = m_posW;
        m_270 = m_38c;
        if (GetFlag(2))
            func_003A88C0(this, dT);
    }
}

/* the countdown run down, true once it has (an infinite one never runs) */
static inline bool Expired(const CAiTimer &t)
{
    return t.m_timer != CAiTimer::infinite && t.m_timer <= 0.0f;
}

static inline bool TickTimer(CAiTimer &t, float dT)
{
    bool r = false;

    if (t.m_timer != CAiTimer::infinite) {
        t.m_timer -= dT;
        r = Expired(t);
    }
    return r;
}

/* the countdown started over from a random period: the random factor in a local of its own, as
 * aitimer.h's CAiTimer::Period (and SOCOM 1's Fire) -- the target computes the range's product
 * into $f0 and pads the store's join with two nops; written inline in the sum it does neither */
static inline float RandomPeriod(const CAiTimer &t)
{
    float f;

    if (t.m_range == 0.0f)
        f = t.m_min;
    else {
        float r = 4.656612873e-10f * (float)rand();

        f = t.m_min + t.m_range * r;
    }
    return f;
}

static inline void ResetTimer(CAiTimer &t)
{
    t.m_timer = RandomPeriod(t);
}

/* the network test is the player in a local, assigned in the test (`pl = ftsGetPlayer(), m_control
 * != pl->m_control`: the player's controller loaded into v0 after the call, this one's into v1,
 * `beq v1,v0`) -- PrePostTick's player local, the comma keeping the call ahead of this one's load
 * (`m_control != ftsGetPlayer()->m_control` hoists that load above the call into a saved register,
 * the call's side first swaps the two registers); the timer's reset is aitimer.h's Period shape */
void CTurret::Tick(float dT)
{
    CZSealBody *pl;

    if (!GetFlag(1))
        return;
    if (GetFlag(2))
        func_003A84E0(this);
    else if (theNetwork.m_9 && (pl = ftsGetPlayer(), m_control != pl->m_control))
        func_003A78E0(this, dT);
    else if (m_control) {
        CEntityCtrl *ctrl = m_control;

        m_3a8 = m_def->m_yawRate * ctrl->m_throttle[2];
        m_3ac = m_def->m_pitchRate * ctrl->m_throttle[3];
        m_velR.x = m_3ac;
        m_velR.y = m_3a8;
        m_velR.z = 0.0f;
    }
    Rotate(dT);
    if (GetFlag(4)) {
        if (TickTimer(m_fireTimer, dT))
            Fire();
    } else
        ResetTimer(m_fireTimer);
}

void CTurret::PrePostTick(float dT)
{
    if (!GetFlag(1)) {
        UpdateCompassAnchor();
        return;
    }
    if (GetFlag(2)) {
        func_003A82C0(this);
        UpdateCompassAnchor();
        return;
    }
    if (theNetwork.m_9) {
        CZSealBody *player = ftsGetPlayer();

        if (m_control == player->m_control)
            CZPlatformState::UpdateLocalPlatform(this, false);
    }
}

void CTurret::UpdatePosW()
{
    m_posW = *(CPnt3D *)m_300.m_matrix[3];
}

void CTurret::ComputeNextPosition(float dT)
{
}

/* the turret's frame sampled: its matrix from the yaw node's parent and the inverse, the fire
 * point's position (the origin without its node) and the velocity as the step over the frame
 * (SOCOM 1's) */
void CTurret::SampleDynamics(float dT)
{
    if (m_yawNode && m_288) {
        sceVu0UnitMatrix(m_300.m_matrix);
        m_yawNode->m_parent->BuildMTW(m_300);
        sceVu0UnitMatrix(m_340.m_matrix);
        Vu0MatrixInverseWithUniformScale(&m_340, &m_300);

        CMatrix m;

        sceVu0UnitMatrix(m.m_matrix);

        CPnt3D old = m_380;

        if (m_288) {
            m_288->BuildMTW(m);
            m_380 = *(CPnt3D *)m.m_matrix[3];
        } else {
            m_380 = CPnt3D::zero;
        }

        CPnt3D d = m_380 - old;
        CPnt3D v = d * (1.0f / dT);

        m_velW = v;
    }
}

/* the joints turned: the yaw's angle kept in 0x3B0, the pitch's in 0x3B8 (its extra cleared), then
 * the aim -- the pitch joint's world Z negated -- and bit 3 set, and while bit 1 is clear the scope's
 * frame from the pitch joint (SOCOM 1's Rotate, split; the name ours) */
void CTurret::Rotate(float dT)
{
    CMatrix m;
    float yaw;
    float pitch;

    if (func_003A76D0(this, dT, m_3a8, m_yawNode, &m, &yaw)) {
        m_yawNode->SetMatrix(&m);
        m_3b0 = yaw;
    }
    if (func_003A7490(this, dT, m_3ac, m_3c0, m_pitchNode, &m, &pitch)) {
        m_pitchNode->SetMatrix(&m);
        m_3b8 = pitch;
        m_3c0 = 0.0f;
    }
    sceVu0UnitMatrix(m.m_matrix);
    if (m_pitchNode->BuildMTW(m)) {
        CPnt3D *z = (CPnt3D *)m.m_matrix[2];

        m_38c.x = -z->x;
        m_38c.y = -z->y;
        m_38c.z = -z->z;
        m_flags |= 8;
    } else
        m_flags |= 8;
    if (!GetFlag(2)) {
        sceVu0UnitMatrix(m_2c0.m_matrix);
        m_pitchNode->BuildMTW(m_2c0);
    }
}

/* the AI's target (ai_target.cpp's class, OVL2): its sighting clock and the aim's spread */
class CAiTarget {
public:
    float GetTimeSinceLastSighting() const;
    void ModifyAimDir(CPnt3D *dir, float error, float limit);
};

/* an animation (zanim.h's CZAnim: only its activation with arguments) */
class CZAnim {
public:
    CZAnim *ActivateArg(int type, ...);
};

/* the entity a controller drives, none without one (inline; the name ours) */
static inline CEntity *ControlledEntity(CEntityCtrl *ctrl)
{
    return ctrl ? ctrl->m_entity : 0;
}

/* the SEAL's zoom (SOCOM 1's ZOOMSTATE m_zoomstate, a signed byte enum: 0x160 there, 0x200 here --
 * zseal.h's CZSealBody does not place it) */
static inline signed char ZoomState(CEntity *e)
{
    return *((signed char *)e + 0x200);
}

/* a round fired: a projectile numbered for the tracers, the weapon's fire animation (the zoomed one
 * while the gunner looks through the scope) started at the fire point with the turret's velocity,
 * the AI's aim spread, the weapon's virtual Fire from the fire point, the fire timer reset (SOCOM 1's
 * Fire; Tick's call when the fire timer runs out). The gunner's test is a controller local tested
 * twice (the target's `beqz v0; beqz v0` on one load: through IsControlled() it is reloaded, through
 * a `c ? c->m_entity : 0` inline the null is materialised) and the flag is set to an int 1 (`andi
 * a1,t0,0xff` off the 1 the tracer index stored; `true` is a daddiu of its own).
 * OPEN, 2 instructions: the weapon's call loads the shooter id (t3) ahead of the node (a1), here the
 * node first -- the same set-up otherwise. Tried: the id or the node in a local (before the call, at
 * the top, before the anim), the call through an inline wrapper in SOCOM 1's static parameter order
 * and in the member's, accessors for the id and the node, the weapon in a local, `false` for the last
 * word -- none moves the two loads */
void CTurret::Fire()
{
    if (!m_weapon || m_weapon->m_ID == EQUIP_NONE)
        return;

    CZAmmo *ammo = m_ammoType ? m_ammoType : m_weapon->GetDefaultAmmo();
    CZProjectile *p = CZWeapon::m_pProjectilelist->GetNextFreeProjectile();

    if (!p)
        return;
    p->m_flyout_idx = 1;
    p->m_tracer_count = ++m_shots;
    p->SetUseFlyout(false);
    p->SetIsPlayer(false);

    bool zoomed = false;
    CEntityCtrl *c = m_control;

    if (c && c && c->m_entity && ZoomState(c->m_entity) > 0)
        zoomed = 1;

    CZAnim *anim = m_weapon->GetFireAnim(zoomed);
    const CPnt3D *pos = &m_380;

    if (anim)
        anim->ActivateArg(3, 3, m_288, 5, &CPnt3D::zero, 6, &m_velW, 0);

    CPnt3D dir = m_38c;

    if (GetFlag(2))
        ((CAiTarget *)m_target)->ModifyAimDir(&dir, m_aimError, 0.267949f);
    m_weapon->Fire(m_node, ammo, pos, &dir, &CPnt3D::zero, p, m_id, 0);
    ResetTimer(m_fireTimer);
}

/* a rate moved toward a step, at most the limit either way: down only while the step is below
 * it, up only while above, zero for no step (inline; the name ours) */
static inline void SlewRate(float &rate, float step)
{
    float limit = 1.2f;

    if (step < 0.0f) {
        float s = step < -limit ? -limit : step;

        if (s < rate)
            rate = s;
    } else if (step > 0.0f) {
        if (step > limit)
            step = limit;
        if (step > rate)
            rate = step;
    } else {
        rate = 0.0f;
    }
}

/* the third and fourth throttles' rates slewed toward their steps, 1.2 at most (new in SOCOM II,
 * not named: Tick calls it, out of line -- pinned) */
#pragma auto_inline off
extern "C" void func_003A78E0(CTurret *t, float dT)
{
    SlewRate(t->m_3a8, t->m_3b4 - t->m_3b0);
    SlewRate(t->m_3ac, t->m_3bc - t->m_3b8);
}
#pragma auto_inline reset

/* the yaw and pitch toward a direction in the turret's frame (0x3A8910, not written) */
extern "C" void func_003A8910(CTurret *t, const CPnt3D *dir, float *yaw, float *pitch);

/* the yaw and pitch toward a point: the direction to it from the fire point, unit length, into the
 * turret's frame; straight ahead (-z) without one (new in SOCOM II, not named: the SEALs' vehicle
 * state asks it for the point it aims at) */
extern "C" void func_003A8BD0(CTurret *t, const CPnt3D *target, float *yaw, float *pitch)
{
    CPnt3D dir;

    if (target) {
        CPnt3D d = *target - t->m_380;

        t->m_398 = d;
        t->m_398.Normalize();
        TransformVectors(&t->m_340, &t->m_398, &dir, 1);
    } else {
        dir = CPnt3D::unit_nz;
    }
    func_003A8910(t, &dir, yaw, pitch);
}

/* (called, never inlined: ~CTurret calls it). Its remove_copy instance (0x3AB9A0) is byte-exact.
 * OPEN, 12 bytes short: the same calls and loop, but the target builds one end() temporary first
 * (sp+76) and serves both std::remove's last and erase's last from it, std::remove's result in a
 * slot of its own (sp+72) copied into erase's parameters; here end() is built twice (and with `e`
 * a named local the temporaries' copies fold away: 0x11C/0x100). Tried: the end in a local, the
 * result in a local, begin() in a local, this->end(), a copy of the pointer */
/* the AI's target's validity test (OVL2 0x60A850, the kit lane's) */
extern "C" bool func_0060A850(void *target);

/* the turret's target in sight: a valid target held (bit 9), or seen within the last three seconds
 * (new in SOCOM II, not named: CTTurret::Tick asks it; the SEALs' hostile anchor makes the same
 * test, the held arm last) */
extern "C" bool func_003A7FA0(CTurret *t)
{
    bool lost = true;

    if (func_0060A850(t->m_target)) {
        if (!(t->m_targetFlags & 0x200))
            lost = ((CAiTarget *)t->m_target)->GetTimeSinceLastSighting() > 3.0f;
        else
            lost = false;
    }
    return !lost;
}

#pragma auto_inline off
void CTurretList::Remove(CTurret *turret)
{
    iterator e = end();
    iterator i = std::remove(begin(), e, turret);

    erase(i, e);
}
#pragma auto_inline reset

/* STAND-IN for a dead-stripped original (no row in the map: it scores as no-target). The insert
 * (0x3ABD90) inlines its temporary's destruction -- the deleter's destructor, with clear inside --
 * which the template tail does only for members generated before it, i.e. requested by the unit's
 * own code; nothing left in the image clears or destroys a vector of definitions, which fits an own
 * function the linker stripped (the integrator's measurement on veh_vehicle.cpp's 0x3AF040) */
void DeadTurretDefinitionsKill()
{
    CTurret::m_turret_def.clear();
    std::vector<CTurretDef> tmp;
}
