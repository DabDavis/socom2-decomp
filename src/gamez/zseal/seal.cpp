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
#include "../zweapon/zweapon.h"
#include <stdio.h>
#include <string.h>

class CValve;

extern "C" {
void func_00525200(void *path);   /* (a CAiPath reset: its member at 0x98's, tail-called) */
}

/* the world's camera (zdb::CWorld at 0x128; only that) */
struct CWorldCamera {
    unsigned char _0[0x128];
    zdb::CCamera *m_camera;   // 0x128
};

/* a halfword test: any of the bits (OVL2 0x5551A0; in the unit's tail, called where 0x54EA80 asks
 * it -- a template member's copy like 0x555170's, so pinned) */
#pragma auto_inline off
extern "C" bool func_005551A0(unsigned short flags, unsigned short bits)
{
    return (flags & bits) != 0;
}
#pragma auto_inline reset

/* an indexed array's current element, or none (OVL2 0x555170; in the unit's tail) */
struct CIndexedArray {
    void **m_data;   // 0x0
    unsigned char _4[4];
    int m_index;   // 0x8
};

/* (RTStack<CZBodyAnimBlend>::Top's copy by its words: an out-of-class template member, never inlined
 * into the unit's own functions -- 0x54F6E0 calls it -- so pinned) */
#pragma auto_inline off
extern "C" void *func_00555170(CIndexedArray *a)
{
    if (a->m_index < 0)
        return 0;
    return a->m_data[a->m_index];
}
#pragma auto_inline reset

/* the application's camera (camera.h's CAppCamera, OVL1 bss 0x415FF0: its first two virtuals, the
 * two flags and the word 0x545EE0 resets, its zdb camera and the SEAL it shows) */
class CAppCamera {
public:
    void SetZoom(float zoom);
    virtual void Tick(float dT);
    virtual bool AttachTo(CZSealBody *prey);
    /* the camera given an entity, or a SEAL, to follow (OVL1 0x2956E0 and its 8-byte jump 0x295AF0: the
     * engine core's round 23 named both; camera.cpp's AttachTo calls the first with (this, prey, 0)) */
    void Follow(CEntity *prey, const char *part);
    void Follow(CZSealBody *seal, const char *part);

    unsigned char _4[0x45 - 0x4];
    unsigned char m_45;   // 0x45 (camera.h's names)
    unsigned char m_46;   // 0x46
    unsigned char _47;
    int m_48;   // 0x48
    unsigned char _4c[0xb4 - 0x4c];
    zdb::CCamera *m_camera;   // 0xb4
    zdb::CNode *m_camera_effects;   // 0xb8
    CZSealBody *m_bc;   // 0xbc: the SEAL the camera shows
};

extern CAppCamera *appCamera;

/* the player (fts_player.cpp's CftsPlayer, OVL1 bss 0x440C38: its SEAL first) */
class CftsPlayer {
public:
    CZSealBody *m_body;   // 0x0
    int m_4;   // 0x4
};

extern CftsPlayer ftsPlayer;

extern "C" {
/* a SEAL a spectator may not watch online (OVL1 0x2C2FD0, unowned: hud_tcmcmds.cpp's reading) */
bool func_002C2FD0(CZSealBody *seal);
}

/* the SEAL's stats, CSealStats, are zseal.h's (CZSealBody::m_stats) */

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
 * 0x544CF0 -- SOCOM 1's Death, HeadShot and WasHit named by their callers, the round 56; the
 * suicide 0x545C90 and the team-mate's kill 0x545C10 are SOCOM 1's DeductKill split in two, the
 * rest new) */
void CSealStats::Death(const CZSealBody &seal)
{
    if (StatCounts(&seal))
        m_count[0x12 / 2]++;
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

void CSealStats::HeadShot(const CZSealBody &seal)
{
    if (StatCounts(&seal))
        m_count[0]++;
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

void CSealStats::WasHit(const CZSealBody &seal)
{
    if (StatCounts(&seal))
        m_count[0x10 / 2]++;
}

/* the hit percentage made again from the hits and the shots (the counters converted as unsigned
 * ints, the shots through a local) */
static inline void UpdateHitPercent(CSealStats *s)
{
    unsigned int shots = s->m_count[8 / 2];

    s->m_hit_percent = shots ? (float)(unsigned int)s->m_count[4 / 2] / (float)shots : 0.0f;
}

/* a hit: counted (a second count when the two share a team) and, when it counts, the hit
 * percentage made again (OVL2 0x5458A0; SOCOM 1's Hit, its callers SOCOM 1's -- the SEAL hit is new) */
void CSealStats::Hit(const CZSealBody &seal, const CZSealBody &other)
{
    m_count[4 / 2]++;
    if (other.m_team & seal.m_team)
        m_count[6 / 2]++;
    if (StatCounts(&seal))
        UpdateHitPercent(this);
}

/* a shot: counted, and offline a shot by someone off the player's team counted in the player's
 * stats; when it counts, the hit percentage made again (OVL2 0x544FC0; SOCOM 1's Shot by its shape:
 * a weapon's fire, 0x5BE9A0, calls it) */
void CSealStats::Shot(const CZSealBody &seal)
{
    m_count[8 / 2]++;
    if (!theNetwork.m_9) {
        /* (the player's team taken first: a local keeps its load ahead of the shooter's) */
        unsigned int team = ftsGetPlayer()->m_team;

        if (!(team & seal.m_team))
            ftsGetPlayer()->m_stats.m_count[0xa / 2]++;
    }
    if (StatCounts(&seal))
        UpdateHitPercent(this);
}

/* the entity as a SEAL, if it is one */
static inline CZSealBody *AsSeal(CEntity *e)
{
    return e && e->m_type == CEntity::ENT_TYPE_SEAL ? (CZSealBody *)e : 0;
}

/* an entity's team bits, by reference (an inline: the address is taken once, ahead of the walk) */
static inline const unsigned int &TeamOf(const CEntity *e)
{
    return e->m_team;
}

/* the sides' scores passed on, online: each SEAL's score summed into its side's (the two tests of
 * 0x2C31D0 and 0x2C30B0), and when asked a side's earnings stepped for each living SEAL of the
 * scorer's team on it; each side's valve set to its sum and its earnings (OVL2 0x544D60, new in
 * SOCOM II: the stats' events end on it; the list walked through a const view, as 0x545D80, and
 * the team test through TeamOf, the scorer's first: its address taken once, ahead of the walk) */
extern "C" void func_00544D60(const CZSealBody *seal, int a)
{
    if (theNetwork.m_9 && data_00437CE8->m_score1 && data_00437CE8->m_score2) {
        int score1 = 0;
        int score2 = 0;
        const CEntityList &list = CEntity::m_list;

        for (CEntityList::const_iterator i = list.begin(); i != list.end(); ++i) {
            CZSealBody *other = AsSeal(*i);

            if (func_002C31D0(other)) {
                score1 += other->m_stats.m_score;
                if (a && other->m_isAlive && (TeamOf(seal) & TeamOf(other)))
                    data_00437CE8->m_bonus1++;
            } else if (func_002C30B0(other)) {
                score2 += other->m_stats.m_score;
                if (a && other->m_isAlive && (TeamOf(seal) & TeamOf(other)))
                    data_00437CE8->m_bonus2++;
            }
        }
        score1 += data_00437CE8->m_bonus1;
        score2 += data_00437CE8->m_bonus2;
        data_00437CE8->m_score1->SetValue(score1);
        data_00437CE8->m_score2->SetValue(score2);
    }
}

/* the SEAL's stats (OVL2 0x545890; the stats events' member pointer) */
CSealStats *CZSealBody::GetStats()
{
    return &m_stats;
}

/* a kill: counted, the score up two and, for the local player in a squad whose order is 19, a
 * second counter; the game's kill valve stepped (OVL2 0x545290; the killer's credit, 0x59EE20, calls
 * it -- SOCOM 1's Kill, HandleKiller's enemy arm in both) */
void CSealStats::Kill(const CZSealBody &seal)
{
    if (StatCounts(&seal)) {
        m_count[0xc / 2]++;
        m_score += 2;
        if (&seal == ftsGetPlayer()) {
            CSealUnit *unit = ((CSealCtrl *)seal.m_control)->m_unit;

            if (unit && unit->m_9c->m_8 == 19)
                m_count[0xe / 2]++;
        }
        data_00437CE8->m_70->SetValue(data_00437CE8->m_70->m_value + 1);
    }
    func_00544D60(&seal, 0);
}

/* a SEAL's stats, and a stats event told to a SEAL (member pointers: 12 bytes, {0, -1, function}
 * for these) */
typedef CSealStats *(CZSealBody::*SEAL_STATS_GETTER)();
typedef void (CSealStats::*SEAL_STATS_EVENT)(const CZSealBody *seal, bool teammate);

extern "C" {
/* the death's end, once its anim has run (OVL2 0x5A0040, seal_env.cpp's range, not written yet) */
void func_005A0040(CZSealBody *seal);
/* the spectator's walks (below) */
void func_00547170(CZSealBody *seal, bool team);
void func_00546FA0(CZSealBody *seal, bool team);
}

/* bit 12 of the flags at 0x1060 set (an inline setter: the constant is masked and shifted in place,
 * `andi zero,1; sll 4`) */
static inline void SetDeathPending(CZSealBody *seal, bool b)
{
    seal->m_1060_bit12 = b;
}

/* the dead SEAL's tick (OVL2 0x547350, CZSealBody's own virtual at +0x8C, new in SOCOM II): a death
 * pending ended by 0x5A0040 and, unless remote, its network life set to 3 (IsRemote asked again,
 * the answer unused); online a respawn asked for brought about -- the node made inactive while it is
 * still drawn -- and, out of the menus, the spectator walked to the next or previous SEAL (of the
 * team unless the player may not watch); offline only the respawn */
void CZSealBody::Virtual8C()
{
    if (m_1060_bit12) {
        func_005A0040(this);
        SetDeathPending(this, false);
        if (!IsRemote() && m_netlife != 3) {
            m_netlife = NETLIFE_DEAD;
            IsRemote();
        }
    }
    if (theNetwork.m_9) {
        if (m_needs_respawn) {
            if (GetNode()->Rendered(2))
                GetNode()->SetActive(false);
            else
                RemoteRespawn(true);
        }
        if (!((CSealCtrl *)m_control)->m_menustate && (m_spectate_next || m_spectate_prev)) {
            bool team = true;

            if (theNetwork.m_9 && func_002C2FD0(ftsGetPlayer()))
                team = false;
            if (m_spectate_next)
                func_00547170(this, team);
            else if (m_spectate_prev)
                func_00546FA0(this, team);
        }
    } else if (m_needs_respawn) {
        if (GetNode()->Rendered(2))
            GetNode()->SetActive(false);
        else
            DoRespawn();
    }
}

/* the spectator's camera moved to the SEAL after the one it shows in the entity list, round to the
 * start and once round at most, by 0x546FA0's tests (OVL2 0x547170; new in SOCOM II, its name
 * unknown -- n declared ahead of the SEALs here, after them there: the two number them so) */
extern "C" void func_00547170(CZSealBody *seal, bool team)
{
    int i = CEntity::GetEntityIndex(appCamera->m_bc);
    int start = i;
    unsigned int n = (unsigned int)-1;
    CZSealBody *s = ftsPlayer.m_body;
    CZSealBody *player = s;

    i++;
    while (i != start) {
        n++;
        s = (CZSealBody *)CEntity::m_list.GetEntityByIndex(i);
        if (!s) {
            s = ftsPlayer.m_body;
            i = 0;
        } else {
            s = AsSeal(s);
            if (s && theNetwork.m_9 && func_002C2FD0(s))
                s = 0;
            if (s && (s->m_isAlive || player == s) && s->m_type == CEntity::ENT_TYPE_SEAL &&
                (!team || ((TeamOf(seal) & TeamOf(s)) && !(TeamOf(s) & 0x20000))))
                break;
            if (++i >= CEntity::m_list.size()) {
                i = 0;
                s = ftsPlayer.m_body;
            }
        }
        if (CEntity::m_list.size() < n)
            i = start;
    }
    if (i != start) {
        appCamera->AttachTo(s);
        s->m_kit.SetWeaponInfoChanged();
    }
}

/* the spectator's camera moved to the SEAL before the one it shows in the entity list, round to the
 * end and once round at most: a living SEAL, or the player, that may be watched -- of the SEAL's
 * team when asked, and not one of team bit 17's (OVL2 0x546FA0; new in SOCOM II, its name unknown) */
extern "C" void func_00546FA0(CZSealBody *seal, bool team)
{
    int i = CEntity::GetEntityIndex(appCamera->m_bc);
    int start = i;
    CZSealBody *s = ftsPlayer.m_body;
    CZSealBody *player = s;
    unsigned int n = (unsigned int)-1;

    i--;
    while (i != start) {
        n++;
        if (i < 0)
            i = CEntity::m_list.size() - 1;
        s = (CZSealBody *)CEntity::m_list.GetEntityByIndex(i);
        if (!s) {
            s = ftsPlayer.m_body;
            i--;
        } else {
            s = AsSeal(s);
            if (s && theNetwork.m_9 && func_002C2FD0(s))
                s = 0;
            if (s && (s->m_isAlive || player == s) && s->m_type == CEntity::ENT_TYPE_SEAL &&
                (!team || ((TeamOf(seal) & TeamOf(s)) && !(TeamOf(s) & 0x20000))))
                break;
            i--;
        }
        if (CEntity::m_list.size() < n)
            i = start;
    }
    if (i != start) {
        appCamera->AttachTo(s);
        s->m_kit.SetWeaponInfoChanged();
    }
}

/* the stats' event handed to the SEAL's teammates: each living SEAL of its team but itself has its
 * stats (the first member pointer) given the event (the second, called with true) -- OVL2 0x545D80,
 * new in SOCOM II (the stats themselves unused). Out of line: both callers call it. The list walked
 * through a const view, which keeps the target's iterator temporaries in the frame */
#pragma auto_inline off
extern "C" void func_00545D80(CSealStats *s, const CZSealBody *seal, SEAL_STATS_GETTER stats,
                              SEAL_STATS_EVENT event)
{
    const CEntityList &list = CEntity::m_list;

    for (CEntityList::const_iterator i = list.begin(); i != list.end(); ++i) {
        CZSealBody *other = AsSeal(*i);

        if (other && seal != other && other->m_isAlive && (other->m_team & seal->m_team))
            ((other->*stats)()->*event)(other, true);
    }
}
#pragma auto_inline reset

extern "C" {
/* the two member-pointer constants (OVL2 data): CZSealBody's stats (0x545890) and this event
 * (0x545650) */
extern const SEAL_STATS_GETTER data_0064FD58;
extern const SEAL_STATS_EVENT data_0064FD68;
}

/* a bomb's event (CZBombState::ChangeBombState calls it): for the SEAL itself, the score up two when
 * it counts, the score passed on, a counter, and online in game mode 1 the teammates told; for a
 * teammate told, only their counter (OVL2 0x545650) */
extern "C" void func_00545650(CSealStats *s, const CZSealBody *seal, bool teammate)
{
    if (!teammate) {
        bool mode1 = data_00437CE8->m_mode == 1;

        if (StatCounts(seal))
            s->m_score += 2;
        func_00544D60(seal, 1);
        s->m_count[0x36 / 2]++;
        if (theNetwork.m_9 && mode1) {
            s->m_count[0x32 / 2]++;
            func_00545D80(s, seal, data_0064FD58, data_0064FD68);
        }
    } else
        s->m_count[0x32 / 2]++;
}

extern "C" {
/* this event's member-pointer constants (OVL2 data): CZSealBody's stats (0x545890) and 0x545750 */
extern const SEAL_STATS_GETTER data_0064FD38;
extern const SEAL_STATS_EVENT data_0064FD48;
}

/* the other bomb event (ChangeBombState calls it): the score up four in game mode 1, else two, when
 * it counts; the score passed on, a counter, and online in modes 1 and 2 the teammates told; for a
 * teammate told, only their counter (OVL2 0x545750) */
extern "C" void func_00545750(CSealStats *s, const CZSealBody *seal, bool teammate)
{
    if (!teammate) {
        bool mode1 = false;
        bool mode2 = false;

        if (data_00437CE8) {
            mode1 = data_00437CE8->m_mode == 1;
            mode2 = data_00437CE8->m_mode == 2;
        }
        if (StatCounts(seal)) {
            if (mode1)
                s->m_score += 4;
            else
                s->m_score += 2;
        }
        func_00544D60(seal, 1);
        s->m_count[0x1a / 2]++;
        if (theNetwork.m_9 && (mode1 || mode2)) {
            s->m_count[0x32 / 2]++;
            func_00545D80(s, seal, data_0064FD38, data_0064FD48);
        }
    } else
        s->m_count[0x32 / 2]++;
}

/* the stats zeroed (SOCOM 1's Reset, between IncrTimesSeen and Kill there too; OVL2 0x545200, its
 * draft's stores in its order: the halfwords by offset, the hit percentage, then the MVP score and the
 * counters around it) */
void CSealStats::Reset()
{
    m_count[0x0 / 2] = 0;
    m_count[0x2 / 2] = 0;
    m_count[0x4 / 2] = 0;
    m_count[0x6 / 2] = 0;
    m_count[0x8 / 2] = 0;
    m_count[0xa / 2] = 0;
    m_count[0xc / 2] = 0;
    m_count[0xe / 2] = 0;
    m_count[0x14 / 2] = 0;
    m_count[0x10 / 2] = 0;
    m_count[0x12 / 2] = 0;
    m_count[0x16 / 2] = 0;
    m_count[0x18 / 2] = 0;
    m_count[0x1e / 2] = 0;
    m_count[0x20 / 2] = 0;
    m_count[0x22 / 2] = 0;
    m_count[0x24 / 2] = 0;
    m_count[0x2c / 2] = 0;
    m_count[0x26 / 2] = 0;
    m_count[0x28 / 2] = 0;
    m_count[0x2a / 2] = 0;
    m_count[0x2e / 2] = 0;
    m_count[0x30 / 2] = 0;
    m_count[0x32 / 2] = 0;
    m_count[0x34 / 2] = 0;
    m_count[0x1c / 2] = 0;
    m_count[0x1a / 2] = 0;
    m_hit_percent = 0.0f;
    m_score = 0;
    m_count2[0] = 0;
    m_count2[1] = 0;
    m_count2[2] = 0;
    m_count[0x36 / 2] = 0;
    m_count[0x38 / 2] = 0;
    m_count[0x3a / 2] = 0;
}

/* eight counters each added to (OVL2 0x5451E0, 0x5451C0, 0x5451A0, 0x545180, 0x545160, 0x545140,
 * 0x545120, 0x545100; SOCOM 1's Incr* took ints -- its four are the first and the last three, by their
 * callers: the sightings, then a grenade, a primary's and a secondary's rounds; the four between them
 * new: HandleKiller's two AI tallies and CAiSHostage::Tick's two) */
void CSealStats::IncrTimesSeen(int n)
{
    m_count[0x1e / 2] += (unsigned short)n;
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

void CSealStats::IncrGrenadesUsed(int n)
{
    m_count[0x2a / 2] += (unsigned short)n;
}

void CSealStats::IncrPrimaryRoundsFired(int n)
{
    m_count[0x2e / 2] += (unsigned short)n;
}

void CSealStats::IncrSecondaryRoundsFired(int n)
{
    m_count[0x30 / 2] += (unsigned short)n;
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
    grenade->SetVelW(&vel);
}

/* the AI's aiming helper, CAi, is ai.h's */

extern "C" {
/* (CValve::AddCallback, SOCOM 1's, OVL1 0x351960, is zanim.h's since the game states' round 35)
 * the other kinds' takers: a weapon, ammunition (OVL2 0x59F610, 0x59F6F0, seal_env.cpp's, matched
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

                valve->AddCallback(1, _rethrow_grenade, pickup);
                ((CZProjectile *)pickup->m_data)->SetVelW(0);
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
        m_snakePath.SetSnakeLength(length);
    } else {
        m_snake_path = false;
        m_snakePath.SetSnakeLength(0);
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
    func_00525200(&m_snakePath);
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
    func_00525200(&m_snakePath);
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
    m_kit.PostSetupItems();
    ((CSealCtrl *)m_control)->OnMissionStart();
}

/* the mission's end: the SEAL's network pickups destroyed (SOCOM 1's tested m_id first), then
 * the controller's (OVL2 0x553E20) */
void CZSealBody::OnMissionEnd()
{
    CPickup::DestroyNetPickups(m_id);
    ((CSealCtrl *)m_control)->OnMissionEnd();
}

/* a weapon's node made, kept for freeing and named, parented by the item: the rifle in hand to the
 * right hand, slung to the back; the pistol to the hand or the hips; a throwable to the hand; its part
 * the item's (OVL2 0x553290; SOCOM 1's AddWeapon, the constructor calls it twice in both builds --
 * SOCOM II keeps the parts in m_item_gear) */
bool CZSealBody::AddWeapon(SEAL_ITEM_BYTE item)
{
    CZBodyPart *parent = 0;
    zdb::CNode *node = new zdb::CNode;

    m_deallocList.push_back(node);
    switch (item) {
    case ITEM_RIFLE:
        if (m_item == item)
            parent = m_rhand;
        else
            parent = m_spinelo;
        node->SetName("rifle");
        break;
    case ITEM_PISTOL:
        if (m_item == item)
            parent = m_rhand;
        else
            parent = m_hips;
        node->SetName("pistol");
        break;
    case ITEM_THROWABLE:
        parent = m_rhand;
        node->SetName("grenade");
        break;
    }
    /* (CBody::AddBodyPart, OVL1 0x28EBE0, returns the part it made: zbody.h declares it so since the
     * engine core's round 3) */
    return (m_item_gear[item].m_part = AddBodyPart(node, parent)) != 0;
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
/* the victim struck down (OVL2 0x547860, seal.cpp's) -- not written yet; the killer's credit is
 * CZSealBody::HandleKiller (OVL2 0x59EE20, seal_env.cpp's) */
void func_00547860(CZSealBody *victim);
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
            victim->HandleKiller(id, false);
        }
        CZAnim *anim = ZAnim.GetAnim("ai_cqb_attack", 0);
        CZAnim *act = anim ? anim->ActivateEx(2, m_select, victim->m_select, 0) : 0;

        if (act) {
            if (m_animset != data_0044D400) {
                m_stats.m_count[0x2c / 2]++;
                m_stats.m_count[0x24 / 2] += 2;
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

/* the action the HUD shows selected: its bit alone in the selection mask (OVL2 0x544C80; SOCOM 1's
 * ActionSelectionMask(unsigned int, bool) cleared or added a bit, SOCOM II's takes the action alone
 * -- hud_actionbitmap.cpp's call, the HUD's round 39). An int narrowed to a byte inside (the HUD's
 * round 42: the caller loads the byte with `lb`, which an `unsigned char` parameter folds into `lbu`
 * in every spelling; the int gives the caller and these five instructions both) */
void CZSealBody::ActionSelectionMask(int id)
{
    m_action_selection = 1 << (unsigned char)id;
}

/* a UI variable's value (SOCOM 1's _zrdr through CUIVarManager::Find: a type byte, a string for type
 * 3) and the variables (OVL1 bss 0x437E80) */
struct CUIVarValue {
    unsigned char m_type;   // 0x0
    unsigned char _1[3];
    char *m_string;   // 0x4

    const char *GetString() { return m_type == 3 ? m_string : 0; }
};

/* the UI variables (zui.h's CUIVarManager, OVL1 bss 0x437E80): only Find (SOCOM 1's Find__13CUIVarManager
 * FPCc by this call; OVL1 0x385900) */
class CUIVarManager {
public:
    CUIVarValue *Find(const char *name);
};

extern CUIVarManager theUIVarManager;

extern "C" {
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
    CUIVarValue *var = theUIVarManager.Find("PLAYERNAMEVAR");

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

/* a player's targets forgotten as seen: each one's bit 9 cleared and its visibility zeroed (OVL2
 * 0x544B60, new in SOCOM II: SetCurZoomState calls it twice -- out of line, so pinned) */
#pragma auto_inline off
void CZSealBody::ResetTargetsSeen()
{
    if (IsPlayer()) {
        /* (the list through a local: the loop re-reads its end through it, as the target's does) */
        CTargetList *list = &m_targets;

        for (CTargetList::iterator i = list->begin(); i != list->end(); ++i) {
            i->m_flags &= ~0x200;
            i->m_visibility = 0.0f;
        }
    }
}
#pragma auto_inline reset

/* the mission's view preferences by weapon class (theMission, OVL1 bss 0x4364E0: night vision
 * wanted or not, when set, for the primaries and the sidearms; only these here, the names ours) */
struct CZoomPref {
    unsigned char m_set;   // 0x0
    int m_nvg;   // 0x4
};

class CMission {
public:
    void RefreshTeamValves();
    unsigned char _0[0x194];
    CZoomPref m_primary_zoom;   // 0x194
    CZoomPref m_secondary_zoom;   // 0x19c
};

extern CMission theMission;

extern "C" {
/* a ZAnim let go (OVL1 0x2707C0, unowned: seal_ai.cpp's reading) */
void func_002707C0(CZAnim *anim, bool now);
/* the night-vision goggles' sounds (OVL1 bss 0x44CE30, 0x44CE38: seal_env.cpp's
 * ReInitDatabaseHandles looks up ".NV_GOGGLES_ON" and ".NV_GOGGLES_OFF" into them) */
extern CSnd *data_0044CE30;
extern CSnd *data_0044CE38;
}

/* the view set (OVL2 0x5448A0; SOCOM 1's with SOCOM II's states: the goggles' sound in or out of
 * night vision where SOCOM 1 reset the power, the screen offsets zeroed off the first person, the
 * zoom's anim held while the binoculars or a weapon's zoom are up) */
void CZSealBody::SetCurZoomState(ZOOMSTATE zs)
{
    if (zs == m_zoomstate)
        return;
    m_zoomstate_prev = m_zoomstate;
    m_zoomstate = zs;
    m_zoomstate_echo = 1;
    if (m_zoomstate_prev == ZOOMSTATE_NVG) {
        if (data_0044CE38)
            data_0044CE38->PlaySound((CSndInstance *)0, 255);
    } else if (m_zoomstate == ZOOMSTATE_NVG) {
        if (data_0044CE30)
            data_0044CE30->PlaySound((CSndInstance *)0, 255);
    }
    if (IsPlayer())
        m_iszooming = true;
    if (m_zoomstate != ZOOMSTATE_1STPERSON) {
        appCamera->m_camera->SetScreenOffsetX(0.0f);
        appCamera->m_camera->SetScreenOffsetY(0.0f);
    }
    switch (m_zoomstate) {
    case ZOOMSTATE_3RDPERSON:
        m_zoom_range = 1.0f;
        if (m_zoom_anim) {
            func_002707C0(m_zoom_anim, false);
            m_zoom_anim = 0;
        }
        break;
    case ZOOMSTATE_1STPERSON:
    case ZOOMSTATE_THERMAL:
        m_zoom_range = 1.01f;
        if (m_zoom_anim) {
            func_002707C0(m_zoom_anim, false);
            m_zoom_anim = 0;
        }
        break;
    case ZOOMSTATE_NVG:
        m_zoom_range = 1.01f;
        ResetTargetsSeen();
        if (m_zoom_anim) {
            func_002707C0(m_zoom_anim, false);
            m_zoom_anim = 0;
        }
        break;
    case ZOOMSTATE_BINOCS:
        m_zoom_range = 9.0f;
        ResetTargetsSeen();
        if (!m_zoom_anim) {
            m_zoom_anim = ZAnim.GetAnim("zoom_control", 0);
            if (m_zoom_anim)
                m_zoom_anim->Activate(0);
        }
        m_kit.InitializeSniperKick();
        break;
    case ZOOMSTATE_WEAPONDEF1:
    case ZOOMSTATE_WEAPONDEF2:
    case ZOOMSTATE_WEAPONDEF3:
    case ZOOMSTATE_WEAPONDEF4:
    case ZOOMSTATE_WEAPONDEF5:
    case ZOOMSTATE_WEAPONDEF6:
    case ZOOMSTATE_WEAPONDEF7:
    case ZOOMSTATE_WEAPONDEF8: {
        CZFTSWeapon *w = m_kit.GetCurItem();

        if (w && m_zoomstate - 4 < w->m_zoommode.size())
            m_zoom_range = w->GetZoomRange(m_zoomstate - 4);
        if (m_zoomstate_prev != ZOOMSTATE_WEAPONDEF2)
            m_kit.InitializeSniperKick();
        if (!m_zoom_anim) {
            m_zoom_anim = ZAnim.GetAnim("zoom_control", 0);
            if (m_zoom_anim)
                m_zoom_anim->Activate(0);
        }
        break;
    }
    }
}

/* the view stepped out one place (OVL2 0x544400; SOCOM 1's switch with SOCOM II's states and its
 * view preference: out of the first weapon zoom, night vision when the mission's preference for the
 * class in hand says so, else when the mission is at night) */
ZOOMSTATE CZSealBody::DecrZoomState()
{
    ZOOMSTATE zs = m_zoomstate;
    bool pref = false;
    bool nvg = false;
    CZFTSWeapon *item = m_kit.GetCurItem();

    if (theMission.m_primary_zoom.m_set && CZWeapon::IsPrimary(item->m_ID)) {
        pref = true;
        nvg = theMission.m_primary_zoom.m_nvg;
    } else if (theMission.m_secondary_zoom.m_set && CZWeapon::IsSecondary(item->m_ID)) {
        pref = true;
        nvg = theMission.m_secondary_zoom.m_nvg;
    }
    switch (m_zoomstate) {
    case ZOOMSTATE_1STPERSON:
        zs = ZOOMSTATE_3RDPERSON;
        break;
    case ZOOMSTATE_THERMAL:
        zs = ZOOMSTATE_3RDPERSON;
        break;
    case ZOOMSTATE_NVG:
        zs = ZOOMSTATE_1STPERSON;
        break;
    case ZOOMSTATE_BINOCS:
        zs = ZOOMSTATE_1STPERSON;
        m_kit.ResetRetOffsetXY();
        break;
    case ZOOMSTATE_WEAPONDEF1:
    case ZOOMSTATE_WEAPONDEF2:
    case ZOOMSTATE_WEAPONDEF3:
    case ZOOMSTATE_WEAPONDEF4:
    case ZOOMSTATE_WEAPONDEF5:
    case ZOOMSTATE_WEAPONDEF6:
    case ZOOMSTATE_WEAPONDEF7:
    case ZOOMSTATE_WEAPONDEF8: {
        CZFTSWeapon *w = m_kit.GetCurItem();

        if (w) {
            int level = m_zoomstate - 4;

            if (level - 1 <= 0) {
                if ((!pref && zdb::CWorld::m_world->m_night_mission) || nvg)
                    zs = ZOOMSTATE_NVG;
                else
                    zs = ZOOMSTATE_1STPERSON;
                m_kit.ResetRetOffsetXY();
            } else if (level - 1 < (int)w->m_zoommode.size())
                zs = (ZOOMSTATE)(m_zoomstate - 1);
        }
        break;
    }
    }
    SetCurZoomState(zs);
    return zs;
}

/* a turret (zveh.h's CTurret, whose header clashes with zseal.h's stand-ins: only its thermal look,
 * at zveh.h's offset, and the six nodes' opacity and visibility -- SetOpacity and SetVisible, OVL1
 * 0x3A9FB0 and 0x3AA020, the weapons' veh_turret.cpp) */
class CTurret : public CEntity {
public:
    virtual ~CTurret();
    void SetThermal(zdb::CNode *node, bool thermal);
    void SetOpacity(float f);
    void SetVisible(bool on);

    unsigned char _170[0x298 - 0x170];
    bool m_thermal;   // 0x298
    unsigned char _299[0x2a4 - 0x299];
    /* zveh.h's: the ammunition, the rounds 0x545EE0 hands back from the gunner's kit and the weapon
     * 0x5463C0 hands the gunner */
    void *m_ammoType;   // 0x2a4
    int m_ammo;   // 0x2a8
    class CZWeapon *m_weapon;   // 0x2ac
};


/* the view stepped in one place (OVL2 0x5445B0; SOCOM 1's switch with SOCOM II's states: out of the
 * third person a turret's thermal view (2) when it has one; out of the first person, nothing while a
 * turret or a launcher is worked, else night vision as DecrZoomState picks it, else the weapon's
 * first zoom or the binoculars) */
ZOOMSTATE CZSealBody::IncrZoomState()
{
    ZOOMSTATE zs = m_zoomstate;
    bool pref = false;
    bool nvg = false;
    CZFTSWeapon *item = m_kit.GetCurItem();

    if (theMission.m_primary_zoom.m_set && CZWeapon::IsPrimary(item->m_ID)) {
        pref = true;
        nvg = theMission.m_primary_zoom.m_nvg;
    } else if (theMission.m_secondary_zoom.m_set && CZWeapon::IsSecondary(item->m_ID)) {
        pref = true;
        nvg = theMission.m_secondary_zoom.m_nvg;
    }
    CZFTSWeapon *w = m_kit.GetCurItem();

    switch (m_zoomstate) {
    case ZOOMSTATE_3RDPERSON:
        if (TurretHasThermal()) {
            zs = ZOOMSTATE_THERMAL;
            GetTurret()->SetVisible(false);
        } else
            zs = ZOOMSTATE_1STPERSON;
        break;
    case ZOOMSTATE_1STPERSON:
        if (m_eb8)
            break;
        if (item && CZWeapon::IsLauncher(item->m_ID))
            break;
        if (m_kit.CurWeaponIsLauncherMode())
            break;
        if ((!pref && zdb::CWorld::m_world->m_night_mission) || nvg)
            zs = ZOOMSTATE_NVG;
        else if (w && w->m_zoommode.size() > 1)
            zs = ZOOMSTATE_WEAPONDEF1;
        else
            zs = ZOOMSTATE_BINOCS;
        break;
    case ZOOMSTATE_NVG:
    case ZOOMSTATE_BINOCS:
        if (w && w->m_zoommode.size() > 1)
            zs = ZOOMSTATE_WEAPONDEF1;
        break;
    case ZOOMSTATE_WEAPONDEF1:
    case ZOOMSTATE_WEAPONDEF2:
    case ZOOMSTATE_WEAPONDEF3:
    case ZOOMSTATE_WEAPONDEF4:
    case ZOOMSTATE_WEAPONDEF5:
    case ZOOMSTATE_WEAPONDEF6:
    case ZOOMSTATE_WEAPONDEF7:
        if (w) {
            /* (unsigned: an int folds the two steps into one `addiu -3`, as the target does not) */
            unsigned int level = m_zoomstate - 4;

            if (level + 1 < w->m_zoommode.size())
                zs = (ZOOMSTATE)(m_zoomstate + 1);
        }
        break;
    case ZOOMSTATE_WEAPONDEF8:
        break;
    }
    SetCurZoomState(zs);
    return zs;
}

/* the life the network reports set: a remote SEAL coming back up (1) from down (2 or 3) is marked
 * for its respawn (OVL2 0x544210, new in SOCOM II) */
void CZSealBody::SetNetLife(unsigned char life)
{
    if (life != m_netlife) {
        unsigned char respawn = 0;
        bool down = m_netlife == 2 || m_netlife == 3;

        if (down && life == 1)
            respawn = 1;
        m_netlife = (NET_LIFE)life;
        if (IsRemote() && respawn)
            m_needs_respawn = 1;
    }
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

bool CZSealBody::Virtual5C(CZSealBody *seal, CTurret *turret)
{
    return false;
}

extern "C" {
/* (OVL2 0x588080, 0x579EA0, 0x5857E0, 0x584C10: seal_body.cpp's and seal_anim.cpp's tests) */
bool func_00588080(CZSealBody *seal);
bool func_00579EA0(CZSealBody *seal);
bool func_005857E0(CZSealBody *seal);
bool func_00584C10(CZSealBody *seal);
bool func_0054D620(CZSealBody *seal, float back, float ahead, float rise);
void func_005B0BB0(CZSealBody *seal, const zdb::IntersectStruct *hit);
}

bool OKToStand(CZSealBody *seal, SEAL_STATE stance);

static inline float Len3(float x, float y, float z)
{
    return sqrtf(x * x + y * y + z * z);
}

extern "C" {
/* the carried body let go (OVL2 0x545EE0, this unit's), the ladder let go (0x5B14E0, seal_env.cpp's) */
void func_00545EE0(CZSealBody *seal, int x);
void func_005B14E0(CZSealBody::CSealClimb *climb, CZSealBody *seal);
void func_005781C0(CZSealBody *seal);
void func_0059D750(CZSealBody *seal);
extern unsigned char data_0045A0C1;
extern int data_003DF148;
extern int data_003DF150;
extern int data_003DF158;
extern int data_003DF160;
/* (the clutter's and the world's, by their mangled names: zdb::CClutterAnimManager::UnRegisterEffector, OVL1
 * 0x2D7EA0; zdb::CWorld::DeleteTextureAssetCharacter(char *, CNode *), OVL1 0x31D130) */
void UnRegisterEffector__Q23zdb19CClutterAnimManagerFPCv(void *mgr, const void *effector);
void DeleteTextureAssetCharacter__Q23zdb6CWorldFPcPQ23zdb5CNode(void *world, const char *lib, zdb::CNode *node);
}

/* the view pulled back and the zoom's lens (OVL1 0x1F1470, 0x1EFEA0: st_core.cpp's) */
class CameraZoom {
public:
    static void ResetLensColors();
    static void SetLensFx(unsigned char fx);
};
void SeeFarther(bool bEnable, float SeeFartherFactor);

/* a SEAL's network state (SOCOM 1's CZSealState: OVL1 0x2BD380, 0x2BEF20) */
class CZSealState {
public:
    static void RemoveNetSeal(int id, bool player);
    static void SealDeleteCleanup(int id);
};

extern "C" void Clear__10CZBodyPartFv(CZBodyPart *part);

/* each node deleted (an inline, the name ours: its iterators by value in the frame, end first) */
static inline void DeleteAll(std::vector<zdb::CNode *>::iterator first, std::vector<zdb::CNode *>::iterator last)
{
    for (; first != last; ++first)
        zdb::CNode::Delete(*first);
}

/* a part deleted: its Clear, zbody's in-class inline, is the real function OVL1 0x28EF30 here (the target
 * calls it, OVL2 has no copy) */
static inline void DeletePart(CZBodyPart *part)
{
    if (part) {
        Clear__10CZBodyPartFv(part);
        operator delete(part);
    }
}

/* a flag through a bool (an inline, the name ours: false as `andi v0,zero,1`) */
static inline void SetZooming(CZSealBody *seal, bool on) { seal->m_iszooming = on; }

/* the SEAL taken out of the world: what it carries and holds let go, a player's view put back, its network
 * entry and controller gone, its blends popped, its kit and anchors freed, its node, gear and parts deleted
 * (OVL2 0x5533E0, slot 0x08 of its table; SOCOM 1's ~CZSealBody grown) */
CZSealBody::~CZSealBody()
{
    if (m_eb8)
        func_00545EE0(this, 1);
    CBlendOverlay &ov = m_overlay;

    if (ov.m_anim && ov.m_anim->m_0) {
        m_active_anim_pool.PutBack(ov.m_anim);
        ov.Clear();
    }
    func_005781C0(this);
    if (m_carry) {
        m_carry->m_carry = 0;
        m_carry = 0;
    }
    func_0059D750(this);
    func_005B14E0(&m_climb, this);
    m_shock.m_1314 = CPnt3D::zero;
    m_shock.m_1320 = 0.0f;
    m_shock.m_1324 = 0.0f;
    if (m_control ? ((CSealCtrl *)m_control)->IsPlayer() : false) {
        CSealCtrl *ctrl = (CSealCtrl *)m_control;

        if (ctrl) {
            m_kit.Apply_LensEffects(false, 0);
            m_kit.NVG_Enabled(false);
            m_kit.Binocs_Enabled(false);
            SeeFarther(false, 0.0f);
            CameraZoom::SetLensFx(1);
            StopAllHeartbeatSounds();
            ctrl->StopRumbler();
            CZWeapon::ClearTimer(0);
            SetZooming(this, false);
            SetCurZoomState(ZOOMSTATE_3RDPERSON);
            appCamera->SetZoom(1.0f);
        }
    }
    if (ftsPlayer.m_body == this)
        ftsPlayer.m_body = 0;
    m_zoomstate = ZOOMSTATE_3RDPERSON;
    m_zoomstate_prev = ZOOMSTATE_UNKNOWN;
    m_zoomstate_echo = 0;
    m_zoom_range = 1.0f;
    if (!m_respawning) {
        if (data_0045A0C1) {
            if (!(m_control ? ((CSealCtrl *)m_control)->IsRemote() : false)) {
                CZSealState::RemoveNetSeal(m_id, m_control ? ((CSealCtrl *)m_control)->IsPlayer() : false);
                m_id = CEntity::deletedId;
            } else
                CZSealState::SealDeleteCleanup(m_id);
        }
        delete m_control;
        m_control = 0;
    }
    UnRegisterEffector__Q23zdb19CClutterAnimManagerFPCv(zdb::CWorld::m_world->m_grid.m_ClutterAnimManager, this);
    CBodyAnimBlendStack *stack = &m_blends;

    while (stack->m_top + 1 > 0)
        CBody::PopBlend();
    m_kit.UnInit();
    delete m_adrenaline;
    m_adrenaline = 0;
    CCompass::DeleteAnchor(m_mpbomb_anchor);
    m_mpbomb_anchor = 0;
    CCompass::DeleteAnchor(m_mpbase_anchor);
    m_mpbase_anchor = 0;
    CCompass::DeleteAnchor(m_compass_anchor);
    m_compass_anchor = 0;
    CCompass::DeleteAnchor(m_hostile_anchor);
    m_hostile_anchor = 0;
    zdb::CNode *parent = m_node->m_parent;

    zdb::CWorld::m_world->m_grid.gridRemoveNodeFromGrids(m_node);
    DeleteTextureAssetCharacter__Q23zdb6CWorldFPcPQ23zdb5CNode(zdb::CWorld::m_world, "clib", m_node);
    if (zdb::CAssetMgr::m_assets.FindLib("flib"))
        DeleteTextureAssetCharacter__Q23zdb6CWorldFPcPQ23zdb5CNode(zdb::CWorld::m_world, "flib", m_node);
    if (CZWeapon::m_pProjectilelist)
        CZWeapon::m_pProjectilelist->ForgetHitNode(m_node);
    parent->DeleteChild(m_node);
    DeleteCameraEffectsNode();
    if (m_node->m_model)
        m_node->m_model->Release(m_node);
    delete m_node;
    m_node = 0;
    DeletePart(m_reyeball);
    m_reyeball = 0;
    DeletePart(m_reyelid);
    m_reyelid = 0;
    DeletePart(m_leyeball);
    m_leyeball = 0;
    DeletePart(m_leyelid);
    m_leyelid = 0;
    for (unsigned int i = 0; i < 5; i++)
        DeletePart(m_item_gear[i].m_part);
    m_rifle = 0;
    m_pistol = 0;
    DeletePart(m_weapon);
    m_weapon = 0;
    SetBodyPart(data_003DF148, 0);
    SetBodyPart(data_003DF150, 0);
    SetBodyPart(data_003DF158, 0);
    SetBodyPart(data_003DF160, 0);
    int id = 0;
    std::vector<CZBodyPart *> &parts = m_body_parts;

    for (std::vector<CZBodyPart *>::iterator it = parts.begin(); it != parts.end(); ++it, ++id)
        if (*it && (unsigned int)(*it)->m_id >= siNumBodyObjects)
            SetBodyPart(id, 0);
    m_root = 0;
    m_lfoot = 0;
    m_rfoot = 0;
    m_lhand = 0;
    m_spinelo = 0;
    m_rhand = 0;
    m_hips = 0;
    m_head = 0;
    m_neck = 0;
    m_spinehi = 0;
    m_lthigh = 0;
    m_rthigh = 0;
    m_rcalf = 0;
    m_rbicep = 0;
    m_rforearm = 0;
    m_lbicep = 0;
    m_lforearm = 0;
    m_lscap = 0;
    m_rscap = 0;
    m_lshoulder_wgt = 0;
    m_rshoulder_wgt = 0;
    m_lcalf = 0;
    m_aimnodes = 0;
    m_ltoe = 0;
    m_rtoe = 0;
    delete m_projectiledi;
    memset(m_item_gear, 0, sizeof(m_item_gear));
    m_node = 0;
    m_charType->Release();
    if (m_select)
        _free(m_select, 0, 0);
    m_select = 0;
    CAiEvent::Release(m_aiEvent);
    m_aiEvent = 0;
    DeleteAll(m_deallocList.begin(), m_deallocList.end());
    m_deallocList.clear();
    m_538 = 0;
}

/* whether the SEAL may take a state now: not out of a blend that holds it, not in state 7; to stand or
 * crouch a SEAL prone or crouched must have room over it (OKToStand) -- a remote one always -- its asked
 * state put back if not; to crouch or go prone a SEAL standing also clear before and behind for its speed
 * (0x54D620) (OVL2 0x552EC0, new in SOCOM II: UpdateStance asks it with the desired state; the name
 * unknown) */
extern "C" bool func_00552EC0(CZSealBody *seal, SEAL_STATE state)
{
    if (!func_005551A0(((CZBodyAnimBlend *)func_00555170((CIndexedArray *)&seal->m_blends))->m_flags, 0x40) &&
        !func_00588080(seal))
        return false;
    if (func_00579EA0(seal))
        return false;
    if (seal->m_state == 7)
        return false;
    switch (state) {
    case stateStand:
        if (seal->IsCurrentBlend((AnimType)19))
            return false;
        if (seal->m_state == stateCrouch || seal->m_state == stateProne) {
            if (seal->IsRemote())
                return true;
            if (OKToStand(seal, stateStand))
                return true;
            seal->m_desiredState = (SEAL_STATE)seal->m_state;
        }
        break;
    case stateCrouch:
        if (seal->IsCurrentBlend((AnimType)25))
            return false;
        if (seal->m_state == stateStand || seal->m_state == stateProne) {
            float len = Len3(seal->m_curr_firepos.x, 0.0f, seal->m_curr_firepos.z);
            float back = 1.2f * len;

            if (seal->m_state == stateProne)
                back = 0.5f + len;
            if (func_0054D620(seal, back, 7.0f, 1.2f * theCharacterDynamics.m_stepHeight))
                return false;
        }
        if (!seal->IsRemote() && !func_005857E0(seal))
            return false;
        if (seal->m_state == stateStand)
            return true;
        if (seal->m_state == stateProne) {
            if (seal->IsRemote())
                return true;
            if (OKToStand(seal, stateCrouch))
                return true;
            seal->m_desiredState = (SEAL_STATE)seal->m_state;
        }
        break;
    case stateProne:
        if (seal->IsCurrentBlend((AnimType)17))
            return false;
        if (seal->m_state != stateProne &&
            func_0054D620(seal, 11.2f, 8.0f, 0.5f * theCharacterDynamics.m_stepHeight)) {
            func_005B0BB0(seal, 0);
            return false;
        }
        if (!seal->IsRemote() && !func_00584C10(seal))
            return false;
        if (seal->m_state == stateStand || seal->m_state == stateCrouch)
            return true;
        break;
    }
    return false;
}

extern "C" {
/* an empty kit stub (OVL2 0x5BE2F0, the kit lane's) */
void func_005BE2F0(CZKit *kit);
/* (OVL2 0x584B00, seal_body.cpp's, and 0x57E540, not written yet) */
bool func_00584B00(CZSealBody *seal);
void func_0057E540(CZSealBody *seal);
}

/* the desired state taken when the SEAL may take it: the stance's change flagged, the stance's blend
 * asked for -- in state 2 an AI SEAL that 0x584B00 allows takes 0x57E540's instead of Prone -- and the
 * state set, then the kit told (OVL2 0x552D60; SOCOM 1's UpdateStance, in the same place -- after
 * UpdateLODFlags, before AddWeapon -- with SOCOM II's state test ahead of the switch) */
void CZSealBody::UpdateStance()
{
    if (func_00552EC0(this, m_desiredState)) {
        switch (m_desiredState) {
        case stateStand:
            m_stancechanged = true;
            Stand(1);
            SetStance(stateStand);
            break;
        case stateCrouch:
            m_stancechanged = true;
            Crouch();
            SetStance(stateCrouch);
            break;
        case stateProne:
            m_stancechanged = true;
            /* (the AI test as IsPlayer's ternary: the bool narrowed once, in v0 -- the IsAi inline's
             * result local takes a1) */
            if (m_control ? ((CSealCtrl *)m_control)->IsAi() : false) {
                if (func_00584B00(this))
                    func_0057E540(this);
                else
                    Prone();
            } else
                Prone();
            SetStance(stateProne);
            break;
        }
        func_005BE2F0(&m_kit);
    }
}

/* a fade of the SEAL's opacity asked for, unless a higher priority's runs: the target and rate kept,
 * the rate turned toward the target, the fade started when the node is not there already (OVL2
 * 0x552780, new in SOCOM II: CSealCtrl::OnDeath and ReCreate ask it; the name unknown) */
extern "C" void func_00552780(CZSealBody *seal, unsigned char priority, float target, float rate)
{
    if (priority >= seal->m_fade_priority) {
        seal->m_fade_priority = priority;
        seal->m_fade_target = target;
        seal->m_fade_rate = rate;
        float cur = seal->GetNode()->m_Opacity;

        if (cur != seal->m_fade_target) {
            if ((cur < seal->m_fade_target && seal->m_fade_rate < 0.0f) ||
                (cur > seal->m_fade_target && seal->m_fade_rate > 0.0f))
                seal->m_fade_rate *= -1.0f;
            seal->m_fading = true;
        }
    }
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

extern "C" {
/* the SEALs' queued ground probes, run as one batch (OVL1 bss: the count at 0x44D598, the list at
 * 0x44D588; OVL2 0x5B0800, seal_env.cpp's range, passes both to the world's GetIntersectBatch and
 * clears the count -- new in SOCOM II, the names unknown), and 0x5B0420 (seal_env.cpp's range: its
 * work done once the batch has run) */
extern int data_0044D598;
void func_005B0800();
void func_005B0420(CZSealBody *seal);
}

/* the highest bit of a region mask as a shift, none 0 (SOCOM 1's zdb::IntersectStruct::
 * GetMaxRegionShift, a static member SOCOM 1 calls out of line from here, the copy right after it;
 * SOCOM II inlines it -- node_world.h is another lane's, so the helper is the unit's) */
static inline unsigned int GetMaxRegionShift(unsigned int mask)
{
    unsigned int shift = 0;

    if (mask) {
        shift = -1;
        while (mask) {
            mask >>= 1;
            shift++;
        }
    }
    return shift;
}

/* the queued probes run, then 0x5B0420; seen by the player's camera, the node's region shift is the
 * camera's highest region; carrying a body, the carried node takes the SEAL's shift (OVL2 0x550570,
 * slot 0x24; SOCOM 1's, which then ran the carry's and the CQB's anims -- SOCOM II adds the batch, the
 * call and the carry's null test) */
void CZSealBody::PostTick(float dT)
{
    if (data_0044D598 > 0)
        func_005B0800();
    func_005B0420(this);
    zdb::CCamera *cam = ((CWorldCamera *)zdb::CWorld::m_world)->m_camera;

    if (cam->PlayerCanSeeRegion(m_last_altitude.GetRegion()))
        GetNode()->m_region_shift = GetMaxRegionShift(cam->GetPlayerCanSeeRegion());
    if (m_state == stateCarrying) {
        CZSealBody *carry = m_carry;

        if (carry)
            carry->GetNode()->m_region_shift = GetNode()->m_region_shift;
    }
}

extern "C" {
/* the SEAL's altitude kept for the probes (seal_env.cpp's), and the post-move steps in their order: the
 * probes and the collision (OVL2 0x550A80, 0x5483D0), the kit's tick (OVL2 0x5C0FD0) */
extern float data_0066AAB8;
void func_00550A80(CZSealBody *seal);
void func_005483D0(CZSealBody *seal, float dT);
void func_005B0840(CZSealBody *seal);
void func_005762E0(CZSealBody *seal);
void func_00548090(CZSealBody *seal);
void func_005C0FD0(CZKit *kit, float dT);
}

/* the tick after the move: the probes and collision steps; a SEAL carried, stabbed or being restrained takes
 * its root part's place and then its carrier's node, quaternion and position; the compass anchor kept on the
 * SEAL, a player's restrained SEAL's point by whether its AI is held or not; the last tick's points kept; the
 * aim, the controller's PostTick, the grenades, the kit, the stun, the step and the blood (OVL2 0x5506A0, slot
 * 0x20; SOCOM 1's slot was empty) */
void CZSealBody::PrePostTick(float dT)
{
    if (!m_f40)
        return;
    func_00550A80(this);
    data_0066AAB8 = m_posW.y;
    func_005483D0(this, dT);
    func_005B0840(this);
    func_005762E0(this);
    if (m_carry) {
        if (m_state == stateCarried || IsCurrentBlend((AnimType)AnimTypes::Stabbed) ||
            IsCurrentBlend((AnimType)AnimTypes::ReceiveRestraints)) {
            CPnt3D pos;
            CQuat rot;

            PartInSpace(GetRootPart(), 0, &pos, &rot);
            GetRootPart()->m_node->SetPosition(&pos);
            GetRootPart()->m_node->SetRotation((const CQuat *)(const void *)&rot);
            const CMatrix *m = &m_carry->GetNode()->m_matrix;

            GetNode()->SetMatrix(m);
            m_quat = m_carry->m_quat;
            m_next_quat = m_quat;
            GetNode()->SetPosition((const PNT3D *)m_carry->GetNode()->m_matrix.m_matrix[3]);
            UpdatePosW();
        }
    }
    func_00548090(this);
    if (m_compass_anchor) {
        m_compass_anchor->m_posW = m_posW;
        if (m_compass_anchor->m_type == 4) {
            int point = 0;

            if (m_restrained) {
                CSealCtrlAi *ai = ((CSealCtrl *)m_control)->GetSealCtrlAi();
                CSealCtrl *remote = ai ? (CSealCtrl *)ai->GetRemoteControl() : 0;

                if (ai) {
                    if (ai->AiStateIs((AI_STATE)9))
                        point = 2;
                    else
                        point = 1;
                } else if (remote) {
                    point = remote->GetJumpFlag() == (AI_STATE)9 ? 2 : 1;
                }
            }
            m_compass_anchor->SetPoint(point);
        }
    }
    m_prev_1458 = m_1458;
    m_prev_1470 = m_1470;
    m_prev_1464 = m_1464;
    m_prev_have_aim_norm = m_have_aim_norm;
    UpdateAimParams();
    ((CSealCtrl *)m_control)->PostTick(dT);
    HandleGrenadeExplosion();
    func_005C0FD0(&m_kit, dT);
    TickStun(dT);
    HandleSealStep();
    HandleBloodDrip(dT);
}

/* the two roots of a*t^2 - 2*b*t + c, smaller first, when real (an inline bool: 0x550330's) */
static inline bool SolveQuadratic(float a, float b, float c, float *t0, float *t1)
{
    float d = b * b - a * c;

    if (d < 0.0f)
        return false;

    float inv = 1.0f / a;
    float s = sqrtf(d);

    *t0 = inv * (b - s);
    *t1 = inv * (b + s);
    return true;
}

/* a moving upright cylinder's sweep against a standing one: the first time in 0..1 where the mover --
 * at a point with a radius and a height, moving by a step -- overlaps the other in height and touches it
 * in the ground plane; the hit point, and the way out of the other's axis level and unit; whether it
 * hits (OVL2 0x550330, new in SOCOM II: 0x54F7E0 calls it, the standing cylinder's height by the
 * stance; the name unknown) */
extern "C" bool func_00550330(CPnt3D *hit, CPnt3D *normal, float cx, float cy, float cz, float r, float h,
                              float px, float py, float pz, float r2, float h2, float dx, float dy, float dz)
{
    float t0 = 0.0f;
    float t1;

    if (dy == 0.0f) {
        if (!(py <= cy + h) || !(cy <= py + h2))
            return false;
        t1 = 1.0f;
    } else {
        float inv = 1.0f / dy;

        if (dy > 0.0f) {
            t0 = inv * (cy - py - h2);
            t1 = inv * (cy + h - py);
        } else {
            t0 = inv * (cy + h - py);
            t1 = inv * (cy - py - h2);
        }
        if (t1 < 0.0f || t0 > 1.0f)
            return false;
        if (t0 < 0.0f)
            t0 = 0.0f;
        if (t1 > 1.0f)
            t1 = 1.0f;
    }

    float rr = r + r2;
    float ox = cx - px;
    float oz = cz - pz;
    float a = dx * dx + dz * dz;
    float b = ox * dx + oz * dz;
    float c = ox * ox + oz * oz - rr * rr;
    float ta, tb;

    if (!SolveQuadratic(a, b, c, &ta, &tb))
        return false;
    if (t0 > tb || t1 < ta)
        return false;

    float t = t0 <= ta ? ta : t0;

    hit->x = px + t * dx;
    hit->y = py + t * dy;
    hit->z = pz + t * dz;
    normal->x = hit->x - cx;
    normal->y = 0.0f;
    normal->z = hit->z - cz;
    normal->Normalize();
    return true;
}

extern "C" {
/* three anim types the code reads as variables (OVL1 data, -1 in the image) */
extern short data_003DEF40;
extern short data_003DEF50;
extern short data_003DEF60;
}

/* whether the SEAL is on a ladder: its state says so, or -- each when asked -- the top blend's state,
 * the anim type 46 on top, or one of the three variable anim types (OVL2 0x54F6E0, new in SOCOM II:
 * CAppCamera::GetThirdPersonCamPos, both controllers' PreTick and SetAiMode ask it; the name unknown) */
extern "C" bool func_0054F6E0(CZSealBody *seal, bool blend, bool mount, bool anims)
{
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170((CIndexedArray *)&seal->m_blends);

    return seal->m_state == stateClimbLadder || (blend && top && top->m_state == stateClimbLadder) ||
           (mount && seal->IsCurrentBlend((AnimType)46)) ||
           (anims && (seal->IsCurrentBlend((AnimType)data_003DEF50) ||
                      seal->IsCurrentBlend((AnimType)data_003DEF60) ||
                      seal->IsCurrentBlend((AnimType)data_003DEF40)));
}

extern "C" {
void func_0054F7E0(CZSealBody *seal, float dT);
/* the speed below which a SEAL stops checking player collision, squared (OVL2 data, 0.0001: a
 * variable -- read with lwc1 for the compare) */
extern float data_0064FCD0;
}

/* the new-altitude bit through an accessor, and the player-collision and new-altitude bits set
 * together (inlines, the names ours: the target reads the byte twice -- the bit passed back read
 * through the accessor, which MW never shares with the setter's read-modify-write; read in place
 * the two loads merge, and `!= 0` in the accessor merges them too) */
static inline bool GetNewAltitude(const CEntity *e) { return e->m_get_new_altitude; }
static inline void SetCollisionBits(CEntity *e, bool check, bool alt)
{
    e->m_check_player_collision = check;
    e->m_get_new_altitude = alt;
}

/* the SEALs' collision tick, the scheduler's "body_collide_tick" (OVL2 0x54F570, new in SOCOM II:
 * st_core.cpp adds it): offline, each SEAL out of state 8 that checks player collision calls
 * 0x54F7E0 -- once its motion all but stops (squared under 0.0001) the check goes off and the call
 * is skipped (the name unknown). GetWeapon's walk: the iterator assigned from begin(), the end
 * taken once; the `&&` held in a bool (`!(x == 8)`: xori/sltiu/xori) and its test nesting the rest */
extern "C" bool func_0054F570(float dT, void *data)
{
    if (!theNetwork.m_9) {
        CEntityList::iterator i;

        i = CEntity::m_list.begin();
        for (CEntityList::iterator e = CEntity::m_list.end(); i != e; ++i) {
            CZSealBody *seal = AsSeal(*i);

            if (seal) {
                bool check = !(seal->m_state == stateDead) && seal->m_check_player_collision;

                if (check) {
                    if (seal->m_f44.x * seal->m_f44.x + seal->m_f44.y * seal->m_f44.y +
                            seal->m_f44.z * seal->m_f44.z <
                        data_0064FCD0) {
                        SetCollisionBits(seal, false, GetNewAltitude(seal));
                        check = false;
                    }
                    if (check)
                        func_0054F7E0(seal, dT);
                }
            }
        }
    }
    return false;
}

extern "C" {
/* the SEAL's delayed ground probes: fifty intersection sets and the count used this frame (OVL1
 * data, the SEAL's alone: PreTick zeroes the count, 0x5483D0 reads the sets; the names unknown) */
extern zdb::DiIntersect *data_003DE940[];
extern int data_0044AF78;
}

/* a delayed probe from a point to another, the SEAL's node its own and characters not hit -- the
 * height kept when m_1340 is set -- or none when the fifty are spent (OVL2 0x54F410, new in SOCOM
 * II: PostPreTick 0x54AA30 asks it 31 times; the name unknown) */
extern "C" bool func_0054F410(CZSealBody *seal, const CPnt3D *from, CPnt3D *to)
{
    if (seal->m_1340 != 0.0f)
        to->y = from->y;
    if (data_0044AF78 >= 50)
        return false;
    data_003DE940[data_0044AF78]->m_Type = 2;
    data_003DE940[data_0044AF78]->SetMask(seal->m_last_altitude.GetRegion());
    zdb::CWorld::m_world->RegIntersectDelayed(data_003DE940[data_0044AF78], from, to, (unsigned int)-1);
    data_003DE940[data_0044AF78]->m_Node = seal->GetNode();
    data_003DE940[data_0044AF78]->SetIntersectCharacters(false);
    data_0044AF78++;
    return true;
}

/* whether a hit stops the SEAL: a node's, not of kind 1; on a slope too steep to stand on, unless its
 * edge drops less than 4 under a SEAL that is not prone; on a flatter one, only kind 0 -- the kind is
 * the polygon's three bits at 28, 4 with no polygon (OVL2 0x54F330, new in SOCOM II: 0x54EDF0 asks
 * it of the ground hit; the name unknown) */
extern "C" bool func_0054F330(CZSealBody *seal, zdb::IntersectStruct *hit)
{
    if (!hit->m_node)
        return false;
    CDIType type = hit->m_poly ? (CDIType)hit->m_poly->m_flags.m_bits28 : (CDIType)4;

    if (type == 1)
        return false;
    if (hit->m_norm.y < theCharacterDynamics.m_maxSlopeCos) {
        CPnt3D b, a;

        if (hit->GetIntersectedEdgePoints(&a, &b) && seal->m_state != stateProne && a.y - b.y < 4.0f)
            return false;
        return true;
    }
    return type == 0;
}

/* the height a direction reaches over the length of b - a, turned by a matrix's rotation (OVL2
 * 0x54ECD0, new in SOCOM II: PostPreTick 0x54AA30 asks it eight times; the name unknown) */
extern "C" float func_0054ECD0(const CMatrix *m, const CPnt3D *dir, const CPnt3D *a, const CPnt3D *b)
{
    CPnt3D d(b->x - a->x, b->y - a->y, b->z - a->z);
    CPnt3D r = *dir * d.Length();

    TransformVectors(m, &r, &r, 1);
    return r.y;
}

extern "C" {
/* 1.09 (OVL1 data): how far along its part a planted foot's spot sits */
extern float data_003F3430;
}

/* a foot pinned through a turn in place: with both feet, under a turn anim (the top blend's flag
 * 0x40) while the SEAL stands turning with nothing in hand and no draw running, the right foot
 * about the anim's middle (within 0.2499 of 0.65), else the left -- its spot in the world taken
 * once (its bit set, the other's cleared), and the SEAL's push m_f44 made what keeps it there
 * (OVL2 0x54EA80, new in SOCOM II: ComputeNextPosition calls it; TurnSetup clears the two bits;
 * the name unknown) */
extern "C" void func_0054EA80(CZSealBody *seal)
{
    if (!seal->m_lfoot || !seal->m_rfoot)
        return;
    if (func_005551A0(((CZBodyAnimBlend *)func_00555170((CIndexedArray *)&seal->m_blends))->m_flags, 0x40) &&
        seal->m_is_turning && seal->m_state == stateStand && seal->m_weapon_id == 0 &&
        seal->m_wep_seq.LinearWeight() == 0.0f) {
        float t = ((CZBodyAnimBlend *)func_00555170((CIndexedArray *)&seal->m_blends))->m_tfrac;
        CPnt3D p, q, d;
        CPnt3D off(data_003F3430, 0.0f, 0.0f);
        static float mid = 0.65f;
        static float range = 0.2499f;

        if (_close_enough(t, mid, range)) {
            seal->PartInSpace(seal->m_rfoot, &off, &p, 0);
            if (!seal->m_bit7) {
                seal->m_node->m_matrix.Transform(&p, &seal->m_rfootW, 1);
                seal->m_bit7 = true;
                seal->m_bit6 = false;
            }
            seal->m_next_quat.Apply(&p, &q, 1);
            seal->m_rfootW.Sub(&q, &d);
            d.Sub(&seal->m_posW, &seal->m_f44);
        } else {
            seal->PartInSpace(seal->m_lfoot, &off, &p, 0);
            if (!seal->m_bit6) {
                seal->m_node->m_matrix.Transform(&p, &seal->m_lfootW, 1);
                seal->m_bit6 = true;
                seal->m_bit7 = false;
            }
            seal->m_next_quat.Apply(&p, &q, 1);
            seal->m_lfootW.Sub(&q, &d);
            d.Sub(&seal->m_posW, &seal->m_f44);
        }
    }
}

/* where the item in hand is held out from the SEAL: the aim point's height on the SEAL's spot, and that
 * spot pushed back along the node's forward axis by twice the item's width and m_f44 added, at the same
 * height -- none without the item's part and node, each half only while its point is set (OVL2
 * 0x54E8F0, new in SOCOM II: PostPreTick 0x54AA30 asks it; the name unknown) */
static inline float Twice(float v)
{
    v *= 2.0f;
    return v;
}

extern "C" bool func_0054E8F0(CZSealBody *seal, CPnt3D *pos, CPnt3D *pt)
{
    bool ok = false;
    CZBodyPart *part = seal->m_item_gear[seal->m_item].m_part;

    if (!part || !part->m_node)
        return false;
    ok = true;
    if (seal->m_curr_aimpt) {
        *pos = seal->m_posW;
        pos->y += seal->m_root->m_pos.y;
    } else
        ok = false;
    if (seal->m_curr_firept) {
        *pt = *(CPnt3D *)seal->m_node->m_matrix.m_matrix[2];
        pt->Scale(-1.0f, pt);
        const CBBox *b = part->m_node->GetBBox();

        if (b) {
            pt->Scale(Twice(fabsf(b->m_max.x - b->m_min.x)), pt);
            pt->Add(pos, pt);
            pt->Add(&seal->m_f44, pt);
            pt->y = pos->y;
        } else
            ok = false;
    } else
        ok = false;
    return ok;
}

/* where a carrying SEAL holds the body it carries, in the world: moving up (m_velM.z over 0.01),
 * midway between the carried body's feet and 1.6 on, else 2.5 short of its head -- each part placed
 * in the SEAL's body, turned onto the world by its node and m_f44 added; the SEAL's own spot at the
 * same height given back too (OVL2 0x54E760, new in SOCOM II: PostPreTick 0x54AA30 asks it; the
 * name unknown) */
extern "C" bool func_0054E760(CZSealBody *seal, CPnt3D *pos, CPnt3D *pt)
{
    CZSealBody *carry = seal->m_state == stateCarrying ? seal->m_carry : 0;

    if (seal->m_velM.z > 0.01f) {
        CPnt3D zero(0.0f, 0.0f, 0.0f);
        CPnt3D r;

        seal->PartInSpace(carry->m_lfoot, &zero, pt, 0);
        seal->PartInSpace(carry->m_rfoot, &zero, &r, 0);
        pt->Add(&r, pt);
        pt->Scale(0.5f, pt);
        pt->z += 1.6f;
    } else {
        CPnt3D zero(0.0f, 0.0f, 0.0f);

        seal->PartInSpace(carry->m_head, &zero, pt, 0);
        pt->z -= 2.5f;
    }
    seal->m_node->m_matrix.Transform(pt, 1);
    pt->Add(&seal->m_f44, pt);
    *pos = *(CPnt3D *)seal->m_node->m_matrix.m_matrix[3];
    pos->y = pt->y;
    return true;
}

/* a material's extension (int_material.h's CMaterialEx, which zweapon.h's stand-in of the table leaves
 * incomplete -- seal_env.cpp's view): only its first two flags */
struct CMaterialEx {
    unsigned char _0[0x3c];
    unsigned int m_volumetric : 1;   // 0x3c: VOLUMETRIC
    unsigned int m_liquid : 1;   // bit 1: LIQUID
    unsigned int m_flags2 : 30;

    /* (written `? true : false`, as seal_env.cpp's: the bit converted, sltu, before its test) */
    bool IsVolumetric() { return m_volumetric ? true : false; }
    bool IsLiquid() { return m_liquid ? true : false; }
};

/* a material's extension, the first one for an index past the table (seal_env.cpp's) */
static inline CMaterialEx *MaterialEx(unsigned int i)
{
    if (i >= CMaterialExArray::m_materialex_list.size())
        i = 0;
    return (CMaterialEx *)CMaterialExArray::m_materialex_list[i];
}

/* a part's height in the SEAL's body, raised by an offset of none (an inline, the name ours: the
 * target stores the zero to a stack slot of its own ahead of the call -- not moved into its delay
 * slot -- and reloads it after: a volatile local; a plain one, a parameter, a const reference or a
 * type-punned store are rematerialised, pooled or scheduled into the delay slot. The inline's point
 * is allocated after the caller's operator* temporary, as the target's is) */
static inline float PartHeight(CZSealBody *seal, CZBodyPart *part)
{
    CPnt3D p;
    volatile float dy = 0.0f;

    seal->PartInSpace(part, 0, &p, 0);
    p.y += dy;
    return p.y;
}

/* whether a wall may be passed through: no steeper than the dynamics' slope limit, or of a volumetric
 * or liquid material (an inline, the name ours) */
static inline bool PassThrough(zdb::IntersectStruct *wall)
{
    if (!(wall->m_norm.y <= theCharacterDynamics.m_maxSlopeCos))
        return true;
    CMaterialEx *m = MaterialEx(wall->GetMaterial());

    /* (an if returning true: `return m && (...)` materialises the bool and masks it) */
    if (m && (m->IsVolumetric() || m->IsLiquid()))
        return true;
    return false;
}

extern "C" {
/* a wall's hit taken for the SEAL to hold to, its two side points kept (OVL2 0x5B0D30, a draft) */
void func_005B0D30(CZSealBody *seal, const zdb::IntersectStruct *hit, CPnt3D *a, CPnt3D *b);
}

/* the wall's hit the SEAL holds to, checked against what lies between the SEAL and it: the hit's
 * height -- 0.5 over its upper edge point, when it has its edge points, under 21.1 over the SEAL's
 * spot; else the SEAL's head, the hit under it by 0.1 -- and when it is, a ray from the SEAL's spot
 * to 1.1 times the way to the hit at that height, the SEAL's node switched off; a wall it meets that
 * is steeper than the dynamics' slope limit, not volumetric or liquid and another node's taken in its
 * place; the hit held then, or none (OVL2 0x54E430, new in SOCOM II: CanTraverse calls it; the name
 * unknown) */
extern "C" zdb::IntersectStruct *func_0054E430(CZSealBody *seal)
{
    zdb::IntersectStruct *hit = seal->m_1090.m_node ? &seal->m_1090 : 0;

    if (hit) {
        bool blocked = false;
        const CPnt3D *pos = (const CPnt3D *)seal->m_node->m_matrix.m_matrix[3];
        CPnt3D lo, hi, a, b;
        float y;

        if (hit->GetIntersectedEdgePoints(&hi, &lo)) {
            y = hi.y + 0.5f;
            if (y < pos->y + 19.1f + 2.0f)
                blocked = true;
        } else {
            y = pos->y + PartHeight(seal, seal->m_head);
            if (hit->m_pos.y < y - 0.1f)
                blocked = true;
        }
        if (blocked) {
            a = *pos;
            b = hit->m_pos;
            b.y = y;
            a.y = y;
            b.Sub(&a, &b);
            b = b * 1.1f;
            b.Add(&a, &b);

            bool was = seal->GetNode()->m_active;

            seal->GetNode()->SetActive(false);
            zdb::IntersectStruct *wall = zdb::CWorld::m_world->GetIntersect(&a, &b);

            seal->GetNode()->SetActive(was);
            if (wall) {
                if (!PassThrough(wall) && wall->m_node != hit->m_node) {
                    func_005B0D30(seal, wall, (CPnt3D *)((char *)seal + 0x10b0), (CPnt3D *)((char *)seal + 0x10bc));
                    hit = seal->m_1090.m_node ? &seal->m_1090 : 0;
                }
            }
        }
    }
    return hit;
}

extern "C" {
/* the SEAL's ground hit kept, its point, normal and node -- or none (OVL2 0x5B0C10, seal_env.cpp's) */
void func_005B0C10(CZSealBody *seal, zdb::IntersectStruct *hit);
}

/* the ground under the SEAL found again: a probe from 2.5 under its spot along its push m_f44 flat --
 * five long when the push is all but none -- reaching 1.5 of it back, the SEAL's node switched off for
 * the probe; the hit kept, or false (OVL2 0x54A8B0, new in SOCOM II: 0x59AD30 calls it twice; the name
 * unknown) */
extern "C" bool func_0054A8B0(CZSealBody *seal)
{
    CPnt3D from = seal->m_posW;

    from.y -= 2.5f;
    CPnt3D d = seal->m_f44;

    d.y = 0.0f;
    if (d.x * d.x + d.z * d.z < 0.099999994f) {
        d.Normalize();
        d.Scale(5.0f, &d);
    }
    from.Add(&d, &from);
    d.Scale(-1.5f, &d);
    CPnt3D to;

    from.Add(&d, &to);
    zdb::IntersectStruct *hit = 0;
    bool active = seal->GetNode()->m_active;

    seal->GetNode()->SetActive(false);
    hit = zdb::CWorld::m_world->GetIntersect(&from, &to);

    seal->GetNode()->SetActive(active);
    if (!hit)
        return false;
    func_005B0C10(seal, hit);
    return true;
}

/* a push taken out to a value of either sign: the larger of the same sign kept (an inline, the name
 * ours: the zero loaded again for each) */
static inline void PushOut(float &cur, float v)
{
    if (v >= 0.0f) {
        if (cur < v)
            cur = v;
    } else if (v < 0.0f) {
        if (cur > v)
            cur = v;
    }
}

/* the point's words copied one by one (an inline: SOCOM 1's PNT3D::operator=, whose out-of-line copies
 * its builds call) */
static inline void CopyPnt(CPnt3D &to, const CPnt3D &from)
{
    to.x = from.x;
    to.y = from.y;
    to.z = from.z;
}

extern "C" {
/* (seal_env.cpp's: two rays' hits at once; the hit taken as the SEAL's ground) */
void func_005AB590(CEntity *ent, zdb::IntersectStruct **hit0, const CPnt3D *tail0, const CPnt3D *tip0,
                   zdb::IntersectStruct **hit1, const CPnt3D *tail1, const CPnt3D *tip1);
void func_005B0BB0(CZSealBody *seal, const zdb::IntersectStruct *hit);
}

/* what lies before and behind a SEAL on another's ground: two rays from the SEAL's place, risen by a
 * height, along its facing -- back and ahead by their lengths, the facing tilted onto the ground's normal
 * -- their first hit kept as the ground; whether either hit. None for a remote SEAL (OVL2 0x54D620, new
 * in SOCOM II: 0x552EC0 calls it twice; the name unknown) */
extern "C" bool func_0054D620(CZSealBody *seal, float back, float ahead, float rise)
{
    if (seal->IsRemote())
        return false;
    CPnt3D fwd;

    CopyPnt(fwd, *(const CPnt3D *)seal->m_node->m_matrix.m_matrix[2]);
    fwd.Normalize();

    CPnt3D rev;

    rev.x = -fwd.x;
    rev.y = -fwd.y;
    rev.z = -fwd.z;
    rev.Scale(back);
    fwd.Scale(ahead);

    CPnt3D pos;

    CopyPnt(pos, *(const CPnt3D *)seal->m_node->m_matrix.m_matrix[3]);
    pos.y += rise;

    CPnt3D from;
    CPnt3D tip0;
    CPnt3D tip1;

    CopyPnt(from, pos);
    CopyPnt(tip0, pos + rev);
    CopyPnt(tip1, from + fwd);

    CQuat q;
    CPnt3D up(0.0f, 1.0f, 0.0f);

    q.Align(up, seal->m_last_altitude.m_norm);

    CPnt3D d[2];

    CopyPnt(d[0], tip0 - pos);
    CopyPnt(d[1], tip1 - from);
    q.Apply(d, d, 2);
    CopyPnt(tip0, pos + d[0]);
    CopyPnt(tip1, from + d[1]);

    zdb::IntersectStruct *hit0 = 0;
    zdb::IntersectStruct *hit1 = 0;

    func_005AB590(seal, &hit0, &pos, &tip0, &hit1, &from, &tip1);
    func_005B0BB0(seal, hit0);
    if (hit0 || hit1)
        return true;
    return false;
}

/* the push off a wall: a step of r from the point into the hit's plane, against its normal and at the
 * point's height; what of it the plane cuts off, 0.7 of it along the normal, taken into m_f44's x and z
 * -- false with no node hit (OVL2 0x54A6D0, new in SOCOM II: ComputeNextPosition 0x54D9A0 calls it
 * twice, the hit its ground's when that has a node; the name unknown). The step is copied after the
 * null test through a no-op `(float)` cast (the round 56, open since round 51): a plain copy (`float r
 * = step;`) is coalesced into the parameter and made at entry, MW then scheduling it into the test's
 * delay slot; the cast makes the copy an expression, kept where it is written -- the target's `mov.s`
 * just before the `neg.s`, the hit's copy in the delay slot, both alignment pads following */
extern "C" bool func_0054A6D0(CZSealBody *seal, const CPnt3D *pos, zdb::IntersectStruct *hit, float step)
{
    if (!hit->m_node)
        return false;
    float r = (float)step;
    CPnt3D p = *pos;
    CPnt3D n = hit->m_norm;

    n.Scale(-r, &n);
    p.Add(&n, &p);
    p.y = pos->y;
    float t = LineIntersectPlane(PlaneConstant(hit->m_norm, hit->m_pos), hit->m_norm, *pos, p);

    if (t < 1.0f) {
        CPnt3D push = hit->m_norm;

        push.Scale(0.7f * (r * (1.0f - t)), &push);
        PushOut(seal->m_f44.x, push.x);
        PushOut(seal->m_f44.z, push.z);
    }
    return true;
}

/* a switch that turns the weapon's wall push-out off -- never written, so off (OVL2 bss 0x66B000: 0x549C30
 * and 0x549F30 give up at it, 0x5483D0 and PostPreTick test it; the name ours). The steepest wall normal
 * the points are pushed back from is the dynamics' slope limit (0x44C268, theCharacterDynamics + 0x18) */
extern unsigned char noWeaponWallPush;

/* bit 0 of 0x258 set (an inline, the name ours: the target inserts the constant, `andi v1,zero,1`) */
static inline void Set258(CZSealBody *seal, bool b) { seal->m_258_0 = b; }

/* the weapon's aim and fire points kept out of a wall: with a node hit, a unit normal, both points on
 * nodes, the top blend's flag 0x40, the body not prone, hanging or on a ladder and the switch off, the
 * points moved by the offset and the weapon's sequence weighing half or more -- each point (the fire
 * point first) behind the hit's plane pushes m_f44 back out along the normal, 0.85 of its depth, when
 * the normal faces up no more than the slope limit, else clears bit 0 of 0x258; false when a test fails or a
 * point is in front (OVL2 0x549C30, new in SOCOM II; the name unknown) */
extern "C" bool func_00549C30(CZSealBody *seal, const CPnt3D *offset, zdb::IntersectStruct *hit)
{
    if (!hit->m_node)
        return false;
    if (!_close_enough(Dot(hit->m_norm, hit->m_norm), 1.0f, 0.0001f))
        return false;
    if (!seal->m_curr_aimpt || !seal->m_curr_firept)
        return false;
    bool push = true;

    if (!(hit->m_norm.y <= theCharacterDynamics.m_maxSlopeCos))
        push = false;
    if (!func_005551A0(((CZBodyAnimBlend *)func_00555170((CIndexedArray *)&seal->m_blends))->m_flags, 0x40))
        return false;
    if (seal->m_state == stateProne)
        return false;
    if (seal->m_state == stateClimbLadder || seal->m_state == stateHang)
        return false;
    if (noWeaponWallPush)
        return false;
    CPnt3D aim;
    CPnt3D fire;

    seal->m_curr_aimpos.Add(offset, &aim);
    seal->m_curr_firepos.Add(offset, &fire);
    if (seal->m_wep_seq.Weight() < 0.5f)
        return false;
    for (int i = 0; i < 2; i++) {
        CPnt3D p;
        CPnt3D d;

        if (i == 0)
            p = fire;
        else
            p = aim;
        p.Sub(&hit->m_pos, &d);
        float dot = Dot(d, hit->m_norm);

        if (!(dot < 0.0f))
            return false;
        if (push) {
            CPnt3D n = hit->m_norm;

            n.Scale(0.85f * -dot, &n);
            seal->m_f44.Add(&n, &seal->m_f44);
        } else
            Set258(seal, false);
    }
    return true;
}

/* whether the throw the trigger's power would make is clear: the throw GetThrowAnim picks (none: not
 * clear), its point to a point ahead by the state -- a toss's nearer, a run's the stand's -- both
 * turned by the node, the first lifted to the ground plane, and nothing of the world between (OVL2
 * 0x5499E0, new in SOCOM II: the kit's 0x5BE9A0 asks it; the name unknown -- any other state is clear).
 * GetThrowAnim's toss test again, word for word; the aim declared uninitialised first (numbered there:
 * the power then takes $f20, as the target -- declared where it is read the two swap), the call's bool
 * tested `== false` (its andi), the states an if chain (a switch tests 2 first) */
extern "C" bool func_005499E0(CZSealBody *seal)
{
    float aim;
    CPnt3D ahead(1.155273f, 13.35443f, -15.75f);
    float power = ((CSealCtrl *)seal->m_control)->m_trigger_power;
    AnimType type;
    float frac;
    float base;
    CPnt3D p;

    if (seal->GetThrowAnim(power, type, frac, base, p) == false)
        return false;
    aim = seal->m_aim_goal.y;
    CharacterDynamics::throw_params &tp = seal->GetDynamics()->m_throw_params;
    bool toss = power < tp.toss_power_threshold && aim < tp.toss_aim_threshold;
    CPnt3D from = p;
    CPnt3D to = ahead;

    if (seal->m_state == stateStand) {
        if (toss) {
            to.x = 3.457066f;
            to.y = 12.454332f;
            to.z = -10.0f;
        } else
            to = ahead;
    } else if (seal->m_state == stateCrouch) {
        to.x = 0.924411f;
        to.y = 10.631366f;
        to.z = -12.1f;
    } else if (seal->m_state == stateProne) {
        if (toss) {
            to.x = 1.819537f;
            to.y = 5.580177f;
            to.z = -10.6f;
        } else {
            to.x = 0.59401f;
            to.y = 9.082238f;
            to.z = -6.75f;
        }
    } else
        return true;
    if (from.z < 0.0f)
        from.z = 0.0f;
    seal->m_node->m_matrix.Transform(&from, 1);
    seal->m_node->m_matrix.Transform(&to, 1);
    if (zdb::CWorld::m_world->GetIntersect(&from, &to, true))
        return false;
    return true;
}


extern "C" {
/* the up the root is eased onto while the SEAL stands (OVL2 bss 0x66ACD0; the name ours), and an
 * AnimType static (OVL1 data 0x3DEF08: seal_body.cpp and seal_create.cpp test the same blend) */
extern short data_003DEF08;
extern CPnt3D data_0066ACD0;
}

/* the root tilted onto the ground: through the top blend's span -- its anim's two marks, 0.5 to 0.9 for
 * the blend AnimTypes 0x3DEF08 names, or 0.8 to 0.99 airborne in blend state 8 -- the ground's normal
 * blended in from straight up, turned into the SEAL's frame, eased from 0x66ACD0's up for a standing
 * SEAL not in blend 19 or 25, and set as the root part's turn (OVL2 0x548090, new in SOCOM II:
 * PrePostTick calls it; the name unknown) */
extern "C" void func_00548090(CZSealBody *seal)
{
    CZBodyAnimBlend *top = (CZBodyAnimBlend *)func_00555170((CIndexedArray *)&seal->m_blends);
    float w;
    float a, b;

    if (!seal->m_airborne) {
        CZSIScriptCommon *anim = top->Representative()->m_anim;

        a = anim->m_24;
        b = anim->m_28;
        if (seal->IsCurrentBlend((AnimType)data_003DEF08)) {
            a = 0.5f;
            b = 0.9f;
        }
    } else if (top->m_state == 8) {
        a = 0.8f;
        b = 0.99f;
    } else
        return;

    /* where t lies from a to b, either way round, a step when the two are equal (nothing set for a t
     * between equal ends, which cannot be). Written in place, the weight declared ahead of the span
     * (f22 / f21 / f20 as the target): an inline ramp's weight is numbered after the caller's locals
     * and takes f20; t and the altitude's address are taken before the tests */
    float t = top->m_tfrac;
    zdb::IntersectStruct *alt = &seal->m_last_altitude;

    if (a == b) {
        if (t <= a)
            w = 0.0f;
        else if (t > b)
            w = 1.0f;
    } else if (!(b <= a)) {
        if (t <= a)
            w = 0.0f;
        else if (t >= b)
            w = 1.0f;
        else
            w = (t - a) / (b - a);
    } else {
        if (t <= b)
            w = 1.0f;
        else if (t >= a)
            w = 0.0f;
        else
            w = 1.0f - (t - b) / (a - b);
    }
    if (!(w <= 0.0f)) {
        CPnt3D up = CPnt3D::unit_y;
        CPnt3D n;
        CQuat r;

        up.Interp(&alt->m_norm, w, &n);
        CQuat q = seal->m_quat;

        q.vec.x = -q.vec.x;
        q.vec.y = -q.vec.y;
        q.vec.z = -q.vec.z;
        q.Apply(&n, &up, 1);
        if (seal->m_state == stateProne && !seal->IsCurrentBlend((AnimType)19) && !seal->IsCurrentBlend((AnimType)25))
            data_0066ACD0.Interp(&up, 0.2f, &n);
        else
            n = up;
        r.Align(CPnt3D::unit_y, n);
        seal->m_root->m_rot = r;
        /* (the cast to const keeps the pointer form's null test of the local's address, as the
         * target has it: `&r`, a pointer or reference local or an inline fold it away) */
        seal->GetRootPart()->m_node->SetRotation((const CQuat *)&r);
    }
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

/* a friend's marker given its four points, and offline turned off and locked (OVL2 0x543680) */
void CZSealBody::AddFriendAnchorPoints(CCompassAnchor *anchor)
{
    CPnt3D p;

    p.x = 62.0f;
    p.y = 49.0f;
    p.z = 36.0f;
    anchor->AddPoint(&p);
    p.x = 80.0f;
    p.y = 42.0f;
    p.z = 30.0f;
    anchor->AddPoint(&p);
    p.x = 30.0f;
    p.y = 42.0f;
    p.z = 80.0f;
    anchor->AddPoint(&p);
    p.x = 30.0f;
    p.y = 76.0f;
    p.z = 80.0f;
    anchor->AddPoint(&p);
    if (!theNetwork.m_9) {
        anchor->SetEnabled(false);
        anchor->SetArrowEnabled(false);
        anchor->m_locked = 1;
    }
}

/* the SEAL's own compass marker made, once: a friend's (kinds 4 and 5) where the SEAL is on the
 * player's team, at the SEAL, with its points; "ABLE" (12) or "BRAVO" (13) at the origin with one
 * point; named when given a name, and given its kind (OVL2 0x5434B0) */
CCompassAnchor *CZSealBody::CreateCompassAnchor(int type, const char *name)
{
    static CPnt3D point(56.0f, 56.0f, 56.0f);
    CCompassAnchor *anchor = 0;

    if (m_compass_anchor)
        return 0;
    switch (type) {
    case 4:
    case 5: {
        bool friendly = true;
        /* (the player's team taken first, as in 0x544FC0) */
        unsigned int team = ftsGetPlayer()->m_team;

        if (!(team & m_team))
            friendly = false;
        if (friendly) {
            anchor = CCompass::CreateAnchor(m_posW, "FRIENDLY");
            AddFriendAnchorPoints(anchor);
        }
        break;
    }
    case 12:
        anchor = CCompass::CreateAnchor(CPnt3D::zero, "ABLE");
        anchor->AddPoint(&point);
        anchor->SetPoint(-1);
        break;
    case 13:
        anchor = CCompass::CreateAnchor(CPnt3D::zero, "BRAVO");
        anchor->AddPoint(&point);
        anchor->SetPoint(-1);
        break;
    }
    if (anchor) {
        if (name)
            anchor->SetName(name);
        anchor->m_type = type;
    }
    m_compass_anchor = anchor;
    return anchor;
}

/* the charges set down that the SEAL comes near set off: each one whose time has run out, within
 * its ammunition's proximity distance of the SEAL, detonated and taken off the list (OVL2 0x543930,
 * new in SOCOM II: Tick calls it; the name ours) */
void CZSealBody::CheckProximityExplosives()
{
    /* (the charge declared first: MW's numbering gives it the register after this's) */
    PlacedExplosive *pe;
    CPlacedExplosiveList &list = CZWeapon::m_placedExplosives;
    int n = list.Count();

    for (int i = 0; i < n; i++) {
        pe = list.Get(i);

        if (pe->m_time <= 0.0f) {
            CZAmmo *ammo = pe->m_ammo;
            CPnt3D pos = pe->m_framestartpos;
            CPnt3D d = pos - m_posW;

            if (d.x * d.x + d.y * d.y + d.z * d.z < ammo->GetProximityDistance()) {
                pe->RemoteDetonate();
                list.Remove(pe);
                i--;
                n--;
            }
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

/* the state a SEAL's controller sends over the network, packed into a word through memcpy (SOCOM
 * II's 4 bytes: remote_control.cpp reads the jump first; CRemoteCtrl::SetAiNetState reads the fourth
 * byte -- the node's fade in fifteenths, 15 hidden, and bit 5 a friend's compass marker; the names
 * ours) */
struct SEAL_NET_STATE {
    bool m_jump;   // 0x0
    unsigned char _1[2];
    unsigned char m_fade : 4;   // 0x3
    unsigned char m_bit4 : 1;
    unsigned char m_friend : 1;
    unsigned char m_bit6 : 1;
    unsigned char m_bit7 : 1;

    void SetBit6(bool on) { m_bit6 = on; }
};

extern "C" float floorf(float);

/* a node's fade in fifteenths: 15 times the faded part when it is shown, else none (a helper: the
 * target tests the shown flag again inside it. A ternary, an if/else or an early return all match;
 * `level = 0; if (active) level = ...` leaves the conversion's join a block of its own, +1 nop) */
static inline unsigned int FadeLevel(bool active, float opacity)
{
    return active ? (unsigned int)floorf(15.0f * (1.0f - opacity)) : 0;
}

extern "C" {
/* the top blend's held object let go (OVL2 0x5781C0, seal_body.cpp's), the overlay's last anim let go
 * (OVL2 0x5A5DA0), the SEAL off its ladder and its hold on one let go (OVL2 0x59D750, 0x5B14E0), an
 * anim type for a hit and a state (OVL2 0x5A0950: seal_env.cpp's) and the anim pushed on the body
 * (OVL2 0x588BC0, seal_body.cpp's) */
void func_005781C0(CZSealBody *seal);
void func_005A5DA0(CZSealBody *seal);
void func_0059D750(CZSealBody *seal);
AnimType func_005A0950(CZSealBody *seal, DAMAGE_LOCATION loc, SEAL_STATE state);
void *func_00588BC0(CZSealBody *seal, AnimType type, int state, int flag, int kind);
}

/* SOCOM 1's CAiTimer as the controller's weapon timer is read here: an Rfloat-shaped period (its
 * operator float inline, an early return and the random factor through a local) and the countdown,
 * restarted and run out (an infinite one left as drawn). Each inline works through a local copy of
 * `this`: the target keeps the timer's base in s1 across rand() -- the range's first read folded off
 * the controller -- and takes it again for the run-out's store; on `this` itself MW bases the
 * accesses on the range's and the countdown's own addresses */
struct WEP_PERIOD {
    operator float() const
    {
        if (m_range == 0.0f)
            return m_min;
        float r = 4.656613e-10f * (float)rand();

        return m_min + m_range * r;
    }

    float m_min;   // 0x0
    float m_range;   // 0x4
};

struct WEP_TIMER {
    void Restart()
    {
        WEP_TIMER *t = this;

        t->m_timer = t->m_period;
    }
    void RunOut()
    {
        WEP_TIMER *t = this;

        if (t->m_timer != CAiTimer::infinite)
            t->m_timer = 0.0f;
    }

    WEP_PERIOD m_period;   // 0x0
    float m_timer;   // 0x8
};

/* online, a SEAL whose network life is not up for more than 20 s ended: killed while alive (DEATH_MAX),
 * its hold, body, overlay, ladder and climb let go, unstunned, the weapon in state 2, the controller's
 * weapon timer restarted and run out, the weapon's envelope cleared, the blends cleared to the base
 * and the hit anim of location 3 pushed in state 8 -- the body set to state 8, or with bit 10 of
 * 0x105C the body's two interpolations zeroed and the blend's fraction full -- the network life 3
 * and bit 13 of 0x1060 set; then the time run on (OVL2 0x543F70, new in SOCOM II: CZSealBody::PreTick
 * calls it; the name unknown). Byte-exact (the SEALs, round 49): the network life a byte enum read
 * into a local ahead of the network test (the target's `lb` in the first block, its `andi 0xff` at
 * the compare); SetHealth inlined under an alive test of its own (the bit read once) */
extern "C" void func_00543F70(CZSealBody *seal)
{
    NET_LIFE life = seal->m_netlife;

    if (theNetwork.m_9 && life != NETLIFE_UP) {
        if (seal->m_f7c > 20.0f) {
            if (seal->m_isAlive)
                seal->SetHealth(0.0f, DEATH_MAX);
            func_005781C0(seal);
            seal->DropBody();
            func_005A5DA0(seal);
            func_0059D750(seal);
            func_005B14E0(&seal->m_climb, seal);
            seal->m_am_stunned = false;
            seal->SetWeaponState((WEAPON_STATE)2);

            CSealCtrl *ctrl = (CSealCtrl *)seal->m_control;

            if (ctrl) {
                ((WEP_TIMER *)&ctrl->m_wep_state)->Restart();
                ((WEP_TIMER *)&ctrl->m_wep_state)->RunOut();
            }
            seal->m_wep_seq.Clear();
            seal->ClearToBase(false);

            CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00588BC0(seal,
                func_005A0950(seal, (DAMAGE_LOCATION)3, (SEAL_STATE)2), 8, 0, 13);

            if (!seal->m_limp)
                seal->BodySetState(8);
            else {
                seal->m_interp1 = 0.0f;
                seal->m_interp2 = 0.0f;
                b->m_tfrac = 1.0f;
            }
            if (seal->m_netlife != NETLIFE_DEAD) {
                seal->m_netlife = NETLIFE_DEAD;
                seal->IsRemote();
            }
            seal->m_1060_bit13 = true;
        }
        seal->m_f7c += seal->m_2e0;
    }
}

extern "C" {
/* the wall hit the SEAL keeps (0x1120 on) taken, or its node cleared for none (OVL2 0x5B0BB0, seal_env.cpp's
 * range, a draft; the name ours) */
void func_005B0BB0(CZSealBody *seal, const zdb::IntersectStruct *hit);
/* (OVL2 0x5B3890, seal_env.cpp's) */
void func_005B3890(CZSealBody *seal, int drop_now);
}

/* PushOut, saying whether it moved the value (an inline, the name ours) */
static inline void PushOut(float &cur, float v, bool &moved)
{
    if (v >= 0.0f) {
        if (cur < v) {
            cur = v;
            moved = true;
        }
    } else if (v < 0.0f) {
        if (cur > v) {
            cur = v;
            moved = true;
        }
    }
}

/* the push off a wall ahead: 0x54A6D0's with the point raised 5 and a step of 10 into the hit's plane
 * -- what of it the plane cuts off within 1.3, 0.6 of it along the normal, taken into m_f44's x and z
 * (0x5B3890 asked with 1 when either moved), the weapon put in state 2 and the controller's weapon
 * timer restarted and run out (0x543F70's), the kept wall hit cleared when the push is under 0.1 or
 * none is cut off; false with no node hit (OVL2 0x54A3D0, new in SOCOM II; the name unknown).
 * Byte-exact (round 55; OPEN at one instruction since round 51): the step's product is `r * (float)(1.3f
 * - t) * 0.6f` -- the target multiplies the step first (`mul.s $f1,$f1,$f2`) yet numbers the depth
 * first. MW numbers a binary operator's heavier operand first and emits the lighter first; a (float)
 * cast of a float is a node of its own that costs no code, so the cast makes the depth the heavier
 * without the named depth's or the comma's extra copy (`r * (1.3f - (float)t)` matches too). Rounds
 * 51-54 tried the depth named, `(t, 1.3f - t) * r`, a variable limit, inlines, `-t + 1.3f`, `+r`, `/ 1.0f`
 * and every operand order (build/scratch/seal-r51..r54/v_3d*.py, seal-r55/v_3d2.py) */
extern "C" bool func_0054A3D0(CZSealBody *seal, const CPnt3D *pos, zdb::IntersectStruct *hit)
{
    if (!hit->m_node)
        return false;
    float r = 10.0f;
    CPnt3D p = *pos;

    p.y += 5.0f;
    CPnt3D q = p;
    CPnt3D n = hit->m_norm;

    n.Scale(-r, &n);
    q.Add(&n, &q);
    q.y = p.y;
    float t = LineIntersectPlane(PlaneConstant(hit->m_norm, hit->m_pos), hit->m_norm, p, q);

    if (t < 1.3f) {
        CPnt3D push = hit->m_norm;
        bool moved;

        push.Scale(r * (float)(1.3f - t) * 0.6f, &push);
        moved = false;
        PushOut(seal->m_f44.x, push.x, moved);
        PushOut(seal->m_f44.z, push.z, moved);
        if (moved)
            func_005B3890(seal, 1);
        seal->SetWeaponState((WEAPON_STATE)2);
        CSealCtrl *ctrl = (CSealCtrl *)seal->m_control;

        ((WEP_TIMER *)&ctrl->m_wep_state)->Restart();
        ((WEP_TIMER *)&ctrl->m_wep_state)->RunOut();
        if (Dot(push, push) < 0.01f)
            func_005B0BB0(seal, 0);
    } else
        func_005B0BB0(seal, 0);
    return true;
}

/* the SEAL shown again, whole: its node opaque and active, the state sent with it, and order 31 given
 * to team 24's squad (OVL2 0x543B90, new in SOCOM II: CMPExitState::UnInit calls it for the player
 * once the ghost flag is cleared; the name unknown) */
extern "C" void func_00543B90(CZSealBody *seal)
{
    zdb::CNode *node = seal->GetNode();

    node->m_Opacity = 1.0f;
    node->SetActive(true);

    SEAL_NET_STATE st;
    unsigned int zero = 0;
    unsigned int state;

    memcpy(&st, &zero, sizeof(st));
    bool active = node->m_active;
    float opacity = node->m_Opacity;

    if (active && opacity > 0.0f)
        st.m_fade = FadeLevel(active, opacity);
    else
        st.m_fade = 15;
    st.SetBit6(false);
    memcpy(&state, &st, sizeof(st));
    ((CSealCtrl *)seal->m_control)->SetAiNetState(state);

    CSealUnit *unit = CSealUnit::GetUnitByTeam(24);

    if (unit)
        unit->IssueCommand(seal, (FT_COMMAND)31, 0, false);
}

/* the SEAL made a ghost: dead as one (SetHealth inlined: 0, DEATH_GHOST), its body in state 8, its node
 * clear and inactive, the state sent with bit 6 set, and order 34 given to team 24's squad (OVL2 0x543D50,
 * new in SOCOM II: ftsNetworkPlayerInit calls it three times and ReCreate once -- 0x543B90's undoing;
 * the name unknown) */
extern "C" void func_00543D50(CZSealBody *seal)
{
    seal->SetHealth(0.0f, DEATH_GHOST);
    seal->BodySetState(8);
    zdb::CNode *node = seal->GetNode();

    node->m_Opacity = 0.0f;
    node->SetActive(false);

    SEAL_NET_STATE st;
    unsigned int zero = 0;
    unsigned int state;

    memcpy(&st, &zero, sizeof(st));
    bool active = node->m_active;
    float opacity = node->m_Opacity;

    if (active && opacity > 0.0f)
        st.m_fade = FadeLevel(active, opacity);
    else
        st.m_fade = 15;
    st.SetBit6(true);
    memcpy(&state, &st, sizeof(st));
    ((CSealCtrl *)seal->m_control)->SetAiNetState(state);

    CSealUnit *unit = CSealUnit::GetUnitByTeam(24);

    if (unit)
        unit->IssueCommand(seal, (FT_COMMAND)34, 0, false);
}

/* the message queue (fts_message.cpp's C2DMessage_Q: only its line add, OVL1 0x2B6530 -- OVL1 bss
 * 0x4366A0, the queue) and the message kind its colour is indexed by (fts_state.h's MSGTYPE) */
enum MSGTYPE {
    MSGTYPE_PLAYER = 0,
    MSGTYPE_SYSTEM = 1,
    MSGTYPE_ALERT = 5
};
class C2DMessage_Q {
public:
    void Add_Message(const char *text, MSGTYPE type, bool centered, int time, float scale);
};

extern "C" {
extern unsigned char data_004366A0[];
}

/* a remote message played as from an entity (OVL1 0x5E7F20's, the kit's: PlayRemoteMsg__FP7CEntityii) */
void PlayRemoteMsg(CEntity *from, int msg, int flags);

/* a suicide's and a frag's lines (inlines: the names are taken ahead of the weapon's name and kept in
 * saved registers across that call -- the suicide's as a by-value parameter, the frag's as the inline's
 * locals, the victim's first: as parameters the frag's two load in one order and number in the other) */
static inline void SuicideLine(char *buf, const char *name, EQUIP_ITEM w)
{
    sprintf(buf, "%s commits suicide with %s", name, CZWeapon::GetWeaponName(w));
}

static inline void FragLine(char *buf, const CEntity *killer, const CEntity *victim, EQUIP_ITEM w)
{
    const char *v = victim->m_display_name;
    const char *k = killer->m_display_name;

    sprintf(buf, "%s fragged %s with %s", k, v, CZWeapon::GetWeaponName(w));
}

/* an entity's team bits by value (an inline: two inline calls expand left to right, so the player's
 * load comes first, as the target's) */
static inline unsigned int TeamMask(const CEntity *e)
{
    return e->m_team;
}

/* the victim's death told, online: none without a killer or a kind; a SEAL of team bit 17 plays message
 * 68; else the kill's line shown -- a fall, a suicide or a frag with its weapon -- and, unless the
 * victim is the player or its state's bit 6 is set, message 40 played for a teammate of the player's,
 * else 39 from the killer (OVL2 0x547860, new in SOCOM II: the CQB and knife attacks call it, 0x547A90
 * after setting the kind -- the name unknown) */
extern "C" void func_00547860(CZSealBody *victim)
{
    if (!theNetwork.m_9)
        return;
    if (victim->m_lastkiller_id == 255)
        return;
    if (victim->m_fc0 <= 0)
        return;
    if (victim->m_team & 0x20000) {
        PlayRemoteMsg(victim, 68, 0);
        return;
    }

    CEntity *e = CEntity::GetEntityList().GetEntityById(victim->m_lastkiller_id);
    int kind = victim->m_fc0;
    CZSealBody *killer = e ? AsSeal(e) : 0;
    char buf[256];

    if (e && killer) {
        if (kind == 253)
            sprintf(buf, "%s falls to their death", killer->m_display_name);
        else if (killer == victim)
            SuicideLine(buf, killer->m_display_name, (EQUIP_ITEM)kind);
        else
            FragLine(buf, killer, victim, (EQUIP_ITEM)kind);
        ((C2DMessage_Q *)data_004366A0)->Add_Message(buf, MSGTYPE_PLAYER, false, 0, 0.0f);
    }
    if (ftsGetPlayer() == victim)
        return;

    SEAL_NET_STATE st;
    unsigned int state = ((CSealCtrl *)victim->m_control)->GetAiNetState();

    memcpy(&st, &state, sizeof(st));
    if (st.m_bit6)
        return;
    if (TeamMask(ftsGetPlayer()) & TeamMask(victim))
        PlayRemoteMsg(victim, 40, 0);
    else if (killer)
        PlayRemoteMsg(killer, 39, 0);
}

/* the entity the SEAL works as a turret, if it is one */
static inline CTurret *AsTurret(CEntity *e)
{
    return e && e->m_type == CEntity::ENT_TYPE_TURRET ? (CTurret *)e : 0;
}

inline CTurret *CZSealBody::GetTurret(CEntity *e)
{
    CEntity *v = e ? e : m_eb8;

    return v ? AsTurret(v) : 0;
}

inline CTurret *CZSealBody::GetTurret()
{
    return m_eb8 ? AsTurret(m_eb8) : 0;
}

extern "C" {
/* whether the top blend is one of two anims (OVL2 0x588030, seal_body.cpp's range, its draft's) and the
 * SEAL put on a turret offline (OVL2 0x5463C0, below) */
bool func_00588030(CZSealBody *seal);
bool func_005463C0(CZSealBody *seal, CEntity *e);
/* two of the weapon changes' anims (OVL1 data: seal_env.cpp's IsChangingWeapons reads them too) */
extern short data_003DF108;
extern short data_003DF110;
}

/* the network's platform message (OVL1's CZPlatformState: only the turret's send here) */
class CZPlatformState {
public:
    static void SendPlatform(CTurret *t);
    /* (OVL1 0x2C7A60, fts_net_platform.cpp's) */
    static void UpdateLocalPlatform(CTurret *t, bool b);
};

extern "C" {
/* the kit told of the death (OVL2 0x5B8920, seal_weapon.cpp's) and the entity's sounds let go, by its
 * id (OVL1 0x3428A0, zsnd's range) */
void func_005B8920(CZKit *kit);
void func_003428A0(unsigned int id);
/* (OVL1 0x30E6E0, unowned: told of a death online, with the global at 0x45B718; seal_control.cpp's
 * too) */
void func_0030E6E0(void *obj, int a);
extern void *data_0045B718;
extern float data_004365C0;
void func_00545EE0(CZSealBody *seal, int force);
void func_0059D750(CZSealBody *seal);
void func_005B14E0(CZSealBody::CSealClimb *climb, CZSealBody *seal);
}

/* the camera's zoom (thezoom.cpp's) */

/* dead: the kit told, the entity's own death, its sounds let go, whatever it worked left, its map
 * cell free; whoever followed it online forgets it; the time kept, the climb and the carried let go; a
 * local player's view and heartbeat reset and the rumble's last kick; online the bomb dropped, the
 * team valves refreshed; a remote SEAL's weapons hidden, a local one's net life over; the compass's
 * anchor faded or dropped (OVL2 0x547AF0; SOCOM 1's OnDeath__10CZSealBodyFv, its followers, kit and
 * lens moves new) */
void CZSealBody::OnDeath()
{
    func_005B8920(&m_kit);
    CEntity::OnDeath();
    func_003428A0(m_id);
    if (m_eb8)
        func_00545EE0(this, 1);
    AiMaps.SetOccupied(m_mapLocValid, CAiMapLoc::invalid, m_id);
    if (m_followers) {
        const CEntityList &list = CEntity::m_list;

        for (CEntityList::const_iterator i = list.begin(); i != list.end(); ++i) {
            CZSealBody *s = AsSeal(*i);
            CSealCtrlAi *ai = s ? s->GetSealCtrlAi() : 0;

            if (ai && (ai->m_stateStack.m_10 & 0x200) && ai->GetFollowMaster(false) == this)
                ai->SetFollowMaster(0);
        }
    }
    m_time_of_death = data_004365C0;
    func_0059D750(this);
    func_005B14E0(&m_climb, this);
    m_1060_bit12 = true;
    if (IsPlayer() && !IsRemote()) {
        CSealCtrl *ctrl = (CSealCtrl *)m_control;

        if (ctrl) {
            m_kit.Apply_LensEffects(false, 0);
            m_kit.NVG_Enabled(false);
            m_kit.Binocs_Enabled(false);
            SeeFarther(false, 0.0f);
            CameraZoom::SetLensFx(1);
            StopAllHeartbeatSounds();
            ctrl->StopRumbler();
            CZWeapon::ClearTimer(0);
            m_iszooming = false;
            SetCurZoomState((ZOOMSTATE)0);
            appCamera->SetZoom(1.0f);
        }
        CameraZoom::ResetLensColors();
        CSnd::OnListenerDeath();
        if (theNetwork.m_9)
            func_0030E6E0(data_0045B718, 0);
        else
            Headset::ftsGetHeadset()->Interrupt();
        SetCurZoomState((ZOOMSTATE)0);
        ctrl = (CSealCtrl *)m_control;
        if (ctrl) {
            ctrl->StopRumbler();
            ctrl->SetLowRumbleTime(0.18f, 175.0f);
            ctrl->SetHighRumbleTime(0.04f);
        }
    }
    if (theNetwork.m_9 && IsPlayer())
        DropMPBomb();
    theMission.RefreshTeamValves();
    if (IsRemote()) {
        CZBodyPart *p = m_item_gear[1].m_part;

        if (p && p->m_node)
            p->m_node->SetActive(false);
        p = m_item_gear[2].m_part;
        if (p && p->m_node)
            p->m_node->SetActive(false);
        p = m_item_gear[4].m_part;
        if (p && p->m_node)
            p->m_node->SetActive(false);
    }
    if (!IsRemote() && m_netlife != (NET_LIFE)2) {
        m_netlife = (NET_LIFE)2;
        IsRemote();
    }
    if (m_compass_anchor && !theNetwork.m_9 && (m_team & 2))
        m_compass_anchor->SetPoint(0);
    else {
        CCompass::DeleteAnchor(m_compass_anchor);
        m_compass_anchor = 0;
    }
}

/* whether an entity has a controller (an inline bool written `? true : false`: the target
 * materialises it, sltu, before the branch) */
static inline bool HasController(const CEntity *e)
{
    return e->m_control ? true : false;
}

/* whether the SEAL's top blend is in state 8 (an inline: the call and the compare come ahead of
 * the `||` it is in) */
static inline bool TopBlendIn8(CZSealBody *seal)
{
    return ((CZBodyAnimBlend *)func_00555170((CIndexedArray *)&seal->m_blends))->m_state == 8;
}

/* a turret boarded when it may be: one no one works, the SEAL alive and its top blend not in state
 * 8, none of 0x588030's anims nor the two weapon changes', nothing worked or carried -- online the
 * platform sent, offline 0x5463C0 boards it; whether it was (OVL2 0x546210, new in SOCOM II:
 * CSealCtrl::OnActionButton and CAiSUseVehicle::Board call it; the name unknown) */
extern "C" bool func_00546210(CZSealBody *seal, CEntity *e)
{
    bool ok = true;
    CTurret *t = AsTurret(e);

    if (t && HasController(t))
        ok = false;
    /* (a value of its own: the target makes the call and the compare ahead of the alive test) */
    bool in8 = TopBlendIn8(seal);

    if (!seal->m_isAlive || in8)
        ok = false;
    if (func_00588030(seal))
        ok = false;
    if (ok && (!e || seal->m_eb8))
        ok = false;
    if (ok && (seal->IsCurrentBlend((AnimType)data_003DF108) || seal->IsCurrentBlend((AnimType)data_003DF110)))
        ok = false;
    if (seal->m_carry)
        ok = false;
    if (ok) {
        if (theNetwork.m_9) {
            CTurret *turret = AsTurret(e);

            if (turret)
                CZPlatformState::SendPlatform(turret);
        } else
            func_005463C0(seal, e);
    }
    return ok;
}

extern "C" {
/* the SEAL's weapon put away for the turret's (OVL2 0x5A6FC0, seal_env.cpp's range, its draft's: true
 * when the blends are to be homed onto the turret; the name unknown) */
bool func_005A6FC0(CZSealBody *seal, char item, int n);
/* an anim's homing point, zero when it has none (OVL2 0x58C2F0, its draft's; the name unknown) */
bool func_0058C2F0(CZSealBody *seal, CZSIScriptCommon *anim, CPnt3D *out);
/* CPnt3D's implicit assignment, OVL2's copy (0x539E90: the homing's points are set through it) */
CPnt3D *func_00539E90(CPnt3D *p, const CPnt3D *src);
}

/* the SEAL put on a turret: refused with no entity or one worked already, dead or with the top blend in
 * state 8, while 0x588030's anims or a weapon change play on this console, or carrying -- refused online,
 * the platform sent back for a turret; else the turret given the SEAL's controller and the camera's
 * follow, the grab sound, for a turret a network wait of 1.25 seconds, the kit's item kept and the
 * turret's weapon handed in, its scope's look and the view by its thermal sight, the weapon put away and
 * the top blend homed onto the turret's front, and the entity kept (OVL2 0x5463C0, new in SOCOM II:
 * 0x546210 boards it offline; the name unknown).
 * OPEN, 0x5F4 against 0x5FC (round 53, unwritten before; every call, constant and block the target's):
 * (1) the turret tests -- the target MATERIALISES `e->m_type == 3` (`xori; sltiu; beqz`, then the
 * triangle `daddiu 1`) in all three (the platform's, the boarding's and HasThermal's inlined one), as
 * the UI's round-31 function-wide mode does, where AsTurret here (and in 0x546210 / 0x545EE0, which
 * match) branches `li 3; bne` -- an IsTurretEnt bool inline under the ternary lengthens it (0x604);
 * (2) the saved registers: the SEAL s4, the entity s3, ok s2 in the target, s2 / s1 / s0 here. Right:
 * the matrix read (GetPosW) before the unit_nz copy, TransformVectors (VU0) in place, the position
 * through CNode::GetPosition (the copy stored through one pointer), the homing's five points through
 * CPnt3D's OVL2 assignment copy and the target point stored last through the blend */
extern "C" bool func_005463C0(CZSealBody *seal, CEntity *e)
{
    bool ok = true;

    if (ok && (!e || seal->m_eb8))
        ok = false;
    bool in8 = TopBlendIn8(seal);

    if (ok && (!seal->m_isAlive || in8))
        ok = false;
    if (ok && func_00588030(seal) && !seal->IsRemote())
        ok = false;
    if (ok && (seal->IsCurrentBlend((AnimType)data_003DF108) || seal->IsCurrentBlend((AnimType)data_003DF110)) &&
        !seal->IsRemote())
        ok = false;
    if (seal->m_carry)
        ok = false;
    if (!ok && theNetwork.m_9 && !seal->IsRemote()) {
        CTurret *t = AsTurret(e);

        if (t)
            CZPlatformState::UpdateLocalPlatform(t, true);
    }
    if (ok) {
        e->m_control = seal->m_control;
        if (appCamera->m_bc == seal) {
            appCamera->Follow(e, 0);
            CAppCamera *cam = appCamera;

            cam->m_45 = 1;
            cam->m_46 = 1;
            cam->m_48 = 0;
        }
        CSnd *snd = CSnd::GetSoundByName(".GRAB_TURRET");

        if (snd)
            snd->PlaySound3D(&seal->m_posW, 1.0f, 0, 0, 255);
        CTurret *t = AsTurret(e);

        if (t) {
            if (theNetwork.m_9)
                seal->m_22c = theNetwork.zNetGetTime() + 1250;
            seal->m_ebc = seal->m_kit.GetSelectedItemNum();
            int n = seal->m_kit.GetNumItems();

            seal->m_kit.SetItem(n, t->m_weapon, t->m_ammoType, t->m_ammo);
            seal->m_kit.SetCurItemByIndex(n);
            bool thermal = seal->HasThermal(e);

            t->SetThermal(0, thermal);
            if (seal->m_zoomstate != ZOOMSTATE_3RDPERSON) {
                if (thermal) {
                    t->SetVisible(false);
                    seal->SetCurZoomState(ZOOMSTATE_THERMAL);
                } else
                    seal->SetCurZoomState(ZOOMSTATE_1STPERSON);
            }
        }
        if (func_005A6FC0(seal, seal->m_item, 0)) {
            CZBodyAnimBlend *b = (CZBodyAnimBlend *)func_00555170((CIndexedArray *)&seal->m_blends);
            CPnt3D target = CPnt3D::zero;
            CPnt3D refpt;

            func_0058C2F0(seal, b->Representative()->m_anim, &refpt);
            refpt.x = -refpt.x;
            refpt.y = -refpt.y;
            refpt.z = -refpt.z;
            const CMatrix *m = e->GetPosW();
            CPnt3D norm = CPnt3D::unit_nz;

            TransformVectors(m, &norm, &norm, 1);
            norm.y = 0.0f;
            norm.Normalize();
            CPnt3D pos = e->m_node->GetPosition();
            CHoming *h = b->Homing();

            h->m_blend = b;
            func_00539E90(&h->m_refpt_m, &refpt);
            func_00539E90(&h->m_norm_w, &norm);
            func_00539E90(&h->m_int_w, &pos);
            func_00539E90(&h->m_refvec_m, &CPnt3D::unit_nz);
            func_00539E90(&h->m_target_refpt_m, &CPnt3D::zero);
            h->m_1c = 0;
            h->m_2c = 0;
            b->m_homing.m_target_refpt_m = target;
        }
        seal->m_eb8 = e;
    }
    return ok;
}

extern "C" {
/* the SEAL's rifle or pistol drawn again by the item kept (OVL2 0x5A6EB0, seal_env.cpp's range,
 * unwritten: it pushes 0x3DF108 or 0x3DF110 and sets m_item; the name unknown -- the item a plain
 * char: the caller loads m_item with lb, an unsigned char or int parameter gives lbu) */
void func_005A6EB0(CZSealBody *seal, char item, int n);
}

/* the SEAL off the turret it works: refused while one of the two weapon changes plays on a live SEAL
 * of this console, unless forced (or 0x588030's anims play) -- the release sound played, the turret
 * let go and, if the SEAL was its gunner, its controller cleared, the camera given back the SEAL
 * (faded back in, priority 1), the turret's rounds handed back from the gunner's kit and the old item
 * selected, online the platform sent, the thermal view left, the turret shown and its scope's look
 * reset, the weapon drawn again (OVL2 0x545EE0, new in SOCOM II: seal_body.cpp's Stand / the carry and
 * seal_env.cpp's carry release call it with 1; the name unknown). Byte-exact (round 53, unwritten
 * before): 0x552780's fade inlined; the camera's three stores through a local read after the call (as
 * `appCamera->` each store reloads the global) */
extern "C" void func_00545EE0(CZSealBody *seal, int force)
{
    bool ok = true;

    if ((seal->IsCurrentBlend((AnimType)data_003DF108) || seal->IsCurrentBlend((AnimType)data_003DF110)) &&
        seal->m_isAlive && !func_00588030(seal) && !seal->IsRemote() && !force)
        ok = false;
    if (ok && seal->m_eb8) {
        CSnd *snd = CSnd::GetSoundByName(".RELEASE_TURRET");

        if (snd)
            snd->PlaySound3D(&seal->m_posW, 1.0f, 0, 0, 255);
        CEntity *e = seal->m_eb8;

        seal->m_eb8 = 0;
        if (e->m_control == seal->m_control) {
            e->m_control = 0;
            if (appCamera->m_bc == seal) {
                appCamera->Follow(seal, 0);
                CAppCamera *cam = appCamera;

                cam->m_45 = 1;
                cam->m_46 = 1;
                cam->m_48 = 0;
                func_00552780(seal, 1, 1.0f, 1.0f);
            }
            CTurret *t = AsTurret(e);

            if (t) {
                int item = seal->m_kit.GetSelectedItemNum();

                t->m_ammo = seal->m_kit.GetAmmoTotal(item);
                seal->m_kit.SetCurItemByIndex(seal->m_ebc);
                seal->m_kit.RemoveItemAndAmmo(item);
                if (theNetwork.m_9 && !seal->IsRemote())
                    CZPlatformState::UpdateLocalPlatform(t, true);
                if (seal->m_zoomstate == ZOOMSTATE_THERMAL)
                    seal->SetCurZoomState(ZOOMSTATE_1STPERSON);
                t->SetVisible(true);
                t->SetThermal(0, false);
            }
            func_005A6EB0(seal, seal->m_item, 0);
        }
    }
}

/* whether the SEAL sees thermally: with nothing worked, by the goggles (item 195); with a turret, by
 * its thermal scope (OVL2 0x5433C0) */
bool CZSealBody::HasThermal(CEntity *e)
{
    bool r = false;

    if (!e) {
        if (m_kit.HasItem((EQUIP_ITEM)195))
            r = true;
    } else {
        CTurret *t = GetTurret(e);

        if (t && t->m_thermal)
            r = true;
    }
    return r;
}

/* whether the SEAL works a turret with a thermal scope (OVL2 0x5432F0). The target's inlined
 * HasThermal keeps GetTurret's fallback (`beqz a1; b; lw a1,0xEB8(a0)`) though the entity passed is
 * the 0xEB8 it just loaded -- here and in IncrZoomState's copy, where UpdateTurretScope's copy folds
 * it away -- so the entity is read through a volatile (a STAND-IN: MW then cannot see the two loads
 * equal; a volatile fallback in GetTurret matched these two and lost UpdateTurretScope) */
bool CZSealBody::TurretHasThermal()
{
    bool r = false;
    CEntity *e = *(CEntity *volatile *)&m_eb8;

    if (e && HasThermal(e))
        r = true;
    return r;
}

/* the turret the SEAL works given the value, its scope's look set by the SEAL's thermal vision
 * (OVL2 0x543200; HandleNewZoom passes 1) */
void CZSealBody::UpdateTurretScope(float f)
{
    CTurret *t = GetTurret();

    if (t) {
        t->SetOpacity(f);
        t->SetThermal(0, HasThermal(m_eb8));
    }
}

/* the AI's target (ai_target.cpp's CAiTarget: only its entity and the sighting clock here) and the
 * test the AI's code makes of it (OVL2 0x60A850, unowned: an entity, alive, and an AI's unless it
 * holds bit 8 or 9) */
class CAiTarget {
public:
    float GetTimeSinceLastSighting() const;

    CEntity *m_entity;   // 0x0
};

extern "C" bool func_0060A850(void *target);

/* the AI's target held, with its bit 9 (an inline bool, as seal_ai.cpp's: the `&&` materialised) */
static inline bool TargetHeld(CSealCtrlAi *ai)
{
    return func_0060A850(ai->m_67c) && (ai->m_6a8 & 0x200);
}

/* (OVL2 0x543030) */
void CZSealBody::UpdateHostileAnchor()
{
    if (!(m_team & 2))
        return;
    if (IsPlayer())
        return;

    bool shown;
    CSealCtrlAi *ai = ((CSealCtrl *)m_control)->GetSealCtrlAi();

    shown = false;
    if (ai) {
        bool lost = true;

        /* (an if/else on the negated test: the held arm last, the `&&` materialised) */
        if (func_0060A850(ai->m_67c)) {
            if (!TargetHeld(ai))
                lost = ((CAiTarget *)ai->m_67c)->GetTimeSinceLastSighting() > 3.0f;
            else
                lost = false;
        }
        if (!lost) {
            CEntity *e = ((CAiTarget *)ai->m_67c)->m_entity;

            if (e && !(e->m_team & 2)) {
                const CPnt3D &pos = e->m_posW;

                if (!m_hostile_anchor) {
                    m_hostile_anchor = CCompass::CreateAnchor(pos, "HOSTILE ACQUIRED");
                    m_hostile_anchor->m_type = 10;
                } else
                    m_hostile_anchor->m_posW = pos;
                shown = true;
            }
        }
    }
    if (!shown && m_hostile_anchor) {
        CCompass::DeleteAnchor(m_hostile_anchor);
        m_hostile_anchor = 0;
    }
}
