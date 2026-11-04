/* st_exit.cpp -- the end of a mission: the stats screen (SOCOM 1's Apps/FTS/st_exit.cpp; OVL1
 * 0x1F1A00-0x1F3B1F). Defined in reverse of their image order (-inline deferred). */

#include "fts_state.h"

#include <iostream>

/* unnamed yet (SOCOM 1's names in the comments) */
extern "C" {
void func_003654C0(CGameMenu *menu, float dT);   /* CGameMenu::Tick */
void func_003AFF30(bool a, bool b);              /* zVid_Swap */

}

/* the localised string tables (util_loc.cpp's; only what is called: SOCOM 1 called CSnd::Close
 * where SOCOM II unloads the UI's table); the language a word here, as st_mpexit.cpp has it */
enum LANGUAGE {
    LANGUAGE_ALIGNMENT_HACK = 0x7FFFFFFF
};
LANGUAGE GetLanguage();   /* 0x3A4460 */

class CStringTables {
public:
    void Load(const char *name, LANGUAGE language);   /* 0x3A3900 */
    void Unload(const char *name);   /* 0x3A3740 */
};
CStringTables *GetStringTables();   /* 0x361BC0 */

/* the reader archives (zrdr.h's CRdrArchive: only its statics), the asset recycler, the UI's
 * variables and the menu history (their units'; only what is called) */
class CRdrArchive {
public:
    static CRdrArchive *AddArchive(const char *name, const char *path);   /* 0x32C1E0 */
    static bool RemoveArchive(const char *name, const char *path);   /* 0x32C2B0 */
    static void OpenAll();   /* 0x32C570 */
};

namespace zdb {
void EmptyAssetRecycleBin();   /* 0x321D30 */
}

class CUIVarManager {
public:
    void Reset();   /* 0x385C30 */
};
extern CUIVarManager theUIVarManager;

class CMenuStack {
public:
    void FlushHistory(bool all);   /* 0x27EDF0 */
};
CMenuStack *GetMenuStack();   /* 0x274930 */

namespace {
/* the stats screen's anims */
CZAnimZAR *animfile;
}

const char *CExitMenu::GetReaderFileName()
{
    return "dlgExitState.rdr";
}

#include <list>

/* a SEAL (zseal.h's: only the mission's counters a career takes up, halfwords from 0x58C; the
 * names ours) */
class CZSealBody {
public:
    unsigned char _0[0x58c];
    unsigned short m_58c;   // 0x58c
    unsigned char _58e[0x594 - 0x58e];
    unsigned short m_594;   // 0x594
    unsigned char _596[0x598 - 0x596];
    unsigned short m_kills;   // 0x598
    unsigned char _59a[0x5a2 - 0x59a];
    unsigned short m_5a2;   // 0x5a2
    unsigned char _5a4[0x5ae - 0x5a4];
    unsigned short m_5ae;   // 0x5ae
    unsigned char _5b0[0x5b4 - 0x5b0];
    unsigned short m_5b4;   // 0x5b4
    unsigned char _5b6[0x5b8 - 0x5b6];
    unsigned short m_5b8;   // 0x5b8
};

CZSealBody *ftsGetPlayer();

/* the single-player career's stats (fts_mission.cpp's CCareerStats, theMission +0x1950: the
 * missions counted, five percentages averaged over them and seven of the player's counters summed;
 * the names ours) */
class CCareerStats {
public:
    void AddMission(float a, float b, float c, float d, float e);

    int m_missions;   // 0x0
    int m_avg[5];   // 0x4
    int m_sum[7];   // 0x18: [1] the kills
};

/* a fraction as a whole percentage, and a value rounded (the names ours) */
static inline int Round(float f) { return (int)(0.5f + f); }
static inline int Percent(float f) { return Round(100.0f * f); }

/* a mission taken into the career (OVL1 0x1F1FB0; SetRoundStatUiVars 0x1F21B0 calls it on
 * theMission +0x1950, the name ours): each percentage averaged in over the missions so far, the
 * count up one, the player's counters added.
 * OPEN, 0x1EC against 0x1F8 (scheduling): the operations are the target's, but the target runs
 * every percentage and quotient through $f1 (and the count's conversion through $f1 into $f3),
 * waiting a nop after each mtc1, where this spreads them over $f1-$f3 and schedules three fewer
 * slots. Tried: the percentages inline in the averages, as locals ahead (in a-e order, the count's
 * conversion before, between or after them), `f + 0.5f`, the count converted implicitly, the sum
 * divided as an int */
void CCareerStats::AddMission(float a, float b, float c, float d, float e)
{
    int n = m_missions;
    int pa = Percent(a);
    int pb = Percent(b);
    int pc = Percent(c);
    int pd = Percent(d);
    int pe = Percent(e);
    float n1 = (float)(n + 1);

    m_avg[0] = Round((float)(pa + n * m_avg[0]) / n1);
    m_avg[1] = Round((float)(pb + n * m_avg[1]) / n1);
    m_avg[2] = Round((float)(pc + n * m_avg[2]) / n1);
    m_avg[3] = Round((float)(pd + n * m_avg[3]) / n1);
    m_avg[4] = Round((float)(pe + n * m_avg[4]) / n1);
    m_missions++;
    CZSealBody *player = ftsGetPlayer();

    m_sum[1] += player->m_kills;
    m_sum[0] += player->m_5b4;
    m_sum[2] += player->m_5a2;
    m_sum[3] += player->m_5ae;
    m_sum[4] += player->m_5b8;
    m_sum[5] += player->m_594;
    m_sum[6] += player->m_58c;
}

/* a thing in the world: only its type (2, a SEAL) at 0x10 (SOCOM 1's DWARF) */
class CEntity {
public:
    unsigned int _0;
    unsigned int m_id;   // 0x4
    unsigned int m_index;   // 0x8
    unsigned int m_vehicle_index;   // 0xc
    unsigned char m_type;   // 0x10

    static std::list<CEntity *> m_list;
};

extern "C" bool func_002C2FD0(CZSealBody *seal);

/* a walk over the world's entities, counting (SOCOM 1's DWARF: the list's iterator, an index) */
class CEntityItr {
public:
    CZSealBody &GetSeal() const;
    bool IsSeal() const;
    bool IsLiveSeal() const;
    void Advance();

    /* the world's entities, const (CEntity::GetEntityList() returning a const reference -- SOCOM 1
     * links its out-of-line copy, GetEntityList__7CEntityFv, 0x1EA1C0; SOCOM II's is 0x299140 --
     * here until this unit's CEntity stub gives way to zentity.h's; the name is ours) */
    static const std::list<CEntity *> &GetEntityList() { return CEntity::m_list; }

    std::list<CEntity *>::const_iterator _it;   // 0x0
    int _iIdx;   // 0x4
};

/* on to the next SEAL, counting every entity passed. The walk is a const_iterator and the end the
 * const list's (SOCOM 1's called list<CEntity *>::end() const, whose const_iterator is built from
 * the base's by value): a temporary copied bitwise into a second, rebuilt at every step; through
 * the accessor, not a cast of the list (that builds it once, before the loop) */
void CEntityItr::Advance()
{
    do {
        ++_it;
        _iIdx++;
    } while (_it != GetEntityList().end() && &GetSeal() == 0);
}

/* whether the walk is on a SEAL (the name ours) */
bool CEntityItr::IsSeal() const
{
    if (!&GetSeal())
        return false;
    return true;
}

/* whether the walk is on a SEAL for which func_002C2FD0 says no (the name ours) */
bool CEntityItr::IsLiveSeal() const
{
    if (!&GetSeal() || func_002C2FD0(&GetSeal()))
        return false;
    return true;
}

/* the entity the walk is on, as a SEAL -- or none (a null reference) */
CZSealBody &CEntityItr::GetSeal() const
{
    CEntity *e = (CEntity *)*_it;

    return *(CZSealBody *)(e && e->m_type == 2 ? e : 0);
}

/* the mission's objectives counted: complete and total, and the fraction complete, for the
 * primary, the secondary and the third kind (SOCOM 1 counted two in Init itself) */
struct CObjectiveCount {
    CObjectiveCount()
    {
        complete = 0;
        total = 0;
    }

    float fraction;
    int complete;
    int total;
};

struct CObjectiveCounts {
    CObjectiveCount primary;
    CObjectiveCount secondary;
    CObjectiveCount third;
};

extern "C" {
void func_001F38F0(CObjectiveCounts *counts, void *objectives);
void func_001F21B0(CObjectiveCounts *counts);   /* SetRoundStatUiVars */
}


/* the stats screen up: the objectives counted, the mission unloaded around the stats being set,
 * and -- unless the mission ends the game (popped) or the menu is skipped (popped and the load
 * state switched to) -- the reader archive, the menu, its anims, the UI's sound bank */
bool CExitState::Init()
{
    CObjectiveCounts counts;

    func_001F38F0(&counts, theMission.m_objectives);
    theMission.ReOpen_Unload1();
    func_001F21B0(&counts);
    theMission.ReOpen_Unload2();
    if (theMission.m_1ac->m_4 == 2) {
        theGame.Pop(1);
        return true;
    }
    if (theGame.m_menuState.m_skip) {
        theGame.Pop(0);
        theGame.Switch(&theGame.m_loadState, 0);
        return true;
    }
    CRdrArchive::AddArchive("readerx.zar", "run/ui");
    CRdrArchive::OpenAll();
    if (!m_menu)
        m_menu = new CExitMenu;
    animfile = ZAnim.OpenZAR("run/extzanim.zar");
    CStringTables *tables = GetStringTables();

    tables->Load("UIX", GetLanguage());
    m_menu->Init(0, animfile);
    CVideo::Clear(false, 0, 0, 0, 0);
    zVid.m_91 = 1;
    zVid_Sync0();
    return true;
}

/* the splash screen's asset library named; no menu yet */
CExitState::CExitState() : CGameState("ExitState")
{
    m_libname.assign("common/assetlib/splash");
    m_menu = 0;
}

/* a frame of the stats screen: sound, input, the menu ticked and drawn, the frame swapped */
bool CExitState::Tick(float dT)
{
    CSnd::PostTick(dT, 0);
    CInput::Tick(dT);
    func_003654C0(m_menu, dT);
    m_menu->Draw2dOnly();
    zSysFifoEnd();
    func_003AFF30(true, true);
    return false;
}

/* torn down: the UI's sound bank, the screen's anims, the menu, the recycled assets, the UI's
 * variables, the reader archive and the menu history */
void CExitState::UnInit()
{
    zSysFifoEnd();
    sceGsSyncPath(0, 0);
    zVid.m_91 = 0;
    GetStringTables()->Unload("UIX");
    if (animfile) {
        ZAnim.CloseZAR(animfile);
        animfile = 0;
    }
    if (m_menu)
        delete m_menu;
    m_menu = 0;
    zdb::EmptyAssetRecycleBin();
    theUIVarManager.Reset();
    CRdrArchive::RemoveArchive("readerx.zar", "run/ui");
    GetMenuStack()->FlushHistory(true);
}
