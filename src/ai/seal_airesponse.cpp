/* seal_airesponse.cpp -- the states an AI SEAL falls into in response: hostage, avoid, rush, path
 * recovery, flee, target, event, stun response, hold, surrender, no ammo, reload, shot at (SOCOM
 * 1's unit, __sinit_seal_airesponse.cpp -- its CAiSRestrained and CAiSCorpse are gone, CAiSAvoid
 * moved in from seal_aistate.cpp, CAiSHostage is here). OVL2 0x601880-0x609DEF: the states'
 * destructors and the inline virtuals their tables pull in, the NoAmmo/Flee/Reload Ticks and seven
 * OnSignals (round 3). Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "../gamez/zkit/zkit.h"
#include "../gamez/zsnd/zsnd.h"

/* the nearest weapon pickup to a point within a range, of a weapon or of any (255), or 0 (OVL2
 * 0x6084B0: the pickup state's helper 0x5F51A0 asks it; new in SOCOM II, the name ours). The walk
 * tests the call's own result (`while ((p = ...) != 0)`: the copy in the delay slot); a `for` with
 * the call in its step tests the copy. The cursor declared before the result takes the higher
 * saved register */
CPickup *FindNearestPickup(const CPnt3D *pos, unsigned char weapon, float range)
{
    CPickup::iterator it = CPickup::GetFirst();
    float best = range * range;
    CPickup *p;
    CPickup *found = 0;

    while ((p = CPickup::GetNext(it, CPickup::PU_WEAPON)) != 0) {
        CZWeapon *w = p->GetWeapon();

        if (weapon == 255 || weapon == w->m_ID) {
            CPnt3D d;

            pos->Sub(p->GetPosW(), &d);
            float d2 = d.x * d.x + d.y * d.y + d.z * d.z;

            if (d2 < best) {
                best = d2;
                found = p;
            }
        }
    }
    return found;
}

/* a chance taken: a random float in [0, 1) against the odds (an inline, seal_control.cpp's shape:
 * the odds by value, read through a reference -- the target stores them and reads them back) */
inline bool Chance(float odds)
{
    const float &c = odds;
    Rfloat r;

    r.m_min = 0.0f;
    r.m_range = 1.0f;
    return r <= c;
}

/* a SEAL of the player's side (SEAL_AI_PARAMS's bit 1 at 0x264, SOCOM 1's m_navyseal; an inline
 * bool accessor: its callers keep the sltu) */
inline bool NavySeal(CSealCtrlAi *ai)
{
    return ai->m_navyseal;
}

/* a map cell copied field by field (SOCOM 1's written copy constructor, CAiMapLoc(const
 * CAiMapLoc &), whose out-of-line copy is 0x5EA7E0; zentity.h's CAiMapLoc has none -- the name
 * ours) */
struct CAiMapLocCopy {
    CAiMapLocCopy(const CAiMapLoc &o) : m_mapid(o.m_mapid), x(o.x), y(o.y) {}

    unsigned int m_mapid : 6;
    unsigned int x : 13;
    unsigned int y : 13;
};

class CZKit;
class CZFTSWeapon;

extern "C" {
/* CZKit's (seal_kit.cpp; kit.h's CZKit cannot meet zseal.h's here) */
CZFTSWeapon *GetItemByIndex__5CZKitCFi(const CZKit *kit, int index);
void *GetAmmoByIndex__5CZKitFi(CZKit *kit, int index);
/* CZSealBody's (named; zseal.h's CZSealBody does not declare them) */
void SetWeaponState__10CZSealBodyF12WEAPON_STATE(CEntity *seal, int state);
void PickupObject__10CZSealBodyFP7CPickup(CEntity *seal, CPickup *pickup);
/* the AI's state-clock hold (OVL2 0x570F90, the SEAL lane's: SOCOM 1 set the goal timer at 0x1BC
 * and saturated it in place) */
void func_00570F90(CSealCtrlAi *ai, float t);
/* CSealCtrlAi::SetStance, SetRoute, SetDoReaction and GetRouteComplete (SOCOM 1's, called from
 * the same Ticks; OVL2 0x5631A0, 0x5623B0, 0x561E20, 0x5629B0 -- the SEAL lane's rows) */
void SetStance__11CSealCtrlAiF11SEAL_STANCE(CSealCtrlAi *ai, int stance);
bool func_005623B0(CSealCtrlAi *ai, void *loc);
void func_00561E20(CSealCtrlAi *ai, int on, int reaction);
bool func_005629B0(CSealCtrlAi *ai);
/* a traversable cell beside a point (SOCOM 1's free FindTraversableNeighbor, now on the AI maps;
 * OVL2 0x529A30, the AI core's row) */
bool GetTraversableLocation__7CAiMapsFPC6CPnt3DR9CAiMapLoc(CAiMaps *maps, const CPnt3D *pos, void *loc);
/* a cell to flee to, and its centre (OVL2 0x52A3C0, 0x529EE0, the AI core's rows) */
void *GetClosestRegion__7CAiMapsFRC9CAiMapLocQ36CAiMap6region4TYPEb(CAiMaps *maps, CAiMapLocCopy *from, int a, int b);
bool GetRandomLoc__7CAiMapsFPCQ26CAiMap6regionR9CAiMapLoc(CAiMaps *maps, void *cell, unsigned int *loc);
/* the goto pushed to a cell (OVL2 0x5F0830, seal_aistate.cpp's) */
void func_005F0830(CSealCtrlAi *ai, unsigned int *loc, int mode, int kind, int step);
/* (OVL2 0x563050, a goal set with a float) */
void ChangeStanceDelayed__11CSealCtrlAiF11SEAL_STANCE11SEAL_STANCEf(CSealCtrlAi *ai, int a, int b, float f);
/* (OVL2 0x5628F0, 0x5B3890: the SEAL lane's rows -- the first on the controller, the second on
 * the SEAL) and a byte of the SEAL lane's (OVL2 bss 0x66AFE0, cleared by a hold) */
void func_005628F0(CSealCtrlAi *ai);
void func_005B3890(CEntity *seal, int x);
extern unsigned char data_0066AFE0;
/* an order given (OVL2 0x571010) with an anim this unit picks (0x609530); the stance set after a
 * delay (0x563100); a CSealCtrl call taking a signed byte (0x600D40, this lane's range, unwritten)
 * -- the SEAL lane's rows but 0x609530/0x600D40 */
enum ORDER_KIND { ORDER_KIND_0 = 0, ORDER_KIND_LAST = 255 };
void func_00571010(CSealCtrlAi *ai, AnimType anim, int a, ORDER_KIND kind);
AnimType func_00609530(unsigned char kind);
void SetStanceDelayed__11CSealCtrlAiF11SEAL_STANCEf(CSealCtrlAi *ai, signed char stance, float delay);
void ClearLook__9CSealCtrlF7AI_LOOK(CSealCtrlAi *ai, int x);
/* (OVL2 0x567840, 0x570D60: the SEAL lane's rows -- a stun's controller calls, the second
 * taking a float) */
void StopThrottles__11CSealCtrlAiFb(CSealCtrlAi *ai, int x);
/* the kit's reload (CZKit::Reload, OVL2 0x5C2A90, seal_kit.cpp's -- spelled by its mangled name:
 * called on the SEAL's kit at 0x5E0 by the reload state's sub-step, its result unread) */
bool Reload__5CZKitFv(void *kit);
void func_00570D60(CSealCtrlAi *ai, int x, float f);
/* the AI's target held (OVL2 0x60A850, after the response states: CAiTarget's, the target at
 * CSealCtrlAi 0x67C) and a look pushed at a point (OVL2 0x5F0B30, seal_aistate.cpp's) */
bool func_0060A850(void *target);
bool func_005F0B30(CSealCtrlAi *ai, const CPnt3D *pos, int mode, int a, int b);
/* the nearest ammunition for a weapon (SOCOM 1's ai::find_closest_ammo, after the no-ammunition
 * state as there; OVL2 0x6082E0) */
CPickup *func_006082E0(const CPnt3D *pos, float range, CZKit *kit, CZFTSWeapon *w);
/* the surrender's: the player's SEAL (OVL1 0x2B3580), the mission's team valves (CMission::
 * RefreshTeamValves, OVL1 0x2B23A0, on theMission 0x4364E0 -- the game states'), a squad joined
 * (CSealUnit::SealJoinUnit, OVL2 0x5EE250, seal_unit.cpp's -- zseal.h's CSealUnit does not declare
 * it) -- spelled by their mangled names */
CZSealBody *ftsGetPlayer__Fv();
extern int theMission;
void RefreshTeamValves__8CMissionFv(int *mission);
void SealJoinUnit__9CSealUnitFP10CZSealBody(CSealUnit *unit, CZSealBody *seal);
}

/* the destructors (OVL2 0x609D60 back to 0x609710, and CAiSHostage's at 0x601880): each its own
 * table stored, then CAiState's, its sub-state stack destroyed out of line */
CAiSShotAt::~CAiSShotAt()
{
}

CAiSReload::~CAiSReload()
{
}

CAiSNoAmmo::~CAiSNoAmmo()
{
}

CAiSSurrender::~CAiSSurrender()
{
}

CAiSHold::~CAiSHold()
{
}

CAiSStunResponse::~CAiSStunResponse()
{
}

CAiSEvent::~CAiSEvent()
{
}

CAiSTarget::~CAiSTarget()
{
}

CAiSFlee::~CAiSFlee()
{
}

/* (OVL2 0x609810) its members destroyed by hand, as ai_states.h keeps them as bytes: the vector
 * first, as the CAiMapLocVec it is (its deleter's destructor 0x515200 behind ~vector's and
 * ~__vector_imp's this-tests), then the route (~CAiPath, 0x5281B0) */
CAiSPathRecover::~CAiSPathRecover()
{
    ((CAiMapLocVec *)m_vec)->~CAiMapLocVec();
    __dt__7CAiPathFv(m_path, -1);
}

CAiSRush::~CAiSRush()
{
}

CAiSAvoid::~CAiSAvoid()
{
}

/* an ammunition pickup's stash: its entries' count, and an entry's weapon, cartridge and rounds,
 * false past the live ones (OVL1's CZItemStash, zwep_weapon.cpp's class: 0x3CFCF0 Count and
 * 0x3CFD70 Get -- a view of the two members called; the pickup's data is the stash, zseal.h's
 * CPickupAmmo only a name) */
class CZItemStash {
public:
    int Count();
    bool Get(int i, int *a, int *b, int *c);
};

extern "C" {
/* whether a cartridge fits a weapon (OVL1 0x3D2680, zweapon.h's CZWeapon::IsLegalAmmo -- by its
 * mangled name: this unit does not include zweapon.h) */
bool IsLegalAmmo__8CZWeaponFP6CZAmmo(CZFTSWeapon *w, CZAmmo *ammo);
}

/* a weapon's item (zweapon.h's CZWeapon::m_ID at 0x7C, a byte enum: read with lb -- a view) */
struct CAiWeaponID {
    unsigned char _0[0x7c];
    EQUIP_ITEM m_ID;   // 0x7c
};

/* a point's length squared (inline: the difference returned by value stays in the frame, its y and
 * z read back through their addresses -- seal_look.cpp's LenSq) */
static inline float AmmoLenSq(const CPnt3D &v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

/* the nearest ammunition pickup within range of a point with a cartridge the weapon takes and the
 * kit has room for, or 0 (OVL2 0x6082E0: CAiSNoAmmo::Tick's call; new in SOCOM II -- the name
 * unknown). The none-yet result zeroed before the index lookup (the target's jal delay slot); the
 * difference a named point through Sub, read in place by the inline length */
extern "C" CPickup *func_006082E0(const CPnt3D *pos, float range, CZKit *kit, CZFTSWeapon *w)
{
    if (!kit || !w)
        return 0;
    CPickup::iterator it = CPickup::GetFirst();
    CPickup *p;

    range *= range;
    CPickup *best = 0;
    int index = kit->GetItemIndex(((CAiWeaponID *)w)->m_ID);

    while ((p = CPickup::GetNext(it, CPickup::PU_AMMO)) != 0) {
        CPickupAmmo *ammo = p->GetAmmo();
        bool ok = false;
        int n = ((CZItemStash *)ammo)->Count();

        for (int i = 0; i < n; i++) {
            int weapon = 0;
            int cartridge = 0;
            int rounds = 0;

            ((CZItemStash *)ammo)->Get(i, &weapon, &cartridge, &rounds);
            if (IsLegalAmmo__8CZWeaponFP6CZAmmo(w, (CZAmmo *)cartridge) && kit->CanAddAmmo(index, rounds))
                ok = rounds > 0;
        }
        if (ok) {
            CPnt3D v;

            pos->Sub(p->GetPosW(), &v);
            float d = AmmoLenSq(v);

            if (d < range) {
                best = p;
                range = d;
            }
        }
    }
    return best;
}

/* a no-ammunition state's reset (OVL2 0x608290; SOCOM 1's) */
void CAiSNoAmmo::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_pickup = 0;
        break;
    case SIGNAL_0:
        break;
    case SIGNAL_1:
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        break;
    }
}

/* out of ammunition: crouched, weapon down, off to the nearest ammunition for the first item and
 * picking it up -- or, with none to reach, a reaction (not for the player's side) and done (OVL2
 * 0x608060; SOCOM 1's) */
int CAiSNoAmmo::Tick(CSealCtrlAi *ai)
{
    unsigned int phase;
    bool routed = false;
    CEntity *seal = ai->m_entity;

    phase = m_phase;
    CZKit *kit = (CZKit *)((char *)seal + 0x5e0);
    CZFTSWeapon *w = GetItemByIndex__5CZKitCFi(kit, 0);

    GetAmmoByIndex__5CZKitFi(kit, 0);
    switch (phase) {
    case 0:
        func_00570F90(ai, 1.0f);
        SetStance__11CSealCtrlAiF11SEAL_STANCE(ai, 1);
        SetWeaponState__10CZSealBodyF12WEAPON_STATE(seal, 2);
        if ((m_pickup = func_006082E0(&seal->m_posW, 5000.0f, kit, w)) != 0) {
            unsigned int loc;

            *(unsigned int *)&m_pickupLoc = *(unsigned int *)&seal->m_mapLocValid;
            if (GetTraversableLocation__7CAiMapsFPC6CPnt3DR9CAiMapLoc(&AiMaps, m_pickup->GetPosW(), &loc) && func_005623B0(ai, &loc)) {
                phase = 1;
                SetStance__11CSealCtrlAiF11SEAL_STANCE(ai, 0);
                func_00570F90(ai, 1.0f);
                routed = true;
            }
        }
        if (!routed && NavySeal(ai) == false) {
            func_00561E20(ai, 1, -1);
            m_active = false;
        }
        break;
    case 1:
        if (func_005629B0(ai)) {
            if (m_pickup) {
                PickupObject__10CZSealBodyFP7CPickup(seal, m_pickup);
                m_pickup = 0;
                phase = 2;
            } else
                phase = 2;
        }
        break;
    case 2:
        m_active = false;
        break;
    }
    UpdatePhase(phase);
    return Result();
}

/* a team mask without the team bits 4-15 and the two top bits -- but those of `keep` -- and with bit
 * 26 (SOCOM 1's ClearTeamMask(unsigned int, unsigned int), out of line there, 0x2E9090; inlined in
 * SOCOM II, its loop unrolled six times) */
inline unsigned int ClearTeamMask(unsigned int mask, unsigned int keep)
{
    unsigned int m = 0;

    for (int i = 4; i < 16; i++)
        m |= 1 << i;
    m |= 0xc0000000;
    m &= ~keep;
    return (mask & ~m) | 0x04000000;
}

/* a surrender's events: reset; entering -- no target, the team kept and cleared to the player's (as
 * SOCOM 1's), the SEAL shootable, the surrender call (74), standing, the team valves, a script
 * state's hold released; a sub-state (1) -- a living SEAL leaves its squad, gets its team back and
 * rejoins the squad kept, the team valves and the anim set; a second sub-state (2) the anim set,
 * the blends to the base and not restrainable; leaving (3) phase 4 and standing (OVL2 0x607D70;
 * SOCOM 1's 0x2E7640, ClearTeamMask out of line there, the shootable bit, the call, the script's
 * hold and the restrainable bit new) */
void CAiSSurrender::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    CZSealBody *seal = ai ? (CZSealBody *)ai->m_entity : 0;

    switch (sig) {
    case SIGNAL_4:
        m_5c = -1;
        m_58 = -1;
        m_60 = 1;
        m_unit = 0;
        m_team = 0;
        break;
    case SIGNAL_0: {
        ai->SetTarget(0);
        m_team = seal->m_team;
        CZSealBody *player = ftsGetPlayer__Fv();
        unsigned int mask = m_team;

        seal->SetTeamMask(ClearTeamMask(mask, player ? player->m_team : 0));
        seal->m_noshoot = false;
        ai->PlayAiSound((CHARACTER_SOUND)74, 1.0f, CAiEvent::AIE_NONE);
        SetStance__11CSealCtrlAiF11SEAL_STANCE(ai, 0);
        RefreshTeamValves__8CMissionFv(&theMission);
        CAiState *s = ai->FindAiState(AI_STATE_SCRIPT);
        CAiSScript *script = s && s->m_state == AI_STATE_SCRIPT ? (CAiSScript *)s : 0;

        if (script)
            script->SetHold(false);
        break;
    }
    case SIGNAL_1:
        if (seal->m_isAlive) {
            if (ai->m_unit)
                CSealUnit::SealLeaveUnit(seal);
            seal->SetTeamMask(m_team);
            if (m_unit)
                SealJoinUnit__9CSealUnitFP10CZSealBody(m_unit, seal);
        }
        RefreshTeamValves__8CMissionFv(&theMission);
        seal->BindAnimSet(seal->m_530, false);
        break;
    case SIGNAL_2:
        seal->BindAnimSet(seal->m_530, false);
        seal->ClearToBase(false);
        seal->m_restrainable = false;
        break;
    case SIGNAL_3:
        UpdatePhase(4);
        SetStance__11CSealCtrlAiF11SEAL_STANCE(ai, 0);
        break;
    }
}

/* a surrender takes another state only once it is past its fifth phase: any, for an entity of the
 * 0x20000 team bit, else kinds 23 and 28 (OVL2 0x607D10) */
bool CAiSSurrender::CanPushAiState(AI_STATE state, CSealCtrlAi *ai)
{
    bool ok = false;

    if ((int)m_phase >= 5) {
        if (ai->m_entity->m_team & 0x20000)
            ok = true;
        else
            ok = state == 23 || state == 28;
    }
    return ok;
}

/* a SEAL's animation set as the surrender binds one (seal_body.cpp's AnimSet: SOCOM 1's anim.cpp's
 * class, as much as Find needs -- its iterator pieces called, OVL2 0x5788C0-0x578980, the SEALs'
 * copies; GetName five inline levels high, as there) */
class AnimSet {
public:
    std::string &Name0() { return m_name; }
    std::string &Name1() { return Name0(); }
    std::string &Name2() { return Name1(); }
    std::string &Name3() { return Name2(); }
    std::string &GetName() { return Name3(); }
    static AnimSet *Find(const char *name)
    {
        std::vector<AnimSet *>::iterator i;

        for (i = list.begin(); i != list.end(); i++)
            if (!strcmp(name, (*i)->GetName().c_str()))
                return *i;
        return 0;
    }

    static std::vector<AnimSet *> list;

    unsigned char _0[0x38];
    std::string m_name;   // 0x38
};

extern "C" {
/* the top blend of a SEAL's stack and a blend's flag test (seal.cpp's, OVL2 0x555170 / 0x5551A0) */
void *func_00555170(void *stack);
bool func_005551A0(unsigned short flags, unsigned short bits);
/* the hands-up blend's callback (OVL2 0x608050, a tail call to 0x5A0040) */
void func_00608050(void *body, void *userdata);
/* whether a SEAL of a team is within a range (OVL2 0x569C60), the AI's mode 1 kept (0x570D90),
 * a state of the AI's tested (0x5F51A0) and the SEAL's stance put back (0x5A5790) -- the SEAL
 * lane's rows, unnamed */
bool func_00569C60(CZSealBody *seal, float *range, unsigned int team, int a, int b);
void func_00570D90(CSealCtrlAi *ai, int mode);
bool func_005F51A0(CSealCtrlAi *ai, int a, int b);
void func_005A5790(CZSealBody *seal, int stance, float t);
/* an anim played in a state, and whether it still runs (seal_ai.cpp's 0x571150, 0x5712F0: below) */
bool func_00571150(CSealCtrlAi *ai, short anim, SEAL_STATE stance);
bool func_005712F0(CSealCtrlAi *ai, short anim);
}

/* the surrender's anims: the hands going up and the kneel (OVL1 data: AnimTypes statics, unnamed) */
extern AnimType data_003DEF70, data_003DEF78;
/* the wait before the surrender may end and the range a team-mate ends it from (OVL2 data 0x650E00,
 * 0x650E08: 10.0 and 250.0, 8 bytes apart -- function-local statics in the original) and a flag the
 * mission sets (OVL2 bss 0x66BBA0) */
extern float data_00650E00, data_00650E08;
extern unsigned char data_0066BBA0;

/* a surrender played out: the AI stopped, unaware and weapon down, a random wait; the hands-up anim
 * once nothing blends (its callback fixed); the kneel and the "No_weapon anim set" bound; the squad
 * left -- and, for a player-team SEAL, team 17's joined --, the idle stance and restrainable; then,
 * every ten seconds, done once no SEAL of its kept team is within 250 or the mission's flag is set
 * (OVL2 0x6078A0; SOCOM 1's 0x2E7340 grown) */
int CAiSSurrender::Tick(CSealCtrlAi *ai)
{
    unsigned int step = m_phase;
    CZSealBody *seal = (CZSealBody *)ai->m_entity;

    ai->m_aibit19 = false;
    switch (step) {
    case 0: {
        ai->m_aibit16 = true;
        ai->m_aibit17 = false;
        StopThrottles__11CSealCtrlAiFb(ai, 0);
        ai->SetAiMode((AI_MODE)1);
        ai->SetAiAware(AIA_GENERAL, 0, -1.0f);
        SetWeaponState__10CZSealBodyF12WEAPON_STATE(seal, 2);
        Rfloat r;

        r.m_min = 0.25f;
        r.m_range = 0.25f;
        float t = r;

        m_time = t + ai->m_stateStack.m_timer;
        step = 1;
        m_58 = data_003DEF70;
        m_5c = data_003DEF78;
        break;
    }
    case 1:
        if (!(ai->m_stateStack.m_timer < m_time)) {
            CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&seal->m_blends);

            if (!b || !func_005551A0(b->m_flags, 2))
                step = 2;
        }
        break;
    case 2:
        if (func_00571150(ai, (short)m_58, (SEAL_STATE)0)) {
            if (m_60) {
                CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170(&seal->m_blends);

                if (b) {
                    b->m_callback.m_func = func_00608050;
                    b->m_callback.m_tfrac = 0.65f;
                    b->m_callback.m_userdata = 0;
                }
                m_60 = 0;
            }
            step = 3;
        }
        break;
    case 3:
        if (!func_005712F0(ai, (short)m_58)) {
            AnimSet *set = AnimSet::Find("No_weapon anim set");

            if (set) {
                seal->BindAnimSet(set, false);
                seal->m_530 = set;
            }
            step = 4;
        }
        break;
    case 4:
        m_unit = ai->m_unit;
        if (m_unit)
            CSealUnit::SealLeaveUnit(seal);
        if (seal->m_team & 0x20000) {
            CSealUnit *unit = CSealUnit::GetUnitByTeam(17);

            if (unit)
                SealJoinUnit__9CSealUnitFP10CZSealBody(unit, seal);
        }
        RefreshTeamValves__8CMissionFv(&theMission);
        seal->SetStanceIdle((AnimType)m_5c);
        step = 5;
        seal->m_restrainable = true;
        m_time = 0.0f;
        break;
    case 5:
        m_time += m_dT;
        if (!(m_time < data_00650E00)) {
            float range = data_00650E08;

            if (!func_00569C60(seal, &range, m_team, 0, 0) || data_0066BBA0) {
                ai->PopAiState((AI_STATE)6);
                func_00570D90(ai, 1);
                if (!func_005F51A0(ai, 0, 2))
                    func_005A5790(seal, 3, 0.0f);
                return 0;
            }
            m_time = 0.0f;
        }
        break;
    }
    UpdatePhase(step);
    return Result();
}

/* a target state: the AI's mode 4 on entry, its goal cleared on the way out (OVL2 0x604C60;
 * SOCOM 1's) */
void CAiSTarget::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        break;
    case SIGNAL_0:
        ai->SetAiMode((AI_MODE)4);
        break;
    case SIGNAL_1:
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        ai->ClearGoal(GOAL_NONE);
        break;
    }
}

/* a target's sub-step: over once the AI's target (CAiTarget at the controller's 0x67C) no longer
 * holds (OVL2 0x604BF0) */
int CAiSTarget::Virtual14(CSealCtrlAi *ai, CAiState *sub)
{
    if (!func_0060A850((char *)ai + 0x67c))
        SetActive(false);
    return Result();
}

/* the controller's alert amount at 0x718, a float (the name ours) */
struct CAiCtrl718f {
    unsigned char _0[0x718];
    float m_718;   // 0x718
};

/* the controller's target (SOCOM 1's CAiTarget, ai_target.cpp's: only what this unit calls) */
class CAiTarget {
public:
    float GetTimeSinceLastSighting() const;

    CEntity *m_entity;   // 0x0
};

/* the AI's look and moves toward a target (OVL2 0x5F0B30, 0x5F0990: seal_aistate.cpp's), the alert
 * gathered by 0x572240 dropped (OVL2 0x572310, the SEAL lane's row) and two of the SEAL lane's
 * tests (OVL2 0x558F80: the SEAL's kit holds a sniper rifle; 0x558FB0: the SEAL in a region of
 * kind 6) */
extern "C" {
bool func_005F0990(CSealCtrlAi *ai, const CPnt3D *pos, int mode, int a, int b);
void func_00572310(CSealCtrlAi *ai);
bool func_00558F80(CSealCtrlAi *ai);
bool func_00558FB0(CSealCtrlAi *ai);
bool func_00607830(const CSealCtrlAi *ai);
}
/* (seal_airesponse.cpp's, defined with the event state below) */
inline bool TargetHeld(CSealCtrlAi *ai);

/* the target's entity's position, or 0 (an inline: the target re-reads it at each use) */
static inline const CPnt3D *TargetPos(CSealCtrlAi *ai)
{
    CEntity *t = ((CAiTarget *)ai->m_67c)->m_entity;

    return t ? &t->m_posW : 0;
}

/* the target lost: out of sight for over three seconds (an inline bool; the name ours) */
static inline bool SightLost(CSealCtrlAi *ai)
{
    return ((CAiTarget *)ai->m_67c)->GetTimeSinceLastSighting() > 3.0f;
}

/* done with the target: it is gone, or neither held nor seen for three seconds (an inline bool;
 * the name ours) */
static inline bool TargetGone(CSealCtrlAi *ai)
{
    bool r = true;

    if (func_0060A850(ai->m_67c)) {
        if (!TargetHeld(ai))
            r = SightLost(ai);
        else
            r = false;
    }
    return r;
}

/* the target held with an alert gathered (an inline bool: the target materialises the `&&`; the
 * name ours) */
static inline bool HeldAlerted(CSealCtrlAi *ai)
{
    return TargetHeld(ai) && ((CAiCtrl718f *)ai)->m_718 > 0.0f;
}

/* the AI too busy to go for the target: a broken hold (func_00607830's body: the target inlines it,
 * the AI taken into a const local -- read through the parameter, MW hoists the stack's address for
 * GetState above the size test where the target keeps it in the call's delay slot), a sniper rifle,
 * or (not a SEAL of the player's side) in a region of kind 6. Inline bools, each a result preset
 * and changed under nested tests with a self-assigning else arm (the kit lane's round 33: the
 * target's preset-then-clear `||` and set-under-test `&&` -- `return a || b;` gives MW's value
 * select, if-forms without the arm if-convert); `!m_navyseal` an inline of its own (materialised,
 * sltu/xori). The names ours */
inline bool HoldBroken(const CAiState *s);
static inline bool HoldBrokenOf(CSealCtrlAi *ai)
{
    const CSealCtrlAi *c = ai;
    const CAiState *s = c->m_stateStack.size() == 0 ? 0 : c->m_stateStack.GetState();

    return s ? HoldBroken(s) : false;
}

static inline bool HoldOrSniper(CSealCtrlAi *ai)
{
    bool r = true;

    if (!HoldBrokenOf(ai)) {
        if (!func_00558F80(ai))
            r = false;
        else
            r = r;
    }
    return r;
}

static inline bool NotNavySeal(CSealCtrlAi *ai)
{
    return !ai->m_navyseal;
}

static inline bool Region6(CSealCtrlAi *ai)
{
    bool r = false;

    if (NotNavySeal(ai)) {
        if (func_00558FB0(ai))
            r = true;
        else
            r = r;
    }
    return r;
}

static inline bool StayPut(CSealCtrlAi *ai)
{
    bool r = true;

    if (!HoldOrSniper(ai)) {
        if (!Region6(ai))
            r = false;
        else
            r = r;
    }
    return r;
}

/* a target state: once its time is up or the target gone, done (step 3); step 0 goes for the
 * target's cell -- a look pushed while the target is held with an alert gathered, else a move --
 * unless the AI should stay put, step 2 waits for the alert and 1.5 seconds before going again,
 * step 1 for the route (OVL2 0x604880; SOCOM 1's only timed out; byte-exact in the kit lane's round
 * 34: the shapes are the helpers') */
int CAiSTarget::Tick(CSealCtrlAi *ai)
{
    STEP step = (STEP)m_phase;

    if (!(ai->m_stateStack.m_timer < m_time) || TargetGone(ai))
        step = STEP_3;
    switch (step) {
    case STEP_0:
        if (!StayPut(ai)) {
            CAiMapLoc loc;

            if (AiMaps.GetLocation(TargetPos(ai), loc)) {
                if (HeldAlerted(ai))
                    func_005F0B30(ai, TargetPos(ai), 5, 2, 2);
                else
                    func_005F0990(ai, TargetPos(ai), 5, 2, 2);
                func_00572310(ai);
            }
        }
        step = STEP_2;
        break;
    case STEP_2:
        if (((CAiCtrl718f *)ai)->m_718 > 0.0f && m_phaseTime >= 1.5f)
            step = STEP_0;
        break;
    case STEP_1:
        if (func_005629B0(ai))
            step = STEP_2;
        break;
    }
    UpdatePhase(step);
    if (step == STEP_3)
        SetActive(false);
    return Result();
}

/* the controller's word at 0x718 (zseal.h does not name it -- the name ours) */
struct CAiCtrl718 {
    unsigned char _0[0x718];
    unsigned int m_718;   // 0x718
};

/* a random float: a minimum and a range (zkit.h's Rfloat, built in place) */
inline float Random(float min, float range)
{
    Rfloat r;

    r.m_min = min;
    r.m_range = range;
    return r;
}

/* the anim types the response anims are drawn from (OVL1 data: AnimTypes statics 8 bytes apart,
 * unnamed in the map) */
extern AnimType data_003DF010, data_003DF018, data_003DF020, data_003DF028, data_003DF030;
extern AnimType data_003DF048, data_003DF050, data_003DF058, data_003DF060, data_003DF068;
extern AnimType data_003DEFB0;
extern AnimType data_003DEFC8, data_003DEFB8, data_003DEFD8, data_003DF008, data_003DF000;

/* a list of anim types to draw from (the name ours) */
struct CAiAnimList {
    int m_count;   // 0x0
    AnimType m_anims[8];   // 0x4
};

/* an index in [0, max], drawn (inline: zero without a draw for none) */
static inline int RandMod(int n)
{
    return rand() % n;
}

static inline int RandIndex(int max)
{
    return max == 0 ? 0 : RandMod(max + 1);
}

/* one of a list's anim types drawn, -1 for none (inline) */
static inline AnimType PickAnim(const CAiAnimList *lists, unsigned char kind)
{
    AnimType r = animTypeUndefined;
    const CAiAnimList *l = &lists[kind];
    const AnimType *anims = l->m_anims;
    int count = l->m_count;

    if (anims && count > 0)
        r = anims[RandIndex(count - 1)];
    return r;
}

/* a response anim of a kind drawn from its list, the four lists filled on the first call (five,
 * five, one and five types; OVL2 0x609530, the shot-at state's, the event state's and the
 * callouts' orders take it -- the names ours). Round 43's result preset (`r = -1; if (anims && count
 * > 0) r = anims[...]; return r;`) gives the early return; the kit's round 50: the `rand() % n` an
 * inline of its own (RandMod) leaves the zero arm's `b` unfilled with the call's and the join's
 * labels padded (the AI core's round-22 rule; 22 -> 5), and the preset defined before the list's
 * address is taken -- PickAnim indexing the lists itself -- moves the index's first temporary off
 * v0 onto v1 (5 -> 0) */
AnimType func_00609530(unsigned char kind)
{
    static bool first = true;
    static CAiAnimList lists[4];

    if (first) {
        first = false;
        lists[0].m_count = 5;
        lists[0].m_anims[0] = data_003DF010;
        lists[0].m_anims[1] = data_003DF018;
        lists[0].m_anims[2] = data_003DF020;
        lists[0].m_anims[3] = data_003DF028;
        lists[0].m_anims[4] = data_003DF030;
        lists[1].m_count = 5;
        lists[1].m_anims[0] = data_003DF048;
        lists[1].m_anims[1] = data_003DF050;
        lists[1].m_anims[2] = data_003DF058;
        lists[1].m_anims[3] = data_003DF060;
        lists[1].m_anims[4] = data_003DF068;
        lists[2].m_count = 1;
        lists[2].m_anims[0] = data_003DEFB0;
        lists[3].m_count = 5;
        lists[3].m_anims[0] = data_003DEFC8;
        lists[3].m_anims[1] = data_003DEFB8;
        lists[3].m_anims[2] = data_003DEFD8;
        lists[3].m_anims[3] = data_003DF008;
        lists[3].m_anims[4] = data_003DF000;
    }
    return PickAnim(lists, kind);
}

/* shot at: the stance noted and a short random wait on entry, an order (kind 11) in the AI's
 * first two modes; on leaving for a sub-state the stance back after a random delay (OVL2 0x6093F0;
 * SOCOM 1's) */
void CAiSShotAt::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        break;
    case SIGNAL_0:
        m_stance = ai->m_stance;
        m_time = Random(0.05f, 0.2f);
        if (ai->GetAiAware() < AIA_GENERAL)
            func_00571010(ai, func_00609530(0), 2, (ORDER_KIND)11);
        break;
    case SIGNAL_1:
        SetStanceDelayed__11CSealCtrlAiF11SEAL_STANCEf(ai, m_stance, Random(0.25f, 0.25f));
        ClearLook__9CSealCtrlF7AI_LOOK(ai, 6);
        ((CAiCtrl718 *)ai)->m_718 = 0;
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        break;
    }
}

/* the controller's byte at 0x32C (zseal.h does not name it -- the name ours) */
struct CAiCtrl32C {
    unsigned char _0[0x32c];
    unsigned char m_32c;   // 0x32c
};

/* the shot-at state's limit (a function-local static in the original: 8-byte-spaced data) */
static float s_shotAtTime = 15.0f;

extern "C" {
/* the SEAL's grenade about to go off called out, when it may (OVL2 0x584B00 and 0x57E540, the
 * SEALs' seal_body.cpp and seal_ai.cpp rows) */
bool func_00584B00(CZSealBody *seal);
void func_0057E540(CZSealBody *seal);
}

/* the shot-at state's reaction at its step: in step 2 the AI's sound 72 at a quarter's chance, then
 * -- at a quarter's chance, when the SEAL may -- its grenade's callout, else at 0.15's chance and in
 * an AI mode of 3 or more a spray of 0.15 seconds; in step 3 the callout alone, in step 4 the spray
 * alone (0.75 seconds); nothing in step 6 (OVL2 0x609190: CAiSShotAt::Tick calls it four times, a
 * member -- the state in a0; the name unknown). The chances function-local statics (8-byte-spaced
 * data after the shot-at time's) */
extern "C" void func_00609190(CAiState *s, CSealCtrlAi *ai)
{
    CAiState::STEP step = (CAiState::STEP)s->m_phase;
    CZSealBody *seal = (CZSealBody *)ai->m_entity;
    float chance = 4.656613e-10f * rand();
    static float callChance = 0.25f;
    static float sprayChance = 0.15f;

    switch (step) {
    case CAiState::STEP_2:
        ai->PlayAiSound((CHARACTER_SOUND)72, 0.25f, CAiEvent::AIE_UNKNOWN);
        if (chance < callChance && func_00584B00(seal))
            func_0057E540(seal);
        else if (chance < sprayChance && ai->GetAiAware() > AIA_GENERAL)
            ai->SetSprayTimer(0.15f);
        break;
    case CAiState::STEP_3:
        if (chance < callChance && func_00584B00(seal))
            func_0057E540(seal);
        break;
    case CAiState::STEP_4:
        if (chance < sprayChance && ai->GetAiAware() > AIA_GENERAL)
            ai->SetSprayTimer(0.75f);
        break;
    case CAiState::STEP_6:
        break;
    }
}

/* a word's bits under a mask, the mask a parameter (the name ours): asked for all of them (~0) the
 * AND survives as the 32-bit zero extension `dsll32/dsrl32` before the branch -- written in place,
 * `& ~0u` folds away */
inline bool TestBits(unsigned int v, unsigned int mask)
{
    return (v & mask) != 0;
}

/* shot at, the sub-step: over after its time; before, once the AI's byte at 0x32C is set, the
 * sub-state ended and step 3 taken (OVL2 0x609340). The byte is tested through an inline's mask
 * parameter given ~0 (round 41: the target's `lbu; dsll32; dsrl32; beqz` -- an unsigned char, a
 * bitfield, a cast to a 64-bit type, `!= 0L` and `& ~0u` written in place all failed since round 7) */
int CAiSShotAt::Virtual14(CSealCtrlAi *ai, CAiState *sub)
{
    if (!(m_phaseTime < s_shotAtTime))
        SetActive(false);
    else if (TestBits(((CAiCtrl32C *)ai)->m_32c, ~0u)) {
        sub->SetActive(false);
        UpdatePhase(3);
    }
    return Result();
}

/* the controller's bit 17 at 0x170 (zseal.h's m_bit17), set through a bool: an inline setter */
inline void SetBit17(CSealCtrlAi *ai, bool on)
{
    ai->m_bit17 = on;
}

/* an event's events: reset (its points cleared, its clock to -1); on entry the AI's mode 4 and
 * the flags' bits 4 and 5; a sub-state hands it a look at one of its points (bit 8, bit 0 picking)
 * or the AI's own (bit 7); done on the way out unless bit 6 asked for more (OVL2 0x605A80;
 * SOCOM 1's) */
void CAiSEvent::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_rnd_min = 1.0f;
        m_rnd_range = 2.0f;
        m_80 = 0.5f;
        m_58 = 0;
        m_pos = CPnt3D::zero;
        m_pos2 = CPnt3D::zero;
        m_name[0] = 0;
        m_88 = 0;
        m_8c = 0;
        m_8d = 0;
        m_time = -1.0f;
        break;
    case SIGNAL_0:
        ai->SetAiMode((AI_MODE)4);
        SetFlags(0x30, 0);
        break;
    case SIGNAL_1:
        if (m_flags & 0x100) {
            ai->SetAiAware(AIA_DIRECTION, (m_flags & 1) ? &m_pos2 : &m_pos, -1.0f);
        } else if (m_flags & 0x80)
            ai->SetAiAware(AIA_GENERAL, 0, -1.0f);
        ClearLook__9CSealCtrlF7AI_LOOK(ai, 5);
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        m_active = (m_flags & 0x40) != 0;
        SetFlags(0, 0x40);
        break;
    }
}

/* the event's name, up to sixteen characters, or none (OVL2 0x605A10) */
void CAiSEvent::SetName(const char *name)
{
    if (name)
        strncpy(m_name, name, 16);
    else
        m_name[0] = 0;
}

/* the controller's word at 0x6A8 and float at 0x718 as the event state reads them (zseal.h does
 * not name them -- the names ours) */
struct CAiCtrlEvent {
    unsigned char _0[0x6a8];
    unsigned int m_6a8;   // 0x6a8: bit 9 lets a held target interrupt an event
    unsigned char _6ac[0x718 - 0x6ac];
    float m_718;   // 0x718
};

/* the AI's target held, and the AI's bit 9 at 0x6A8 set (an inline bool: the target materialises
 * the `&&`) */
inline bool TargetHeld(CSealCtrlAi *ai)
{
    return func_0060A850((char *)ai + 0x67c) && (((CAiCtrlEvent *)ai)->m_6a8 & 0x200);
}

/* a state's flags tested through an inline (the target loads the word again for it) */
inline bool HasFlags(const CAiState *s, unsigned int bits)
{
    return s->m_flags & bits;
}

/* the event's sub-states dropped and step 8 taken, the step it left kept in m_8d (inlined twice
 * in its sub-step; the name ours) */
inline void CAiSEventInterrupt(CAiSEvent *s, CSealCtrlAi *ai)
{
    s->m_stack.PopAll(ai);
    if (s->m_phase != 8) {
        s->m_8d = s->m_phase;
        s->UpdatePhase(8);
    }
}

/* the next step: 1 after 0; after 1, 2 with the flags' bit 0 or 3 with bit 1; after 2, 3 with bit
 * 1; else 8 -- kept, with the delay, and taken at once or after the wait (7) (OVL2 0x605950) */
void CAiSEvent::NextStep(STEP *step, float delay)
{
    STEP next = STEP_8;

    switch (*step) {
    case 0:
        next = STEP_1;
        break;
    case 1:
        if (m_flags & 1)
            next = STEP_2;
        else if (m_flags & 2)
            next = STEP_3;
        break;
    case 2:
        if (m_flags & 2)
            next = STEP_3;
        break;
    }
    m_84 = delay;
    m_8c = next;
    *step = delay == 0.0f ? next : STEP_7;
}

/* an event, the sub-step: nothing while a step-0 or unrecorded event waits (the flags' bit 2);
 * interrupted by a held target (bit 4, with the AI's bit 9 at 0x6A8) or by the AI's value at
 * 0x718 passing 1 (bit 5) (OVL2 0x605800) */
int CAiSEvent::Virtual14(CSealCtrlAi *ai, CAiState *sub)
{
    unsigned int flags = m_flags;

    if (flags & 4) {
        bool waiting = m_phase == 0 || m_8d == 0;

        if (waiting)
            goto done;
    }
    if ((flags & 0x10) && TargetHeld(ai))
        CAiSEventInterrupt(this, ai);
    else if (HasFlags(this, 0x20) && ((CAiCtrlEvent *)ai)->m_718 > 1.0f)
        CAiSEventInterrupt(this, ai);
done:
    return Result();
}

/* a callout: the sound played at a volume, and with `order` the order of kind 0 given when the
 * SEAL stands (GetStance(true), OVL2 0x58A720) (OVL2 0x605770: the AI's 0x5CA470 and the stack's
 * 0x5F8D40 call it; the name unknown) */
extern "C" void func_00605770(CSealCtrlAi *ai, bool order, CHARACTER_SOUND sound, float vol)
{
    CEntity *seal = ai->m_entity;

    ai->PlayAiSound(sound, vol, CAiEvent::AIE_NONE);
    if (order) {
        SEAL_STATE r = ((CZSealBody *)seal)->GetStance(true);

        if (r == stateStand)
            func_00571010(ai, func_00609530(0), 2, (ORDER_KIND)r);
    }
}

extern "C" {
/* a weapon kind asked of the AI (OVL2 0x5F6100, seal_aistate.cpp's: a CAiState member, unnamed) */
bool func_005F6100(CAiState *s, CSealCtrlAi *ai, int kind, bool keep);
}

/* an event's callout on its first step: a kind-5 event's sound 72 (the order with it when the SEAL
 * stands); else with the flags' bit 2 sound 70 likewise, with bit 3 sound 69 alone, the weapon kind
 * 4 asked (bit 1 keeping the old) and the event's order (kind 11, the second anim list) -- then
 * bits 2 and 3 traded for bit 4 (OVL2 0x6055B0: CAiSEvent::Tick's step after NextStep calls it, a
 * member -- the state in a0; new in SOCOM II, the name unknown). The SEAL read once at the top
 * (the target's s0, the stance after it): 0x605770 inlined reads it in each arm; the kind test a
 * one-case switch (`beq` to the case, `b` to the default) */
extern "C" void func_006055B0(CAiSEvent *s, CSealCtrlAi *ai)
{
    CEntity *seal = ai->m_entity;

    switch ((signed char)s->m_58) {
    case 5: {
        ai->PlayAiSound((CHARACTER_SOUND)72, 0.75f, CAiEvent::AIE_NONE);
        SEAL_STATE r = ((CZSealBody *)seal)->GetStance(true);

        if (r == stateStand)
            func_00571010(ai, func_00609530(0), 2, (ORDER_KIND)r);
        break;
    }
    default:
        if (s->m_flags & 4) {
            ai->PlayAiSound((CHARACTER_SOUND)70, 1.0f, CAiEvent::AIE_NONE);
            SEAL_STATE r = ((CZSealBody *)seal)->GetStance(true);

            if (r == stateStand)
                func_00571010(ai, func_00609530(0), 2, (ORDER_KIND)r);
        } else if (s->m_flags & 8) {
            ai->PlayAiSound((CHARACTER_SOUND)69, 0.25f, CAiEvent::AIE_NONE);
            if (s->m_flags & 2)
                func_005F6100(s, ai, 4, true);
            func_00571010(ai, func_00609530(1), 258, (ORDER_KIND)11);
        }
        s->SetFlags(0x10, 0xc);
    }
}

/* the hold's signed-byte count (a file static: its Tick reads it too; OVL2 data 0x650E10) */
static signed char s_holdCount = 3;

/* a hold's events: reset; entering and leaving, the AI's goals dropped and the bit off; a sub-state
 * (1, 2) puts a goal back and the bit on (OVL2 0x607650; SOCOM 1's) */
void CAiSHold::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    CEntity *seal = ai ? ai->m_entity : 0;

    switch (sig) {
    case SIGNAL_4:
        m_58 = -1;
        *(unsigned int *)&m_cell = *(unsigned int *)&CAiMapLoc::invalid;
        m_78 = 0;
        m_70 = 0;
        m_6c = 0;
        break;
    case SIGNAL_0:
        func_005628F0(ai);
        ai->ClearGoal(GOAL_NONE);
        func_005B3890(seal, 1);
        SetBit17(ai, false);
        break;
    case SIGNAL_1:
        ChangeStanceDelayed__11CSealCtrlAiF11SEAL_STANCE11SEAL_STANCEf(ai, 2, 1, -1.0f);
        ClearLook__9CSealCtrlF7AI_LOOK(ai, s_holdCount);
        if (m_flags & 0x80)
            data_0066AFE0 = 0;
        ai->m_bit17 = true;
        func_005B3890(seal, 1);
        break;
    case SIGNAL_2:
        ChangeStanceDelayed__11CSealCtrlAiF11SEAL_STANCE11SEAL_STANCEf(ai, 2, 1, -1.0f);
        ai->m_bit17 = true;
        break;
    case SIGNAL_3:
        func_005628F0(ai);
        ai->ClearGoal(GOAL_NONE);
        SetBit17(ai, false);
        if (m_flags & 0x20)
            UpdatePhase(0);
        break;
    }
}

/* a hold with its flags' bit 5 or 7 set (an inline bool: its true is daddiu, the result narrowed;
 * `else r = r;` keeps the flags test a branch -- without the arm MW if-converts it into sltu -- and
 * vanishes itself, the kit lane's round 33) */
inline bool HoldBroken(const CAiState *s)
{
    bool r = false;

    if (s->m_state == AI_STATE_HOLD) {
        if (s->m_flags & 0xa0)
            r = true;
        else
            r = r;
    }
    return r;
}

/* whether the AI's current state is a hold with its flags' bit 5 or 7 set (OVL2 0x607830: the SEAL
 * lane's 0x558AE0 and 0x563E10 call it; the name ours). The stack is read through a const AI (its
 * const GetState called, 0x5F8330, the size read at ai+0x41C); byte-exact in the kit lane's round
 * 33 with HoldBroken's self-assigning else arm and the ternary over it (open since round 12: every
 * form without the arm if-converted the flags test) */
extern "C" bool func_00607830(const CSealCtrlAi *ai)
{
    const CAiState *s = ai->m_stateStack.size() == 0 ? 0 : ai->m_stateStack.GetState();

    return s ? HoldBroken(s) : false;
}

/* the response states' callouts, looked up by name once the sounds are open: forty-three cue
 * names and their sounds (OVL2 data 0x650E20; the table's and the functions' names ours) */
struct CAiCallout {
    const char *m_name;
    CSnd *m_snd;
};

static CAiCallout s_callouts[43] = {
    { "^CS_0222", 0 }, { "^CS_0226", 0 }, { "^CS_0292", 0 }, { "^*CS_0275", 0 },
    { "^*CS_0276", 0 }, { "^*CS_0278", 0 }, { "^CS_0270", 0 }, { "^CS_0283", 0 },
    { "^*CS_0285", 0 }, { "^CS_0286", 0 }, { "^CS_0266", 0 }, { "^CS_0265", 0 },
    { "^CS_0264", 0 }, { "^CS_0263", 0 }, { "^CS_0262", 0 }, { "^CS_0261", 0 },
    { "^CS_0260", 0 }, { "^CS_1291", 0 }, { "^CS_1292", 0 }, { "^CS_1293", 0 },
    { "^CS_1294", 0 }, { "^CS_1295", 0 }, { "^CS_1296", 0 }, { "^CS_1297", 0 },
    { "^CS_1298", 0 }, { "^CS_1299", 0 }, { "^CS_1300", 0 }, { "^CS_1301", 0 },
    { "^CS_1302", 0 }, { "^CS_1303", 0 }, { "^CS_1304", 0 }, { "^CS_1305", 0 },
    { "^CS_0268", 0 }, { "^CS_0269", 0 }, { "^CS_0282", 0 }, { "^CS_0281", 0 },
    { "^CS_1309", 0 }, { "^CS_1307", 0 }, { "^CS_1310", 0 }, { "^CS_1308", 0 },
    { "^CS_0288", 0 }, { "^CS_0295", 0 }, { "^CS_0308", 0 },
};

/* each callout's sound looked up and put in mode 2 (OVL2 0x6075C0: the SEAL lane's 0x574C30
 * calls it) */
extern "C" void func_006075C0()
{
    for (int i = 0; i < 43; i++) {
        s_callouts[i].m_snd = s_callouts[i].m_name ? CSnd::GetSoundByName(s_callouts[i].m_name) : 0;
        if (s_callouts[i].m_snd)
            s_callouts[i].m_snd->m_mode = 2;
    }
}

/* the sounds forgotten (OVL2 0x607560: the SEAL lane's 0x574BD0 calls it; each entry through a
 * pointer local -- `s_callouts[i].m_snd = 0` folds the field's offset into the remainder pass's
 * address) */
extern "C" void func_00607560()
{
    for (int i = 0; i < 43; i++) {
        CAiCallout *c = &s_callouts[i];

        c->m_snd = 0;
    }
}

/* the callouts' first float for the positional player (OVL2 data 0x650F78, after the table: three
 * of the unit's functions read it -- 0x6067A0, 0x606DB0, 0x606EC0; the name ours) */
static float s_calloutRange = 5.0f;

/* the positional player (OVL1 0x3428F0, char_type.cpp's reading: the sound, the entity's id, the
 * position, two floats) */
extern "C" void func_003428F0(CSnd *sound, unsigned int id, const CPnt3D *pos, float a, float b);

/* a callout played for a SEAL: at its position, or as its own when the sound's bit 3 is set (inline:
 * its by-value arguments read before the null test, the float first; the name ours) */
inline void PlayCallout(CSnd *snd, CEntity *seal, float range)
{
    if (snd) {
        if (snd->m_socom2_bit3)
            func_003428F0(snd, seal->m_id, 0, range, 1.0f);
        else
            func_003428F0(snd, seal->m_id, &seal->m_posW, range, 1.0f);
    }
}

/* a SEAL's callout on being hit, under the state's flag bit 7: "^CS_0288" when the shooter shares
 * its team, "^CS_0295" otherwise (OVL2 0x606DB0: CSealCtrlAi::OnWeaponHit calls it with the state
 * and controller 0x572880 hands back and the shooter; the name unknown) */
extern "C" void func_00606DB0(CAiState *s, CSealCtrlAi *ai, CEntity *e)
{
    if (ai && e && (s->m_flags & 0x80)) {
        CEntity *seal = ai->m_entity;

        if (seal->m_team & e->m_team)
            PlayCallout(s_callouts[40].m_snd, seal, s_calloutRange);
        else
            PlayCallout(s_callouts[41].m_snd, seal, s_calloutRange);
    }
}

/* the path recovery's tries (a file static: its Tick reads it too; OVL2 data 0x651000) */
static int s_pathTries = 10;

/* a path recovery's events: reset its tries; on entry the AI's mode 4 and a goal, the bit off;
 * the bit on again on leaving for a sub-state; done at the end (OVL2 0x604490; SOCOM 1's) */
void CAiSPathRecover::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_tries = s_pathTries;
        break;
    case SIGNAL_0:
        ai->SetAiMode((AI_MODE)4);
        StopThrottles__11CSealCtrlAiFb(ai, 0);
        ai->m_stamp.Reset();
        ChangeStanceDelayed__11CSealCtrlAiF11SEAL_STANCE11SEAL_STANCEf(ai, 2, 0, 0.1f);
        SetBit17(ai, false);
        m_58 = 0;
        m_time = 0.0f;
        break;
    case SIGNAL_1:
        SetBit17(ai, true);
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        m_active = false;
        break;
    }
}

/* the rush's count of SEALs in it (the SEALs' OVL2 bss 0x66B048: seal_ai.cpp's AI init zeroes it) */
extern "C" int data_0066B048;

/* the countdown run down by dt, whether it has run out -- an infinite one never runs (seal_ai.cpp's
 * TickGoalTimer; the name ours) */
static inline bool RushTimerTick(CAiTimer &t, float dt)
{
    bool r = false;

    if (t.m_timer != CAiTimer::infinite) {
        t.m_timer -= dt;
        r = t.m_timer != CAiTimer::infinite && t.m_timer <= 0.0f;
    }
    return r;
}

/* a rush's events: reset -- no target, the wait 1-5 seconds; entering -- standing, the state's
 * clock at 10, the flags' bits 0 and 2, the wait drawn and already run down by 0.5 to twice the
 * delay's minimum; a sub-state (1) leaves the count of rushing SEALs one less if it was in it and
 * drops the AI's goals; leaving (3) ends it (OVL2 0x6036B0; SOCOM 1's 0x2E51F0 cleared the target and
 * stood the SEAL only -- its signal 0's -1 clock is 10 here, the rest new) */
void CAiSRush::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    static Rfloat s_delay(3.0f, 5.0f);

    switch (sig) {
    case SIGNAL_4:
        m_target = CEntity::invalidId;
        m_timer.m_min = 1.0f;
        m_timer.m_range = 4.0f;
        break;
    case SIGNAL_0:
        SetStance__11CSealCtrlAiF11SEAL_STANCE(ai, 0);
        m_time = 10.0f;
        SetFlags(1, 0);
        SetFlags(4, 0);
        m_timer.m_timer = m_timer.Period();
        RushTimerTick(m_timer, Rfloat(0.5f, 2.0f * s_delay.m_min));
        break;
    case SIGNAL_1:
        if (m_flags & 0x10)
            data_0066B048--;
        func_005628F0(ai);
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        SetActive(false);
        break;
    }
}

/* the SEAL rushed, kept by its id -- the invalid id for none (OVL2 0x603690, between the rush's Tick
 * and OnSignal: SOCOM 1's CAiSRush::SetTarget 0x2E51D0, between the same two, its words with the
 * target at 0x5C for 0x2C) */
void CAiSRush::SetTarget(CZSealBody *seal)
{
    m_target = seal ? seal->m_id : CEntity::invalidId;
}

/* standing or crouched (an inline taking the stance by value: its argument is read ahead of the
 * `&&` it sits in, the compare narrows it again; the name ours) */
inline bool Upright(SEAL_STANCE s)
{
    return s == STANCE_STAND || s == STANCE_CROUCH;
}

/* whether the AI drives a SEAL (CZSealBody::IsPlayer's ternary shape: each arm narrowed) and the
 * AI's current state as an avoidance, or 0 (the names ours) */
inline bool AiDriven(CZSealBody *s)
{
    return s->GetControl() ? ((CSealCtrl *)s->GetControl())->IsAi() : false;
}
inline CAiSAvoid *AvoidState(CSealCtrlAi *ai)
{
    CAiState *s = ai->GetAiState();

    return s && s->m_state == AI_STATE_AVOID ? (CAiSAvoid *)s : 0;
}

/* whether SEAL `a`, `d2` (a squared distance) from SEAL `b`, should step out of its way: either
 * moving and `a` up, when `b` is not the AI's; when it is, unless `b` lies prone, `b` not already
 * avoiding `a`; never when `a` is the AI's and restrainable but not restrained (OVL2 0x602B20: the
 * SEAL lane's 0x558440 asks it; new in SOCOM II, between CAiSAvoid's Tick and OnSignal -- the
 * name ours). Is() where GetState is a call: AiStateIs (zseal.h) */
bool ShouldAvoid(CZSealBody *a, CZSealBody *b, float d2)
{
    static float range = 225.0f;
    static float speed = 50.0f;
    bool r = false;

    if (d2 < range) {
        bool aMoving = ai::is_moving(&a->m_velW, speed);
        bool bMoving = ai::is_moving(&b->m_velW, speed);
        CSealCtrlAi *aAi = a->GetSealCtrlAi();

        if (!AiDriven(b)) {
            SEAL_STANCE st = aAi->m_stance;

            r = (bMoving || aMoving) && Upright(st);
        } else {
            CSealCtrlAi *bAi = b->GetSealCtrlAi();

            if (bAi->m_stance == STANCE_PRONE)
                r = false;
            else if (bAi->AiStateIs(AI_STATE_AVOID))
                r = a != (CZSealBody *)AvoidState(bAi)->m_58;
            else
                r = true;
        }
        if (aAi && a->m_restrainable && !a->m_restrained)
            r = false;
    }
    return r;
}

/* an avoidance's events: reset; up from prone on entry and on the way out; its goal dropped when
 * a sub-state takes over (1) or it is left (2, done) (OVL2 0x602D10; SOCOM 1's) */
void CAiSAvoid::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_58 = 0;
        break;
    case SIGNAL_0:
        if (ai->m_stance == STANCE_PRONE)
            SetStance__11CSealCtrlAiF11SEAL_STANCE(ai, 1);
        break;
    case SIGNAL_2:
        ai->ClearGoal(GOAL_AVOID);
        ai->m_stamp.Reset();
        m_active = false;
        break;
    case SIGNAL_1:
        ai->ClearGoal(GOAL_AVOID);
        ai->m_stamp.Reset();
        break;
    case SIGNAL_3:
        if (ai->m_stance == STANCE_PRONE)
            SetStance__11CSealCtrlAiF11SEAL_STANCE(ai, 1);
        UpdatePhase(0);
        break;
    }
}

/* a free cell in a direction from a map spot (seal_ai.cpp's OVL2 0x559580, the SEAL lane's row,
 * not written: the avoidance's step 3 asks it with a mask and a count) */
extern "C" CAiMap::DIR func_00559580(const CAiMapLoc *loc, const CPnt3D *dir, unsigned int mask, CAiMapLoc *out,
                                      int n, int *got);

/* an avoidance's steps: done once the SEAL stepped away from is gone or dead; then by step -- 0 the
 * wait (a function-local static nothing sets), 1 the AI's mode 4 and, with no goal yet, the point and
 * (flag 2) the facing as the goal, 3 a free cell found the way to step within five as the avoid
 * goal, 4 the wait run down, 5 a quarter second more unless the other SEAL's AI is still avoiding --
 * each set-up then waits in step 4; its wait out, the state ends and its goal is dropped when the
 * stamp's bits 1-2 hold one (OVL2 0x601F80; SOCOM 1's took dT). The goal's kind is the field's own
 * GOAL_TYPE (ai_states.h's since the kit's round 51: a signed char cast to the enum loads with lbu
 * where the target has lb -- round 50 read it through a pointer cast); the SEAL avoided and the AI's entity declared self first (the target's a0 / a1), the cell before the
 * count (the count the lower slot) */
int CAiSAvoid::Tick(CSealCtrlAi *ai)
{
    CEntity *self = ai->m_entity;
    CZSealBody *other = (CZSealBody *)m_58;

    if (!other || !other->m_isAlive)
        return AI_DONE;
    switch ((STEP)m_phase) {
    case STEP_0: {
        static float s_wait;

        m_time = s_wait;
        UpdatePhase(4);
        break;
    }
    case STEP_1: {
        ai->SetAiMode((AI_MODE)4);
        bool has = (bool)ai->m_goal_dir_valid || (bool)ai->m_goal_pos_valid;

        if (!has) {
            ai->SetGoalPos(m_goal, &m_78, 3.0f);
            if (m_flags & 2)
                ai->SetGoalDir(m_goal, &m_84, true);
            m_time = 0.25f;
            UpdatePhase(4);
        }
        break;
    }
    case STEP_3: {
        CAiMapLoc cell;
        int got = 0;

        if (func_00559580(&self->m_mapLocValid, &m_6c, 0x10000, &cell, 5, &got) != CAiMap::DIR_COUNT) {
            ai->SetGoalPos(GOAL_AVOID, cell, 5.0f);
            ai->SetAiMode((AI_MODE)4);
            m_time = 2.0f;
            UpdatePhase(4);
        }
        break;
    }
    case STEP_2:
        break;
    case STEP_4:
        m_time -= m_dT;
        break;
    case STEP_5: {
        CSealCtrlAi *oai = other->GetSealCtrlAi();

        if (!oai) {
            m_time = 0.25f;
            UpdatePhase(4);
        } else if (!oai->AiStateIs(AI_STATE_AVOID)) {
            m_time = 0.25f;
            UpdatePhase(4);
        }
        break;
    }
    }
    if ((STEP)m_phase == STEP_4 && m_time <= 0.0f) {
        SetActive(false);
        if (ai->m_stamp.m_8 & 6)
            ai->ClearGoal(GOAL_AVOID);
    }
    return Result();
}

/* a float of the controller's (CSealCtrl at 0xD8; zseal.h does not name it -- the name ours) */
struct CAiCtrlD8 {
    unsigned char _0[0xd8];
    float m_d8;   // 0xd8
};

extern "C" {
/* an anim played in a state, or not, and whether it still runs (seal_ai.cpp's OVL2 0x571150 and
 * 0x5712F0, the SEAL lane's rows, unnamed) */
bool func_00571150(CSealCtrlAi *ai, short anim, SEAL_STATE stance);
bool func_005712F0(CSealCtrlAi *ai, short anim);
}

/* the countdown run down by dt, whether it has run out -- then drawn again from its period (the
 * rush's RushTimerTick with the restart; the name ours) */
static inline bool StunTimerTick(CAiTimer &t, float dt)
{
    bool r = false;

    if (t.m_timer != CAiTimer::infinite) {
        t.m_timer -= dt;
        r = t.m_timer != CAiTimer::infinite && t.m_timer <= 0.0f;
        if (r)
            t.m_timer = t.Period();
    }
    return r;
}

/* a stun response: its wait run down, the response's anim played in the SEAL's stance (step 2 while
 * it runs, else straight to 3 with the flags' bit 0), then until the AI's state clock reaches the
 * state's time with bit 0, at once without; while it runs, the sound at the SEAL each time the timer
 * runs out unless bit 2 is set without bit 1 -- bit 2 set after (OVL2 0x605C60; SOCOM 1's took dT).
 * The SEAL a local for the stance's call alone (in a0 across the dispatch; the sound reads the
 * entity again), declared ahead of the step so the step's load goes through `this`'s home; the
 * clock test `!(clock >= time)` (materialised, then flipped); the frame time read ahead of the
 * sound and flag tests, in their block */
int CAiSStunResponse::Tick(CSealCtrlAi *ai)
{
    CEntity *self = ai->m_entity;
    unsigned int step = m_phase;

    switch (step) {
    case 0:
        if (!(m_wait <= 0.0f)) {
            m_wait -= m_dT;
            break;
        }
    case 1:
        if (!func_00571150(ai, (short)m_5c, ((CZSealBody *)self)->GetStance(false))) {
            step = 3;
            SetFlags(1, 0);
        } else
            step = 2;
        break;
    case 2:
        if (!func_005712F0(ai, (short)m_5c))
            step = 3;
        break;
    case 3:
        if (m_flags & 1)
            SetActive(!(ai->m_stateStack.m_timer >= m_time));
        else
            SetActive(false);
        break;
    }
    if (m_active) {
        float dt = m_dT;

        if (m_snd && ((m_flags & 2) || !(m_flags & 4)) && StunTimerTick(m_timer, dt)) {
            CSndInstance *inst = m_snd->PlaySound3D(&ai->m_entity->m_posW, 1.0f, 0, 0, 255);

            if (inst)
                inst->TerminateOnFinish();
            SetFlags(4, 0);
        }
    }
    UpdatePhase(step);
    return Result();
}

/* a stun response's events: reset; on entry a random wait under 0.75 and the AI's mode 2; done
 * on the way out (OVL2 0x605F90; SOCOM 1's) */
void CAiSStunResponse::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_wait = 0.0f;
        m_5c = -1;
        m_snd = 0;
        m_time = 0.0f;
        break;
    case SIGNAL_0: {
        Rfloat r;

        r.m_min = 0.0f;
        r.m_range = 0.75f;
        m_wait = r;
        StopThrottles__11CSealCtrlAiFb(ai, 0);
        func_00570D60(ai, 0, 0.0f);
        ai->SetAiMode((AI_MODE)2);
        break;
    }
    case SIGNAL_1:
        func_00570D60(ai, 0, ((CAiCtrlD8 *)ai)->m_d8);
        break;
    case SIGNAL_2:
    case SIGNAL_3:
        m_active = false;
        break;
    }
}

/* the controller's target (CSealCtrlAi at 0x67C: SOCOM 1's CAiTarget m_target at 0x4BC, its
 * entity first; zseal.h does not name it) */
struct CAiCtrlTarget {
    unsigned char _0[0x67c];
    CEntity *m_target;   // 0x67c
};

/* the reload's wait (a function-local static in the original: 8-byte-spaced data) */
static float s_reloadWait = 0.5f;

/* the reload, the sub-step: a shout first when the flags' bit 0 asks (then cleared), the kit
 * reloaded, a stance -- crouched with bit 2, else kept or (bit 1) changed -- then a short wait
 * (OVL2 0x608840) */
int CAiSReload::Virtual14(CSealCtrlAi *ai, CAiState *sub)
{
    unsigned int phase = m_phase;
    CEntity *seal = ai->m_entity;

    switch (phase) {
    case 1:
        if (m_flags & 1) {
            ai->PlayAiSound((CHARACTER_SOUND)68, 1.0f, CAiEvent::AIE_NONE);
            SetFlags(0, 1);
        }
        Reload__5CZKitFv((char *)seal + 0x5e0);
        if (m_flags & 4)
            ChangeStanceDelayed__11CSealCtrlAiF11SEAL_STANCE11SEAL_STANCEf(ai, 2, 1, -1.0f);
        else
            ChangeStanceDelayed__11CSealCtrlAiF11SEAL_STANCE11SEAL_STANCEf(ai, 0, (m_flags & 2) ? 2 : 1, -1.0f);
        m_time = s_reloadWait;
        phase = 2;
        break;
    case 2:
        m_time -= m_dT;
        if (m_time <= 0.0f)
            SetActive(false);
        break;
    }
    UpdatePhase(phase);
    return Result();
}

/* a reload: weapon down, the stance kept, a crouch one time in ten and a move one in four, a look
 * at the target when there is one (the flags' bits 0-2), then the sub-states' turn (OVL2
 * 0x6085C0; SOCOM 1's reloaded itself) */
int CAiSReload::Tick(CSealCtrlAi *ai)
{
    CEntity *seal = ai->m_entity;
    unsigned int phase = m_phase;

    switch (phase) {
    case 0:
        SetWeaponState__10CZSealBodyF12WEAPON_STATE(seal, 2);
        m_stance = ai->m_stance;
        if (Chance(0.1f))
            SetFlags(1, 0);
        else
            SetFlags(0, 1);
        if (Chance(0.25f))
            SetFlags(2, 0);
        else
            SetFlags(0, 2);
        /* the target object's address, spelled apart from its entity's load: taken as
         * &...->m_target MW keeps it in a saved register for both */
        if (func_0060A850((char *)ai + 0x67c)) {
            CEntity *t = ((CAiCtrlTarget *)ai)->m_target;

            if (func_005F0B30(ai, t ? &t->m_posW : 0, 5, -1, -1))
                SetFlags(4, 0);
        }
        phase = 1;
        break;
    case 1:
    case 2:
        Virtual14(ai, 0);
        phase = m_phase;
        break;
    }
    UpdatePhase(phase);
    return Result();
}

/* a flight: to a cell away from danger (the goto pushed, a shout, crouched), the AI's mode 1 for
 * five seconds, then done (OVL2 0x6045B0; SOCOM 1's ran its own route) */
int CAiSFlee::Tick(CSealCtrlAi *ai)
{
    STEP step = (STEP)m_phase;
    CAiMapLocCopy from(ai->m_entity->m_mapLocValid);

    switch (step) {
    case STEP_0: {
        void *cell = GetClosestRegion__7CAiMapsFRC9CAiMapLocQ36CAiMap6region4TYPEb(&AiMaps, &from, 2, 0);

        if (!cell)
            step = STEP_2;
        else if (GetRandomLoc__7CAiMapsFPCQ26CAiMap6regionR9CAiMapLoc(&AiMaps, cell, (unsigned int *)&m_cell)) {
            step = STEP_2;
            func_005F0830(ai, (unsigned int *)&m_cell, 4, 1, 2);
            ai->PlayAiSound((CHARACTER_SOUND)72, 0.75f, CAiEvent::AIE_NONE);
            SetStance__11CSealCtrlAiF11SEAL_STANCE(ai, 0);
            func_00570F90(ai, 1.0f);
            ai->SetTarget(0);
        } else
            step = STEP_2;
        break;
    }
    case STEP_1: {
        static float wait = 5.0f;

        ai->SetAiMode((AI_MODE)1);
        if (!(m_phaseTime < wait))
            step = STEP_4;
        break;
    }
    case STEP_3:
        step = STEP_1;
        break;
    case STEP_2:
        step = STEP_4;
        break;
    case STEP_4:
        m_active = false;
        break;
    }
    if (step != m_phase)
        ai->m_stateStack.SetFlags(1, 0);
    UpdatePhase(step);
    return Result();
}

/* a flight's reset (OVL2 0x604830; SOCOM 1's) */
void CAiSFlee::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_60 = 0;
        m_58 = true;
        break;
    case SIGNAL_0:
        break;
    case SIGNAL_1:
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        break;
    }
}

/* the hostage's captor: its id and team, the flags' bit 0 when asked (OVL2 0x601E50) */
void CAiSHostage::SetCaptor(CEntity *e, bool flag)
{
    if (flag)
        SetFlags(1, 0);
    if (e) {
        m_5c = e->m_id;
        m_58 = e->m_team;
    }
}

/* a hostage's reset: no captor (OVL2 0x601F20; SOCOM 1's cleared one word) */
void CAiSHostage::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_5c = CEntity::invalidId;
        m_58 = -1;
        break;
    case SIGNAL_0:
        break;
    case SIGNAL_1:
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        break;
    }
}

/* a hostage takes any state but kind 16, and kind 19 only in its phases 5 and 6 (OVL2
 * 0x601EC0) */
bool CAiSHostage::CanPushAiState(AI_STATE state, CSealCtrlAi *ai)
{
    bool ok = true;
    unsigned char phase = m_phase;

    switch (state) {
    case 19:
        ok = phase == 5 || phase == 6;
        break;
    case 16:
        ok = false;
        break;
    }
    return ok;
}

CAiSHostage::~CAiSHostage()
{
}
