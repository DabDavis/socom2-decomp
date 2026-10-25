/* seal.cpp -- the SEAL's body, CZSealBody: its ticks, its states, its actions, its stats (SOCOM 1's
 * unit, __sinit_seal.cpp; the press beta's asserts file OpenHeartbeatSounds and
 * ExecuteDelayedAction_Jump under it). OVL2 0x542FE0-0x5552BF (0x542E50 is CPickup::Close, pickup_main.cpp's) in SOCOM 1's order -- the heartbeat
 * first, the constructor and SealInitCharacterDynamics last, then the template tail. Defined in
 * reverse of their image order (-inline deferred). */

#include "zseal.h"
#include "../zkit/zkit.h"
#include "../zanim/zanim.h"
#include "../../ai/ai.h"
#include "../zsnd/zsnd.h"
#include <stdio.h>
#include <string.h>

class CValve;

extern "C" {
void func_005C7840(CZKit *kit);   /* CZKit::PostSetupItems (SOCOM 1's OnMissionStart calls it) */
void func_003CAEE0(CZProjectile *proj, const CPnt3D *vel);   /* CZProjectile::SetVelW (SOCOM 1's
                                                                 _rethrow_grenade calls it) */
void func_00525200(void *path);   /* (a CAiPath reset: its member at 0x98's, tail-called) */
void func_00525210(void *path, unsigned int length);   /* CAiPath::SetSnakeLength (SOCOM 1's
                                                          SetSnakePathUpdate calls it) */
}

/* the world's camera (zdb::CWorld at 0x128; only that) */
struct CWorldCamera {
    unsigned char _0[0x128];
    zdb::CCamera *m_camera;   // 0x128
};

/* a halfword test: any of the bits (OVL2 0x5551A0; in the unit's tail) */
extern "C" bool func_005551A0(unsigned short flags, unsigned short bits)
{
    return (flags & bits) != 0;
}

/* an indexed array's current element, or none (OVL2 0x555170; in the unit's tail) */
struct CIndexedArray {
    void **m_data;   // 0x0
    unsigned char _4[4];
    int m_index;   // 0x8
};

extern "C" void *func_00555170(CIndexedArray *a)
{
    if (a->m_index < 0)
        return 0;
    return a->m_data[a->m_index];
}

/* the SEAL's stats (SOCOM 1's DWARF: CSealStats, 0x54 bytes of ints; SOCOM II's counters are
 * unsigned shorts) -- only the counters the incrementers touch, by halfword */
class CSealStats {
public:
    unsigned short m_count[0x3c / 2];   // 0x0
    short m_score;   // 0x3c: the MVP score (SOCOM 1's m_sMvpScore, a short there too)
    unsigned short m_count2[3];   // 0x3e
    float m_hit_percent;   // 0x44 (SOCOM 1's 0x28)
};

CZSealBody *ftsGetPlayer();

extern "C" {
/* (OVL2 0x544D60, not written yet: the SEAL's score passed on -- the stats' events end on it) */
void func_00544D60(const CZSealBody *seal, int a);
}

/* whether an event counts: always offline, online only for the local player (the inline both
 * builds' stats test first) */
static inline bool StatCounts(const CZSealBody *seal)
{
    return !theNetwork.m_9 || seal == ftsGetPlayer();
}

/* the stats packed for the network: ten bytes, the counters as bytes, the hit percentage made a
 * whole number; in game type 1 only two (OVL2 0x545A30) */
struct SEAL_STATS_PACKET {
    unsigned char m_0, m_1, m_2, m_3, m_4, m_5, m_6;
    unsigned char _7;
    short m_score;   // 0x8
};

extern "C" {
extern unsigned char data_004364F8;   /* (OVL1 bss: a game type, 1 sending only two counters) */
extern CNetGameState *data_00437CE8;
/* (OVL1, unowned: two tests of a SEAL against the game, both builds' net code) */
int func_002C31D0(const CZSealBody *seal);
int func_002C30B0(const CZSealBody *seal);
}

extern "C" void func_00545A30(CSealStats *s, SEAL_STATS_PACKET *p)
{
    memset(p, 0, 10);
    p->m_0 = s->m_count[0xc / 2];
    p->m_3 = s->m_count[0x12 / 2];
    if (data_004364F8 != 1) {
        p->m_1 = s->m_count[0];
        p->m_2 = s->m_count[0x10 / 2];
        p->m_4 = s->m_count[0x1a / 2];
        p->m_5 = (unsigned int)(100.0f * s->m_hit_percent);
        p->m_6 = s->m_count[0x1c / 2];
        p->m_score = s->m_score;
    }
}

/* a death: the score down two, or in game type 3 online moved by the two tests; the counter's new
 * value returned (OVL2 0x545390) */
extern "C" unsigned short func_00545390(CSealStats *s, const CZSealBody *seal)
{
    if (StatCounts(seal)) {
        if (theNetwork.m_9 && data_00437CE8->m_mode == 3) {
            if (func_002C31D0(seal))
                s->m_score -= 2;
            else if (func_002C30B0(seal))
                s->m_score += 3;
        } else
            s->m_score -= 2;
    }
    func_00544D60(seal, 0);
    s->m_count[0x18 / 2]++;
    return s->m_count[0x18 / 2];
}

/* the stats' events: a counter stepped and the score moved when it counts, the score passed on
 * (OVL2 0x545D10, 0x545C90, 0x545C10, 0x545B90, 0x545B10, 0x5459C0, 0x5455D0, 0x545530,
 * 0x544CF0; SOCOM 1's Kill, Hit, Death and the rest by their shape, not yet named) */
extern "C" void func_00545D10(CSealStats *s, const CZSealBody *seal)
{
    if (StatCounts(seal))
        s->m_count[0x12 / 2]++;
}

extern "C" void func_00545C90(CSealStats *s, const CZSealBody *seal)
{
    if (StatCounts(seal)) {
        s->m_count[0x14 / 2]++;
        s->m_score -= 2;
    }
    func_00544D60(seal, 0);
}

extern "C" void func_00545C10(CSealStats *s, const CZSealBody *seal)
{
    if (StatCounts(seal)) {
        s->m_count[0x16 / 2]++;
        s->m_score -= 2;
    }
    func_00544D60(seal, 0);
}

extern "C" void func_00545B90(CSealStats *s, const CZSealBody *seal)
{
    if (StatCounts(seal)) {
        s->m_count2[2]++;
        s->m_score += 1;
    }
    func_00544D60(seal, 0);
}

extern "C" void func_00545B10(CSealStats *s, const CZSealBody *seal)
{
    s->m_count2[0]++;
    if (StatCounts(seal))
        s->m_score += 5;
    func_00544D60(seal, 0);
}

extern "C" void func_005459C0(CSealStats *s, const CZSealBody *seal)
{
    if (StatCounts(seal))
        s->m_count[0]++;
}

extern "C" void func_005455D0(CSealStats *s, const CZSealBody *seal)
{
    if (StatCounts(seal)) {
        s->m_count[0x38 / 2]++;
        s->m_score += 2;
    }
    func_00544D60(seal, 1);
}

extern "C" void func_00545530(CSealStats *s, const CZSealBody *seal, int points)
{
    if (StatCounts(seal) && points > 0) {
        s->m_count[0x3a / 2]++;
        s->m_score += points;
    }
    func_00544D60(seal, 1);
}

extern "C" void func_00544CF0(CSealStats *s, const CZSealBody *seal)
{
    if (StatCounts(seal))
        s->m_count[0x10 / 2]++;
}

/* eight counters each added to (OVL2 0x5451E0, 0x5451C0, 0x5451A0, 0x545180, 0x545160, 0x545140,
 * 0x545120, 0x545100; SOCOM 1's Incr* took ints) */
extern "C" void func_005451E0(CSealStats *s, int n)
{
    s->m_count[0x1e / 2] += (unsigned short)n;
}

extern "C" void func_005451C0(CSealStats *s, int n)
{
    s->m_count[0x24 / 2] += (unsigned short)n;
}

extern "C" void func_005451A0(CSealStats *s, int n)
{
    s->m_count[0x22 / 2] += (unsigned short)n;
}

extern "C" void func_00545180(CSealStats *s, int n)
{
    s->m_count[0x26 / 2] += (unsigned short)n;
}

extern "C" void func_00545160(CSealStats *s, int n)
{
    s->m_count[0x28 / 2] += (unsigned short)n;
}

extern "C" void func_00545140(CSealStats *s, int n)
{
    s->m_count[0x2a / 2] += (unsigned short)n;
}

extern "C" void func_00545120(CSealStats *s, int n)
{
    s->m_count[0x2e / 2] += (unsigned short)n;
}

extern "C" void func_00545100(CSealStats *s, int n)
{
    s->m_count[0x30 / 2] += (unsigned short)n;
}

/* a picked-up grenade thrown back along the pickup's facing, lofted (OVL2 0x547700; SOCOM 1's
 * function-local statics, initialised statically by MW 3.0) */
static void _rethrow_grenade(CValve *valve, void *data)
{
    static float up = 80.0f;
    static float scale = 150.0f;
    CPickup *pickup = (CPickup *)data;

    if (!pickup || !pickup->m_node)
        return;
    CZProjectile *grenade = (CZProjectile *)pickup->m_data;
    CPnt3D vel = *(CPnt3D *)pickup->m_node->m_matrix.m_matrix[2];

    vel.y = up;
    vel.x *= scale;
    vel.z *= scale;
    func_003CAEE0(grenade, &vel);
}

/* the AI's aiming helper, CAi, is ai.h's */

extern "C" {
/* CValve::AddCallback (SOCOM 1's AddCallback__6CValveFUiPFP6CValvePv_vPv; zanim.h's CValve lacks
 * it: OVL1 0x351960 by its address) */
void func_00351960(CValve *valve, unsigned int mask, void (*func)(CValve *, void *), void *user);
/* the other kinds' takers: a weapon, ammunition (OVL2 0x59F610, 0x59F6F0, seal_env.cpp's, matched
 * as drafts; the MP bomb's is HandlePickupMPBomb, 0x59DEF0) */
void func_0059F610(CZSealBody *seal, CPickup *pickup);
void func_0059F6F0(CZSealBody *seal, CPickup *pickup);
}

/* a pickup taken up by the living: a grenade (kind 1) thrown back -- stopped, turned toward the
 * SEAL, and the pickup anim run with a valve whose callback throws it; a weapon, ammunition or the
 * MP bomb each taken by its own function; the anim returned (OVL2 0x547570; SOCOM 1's) */
CZAnim *CZSealBody::PickupObject(CPickup *pickup)
{
    if (!m_isAlive)
        return 0;
    CZAnim *done = 0;

    if (pickup) {
        switch (pickup->m_type) {
        case CPickup::PU_TYPE_1: {
            CZAnim *anim = ZAnim.GetAnim("ai_pickup_grenade", 0);

            if (anim) {
                CValve *valve = CValve::Create(0, (VALVE_TYPE)2);

                func_00351960(valve, 1, _rethrow_grenade, pickup);
                func_003CAEE0((CZProjectile *)pickup->m_data, 0);
                CMatrix m;
                const CMatrix *from = &m_node->m_matrix;

                CAi::AlignToPoint(from, pickup->GetPosW(), &m, 0.0f);
                pickup->m_node->SetMatrix(&m);
                done = anim->ActivateArg(1, 3, pickup->m_node, 1, m_select, 4, valve, 0);
            }
            break;
        }
        case CPickup::PU_WEAPON:
            func_0059F610(this, pickup);
            break;
        case CPickup::PU_AMMO:
            func_0059F6F0(this, pickup);
            break;
        case CPickup::PU_MPBOMB:
            HandlePickupMPBomb(pickup);
            break;
        case 10:
            break;
        }
    }
    return done;
}

/* the health set: a living SEAL brought to nothing dies of the given cause, anything else is
 * stored and the controller told (OVL2 0x5477A0; SOCOM 1's clamped and fiddled more) */
float CZSealBody::SetHealth(float health, SEAL_DEATH_TYPE type)
{
    float old = m_health;

    if (health != old) {
        bool alive = m_isAlive;

        if (alive || health <= 0.0f) {
            if (alive && health <= 0.0f) {
                m_health = 0.0f;
                m_deathType = type;
                OnDeath();
            } else {
                m_health = health;
                ((CSealCtrl *)m_control)->OnSetHealth(health, old);
            }
        }
    }
    return m_health;
}

/* the snake path: on with a length, or off; already on, left alone (OVL2 0x546C40) */
void CZSealBody::SetSnakePathUpdate(bool on, unsigned int length)
{
    if (on && m_snake_path)
        return;
    if (on) {
        m_snake_path = true;
        func_00525210(m_snakePath, length);
    } else {
        m_snake_path = false;
        func_00525210(m_snakePath, 0);
    }
}

/* teleported: the altitude and the last ground hit reset to the new place, the snake path
 * cleared; a player's camera sees every region again (OVL2 0x546E40, 0x546CF0; SOCOM II's) */
void CZSealBody::TeleportTo(const CPnt3D *pos, const CQuat *quat)
{
    if (pos) {
        ResetAltitude(pos->y);
        m_last_altitude.m_pos = CPnt3D::zero;
        m_last_altitude.m_norm = CPnt3D::zero;
        m_last_altitude.m_poly = 0;
        m_last_altitude.m_node = 0;
        m_last_altitude.m_pos = *pos;
        m_last_altitude.m_norm = CPnt3D::unit_y;
    }
    func_00525200(m_snakePath);
    CEntity::TeleportTo(pos, quat);
    if (IsPlayer() && zdb::CWorld::m_world) {
        zdb::CCamera *cam = ((CWorldCamera *)zdb::CWorld::m_world)->m_camera;

        if (cam) {
            cam->SetPlayerCanSeeRegions(-1);
            cam->SetCameraCanSeeRegions(-1);
        }
    }
}

void CZSealBody::TeleportTo(const CMatrix *m)
{
    ResetAltitude(m->m_matrix[3][1]);
    m_last_altitude.m_pos = CPnt3D::zero;
    m_last_altitude.m_norm = CPnt3D::zero;
    m_last_altitude.m_poly = 0;
    m_last_altitude.m_node = 0;
    m_last_altitude.m_pos = *(const CPnt3D *)m->m_matrix[3];
    m_last_altitude.m_norm = CPnt3D::unit_y;
    func_00525200(m_snakePath);
    CEntity::TeleportTo(m);
    if (IsPlayer() && zdb::CWorld::m_world) {
        zdb::CCamera *cam = ((CWorldCamera *)zdb::CWorld::m_world)->m_camera;

        if (cam) {
            cam->SetPlayerCanSeeRegions(-1);
            cam->SetCameraCanSeeRegions(-1);
        }
    }
}

/* the altitude taken from here and a new one asked for (OVL2 0x546CD0: SOCOM 1's
 * m_get_new_altitude is the entity's bit 8 now, zentity.h's m_bit8) */
void CZSealBody::ResetAltitude(float y)
{
    m_alt = y;
    m_bit8 = true;
}

/* the mission's start: the kit's items set up, then the controller's (OVL2 0x553E60) */
void CZSealBody::OnMissionStart()
{
    func_005C7840(&m_kit);
    ((CSealCtrl *)m_control)->OnMissionStart();
}

/* the mission's end: the SEAL's network pickups destroyed (SOCOM 1's tested m_id first), then
 * the controller's (OVL2 0x553E20) */
void CZSealBody::OnMissionEnd()
{
    CPickup::DestroyNetPickups(m_id);
    ((CSealCtrl *)m_control)->OnMissionEnd();
}

/* the root body part (OVL2 0x553DB0; SOCOM 1's, CBody's slot 0x10) */
CZBodyPart *CZSealBody::GetRootPart() const
{
    return m_root;
}

/* (OVL2 0x553E00) */
void CZSealBody::OnRecycleEntity(CEntity *e)
{
    ((CSealCtrl *)m_control)->OnRecycleEntity(e);
}

extern "C" {
/* the victim struck down (OVL2 0x547860, seal.cpp's) and the killer's credit (OVL2 0x59EE20,
 * seal_env.cpp's) -- not written yet */
void func_00547860(CZSealBody *victim);
void func_0059EE20(CZSealBody *victim, unsigned int killer, int a);
/* (OVL1 bss 0x44D400: an animation set the stats skip) */
extern AnimSet *data_0044D400;
}

/* a close-quarters attack on a victim: its hit flag set, a living one killed and the kill
 * credited; the attack's anim run between the two, and counted unless the attacker has the skipped
 * set (OVL2 0x546A40; SOCOM 1's made an AI event instead of the stats) */
bool CZSealBody::ExecuteAction_CQBAttack(CZSealBody *victim)
{
    bool done = false;

    if (victim) {
        bool alive = victim->m_isAlive;

        victim->SetHudHitFlag();
        if (alive) {
            unsigned int id = m_id;

            victim->m_lastkiller_id = id;
            func_00547860(victim);
            func_0059EE20(victim, id, 0);
        }
        CZAnim *anim = ZAnim.GetAnim("ai_cqb_attack", 0);
        CZAnim *act = anim ? anim->ActivateEx(2, m_select, victim->m_select, 0) : 0;

        if (act) {
            if (m_animset != data_0044D400) {
                m_5b8++;
                m_5b0 += 2;
            }
            done = true;
        }
    }
    return done;
}

/* a victim restrained (OVL2 0x546A00) */
bool CZSealBody::ExecuteAction_ApplyRestraint(CZSealBody *victim)
{
    if (!victim)
        return false;
    RestrainBuddy(victim);
    return true;
}

/* the delayed actions: jump and climb, taken on the next tick (OVL2 0x5469E0, 0x5469C0) */
bool CZSealBody::ExecuteDelayedAction_Climb()
{
    m_action_climb = true;
    return true;
}

bool CZSealBody::ExecuteDelayedAction_Jump()
{
    m_action_jump = true;
    return true;
}

/* an action offered or withdrawn: its bit in the mask (OVL2 0x544CA0; SOCOM 1's cleared the whole
 * mask for (0, false), SOCOM II's takes a byte) */
void CZSealBody::ActionAvailableMask(unsigned char id, bool on)
{
    if (on)
        m_action_available |= 1 << id;
    else
        m_action_available &= ~(1 << id);
}

/* a UI variable's value (SOCOM 1's _zrdr through CUIVarManager::Find: a type byte, a string for type
 * 3) and the variables (OVL1 bss 0x437E80) */
struct CUIVarValue {
    unsigned char m_type;   // 0x0
    unsigned char _1[3];
    char *m_string;   // 0x4

    const char *GetString() { return m_type == 3 ? m_string : 0; }
};

class CUIVarManager;
extern CUIVarManager theUIVarManager;

extern "C" {
/* CUIVarManager::Find (SOCOM 1's Find__13CUIVarManagerFPCc by this call; OVL1 0x385900, unowned) */
CUIVarValue *func_00385900(CUIVarManager *mgr, const char *name);
/* a flag the network player's making sets (OVL1 bss 0x44CE50; SOCOM 1's a small-data byte) and the
 * local team's mask (OVL1 bss 0x441470) */
extern unsigned char data_0044CE50;
extern unsigned int data_00441470;
}

bool ftsGetSquirm();
void ftsSetPlayer(CZSealBody *seal);

/* the local player made for a network game: named by the UI's variable or by the client index,
 * of the game's character, a hostage's squirming controller if so set, then made the player
 * (OVL2 0x5442C0; SOCOM 1's with the character read from the game state). Byte-exact since the
 * kit lane's zkit.h made CSealCtrlSquirm 0x280 bytes (`new` allocates 640). */
void CZSealBody::NetworkPlayerCreate()
{
    char name[31];

    data_0044CE50 = 1;
    CUIVarValue *var = func_00385900(&theUIVarManager, "PLAYERNAMEVAR");

    if (var)
        strncpy(name, var->GetString(), 31);
    else
        sprintf(name, "Player%d", theNetwork.zNetGetMyGameClientIndex());
    name[30] = 0;
    CCharacterType *type = CCharacterType::GetByNameMod(data_00437CE8->m_charname);
    CSealCtrl *ctrl;

    if (ftsGetSquirm())
        ctrl = new CSealCtrlSquirm;
    else
        ctrl = new CSealCtrl;
    CZSealBody *seal = Create(type, name, ctrl);

    ftsSetPlayer(seal);
    seal->m_node->SetName(name);
    seal->SetTeamMask(data_00441470);
    seal->m_vehicle_index = 0;
    seal->SetDisplayName(name);
}

/* the compass anchors the SEAL carries: swapped in, whether there was none returned (OVL2
 * 0x544C60, 0x544C40) */
bool CZSealBody::CompassAnchor_MPBomb(CCompassAnchor *anchor)
{
    bool none = !m_mpbomb_anchor;

    m_mpbomb_anchor = anchor;
    return none;
}

bool CZSealBody::CompassAnchor_MPBase(CCompassAnchor *anchor)
{
    bool none = !m_mpbase_anchor;

    m_mpbase_anchor = anchor;
    return none;
}

/* the pickups the controller has in reach (OVL2 0x544C30, 0x544C20) */
CPickup *CZSealBody::CurrentPickup()
{
    return CPickup::FindPickup(((CSealCtrl *)m_control)->m_curpickup_ID);
}

CPickup *CZSealBody::CurrentPickupSecondary()
{
    return CPickup::FindPickup(((CSealCtrl *)m_control)->m_curpickup_secondary_ID);
}

/* the new entity virtuals: a SEAL says no to them too (OVL2 0x543020, 0x543010, 0x543000) */
bool CZSealBody::Virtual68(CPnt3D *out)
{
    return false;
}

bool CZSealBody::Virtual60(CPnt3D *out)
{
    return false;
}

int CZSealBody::Virtual5C()
{
    return 0;
}

/* the SEAL's own AI event: an entity event that never expires, heard only close by, where the SEAL
 * stands, the SEAL its owner (OVL2 0x550E50; SOCOM 1's, which asked GetPosW for the place) */
void CZSealBody::CreateAiEvent()
{
    m_aiEvent = CAiEvent::Acquire();
    m_aiEvent->m_type = CAiEvent::AIE_ENTITY;
    m_aiEvent->m_duration = CAiEvent::NoExpire;
    m_aiEvent->m_rangeSq = 0.0001f;
    m_aiEvent->m_pos = m_posW;
    m_aiEvent->m_owner = m_id;
    CAiEvent::Create(m_aiEvent);
}

/* the heartbeat kept at the adrenaline's pace: three loops by level (none as 1), the one playing
 * swapped when the pace changes -- none when that loop is missing (SOCOM II's test) -- or started
 * when none plays (OVL2 0x5437D0; SOCOM 1's) */
void CZSealBody::TickHeartbeat()
{
    CAdrenaline *a = Adrenaline();
    float level = a ? a->m_level : 1.0f;
    int index;

    if (level < 0.33f)
        index = 0;
    else if (level < 0.66f)
        index = 1;
    else
        index = 2;
    if (m_heartbeat_sndisnt && m_heartbeat_curindex != index) {
        m_heartbeat_sndisnt->Stop();
        CSndInstance::ReleaseSoundInstance(m_heartbeat_sndisnt);
        m_heartbeat_sndisnt = 0;
        CSnd *snd = m_heartbeat_sound[index];

        if (!snd)
            m_heartbeat_sndisnt = 0;
        else
            m_heartbeat_sndisnt = snd->PlaySound((CSndInstance *)0, 255);
        m_heartbeat_curindex = index;
    } else if (!m_heartbeat_sndisnt) {
        CSnd *snd = m_heartbeat_sound[index];

        if (snd) {
            CSndInstance *inst = snd->PlaySound((CSndInstance *)0, 255);

            m_heartbeat_curindex = index;
            m_heartbeat_sndisnt = inst;
        }
    }
}

/* every heartbeat loop stopped: the playing one released, none current (OVL2 0x543A80; SOCOM 1's;
 * StopRumbler calls it) */
void CZSealBody::StopAllHeartbeatSounds()
{
    if (m_heartbeat_sndisnt) {
        m_heartbeat_sndisnt->Stop();
        CSndInstance::ReleaseSoundInstance(m_heartbeat_sndisnt);
        m_heartbeat_sndisnt = 0;
        m_heartbeat_curindex = -1;
    }
}

/* the loops let go: stopped, then the three forgotten (OVL2 0x543AD0; SOCOM 1's, StopAll inlined) */
void CZSealBody::CloseHeartbeatSounds()
{
    StopAllHeartbeatSounds();
    m_heartbeat_sound[0] = 0;
    m_heartbeat_sound[1] = 0;
    m_heartbeat_sound[2] = 0;
}

/* the three loops looked up by name (OVL2 0x543B40; SOCOM 1's) */
void CZSealBody::OpenHeartbeatSounds()
{
    m_heartbeat_sound[0] = CSnd::GetSoundByName("~HEART_SLOW");
    m_heartbeat_sound[1] = CSnd::GetSoundByName("~HEART_MED");
    m_heartbeat_sound[2] = CSnd::GetSoundByName("~HEART_MED_FAST");
}
