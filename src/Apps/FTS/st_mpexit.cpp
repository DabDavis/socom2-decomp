/* st_mpexit.cpp -- the end of an online game: the stats screen (SOCOM 1's
 * Apps/FTS/st_mpexit.cpp; OVL1 0x223290-0x2265F7, its initialiser 0x3FE860). SOCOM 1's order
 * holds: LoadLibs, UnInit, ~CMPExitMenu, Tick, Init and the constructor, then the anonymous
 * namespace's stat writers, the accessors whose addresses they take (SOCOM 1's
 * GetTimeInGameSec/GetTimeInRoundSec, GetGameStats/GetRoundStats, GetHitPct and the team totals'
 * three), the seal comparators, GetReaderFileName and std::sort's instance, which closes the unit.
 * Written: the initialiser, the comparators, the player's and the MVP's stat writers, the team
 * totals (open), SetFinalStatsUiVars, the accessors, LoadLibs and the constructor; SetStatLbUiVar
 * (0x224210) with std::sort's instance over the vector's iterator (0x226060, __sort132 0x226600,
 * __selection_sort 0x2267C0: the raw-pointer sort SOCOM 1 had would be 0x3B4 bytes, not 0x598),
 * SetMpStatUiVars (0x224670, open) and CNetClock's two time accessors (0x224BF0/0x224C80). Still to
 * write: UnInit, Tick (four more SetStatLbUiVar calls, PTMFs 0x3DC7B0-0x3DC7E0), Init, 0x224D10 (a
 * SEAL's round result by the game's type, 1-5: the side tests, counts of each side's living SEALs),
 * 0x225070 (the player's career record read (0x2F98F0), summed into and written back (0x2F96F0))
 * and 0x2253C0. Defined in reverse of their image order (-inline deferred). */

#include "fts_state.h"
#include "../../gamez/zrdr/zrdr.h"

#include <stdio.h>
#include <string.h>
#include <list>
#include <vector>
#include <algorithm>
#include <iostream>

/* unnamed yet */
extern "C" {
void func_001EEB40(CStateString *s, const char *text);   /* std::string::assign */
}

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

    /* the other counters the player's list shows (the names ours, their meaning unproven) */
    unsigned short m_0;   // 0x0
    unsigned char _2[0xc - 0x2];
    unsigned short m_kills;   // 0xc
    unsigned char _e[0x12 - 0xe];
    unsigned short m_deaths;   // 0x12
    unsigned short m_14;   // 0x14
    unsigned short m_16;   // 0x16
    unsigned short m_18;   // 0x18: added to the career record in objective games (unproven)
    unsigned char _1a[0x1c - 0x1a];
    unsigned short m_1c;   // 0x1c: summed over the SEALs by the round result (unproven)
    unsigned char _1e[0x32 - 0x1e];
    unsigned short m_32;   // 0x32
    unsigned short m_34;   // 0x34
    unsigned short m_36;   // 0x36
    unsigned short m_38;   // 0x38: summed over the SEALs by the round result (unproven)
    unsigned short m_3a;   // 0x3a: the round result's score
    short m_score;   // 0x3c: the MVP score (SOCOM 1's m_sMvpScore)
    unsigned char _3e[0x44 - 0x3e];
    float m_hit_percent;   // 0x44
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
    unsigned char _5d4[0xfc8 - 0x5d4];
    int m_fc8;   // 0xfc8: what its clan's tag is looked up by
    unsigned char _fcc[0xfd2 - 0xfcc];
    unsigned char m_fd2;   // 0xfd2
};

/* a SEAL's clan tag, or none (unnamed yet) */
extern "C" const char *func_002C3AC0(int id);

/* a thing in the world (st_exit.cpp's stub: only the list here) */
class CEntity {
public:
    static std::list<CEntity *> m_list;
};

/* a walk over the world's entities, counting (SOCOM 1's DWARF; hud_scorepopup.cpp's view) */
class CEntityItr {
public:
    CEntityItr() : _it(GetEntityList().begin()), _iIdx(0) {}
    bool More() const { return _it != GetEntityList().end(); }

    CZSealBody &GetSeal() const;
    bool IsLiveSeal() const;
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

    unsigned int m_max_game_time;   // 0x0
    unsigned int m_4;   // 0x4
    unsigned int m_time_in_round;   // 0x8
    unsigned int m_game_time;   // 0xc
    unsigned int m_total_time;   // 0x10
};

/* the network game's state (OVL1 bss 0x437CE8; hud_main.cpp's stand-in): its clock (SOCOM 1's
 * at 0xA0), the game's type and each side's rounds won (SOCOM 1's at 0xEC/0xF0) */
class CNetGameState {
public:
    unsigned char _0[0xe0];
    CNetClock m_clock;   // 0xe0
    unsigned char _f4[0x111 - 0xf4];
    unsigned char m_type;   // 0x111: the game's type (1-5)
    unsigned char _112[0x11c - 0x112];
    int m_11c;   // 0x11c: the rating match's rounds (unproven)
    int m_seals_won;   // 0x120
    int m_terrs_won;   // 0x124
    int m_128;   // 0x128: zero when the SEALs' team rates first (unproven)
};

extern CNetGameState *data_00437CE8;

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

    void AddTeam(int id, int count, bool first, float rating);
    int AddPlayer(int team, int player, int score, float rating);
    float Compute();
    bool GetNewRating(int player, float *rating);

    void *m_teams[2];   // 0x0
    bool m_solo;   // 0x8
    bool m_done;   // 0x9
    int m_rounds;   // 0xc
    int m_k;   // 0x10
};

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
 * __ptmf_cmpr (0x1828E0, func_001828E0 in the map) is the PTMF comparison MW calls by name: the
 * row needs that name for the call to resolve */
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
 * rest) */
void SetPlayerStatsUiVar(const char *szUiVar)
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

/* how far a rating moves towards the game's result (ours: through an inline the multiply keeps
 * the target's operand order, 4 instructions to 3 -- see ComputeNewRating's OPEN) */
inline float GetRatingWeight()
{
    return 0.5f;
}

/* the player's new rating from the game just played (the name ours; UnInit's call): an Elo match
 * of the two sides' living, counted SEALs -- their game scores, floored at zero, and their ratings
 * -- the result taken halfway from the career record's rating when the player has one.
 * OPEN, 3 instructions, one FPR pair: the target materialises the 0.5 in $f1 and the difference's
 * conversion in $f0 (`mul.s $f0,$f1,$f0` = 0.5 * diff); here the two swap. Every GPR, the frame
 * (the record 0xE8 bytes, overlapping the walk's iterator) and the code are the target's -- the
 * side test needed `bool bSide = false; int iTeam;` in that order in the inner block (iTeam
 * first, or at function scope, rotates s0-s5; 15 to 49 instructions). Tried for the pair: 0.5f
 * written first or last, `/ 2.0f` (div.s), a float local for the weight, for the difference, or
 * both in either order, `(float)(int)`, `(int)fOld` from a float local, an inline Blend(from, to,
 * t) (4), its difference a float local (3), the constant from an inline (3, adopted) */
int ComputeNewRating()
{
    int iRating = 0;

    if (!func_002C2FD0(ftsGetPlayer())) {
        CRatingMatch *pMatch = new CRatingMatch;
        CNetGameState *pGame = data_00437CE8;

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
                        iRating = iOld + (int)(GetRatingWeight() * (iRating - iOld));
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
    func_001EEB40(&m_libname, "common/assetlib/splash");
    m_menu = 0;
    m_28 = 0;
}

/* nothing to load (SOCOM 1's, empty there too; st_core.cpp calls it, 0x1EBC30 and 0x1EBE00) */
void CMPExitState::LoadLibs()
{
}
