/* veh_turret.cpp -- CTurret, a mounted gun (SOCOM 1's veh_turret.cpp, by its static initialiser:
 * SOCOM II's 0x404240 builds the definitions (0x4A4038) and the turret list (0x4A4058) and sets
 * turret_debug_mask's bits, as SOCOM 1's __sinit_veh_turret.cpp does). OVL1 0x3A71A0-0x3ACA5F,
 * opening with the copies of its inline virtual overrides and closing with the two vectors'
 * destructors; CVehicle's unit follows. Defined in reverse of their image order (-inline
 * deferred). */

#include <string.h>

#include "zveh.h"

using namespace zdb;

/* not yet written here (placeholders): the turret list's remove, the turret's halves of the
 * tick, the network turret's send (fts_*'s, 0x2C7A60) and the AI maps' cell lookup */
extern "C" {
void func_003A71F0(CTurretList *list, CTurret *turret);
void func_003A7350(CTurret *turret, float dT);
void func_003A78E0(CTurret *turret, float dT);
void func_003A82C0(CTurret *turret);
void func_003A84E0(CTurret *turret);
void func_003A88C0(CTurret *turret, float dT);
void func_003A8D20(CTurret *turret);
void func_002C7A60(CTurret *turret, int n);
int func_0052DC70(CAiMaps *maps, const CPnt3D *pos, unsigned int *loc);
void func_003AB8C0(CTurretDefVector *v, unsigned int n);
/* the turrets made, and the most there may be (-1: no limit) -- the unit's statics */
extern int data_004A4020;
extern int data_003E0800;
}

CZSealBody *ftsGetPlayer();

CTurretDefVector CTurret::m_turret_def;
CTurretList CTurret::m_turrets;

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

/* OPEN, 12 instructions: everything but where `i = 0` lands -- the target fills the resize
 * call's delay slot with it (`move a0,this` before the jal), MW here puts the this-move there and
 * sinks the zero to the loop's first test. Tried: the zero before the call (14), in the for's
 * init through a comma (12), declared after the call (12), a member call instead of the C
 * placeholder (12, the same schedule), an int counter (97: Item(i) is then not strength-reduced,
 * so the counter is unsigned). The resize is MSL's class-vector resize (0x3AB8C0), which
 * include/vector does not declare, called through its placeholder. */
int CTurretDefVector::Init(const char *name, const char *path)
{
    _zrdr *rdr = zrdr_read(name, path, 0);

    if (rdr) {
        _zrdr *list = zrdr_findtag(rdr, "TURRET");
        unsigned int i;

        func_003AB8C0(this, (list ? list->Count() : 0) / 2);
        i = 0;
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
    func_003A71F0(&m_turrets, this);
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

    if (func_0052DC70(&AiMaps, &m_posW, &loc)) {
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
        m_258 = r * m_def->m_120;
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

/* the countdown started over from a random period */
static inline float RandomPeriod(const CAiTimer &t)
{
    float f;

    if (t.m_range == 0.0f)
        f = t.m_min;
    else
        f = t.m_min + t.m_range * (4.656613e-10f * rand());
    return f;
}

static inline void ResetTimer(CAiTimer &t)
{
    t.m_timer = RandomPeriod(t);
}

/* OPEN, 2 instructions: the network test loads the player's controller into v0 and this one's
 * into v1, then `beq v1,v0` (this one's first); every spelling here gives either the two
 * registers the other way (the call's side first: `ftsGetPlayer()->m_control != m_control`,
 * `!(... == ...)`) or this one's hoisted above the call into a saved register (`m_control !=
 * ftsGetPlayer()->m_control`, through an inline accessor too, or a cast). PrePostTick's test
 * matched only with the player in a local, which loads this one's first: the opposite. */
void CTurret::Tick(float dT)
{
    if (!GetFlag(1))
        return;
    if (GetFlag(2))
        func_003A84E0(this);
    else if (theNetwork.m_9 && ftsGetPlayer()->m_control != m_control)
        func_003A78E0(this, dT);
    else if (m_control) {
        CEntityCtrl *ctrl = m_control;

        m_3a8 = m_def->m_114 * ctrl->m_throttle[2];
        m_3ac = m_def->m_118 * ctrl->m_throttle[3];
        m_velR.x = m_3ac;
        m_velR.y = m_3a8;
        m_velR.z = 0.0f;
    }
    func_003A7350(this, dT);
    if (GetFlag(4)) {
        if (TickTimer(m_fireTimer, dT))
            func_003A8D20(this);
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
            func_002C7A60(this, 0);
    }
}

void CTurret::UpdatePosW()
{
    m_posW = *(CPnt3D *)m_300.m_matrix[3];
}

void CTurret::ComputeNextPosition(float dT)
{
}
