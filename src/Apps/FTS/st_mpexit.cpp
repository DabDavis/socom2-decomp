/* st_mpexit.cpp -- the end of an online game: the stats screen (SOCOM 1's
 * Apps/FTS/st_mpexit.cpp; OVL1 0x223290-0x2265F7, its initialiser 0x3FE860). SOCOM 1's order
 * holds: LoadLibs, UnInit, ~CMPExitMenu, Tick, Init and the constructor, then the anonymous
 * namespace's stat writers, the accessors whose addresses they take (SOCOM 1's
 * GetTimeInGameSec/GetTimeInRoundSec, GetGameStats/GetRoundStats, GetHitPct and the team totals'
 * three), the seal comparators, GetReaderFileName and std::sort's instance, which closes the unit.
 * All written (round 21): the initialiser, the comparators, the player's and the MVP's stat
 * writers, the team totals, SetFinalStatsUiVars, the accessors, LoadLibs, the constructor,
 * SetStatLbUiVar (0x224210) with std::sort's instance over the vector's iterator (0x226060,
 * __sort132 0x226600, __selection_sort 0x2267C0: the raw-pointer sort SOCOM 1 had would be 0x3B4
 * bytes, not 0x598), SetMpStatUiVars, CNetClock's two time accessors (0x224BF0/0x224C80), the round
 * result (0x224D10), and round 21's UnInit, Tick, Init, UpdateCareerRecord (0x225070: the career
 * record, 0x2F98F0/0x2F96F0) and ComputeNewRating (0x2253C0: mp_rating.cpp's Elo match). Open:
 * Init, GetRoundResult, SetMpStatUiVars, SetFinalTeamTotalsUiVar (each says how far;
 * ComputeNewRating byte-exact since round 53). Defined in reverse of their image order (-inline deferred). */

#include "fts_state.h"
#include "../../gamez/zrdr/zrdr.h"

#include <stdio.h>
#include <string.h>
#include <list>
#include <vector>
#include <algorithm>
#include <iostream>


/* a SEAL's stats (SOCOM 1's CSealStats, 0x54 bytes of ints; SOCOM II's counters are unsigned
 * shorts, 0x48 bytes: the SEAL lane's seal.cpp has the same view). The accessors are SOCOM 1's
 * const members, emitted here because the unit takes their addresses */
class CSealStats {
public:
    /* the three team totals (the names ours: the MVP score, then SOCOM 1's first two counters by
     * their place in the network packet -- unproven) */
    int GetMvpScore() const { return m_score; }
    int GetKills() const { return m_kills; }
    int GetDeaths() const { return m_deaths; }
    /* the hit percentage made a whole number, rounded (SOCOM 1's GetHitPct) */
    int GetHitPct() const { return (int)(0.5f + 100.0f * m_hit_percent); }
    /* the counters the career record sums (ours, by offset: their meanings unproven -- read
     * through accessors, each value is loaded ahead of the record's field it is added to) */
    int Get0() const { return m_0; }
    int Get14() const { return m_14; }
    int Get18() const { return m_18; }
    int Get1c() const { return m_1c; }
    int Get34() const { return m_34; }
    int Get36() const { return m_36; }
    int Get38() const { return m_38; }

    /* every counter (the names ours, by offset: their meanings unproven) */
    unsigned short m_0;   // 0x0
    unsigned short m_2;
    unsigned short m_4;   // 0x4: the hits (the hit percentage's numerator)
    unsigned short m_6;
    unsigned short m_8;   // 0x8: the shots (its denominator)
    unsigned short m_a;
    unsigned short m_kills;   // 0xc
    unsigned short m_e;
    unsigned short m_10;
    unsigned short m_deaths;   // 0x12
    unsigned short m_14;   // 0x14
    unsigned short m_16;   // 0x16
    unsigned short m_18;   // 0x18: added to the career record in objective games (unproven)
    unsigned short m_1a;
    unsigned short m_1c;   // 0x1c: summed over the SEALs by the round result (unproven)
    unsigned short m_1e;
    unsigned short m_20;
    unsigned short m_22;
    unsigned short m_24;
    unsigned short m_26;
    unsigned short m_28;
    unsigned short m_2a;
    unsigned short m_2c;
    unsigned short m_2e;
    unsigned short m_30;
    unsigned short m_32;   // 0x32: the rounds won (Init adds the round result's)
    unsigned short m_34;   // 0x34: the round result's score added
    unsigned short m_36;   // 0x36
    unsigned short m_38;   // 0x38: summed over the SEALs by the round result (unproven)
    unsigned short m_3a;   // 0x3a: the round result's score
    short m_score;   // 0x3c: the MVP score (SOCOM 1's m_sMvpScore)
    unsigned short m_3e;
    unsigned short m_40;
    unsigned short m_42;
    float m_hit_percent;   // 0x44

    /* another set added in, counter by counter (ours: an inline, its order the target's -- 0x20
     * ahead of 0x1c and 0x1e) */
    void Add(const CSealStats &o)
    {
        m_0 += o.m_0;
        m_2 += o.m_2;
        m_4 += o.m_4;
        m_6 += o.m_6;
        m_8 += o.m_8;
        m_a += o.m_a;
        m_kills += o.m_kills;
        m_e += o.m_e;
        m_10 += o.m_10;
        m_deaths += o.m_deaths;
        m_14 += o.m_14;
        m_16 += o.m_16;
        m_18 += o.m_18;
        m_1a += o.m_1a;
        m_20 += o.m_20;
        m_1c += o.m_1c;
        m_1e += o.m_1e;
        m_22 += o.m_22;
        m_24 += o.m_24;
        m_26 += o.m_26;
        m_28 += o.m_28;
        m_2a += o.m_2a;
        m_2c += o.m_2c;
        m_2e += o.m_2e;
        m_30 += o.m_30;
        m_32 += o.m_32;
        m_34 += o.m_34;
        m_36 += o.m_36;
        m_38 += o.m_38;
        m_3a += o.m_3a;
        m_score += o.m_score;
        m_3e += o.m_3e;
        m_40 += o.m_40;
        m_42 += o.m_42;
    }
    /* the hit percentage recomputed from the counters (ours) */
    void UpdateHitPct()
    {
        unsigned int shots = m_8;

        m_hit_percent = shots ? (float)(unsigned int)m_4 / (float)shots : 0.0f;
    }
};   // 0x48

/* a SEAL: its name, its two sets of stats (SOCOM 1's m_mp_stats and m_stats at 0x46C and
 * 0x4C0) and a byte that keeps it off the stats */
class CZSealBody {
public:
    /* the game's and the round's stats (SOCOM 1's accessors) */
    CSealStats &GetGameStats() { return m_game_stats; }
    CSealStats &GetRoundStats() { return m_round_stats; }

    unsigned char _0[0x14];
    const char *m_name;   // 0x14
    unsigned char _18[0xc8 - 0x18];
    unsigned int m_team;   // 0xc8 (CEntity's; 0x20000 a third side the round result counts)
    unsigned char _cc[0xe0 - 0xcc];
    unsigned int _e0 : 12;   // 0xe0: CEntity's flags
    unsigned int m_isAlive : 1;
    unsigned int _e0_13 : 19;
    unsigned char _e4[0x544 - 0xe4];
    CSealStats m_game_stats;   // 0x544
    CSealStats m_round_stats;   // 0x58c
    unsigned char _5d4[0xfb0 - 0x5d4];
    unsigned char m_fb0;   // 0xfb0: 6 a state the round's survival does not count (unproven)
    unsigned char _fb1[0xfc8 - 0xfb1];
    int m_fc8;   // 0xfc8: what its clan's tag is looked up by
    unsigned char _fcc[0xfd1 - 0xfcc];
    unsigned char m_fd1;   // 0xfd1: kept from respawning at the round's end (unproven)
    unsigned char m_fd2;   // 0xfd2

    void RemoteRespawn(bool b);
    void EnableMPBombGear(bool b);
};

/* a SEAL's clan tag, or none (unnamed yet) */
extern "C" const char *func_002C3AC0(int id);

/* a thing in the world (st_exit.cpp's stub: the list, the purge and the type byte) */
/* a SEAL's scene node (zdb's CNode: its vtable pointer at 0x60, only the slot at 0x38 called --
 * the name ours) */
class CSealNode {
public:
    unsigned char m_params[0x60];
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28();
    virtual void v2c();
    virtual void v30();
    virtual void v34();
    virtual void SetActive(bool b);   // 0x38 (the name ours)
};

class CEntity {
public:
    static void PurgeEntities();
    CSealNode *GetNode();

    static std::list<CEntity *> m_list;

    unsigned char _0[0x10];
    unsigned char m_type;   // 0x10: 2 a SEAL
};

/* the entity as a SEAL, if it is one (seal.cpp's AsSeal) */
inline CZSealBody *AsSeal(CEntity *e)
{
    return e && e->m_type == 2 ? (CZSealBody *)e : 0;
}

/* a walk over the world's entities, counting (SOCOM 1's DWARF; hud_scorepopup.cpp's view) */
class CEntityItr {
public:
    CEntityItr() : _it(GetEntityList().begin()), _iIdx(0) {}
    bool More() const { return _it != GetEntityList().end(); }

    CZSealBody &GetSeal() const;
    bool IsLiveSeal() const;
    bool IsSeal() const;
    void Advance();

    static const std::list<CEntity *> &GetEntityList() { return CEntity::m_list; }

    std::list<CEntity *>::const_iterator _it;   // 0x0
    int _iIdx;   // 0x4
};

/* whether a SEAL is on each side (the SEAL lane's, no names yet; bool, as 0x224D10's `andi 0xff`
 * on their results says) */
extern "C" {
bool func_002C31D0(const CZSealBody *seal);
bool func_002C30B0(const CZSealBody *seal);
}

/* the UI's variables (zui.h's, which zrdr.h's _zrdr cannot meet: only what is called) */
enum UIVAR_LONGEVITY {
    UIVL_DORMANT = 0,
    UIVL_TEMPORARY = 1,
    UIVL_PERMANENT = 2,
    UIVL_PERSISTENT = 3
};

class CUIVariable : public _zrdr {
public:
    void UpdateDependents();
    CUIVariable &operator=(const char *str);
    void Set(_zrdr *value);
    /* an integer, the watchers told only when it changes (ours: zui.h's float assignment's twin;
     * SOCOM 1 set the value unconditionally) */
    CUIVariable &operator=(int i)
    {
        if (type != ZRDR_INTEGER || integer != i) {
            DeleteChildren();
            type = ZRDR_INTEGER;
            integer = i;
            UpdateDependents();
        }
        return *this;
    }
};

class CUIVarManager {
public:
    CUIVariable *Add(const char *name, _zrdr *value, UIVAR_LONGEVITY longevity);
    void Reset();
};
extern CUIVarManager theUIVarManager;

CZSealBody *ftsGetPlayer();

/* the game's clock (SOCOM 1's CNetClock, 0x20 bytes there; SOCOM II has a word less ahead of the
 * round's time: its two time accessors read 8 and 16 where SOCOM 1's read 0xC and 0x14) */
class CNetClock {
public:
    /* the game's and the round's time in seconds (SOCOM 1's; SOCOM II inlines the conversion
     * SOCOM 1 called fptoui for). Emitted here, where SetMpStatUiVars takes their addresses:
     * 0x224BF0 and 0x224C80, in reverse of their first use */
    unsigned int GetTimeInGameSec() { return (unsigned int)(0.001f * m_total_time); }
    unsigned int GetTimeInRoundSec() { return (unsigned int)(0.001f * m_time_in_round); }
    /* every time cleared (ours: the target's order) */
    void Reset()
    {
        m_max_game_time = 0;
        m_game_time = 0;
        m_time_in_round = 0;
        m_4 = 0;
        m_total_time = 0;
    }

    unsigned int m_max_game_time;   // 0x0
    unsigned int m_4;   // 0x4
    unsigned int m_time_in_round;   // 0x8
    unsigned int m_game_time;   // 0xc
    unsigned int m_total_time;   // 0x10
};

/* the network game (SOCOM 1's CZNetGame, OVL1 bss 0x437CE8; hud_main.cpp's and st_load.cpp's
 * stand-ins): its clock (SOCOM 1's at 0xA0), the game's type and each side's rounds won (SOCOM 1's
 * at 0xEC/0xF0) */
class CZNetGame {
public:
    enum MP_MINOR_GAME_STATE {
        MP_MINOR_UNKNOWN = 0,
        MP_MINOR_NOTREADY = 1,
        MP_MINOR_READY = 2
    };

    MP_MINOR_GAME_STATE MyMinorGameState(MP_MINOR_GAME_STATE state);
    /* the clock and two flags reset for the next game (ours: an inline -- one load of the game's
     * pointer serves every store, as the target's) */
    void ResetForNextGame()
    {
        m_clock.Reset();
        m_f4 = 0;
        m_f5 = 1;
    }

    unsigned char _0[0x7c];
    int m_7c;   // 0x7c: cleared with the SEALs' team score valve (unproven)
    int m_80;   // 0x80: cleared with the terrorists'
    unsigned char _84[0xd2 - 0x84];
    unsigned char m_d2;   // 0xd2
    unsigned char _d3[0xdc - 0xd3];
    unsigned char m_dc;   // 0xdc: set, the stats screen rates no one (unproven)
    unsigned char _dd[0xe0 - 0xdd];
    CNetClock m_clock;   // 0xe0
    unsigned char m_f4;   // 0xf4
    unsigned char m_f5;   // 0xf5
    unsigned char _f6[0x111 - 0xf6];
    unsigned char m_type;   // 0x111: the game's type (1-5)
    unsigned char _112;
    unsigned char m_113;   // 0x113: the game's major state (st_load.cpp's MP_MAJOR_GAME_STATE: 1 the
                           // menus, 3 in play, 4 the round over, 5 a new round, 6 the game over)
    unsigned char _114[0x11c - 0x114];
    int m_11c;   // 0x11c: the rating match's rounds (unproven)
    int m_seals_won;   // 0x120
    int m_terrs_won;   // 0x124
    int m_128;   // 0x128: zero when the SEALs' team rates first (unproven)
};

extern CZNetGame *data_00437CE8;

/* whether the player is on each side (unnamed yet: the network game's team word against the
 * SEALs' and the terrorists' masks) */
extern "C" {
bool func_002C31A0();
bool func_002C3080();
}

/* whether a SEAL is out of the game's counting (the SEAL lane's, unnamed; st_exit.cpp's view),
 * whether the player has a rating to blend with (-1 none), and a SEAL's rating (fts_net_persona.cpp
 * defines it) */
extern "C" {
bool func_002C2FD0(CZSealBody *seal);
int func_002C2DC0(CZSealBody *seal);
int func_002C3500(CZSealBody *seal);
}

/* the player's career record: the lobby's account of the player (0xF0 bytes, read by 0x2F98F0 and
 * written back by 0x2F96F0 -- fts_net_persona.cpp's LobbyPlayerInfo; every name ours, the
 * counters' meanings unproven) */
struct CareerRecord {
    unsigned int m_0 : 8;   // 0x0
    unsigned int m_hit_pct : 7;   // 0x1: the hit percentage, averaged over the games
    unsigned int m_0_15 : 17;
    unsigned int m_rating;   // 0x4
    unsigned char _8[0x10 - 0x8];
    unsigned int m_time;   // 0x10: the seconds played
    int m_14;   // 0x14
    int m_18;   // 0x18
    int m_kills;   // 0x1c
    unsigned int m_games : 22;   // 0x20
    unsigned int m_20_22 : 10;   // 0x22 (the halfword's bits 6-15)
    unsigned short m_24;   // 0x24
    unsigned short m_26;   // 0x26
    unsigned short m_28;   // 0x28
    unsigned short m_deaths;   // 0x2a
    unsigned short m_2c;   // 0x2c
    unsigned short m_2e;   // 0x2e
    unsigned short m_30;   // 0x30
    unsigned short m_32;   // 0x32
    unsigned char _34[0xe8 - 0x34];
};   // 0xe8

extern "C" {
void func_002F98F0(CareerRecord *rec);
void func_002F96F0(CareerRecord *rec, int arg);
}

/* the network game's ratings (mp_rating.cpp's CRatingMatch: only what is called here) */
class CRatingMatch {
public:
    CRatingMatch();
    ~CRatingMatch();

    void *AddTeam(int id, int count, bool first, float rating);
    int AddPlayer(int team, int player, int score, float rating);
    float Compute();
    bool GetNewRating(int player, float *rating);

    void *m_teams[2];   // 0x0
    bool m_solo;   // 0x8
    bool m_done;   // 0x9
    int m_rounds;   // 0xc
    int m_k;   // 0x10
};

/* what the state's tick calls (only that) */
class CSndInstance {
public:
    static void PostTick(float dT, void *data);
};

const char *GetDatabase();

class CZGameState {
public:
    static void UpdateLocalGame(CZNetGame *game);
};

class CTurret {
public:
    static void ResetAmmo();
};

extern "C" {
void func_00544D60(CZSealBody *seal, int arg);   /* (the SEAL lane's) */
}
void zVid_Swap(bool flip, bool restore);   /* 0x3AFF30 (the weapons' round 31 named it) */

/* what the state's uninitialisation calls (only that; the names the map's) */
enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};

/* a named value (zanim.h's CValve: the bits and the inline setter) */
class CValve {
public:
    static CValve *GetByName(const char *name);
    void MakeCallbacks(VALVE_STATE state);
    bool SetValue(int value)
    {
        VALVE_STATE state = VALVE_SAME;

        if (m_value != value)
            state = value == 0 ? VALVE_OFF : VALVE_ON;
        m_value = value;
        if (state != VALVE_SAME) {
            MakeCallbacks(state);
            return true;
        }
        return false;
    }

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
    unsigned int m_type : 8;
    unsigned int m_flags : 8;
};

class CZLANLobby {
public:
    bool IsActive();
};
extern CZLANLobby data_0044FE10;   /* the LAN lobby */

enum LANGUAGE {
    LANGUAGE_ALIGNMENT_HACK = 0x7FFFFFFF
};
LANGUAGE GetLanguage();

class CStringTables {
public:
    void Load(const char *name, LANGUAGE language);
    void Unload(const char *name);
};
CStringTables *GetStringTables();

class CMenuStack {
public:
    void FlushHistory(bool all);
};
CMenuStack *GetMenuStack();

class CZAnimSet {
public:
    void Stop();
    void Reset();
    void Start();
};

class CNodeAction {
public:
    static void Reset();
};

class CZAnim;

class CameraZoom {
public:
    static void SetLensFxAnim(CZAnim *anim);
};

namespace zdb {
void EmptyAssetRecycleBin();
}

/* the mission's flags byte (fts_state.h's m_fc: bit 0 the core state's fade) */
struct CMissionFlags {
    unsigned char m_fade : 1;
};

/* the fade flag set (ours: an inline's parameter keeps the target's unfolded `andi v1,zero,1`) */
inline void SetMissionFade(bool b)
{
    ((CMissionFlags *)&theMission.m_fc)->m_fade = b;
}

/* what the state's initialisation calls (only that) */
void sealSaveFireModes();
void zVid_Sync0();

/* a queue of the HUD's messages (hud_main.cpp's C2DMessage_Q: only the clear; the mission holds
 * three, at 0x1C0, 0x6A0 and 0xB80) */
class C2DMessage_Q {
public:
    void Clear_Messages();
};

class CZPersonaState {
public:
    static void UpdateLocalPersona();
};

extern "C" {
void func_00545B10(CSealStats *stats, CZSealBody *seal);   /* (the SEAL lane's: a round won) */
void func_00545B90(CSealStats *stats, CZSealBody *seal);   /* (a round survived) */
void func_002BD640(CZSealBody *seal, int arg);
void func_005B61A0();
void func_002B7950();
}

extern CZAnimZAR *data_0040DBC0;   /* the stats screen's anims */
extern unsigned char data_004A4511;

extern "C" {
void func_00545200(CSealStats *stats);   /* (the SEAL lane's: the round's stats cleared) */
bool func_002C2FA0();
void func_00543B90(CZSealBody *seal);
void func_00327E40(int ms);
}

/* the final screen once the game is over, else the round's */
const char *CMPExitMenu::GetReaderFileName()
{
    return m_gameover ? "dlgMultiplayerFinal.rdr" : "dlgMultiplayerRound.rdr";
}

namespace {

/* the team totals' stats, in their columns' order (SOCOM 1's c_arrStatFuncs, six there) */
int (CSealStats::*const c_arrStatFuncs[3])() const = {
    &CSealStats::GetMvpScore,
    &CSealStats::GetKills,
    &CSealStats::GetDeaths
};

/* two SEALs in the order of one set of their stats: the MVP score, then the second counter, then
 * the name (SOCOM 1's CompareSeals, on the round's score alone; ours the stats accessor passed).
 * The values are int locals: compared straight, MW loads the right operand first */
inline bool CompareSeals(CZSealBody *p1, CZSealBody *p2, CSealStats &(CZSealBody::*pfnStats)())
{
    CSealStats &s1 = (p1->*pfnStats)();
    CSealStats &s2 = (p2->*pfnStats)();

    int iScore1 = s1.m_score, iScore2 = s2.m_score;

    if (iScore1 != iScore2)
        return iScore1 < iScore2;
    int iKills1 = s1.m_kills, iKills2 = s2.m_kills;
    if (iKills1 != iKills2)
        return iKills1 < iKills2;
    return strcmp(p1->m_name, p2->m_name) < 0;
}

/* by the round's stats, and by the game's (the names ours) */
bool CompareRoundSeals(CZSealBody *p1, CZSealBody *p2)
{
    return CompareSeals(p1, p2, &CZSealBody::GetRoundStats);
}

bool CompareGameSeals(CZSealBody *p1, CZSealBody *p2)
{
    return CompareSeals(p1, p2, &CZSealBody::GetGameStats);
}

/* a side's totals of the three stats, as a list of strings in a UI variable: only the living
 * SEALs the stats count, the hit percentage averaged (SOCOM 1's SetFinalTeamTotalsUiVar, six
 * totals of every SEAL's game stats there).
 * OPEN, 1 instruction and a register swap (25 differing, the branch offsets with them): the walk's
 * begin() is `lw a2,4(s6)` off the &CEntity::m_list hoisted out of the
 * outer loop, where the target reloads it absolutely (`lui at,0x43; lw a2,0x62E8(at)`) and keeps
 * &m_list in s7 for end() only, the UI variable's name in s6 (here swapped) -- include/list's
 * const begin() reads the sentinel through tail() into a pointer local, which shares the address
 * with end()'s; SetMvpUiVars' and SetStatLbUiVar's walks, outside any loop, cannot tell. Tried:
 * begin/end through CEntity::m_list (no conversion to const_iterator) or an end local. The side
 * test's `bnez v0,count; b skip` is the `if (bSeals) { if (!A) continue; } else if (!B) continue;`
 * form SetStatLbUiVar matched with (round 18: the goto form inverted it; the ternary joins).
 * __ptmf_cmpr (0x1828E0) is the PTMF comparison MW calls by name: the row needs that name for the
 * call to resolve */
void SetFinalTeamTotalsUiVar(bool bSeals, const char *szUiVar)
{
    static int (CSealStats::*pfnHitPct)() const = &CSealStats::GetHitPct;
    CRdrEditor edFinalStats;

    edFinalStats.push(0);
    edFinalStats.push(0);
    for (unsigned int i = 0; i < 3; i++) {
        int iTot = 0;
        int iNum = 0;
        char buf[16];

        for (CEntityItr it; it.More(); it.Advance()) {
            if (it.IsLiveSeal() && !it.GetSeal().m_fd2) {
                CZSealBody &seal = it.GetSeal();

                if (bSeals) {
                    if (!func_002C31D0(&seal))
                        continue;
                } else if (!func_002C30B0(&seal))
                    continue;
                iTot += (seal.GetGameStats().*c_arrStatFuncs[i])();
                iNum++;
            }
        }
        if (pfnHitPct == c_arrStatFuncs[i] && iNum != 0)
            iTot /= iNum;
        sprintf(buf, "%d", iTot);
        edFinalStats.addstringnode(buf);
    }
    edFinalStats.pop();
    edFinalStats.pop();
    theUIVarManager.Add(szUiVar, 0, UIVL_TEMPORARY)->Set(&edFinalStats);
}

/* a number added to a list as its decimal string (ours: the stat writers' common step, each with
 * its own buffer) */
inline void AddIntNode(CRdrEditor &ed, int n)
{
    char buf[16];

    sprintf(buf, "%d", n);
    ed.addstringnode(buf);
}

/* the player's own game stats as a list of strings in a UI variable: the three totals, the hit
 * percentage and five more counters (the name ours; SOCOM 1's SetMpStatUiVars built it with the
 * rest). Init passes the rounds won and lost too, which the code never reads */
void SetPlayerStatsUiVar(const char *szUiVar, int iWon, int iLost)
{
    CSealStats &stats = ftsGetPlayer()->GetGameStats();
    CRdrEditor edStats;

    edStats.push(0);
    AddIntNode(edStats, stats.GetMvpScore());
    AddIntNode(edStats, stats.GetKills());
    AddIntNode(edStats, stats.GetDeaths());
    AddIntNode(edStats, stats.GetHitPct());
    AddIntNode(edStats, stats.m_0);
    AddIntNode(edStats, stats.m_32);
    AddIntNode(edStats, stats.m_34);
    AddIntNode(edStats, stats.m_16);
    AddIntNode(edStats, stats.m_14);
    edStats.pop();
    theUIVarManager.Add(szUiVar, 0, UIVL_TEMPORARY)->Set(&edStats);
}

/* the game's MVP -- the living, counted SEAL first in CompareSeals' order of the game stats --
 * its name (with its clan's tag) and its three totals into two UI variables (the name ours;
 * SOCOM 1's TickMpStats found its MVP the same way) */
void SetMvpUiVars(const char *szNameVar, const char *szUiVar)
{
    CEntityItr it;
    CZSealBody *pMvp = it.IsLiveSeal() ? &it.GetSeal() : 0;

    for (;;) {
        it.Advance();
        if (!it.More())
            break;
        if (it.IsLiveSeal() && !it.GetSeal().m_fd2
            && (func_002C31D0(&it.GetSeal()) || func_002C30B0(&it.GetSeal()))) {
            if (!pMvp)
                pMvp = &it.GetSeal();
            else if (CompareSeals(pMvp, &it.GetSeal(), &CZSealBody::GetGameStats))
                pMvp = &it.GetSeal();
        }
    }
    if (pMvp) {
        char szName[128];
        const char *szClan = func_002C3AC0(pMvp->m_fc8);

        if (szClan && *szClan)
            sprintf(szName, "[%s] %s", szClan, pMvp->m_name);
        else
            sprintf(szName, "%s", pMvp->m_name);
        *theUIVarManager.Add(szNameVar, 0, UIVL_TEMPORARY) = szName;

        CRdrEditor edStats;

        edStats.push(0);
        edStats.push(0);
        for (unsigned int i = 0; i < 3; i++)
            AddIntNode(edStats, (pMvp->GetGameStats().*c_arrStatFuncs[i])());
        edStats.pop();
        edStats.pop();
        theUIVarManager.Add(szUiVar, 0, UIVL_TEMPORARY)->Set(&edStats);
    }
}

/* the player's new rating from the game just played (the name ours; UnInit's call): an Elo match
 * of the two sides' living, counted SEALs -- their game scores, floored at zero, and their ratings
 * -- the result taken halfway from the career record's rating when the player has one. The side
 * test needs `bool bSide = false; int iTeam;` in that order in the inner block (iTeam first, or at
 * function scope, rotates s0-s5). The halfway step's difference is assigned in two arms (round 53,
 * the HUD lane: the replay put the 0.5's FPR ahead of the difference's -- MW generates a multiply's
 * complex operand first, so `0.5f * (iRating - iOld)` numbers the constant last and colours it $f0;
 * a difference held in a variable is a plain operand, generated after the constant, but a single
 * assignment (a named local, `iRating -= iOld`, an inline returning `a - b`) is forward-substituted
 * back into the multiply -- or keeps iRating's s7. Assigned in two branches (here a gain and a loss,
 * alike in this build), the difference stays a variable in v0 and the 0.5 takes $f1, byte-exact; a
 * two-armed ternary on the difference does the same, one on the weight does not) */
int ComputeNewRating()
{
    int iRating = 0;

    if (!func_002C2FD0(ftsGetPlayer())) {
        CRatingMatch *pMatch = new CRatingMatch;
        CZNetGame *pGame = data_00437CE8;

        if (pMatch && pGame) {
            bool bFirst = pGame->m_128 == 0;
            pMatch->m_rounds = pGame->m_11c;
            pMatch->m_solo = false;
            pMatch->AddTeam(0, pGame->m_seals_won, bFirst, 1118.0f);
            pMatch->AddTeam(8, pGame->m_terrs_won, !bFirst, 982.0f);

            int iSeals = 0;
            int iTerrs = 0;

            for (CEntityItr it; it.More(); it.Advance()) {
                if (it.IsLiveSeal()) {
                    CZSealBody *pSeal = &it.GetSeal();

                    if (!pSeal->m_fd2) {
                        bool bSide = false;
                        int iTeam;

                        if (func_002C31D0(pSeal)) {
                            iTeam = 0;
                            bSide = true;
                            iSeals++;
                        } else if (func_002C30B0(pSeal)) {
                            iTeam = 8;
                            bSide = true;
                            iTerrs++;
                        }
                        if (bSide) {
                            int iScore = pSeal->GetGameStats().m_score;

                            if (iScore < 0)
                                iScore = 0;
                            pMatch->AddPlayer(iTeam, (int)pSeal, iScore, (float)func_002C3500(pSeal));
                        }
                    }
                }
            }
            if (iSeals > 0 && iTerrs > 0) {
                float fRating;

                pMatch->Compute();
                if (ftsGetPlayer() && pMatch->GetNewRating((int)ftsGetPlayer(), &fRating)) {
                    iRating = (int)(0.5f + fRating);
                    if (func_002C2DC0(ftsGetPlayer()) != -1) {
                        CareerRecord rec;

                        func_002F98F0(&rec);
                        int iOld = (int)(float)rec.m_rating;
                        int iChange;

                        if (iRating > iOld)
                            iChange = iRating - iOld;
                        else
                            iChange = iRating - iOld;
                        iRating = iOld + (int)(0.5f * iChange);
                    }
                }
            }
            delete pMatch;
        }
    }
    return iRating;
}

/* the player's game added to its career record (the name ours; Init's call, with two counts of
 * the round's -- unproven): the counters summed, the hit percentage averaged over the games, the
 * game's time, one game more */
void UpdateCareerRecord(int a, int b)
{
    CZSealBody *pPlayer = ftsGetPlayer();

    if (pPlayer && !func_002C2FD0(pPlayer) && (func_002C31D0(pPlayer) || func_002C30B0(pPlayer))) {
        CareerRecord rec;
        CSealStats &stats = pPlayer->GetGameStats();

        func_002F98F0(&rec);
        rec.m_kills += stats.GetKills();
        rec.m_deaths += stats.GetDeaths();
        rec.m_2c += stats.Get14();
        rec.m_2e += stats.Get38();
        rec.m_30 += stats.Get36();
        if (data_00437CE8->m_type == 3) {
            if (func_002C31D0(pPlayer))
                rec.m_26 += stats.Get1c();
            else if (func_002C30B0(pPlayer))
                rec.m_28 += stats.Get18();
        } else
            rec.m_24 += stats.Get1c();
        rec.m_32 += stats.Get0();
        rec.m_20_22 = stats.Get34();
        rec.m_hit_pct = (int)(0.5f + (float)(rec.m_hit_pct * rec.m_games + stats.GetHitPct())
                                         / (float)(rec.m_games + 1));
        rec.m_time += data_00437CE8->m_clock.GetTimeInGameSec();
        rec.m_14 += a;
        rec.m_18 += b;
        rec.m_games++;
        func_002F96F0(&rec, 1);
    }
}

/* a SEAL's round: whether it won, by the game's type (1-5) -- the living SEALs, terrorists and
 * third side counted, two of the SEALs' round counters summed -- and its round score (the name
 * ours; Init's call).
 * OPEN, 19 instructions, three temporaries renamed: after the two side calls the target holds the
 * side bool in t0, the game's type in a3 and the alive bool in a1 (the network state's pointer's
 * register, after the type's load); here a3, a1 (the pointer's, coalesced) and a2 (the
 * bitfield's). Every saved register, the frame and the code are the target's. Tried: the alive
 * bool, the stats reference and the side bools declared in each order, uninitialised and assigned
 * later, as unsigned char, through an inline IsAlive(); the type in an int or byte local; the
 * score's add spelled out */
void GetRoundResult(CZSealBody *pSeal, int *pWins, int *pScore)
{
    int iSeals = 0;
    int iTerrs = 0;
    int iOthers = 0;

    *pWins = 0;
    int iSum1c = 0;
    *pScore = 0;
    int iSum38 = 0;
    for (CEntityItr it; it.More(); it.Advance()) {
        if (it.IsLiveSeal()) {
            CZSealBody *seal = &it.GetSeal();

            if (seal) {
                if (seal->m_isAlive) {
                    if (func_002C31D0(seal))
                        iSeals++;
                    else if (func_002C30B0(seal))
                        iTerrs++;
                    else if (seal->m_team & 0x20000)
                        iOthers++;
                }
                if (func_002C31D0(seal)) {
                    iSum1c += seal->GetRoundStats().m_1c;
                    iSum38 += seal->GetRoundStats().m_38;
                }
            }
        }
    }

    bool bSeal = func_002C31D0(pSeal);
    bool bTerr = func_002C30B0(pSeal);
    CSealStats &stats = pSeal->GetRoundStats();
    bool bAlive = pSeal->m_isAlive;

    switch (data_00437CE8->m_type) {
    case 1:
        if (bTerr && ((bAlive && iSeals == 0) || iSum38 == 0))
            (*pWins)++;
        break;
    case 2:
        break;
    case 3:
        if (bSeal) {
            if (bAlive && (iOthers > 0 || iSum1c > 0))
                (*pWins)++;
        } else if (bTerr && bAlive && iOthers == 0)
            (*pWins)++;
        break;
    case 4:
        if (bSeal) {
            if (bAlive && iSum1c >= 2)
                (*pWins)++;
        } else if (bTerr && bAlive && iSeals == 0)
            (*pWins)++;
        break;
    case 5:
        if (bSeal) {
            if (bAlive && iTerrs == 0)
                (*pWins)++;
        } else if (bTerr && bAlive && iSeals == 0)
            (*pWins)++;
        break;
    }
    *pScore += stats.m_3a;
}

void SetStatLbUiVar(bool bSeals, const char *szUiVar, CSealStats &(CZSealBody::*pAccessor)(),
                    bool bGame);

/* a time in seconds as "hh:mm:ss" into a UI variable (ours: SOCOM 1's SetMpStatUiVars wrote it
 * out twice on one line each, an inline taking the clock's accessor by value) */
inline void SetTimeUiVar(const char *szUiVar, unsigned int (CNetClock::*pfnTime)())
{
    char szTime[] = "00:00:00";
    int iSec = (data_00437CE8->m_clock.*pfnTime)();

    szTime[0] = '0' + iSec / 36000;
    iSec %= 36000;
    szTime[1] = '0' + iSec / 3600;
    iSec %= 3600;
    szTime[3] = '0' + iSec / 600;
    iSec %= 600;
    szTime[4] = '0' + iSec / 60;
    iSec %= 60;
    szTime[6] = '0' + iSec / 10;
    iSec %= 10;
    szTime[7] = '0' + iSec;
    *theUIVarManager.Add(szUiVar, 0, UIVL_TEMPORARY) = szTime;
}

/* the round's and the game's stats lists for each side, the player's points, the rounds won and
 * the times (SOCOM 1's SetMpStatUiVars; SOCOM II's lists sort each by its own stats and leave the
 * ladder alone, the team totals to SetFinalStatsUiVars).
 * OPEN, 8 instructions, one register pair: the points (the round's MVP score, loaded before Add)
 * take s0 and PointsEarned's variable s3, where the target has them the other way round; every
 * other register is the target's. Tried (round 18): the score in a named local declared first,
 * after the bools or last; the setter as this inline assignment, as a free inline with a named
 * variable, or written out with a function-scope variable; tools/perm.py over the declaration
 * order of all seven locals (best: this one's 8, from 34) */
void SetMpStatUiVars()
{
    bool bSeals;
    bool bTerrs;
    int iWon;
    int iSealsWon;
    int iTerrsWon;

    bSeals = func_002C31A0();
    bTerrs = func_002C3080();
    SetStatLbUiVar(true, "RoundStatLbContentsSeals", &CZSealBody::GetRoundStats, false);
    SetStatLbUiVar(false, "RoundStatLbContentsTerrs", &CZSealBody::GetRoundStats, false);
    SetStatLbUiVar(true, "GameStatLbContentsSeals", &CZSealBody::GetGameStats, true);
    SetStatLbUiVar(false, "GameStatLbContentsTerrs", &CZSealBody::GetGameStats, true);

    *theUIVarManager.Add("PointsEarned", 0, UIVL_TEMPORARY) = ftsGetPlayer()->GetRoundStats().GetMvpScore();

    iSealsWon = data_00437CE8->m_seals_won;
    iTerrsWon = data_00437CE8->m_terrs_won;
    iWon = 0;
    if (bSeals)
        iWon = iSealsWon;
    else if (bTerrs)
        iWon = iTerrsWon;
    *theUIVarManager.Add("RoundsWonSeals", 0, UIVL_TEMPORARY) = iSealsWon;
    *theUIVarManager.Add("RoundsWonTerrs", 0, UIVL_TEMPORARY) = iTerrsWon;
    *theUIVarManager.Add("RoundsWon", 0, UIVL_TEMPORARY) = iWon;

    SetTimeUiVar("TimeInRound", &CNetClock::GetTimeInRoundSec);
    SetTimeUiVar("TimeInGame", &CNetClock::GetTimeInGameSec);
}

/* a side's SEALs, the living and counted, sorted by one set of their stats (the round's or the
 * game's comparator) and listed best first -- each a row of its name, its clan's tag and its three
 * stats, the player's name and tag coloured -- into a UI variable (SOCOM 1's SetStatLbUiVar, which
 * took the stats accessor alone and sorted by the round's stats) */
void SetStatLbUiVar(bool bSeals, const char *szUiVar, CSealStats &(CZSealBody::*pAccessor)(),
                    bool bGame)
{
    CRdrEditor edStats;

    edStats.push(0);
    std::vector<CZSealBody *> vecSeals;
    for (CEntityItr it; it.More(); it.Advance()) {
        if (it.IsLiveSeal()) {
            if (bSeals) {
                if (!func_002C31D0(&it.GetSeal()))
                    continue;
            } else if (!func_002C30B0(&it.GetSeal()))
                continue;
            if (!it.GetSeal().m_fd2)
                vecSeals.push_back(&it.GetSeal());
        }
    }
    if (vecSeals.size() > 1) {
        if (bGame)
            std::sort(vecSeals.begin(), vecSeals.end(), CompareGameSeals);
        else
            std::sort(vecSeals.begin(), vecSeals.end(), CompareRoundSeals);
    }
    for (int iSeal = vecSeals.size() - 1; iSeal >= 0; iSeal--) {
        char szTag[8];

        edStats.push(0);
        const char *szClan = func_002C3AC0(vecSeals[iSeal]->m_fc8);
        if (szClan && *szClan)
            sprintf(szTag, "[%s]", szClan);
        else
            sprintf(szTag, "");
        if (vecSeals[iSeal] != ftsGetPlayer()) {
            edStats.addstringnode(vecSeals[iSeal]->m_name);
            edStats.addstringnode(szTag);
        } else {
            PNT3D color;

            color.x = 91.0f;
            color.y = 72.0f;
            color.z = 36.0f;
            edStats.push(0);
            edStats.addSTRING("VALUE", vecSeals[iSeal]->m_name);
            edStats.addPNT3D("COLOR", color);
            edStats.pop();
            edStats.push(0);
            edStats.addSTRING("VALUE", szTag);
            edStats.addPNT3D("COLOR", color);
            edStats.pop();
        }
        CSealStats &stats = (vecSeals[iSeal]->*pAccessor)();
        for (unsigned int i = 0; i < 3; i++)
            AddIntNode(edStats, (stats.*c_arrStatFuncs[i])());
        edStats.pop();
    }
    edStats.pop();
    theUIVarManager.Add(szUiVar, 0, UIVL_TEMPORARY)->Set(&edStats);
}

}

/* the game's final stats into the UI's variables: the MVP and each side's totals (the name ours;
 * the network game calls it, 0x27FF08) */
void SetFinalStatsUiVars()
{
    SetMvpUiVars("MvpName", "MvpLbContents");
    SetFinalTeamTotalsUiVar(true, "GameSealsTotLbContents");
    SetFinalTeamTotalsUiVar(false, "GameTerrsTotLbContents");
}

/* the splash screen's asset library named; no menu yet */
CMPExitState::CMPExitState() : CGameState("MPExitState")
{
    m_libname.assign("common/assetlib/splash");
    m_menu = 0;
    m_28 = 0;
}

/* the stats screen opened: the SEALs' round stats settled (the round's winners and survivors
 * credited by the side the game says won it), the player's round result and the round added to its
 * game stats, the hit percentage recomputed, the UI's stat lists set, the player's stats and career
 * record written at the game's end, the decals and messages cleared, the screen's archive, menu,
 * anims and strings loaded, every SEAL's node and bomb gear put away, the game's clock reset at the
 * game's end (SOCOM 1's Init, SOCOM II's stats and rating added) */
/* OPEN, 14 instructions, one register rotation: the network game's pointer takes s0 here and the
 * first walk's list address s1 (the rounds lost s2), where the target has the pointer in s1
 * (shared with the game-over flag), the list address and the rounds lost in s0 -- the pointer is
 * the first variable null-tested, which MW numbers last. Everything else is the target's: the
 * frame, the draws spilled to the stack, the inline Add's order, the hit percentage through an
 * unsigned local, the clock reset through one load (an inline on the game), CMPExitMenu's 0x930
 * bytes. Tried: tools/perm.py over the six top declarations (14 after 1,644 steps), the pointer
 * declared first, tested `!= 0`, the late locals declared ahead, the draws' sum reversed */
bool CMPExitState::Init()
{
    sealSaveFireModes();
    ZAnim.FreeNetworkPacketBuffer();

    int iRounds = -1;
    CZNetGame *pGame = data_00437CE8;
    int iSealsWon = 0;
    int iDraws = 0;
    int iTerrsWon = 0;
    int iSide = 0;

    if (pGame) {
        iSealsWon = pGame->m_seals_won;
        iTerrsWon = pGame->m_terrs_won;
        iRounds = pGame->m_11c;
        iSide = pGame->m_128;
        iDraws = iRounds - (iSealsWon + iTerrsWon);
    }

    CZSealBody *pPlayer = ftsGetPlayer();

    for (CEntityItr it; it.More(); it.Advance()) {
        if (it.IsLiveSeal()) {
            CZSealBody *pSeal = &it.GetSeal();

            pSeal->m_fd2 = pSeal->m_fd1;
            if (iSide == 0 && func_002C31D0(pSeal))
                func_00545B10(&pSeal->GetRoundStats(), pSeal);
            else if (iSide == 8 && func_002C30B0(pSeal))
                func_00545B10(&pSeal->GetRoundStats(), pSeal);
            if (pSeal->m_isAlive && pSeal->m_fb0 != 6)
                func_00545B90(&pSeal->GetRoundStats(), pSeal);
        }
    }

    int iWins = 0;
    int iScore = 0;

    GetRoundResult(pPlayer, &iWins, &iScore);
    pPlayer->GetRoundStats().m_32 += iWins;
    pPlayer->GetRoundStats().m_34 += iScore;
    pPlayer->GetGameStats().Add(pPlayer->GetRoundStats());
    pPlayer->GetGameStats().UpdateHitPct();
    func_002BD640(pPlayer, 0);
    SetMpStatUiVars();

    int iWon;

    if (func_002C31A0())
        iWon = iSealsWon;
    else if (func_002C3080())
        iWon = iTerrsWon;
    else
        iWon = 0;

    int iLost = iRounds - (iWon + iDraws);
    bool bGameOver = pGame->m_113 == 6;

    m_28 = 0;
    if (bGameOver) {
        SetPlayerStatsUiVar("PlayerLbContents", iWon, iLost);
        if (!data_00437CE8->m_dc && !data_0044FE10.IsActive())
            UpdateCareerRecord(iWon, iLost);
    }
    zdb::CWorld::m_world->RemoveDecals();
    ((C2DMessage_Q *)((char *)&theMission + 0x1c0))->Clear_Messages();
    ((C2DMessage_Q *)((char *)&theMission + 0xb80))->Clear_Messages();
    ((C2DMessage_Q *)((char *)&theMission + 0x6a0))->Clear_Messages();
    func_005B61A0();
    CRdrArchive::AddArchive("readerx.zar", "run/ui");
    CRdrArchive::OpenAll();
    if (!m_menu)
        m_menu = new CMPExitMenu;
    m_menu->m_gameover = bGameOver;
    data_0040DBC0 = 0;
    data_0040DBC0 = ZAnim.OpenZAR("run/mpzanim.zar");

    CStringTables *pTables = GetStringTables();

    pTables->Load("UIMPX", GetLanguage());
    m_menu->Init(0, data_0040DBC0);
    for (CEntityItr it; it.More(); it.Advance()) {
        if (it.IsSeal()) {
            CZSealBody *pSeal = &it.GetSeal();

            ((CEntity *)pSeal)->GetNode()->SetActive(false);
            pSeal->EnableMPBombGear(false);
        }
    }
    if (bGameOver) {
        data_00437CE8->ResetForNextGame();
        data_00437CE8->MyMinorGameState(CZNetGame::MP_MINOR_NOTREADY);
        CZPersonaState::UpdateLocalPersona();
        func_002B7950();
    }
    data_004A4511 = 1;
    zVid_Sync0();
    return true;
}

/* the stats screen's tick: by the game's state (the network game's byte at 0x113), the round's
 * end pops the state -- the SEALs respawned, the entities purged, the turrets' ammunition and the
 * mission reset (3 and 5) -- or returns to the menus (1); otherwise the stats are listed, the menu
 * ticked and drawn (SOCOM 1's Tick, a switch over SOCOM II's states) */
bool CMPExitState::Tick(float dT)
{
    unsigned char bMaster = theNetwork.m_8;

    CSndInstance::PostTick(dT, 0);
    CInput::Tick(dT);

    CZNetGame *pGame = data_00437CE8;

    switch (pGame->m_113) {
    case 4:
        break;
    case 6: {
        const char *szDatabase = GetDatabase();

        if (szDatabase && *szDatabase)
            CZGameState::UpdateLocalGame(pGame);
        break;
    }
    case 5:
    case 3:
        if (bMaster)
            CZGameState::UpdateLocalGame(pGame);
        CEntity::PurgeEntities();
        std::list<CEntity *>::const_iterator it;

        for (it = CEntityItr::GetEntityList().begin(); it != CEntityItr::GetEntityList().end(); ++it) {
            CZSealBody *pSeal = AsSeal(*it);

            if (pSeal && !pSeal->m_fd1)
                pSeal->RemoteRespawn(false);
        }
        CTurret::ResetAmmo();
        theMission.OnMissionStart();
        theGame.Pop(0);
        theUIVarManager.Reset();
        return true;
    case 1:
        theGame.Pop(0);
        theGame.Switch(&theGame.m_menuState, 0);
        return true;
    }
    func_00544D60(ftsGetPlayer(), 0);
    SetStatLbUiVar(true, "RoundStatLbContentsSeals", &CZSealBody::GetRoundStats, false);
    SetStatLbUiVar(false, "RoundStatLbContentsTerrs", &CZSealBody::GetRoundStats, false);
    SetStatLbUiVar(true, "GameStatLbContentsSeals", &CZSealBody::GetGameStats, true);
    SetStatLbUiVar(false, "GameStatLbContentsTerrs", &CZSealBody::GetGameStats, true);
    SetMvpUiVars("MvpName", "MvpLbContents");
    m_menu->Tick(dT);
    m_menu->Draw2dOnly();
    zSysFifoEnd();
    zVid_Swap(true, true);
    return false;
}

/* the stats screen closed: the player's new rating written to its career record (when the game
 * rates), every SEAL's round stats cleared, the two team score valves and their counts zeroed, the
 * menu deleted, the screen's strings, anims and archive released, the anim sets restarted, the
 * death camera, lens effects, the asset bin, the UI's variables and the menu history reset (SOCOM
 * 1's UnInit, SOCOM II's rating and valves added) */
void CMPExitState::UnInit()
{
    zSysFifoEnd();
    sceGsSyncPath(0, 0);
    data_004A4511 = 0;
    if (!data_00437CE8->m_dc && !data_0044FE10.IsActive()) {
        m_28 = ComputeNewRating();
        if (m_28 > 0) {
            CareerRecord rec;

            func_002F98F0(&rec);
            rec.m_rating = m_28;
            func_002F96F0(&rec, 1);
        }
    }
    for (CEntityItr it; it.More(); it.Advance()) {
        if (it.IsSeal())
            func_00545200(&it.GetSeal().GetRoundStats());
    }

    CValve *pValve = CValve::GetByName("seals_team_score");

    if (pValve)
        pValve->SetValue(0);
    data_00437CE8->m_7c = 0;
    pValve = CValve::GetByName("terrs_team_score");
    if (pValve)
        pValve->SetValue(0);
    data_00437CE8->m_80 = 0;

    if (m_menu)
        delete m_menu;
    GetStringTables()->Unload("UIMPX");
    if (data_0040DBC0) {
        ZAnim.CloseZAR(data_0040DBC0);
        data_0040DBC0 = 0;
    }
    m_menu = 0;
    if (data_00437CE8->m_d2 && !func_002C2FA0()) {
        data_00437CE8->m_d2 = 0;
        func_00543B90(ftsGetPlayer());
    }

    CZAnimSet *pCommon = ZAnim.GetAnimSet("common");

    if (pCommon) {
        pCommon->Stop();
        pCommon->Reset();
    }

    CZAnimSet *pMission = ZAnim.GetAnimSet("mission");

    if (pMission) {
        pMission->Stop();
        pMission->Reset();
    }
    if (pCommon)
        pCommon->Start();
    if (pMission)
        pMission->Start();
    func_00327E40(2000);
    CNodeAction::Reset();
    appCamera->ResetDeathCam();
    SetMissionFade(false);
    CameraZoom::SetLensFxAnim(0);
    zdb::EmptyAssetRecycleBin();
    theUIVarManager.Reset();
    CRdrArchive::RemoveArchive("readerx.zar", "run/ui");
    GetMenuStack()->FlushHistory(true);
}

/* nothing to load (SOCOM 1's, empty there too; st_core.cpp calls it, 0x1EBC30 and 0x1EBE00) */
void CMPExitState::LoadLibs()
{
}
