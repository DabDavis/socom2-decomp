/* st_mpexit.cpp -- the end of an online game: the stats screen (SOCOM 1's
 * Apps/FTS/st_mpexit.cpp; OVL1 0x223290-0x2265F7, its initialiser 0x3FE860). SOCOM 1's order
 * holds: LoadLibs, UnInit, ~CMPExitMenu, Tick, Init and the constructor, then the anonymous
 * namespace's stat writers, the accessors whose addresses they take (SOCOM 1's
 * GetTimeInGameSec/GetTimeInRoundSec, GetGameStats/GetRoundStats, GetHitPct and the team totals'
 * three), the seal comparators, GetReaderFileName and std::sort's instance, which closes the unit.
 * Defined in reverse of their image order (-inline deferred). */

#include "fts_state.h"
#include "../../gamez/zrdr/zrdr.h"

#include <stdio.h>
#include <string.h>
#include <list>

/* MSL's <iostream> static init objects: every unit that includes it builds one of each first
 * (SOCOM 1's initialisers alike; the object names ours) */
namespace std {
class __nInit {
public:
    __nInit();
    ~__nInit();
};

class __wInit {
public:
    __wInit();
    ~__wInit();
};

static __nInit __msl_nInit;
static __wInit __msl_wInit;
}

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

    unsigned char _0[0xc];
    unsigned short m_kills;   // 0xc
    unsigned char _e[0x12 - 0xe];
    unsigned short m_deaths;   // 0x12
    unsigned char _14[0x3c - 0x14];
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
    unsigned char _18[0x544 - 0x18];
    CSealStats m_game_stats;   // 0x544
    CSealStats m_round_stats;   // 0x58c
    unsigned char _5d4[0xfd2 - 0x5d4];
    unsigned char m_fd2;   // 0xfd2
};

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

/* whether a SEAL is on each side (the SEAL lane's, no names yet; int here: the totals test the
 * results unnormalised, where a bool adds an `andi 0xff` in the ternary) */
extern "C" {
int func_002C31D0(const CZSealBody *seal);
int func_002C30B0(const CZSealBody *seal);
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
    void Set(_zrdr *value);
};

class CUIVarManager {
public:
    CUIVariable *Add(const char *name, _zrdr *value, UIVAR_LONGEVITY longevity);
};
extern CUIVarManager theUIVarManager;

namespace std {
template <class RandomAccessIterator, class Compare>
void sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp);
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
 * OPEN, 3 instructions (the size is right; scdiff counts 27 with the branch offsets): (1) the
 * walk's begin() is `lw a2,4(s6)` off the hoisted &CEntity::m_list where the target reloads it
 * absolutely (`lui at,0x43; lw a2,0x62E8(at)`) and keeps &m_list in s7 for end() only, the UI
 * variable's name in s6 (here swapped); (2) the round-side test branches `bnez v0,count; b skip`
 * in the target, `beqz v0,skip; b count` here (tried: the ternary -- a join and one test, `andi`
 * with bool callees; `&&`/`||` -- bSeals re-tested; `continue` and empty-then forms; begin/end
 * through CEntity::m_list or an end local). __ptmf_cmpr (0x1828E0, func_001828E0 in the map) is
 * the PTMF comparison MW calls by name: the row needs that name for the call to resolve */
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
                    if (func_002C31D0(&seal))
                        goto count;
                } else if (func_002C30B0(&seal)) {
                count:
                    iTot += (seal.GetGameStats().*c_arrStatFuncs[i])();
                    iNum++;
                }
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

}

/* (OVL1 0x2256F0, not written yet: the MVP's name and the stats list) */
extern "C" void func_002256F0(const char *szName, const char *szUiVar);

/* the game's final stats into the UI's variables: the MVP and each side's totals (the name ours;
 * the network game calls it, 0x27FF08) */
void SetFinalStatsUiVars()
{
    func_002256F0("MvpName", "MvpLbContents");
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
