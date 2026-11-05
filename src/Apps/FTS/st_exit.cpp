/* st_exit.cpp -- the end of a mission: the stats screen (SOCOM 1's Apps/FTS/st_exit.cpp; OVL1
 * 0x1F1A00-0x1F3B1F). Defined in reverse of their image order (-inline deferred). */

#include "fts_state.h"

#include <iostream>

/* unnamed yet (SOCOM 1's names in the comments) */
extern "C" {
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

enum UIVAR_LONGEVITY {
    UIVL_DORMANT = 0,
    UIVL_TEMPORARY = 1,
    UIVL_PERMANENT = 2,
    UIVL_PERSISTENT = 3
};

class CUIVariable;
class CUIVarManager {
public:
    void Reset();   /* 0x385C30 */
    CUIVariable *Add(const char *name, _zrdr *value, UIVAR_LONGEVITY longevity);   /* 0x3857C0 */
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
    unsigned char _58e[0x590 - 0x58e];
    unsigned short m_590;   // 0x590: the shots that hit (the accuracy rating's)
    unsigned char _592[0x594 - 0x592];
    unsigned short m_594;   // 0x594: the shots fired
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


/* an objective (fts_objectivecam.cpp's CMissionObjectiveCam: only its two tests and its kind's bits) */
class CMissionObjectiveCam {
public:
    bool IsDisplayOn() const;   /* 0x2C70A0 */
    bool IsComplete() const;   /* 0x2C70D0 */

    unsigned char _0[0x40];
    unsigned int m_isSecondary : 1;   // 0x40
    unsigned int m_isSubObjective : 1;   // 0x40
    unsigned int m_isCompass : 1;   // 0x40
    unsigned int m_bExtraction : 1;   // 0x40
    unsigned int m_bit4 : 1;   // 0x40: SOCOM II's, the third kind
    unsigned int m_unused : 27;   // 0x40
};

/* the objectives counted (OVL1 0x1F38F0, Init's first call on theMission's list; SOCOM 1's Init
 * counted inline): each objective's completion asked first; one of the third kind counts there,
 * else one shown counts as a secondary or a primary; then each kind's fraction, none for none --
 * the third's first (the name ours) */
extern "C" void func_001F38F0(CObjectiveCounts *counts, const std::list<CMissionObjectiveCam *> *objectives)
{
    for (std::list<CMissionObjectiveCam *>::const_iterator it = objectives->begin(); it != objectives->end();
         ++it) {
        bool complete = (*it)->IsComplete();

        if ((*it)->m_bit4) {
            counts->third.total++;
            if (complete)
                counts->third.complete++;
        } else if ((*it)->IsDisplayOn()) {
            if ((*it)->m_isSecondary) {
                counts->secondary.total++;
                if (complete)
                    counts->secondary.complete++;
            } else {
                counts->primary.total++;
                if (complete)
                    counts->primary.complete++;
            }
        }
    }
    if (counts->third.total > 0)
        counts->third.fraction = (float)counts->third.complete / (float)counts->third.total;
    else
        counts->third.fraction = 0.0f;
    if (counts->secondary.total > 0)
        counts->secondary.fraction = (float)counts->secondary.complete / (float)counts->secondary.total;
    else
        counts->secondary.fraction = 0.0f;
    if (counts->primary.total > 0)
        counts->primary.fraction = (float)counts->primary.complete / (float)counts->primary.total;
    else
        counts->primary.fraction = 0.0f;
}

/* a fraction as a whole percentage, and a value rounded (the names ours) */
static inline int Round(float f) { return (int)(0.5f + f); }
static inline int Percent(float f) { return Round(100.0f * f); }

/* the round's ratings, each a percentage put in the UI's variables (SetRoundStatUiVars' four;
 * the names ours) -- what they read: */

/* a SEAL's stats (zseal.h's CSealStats: SOCOM II's halfword counters at the SEAL's +0x58C) */
class CSealStats {
public:
    unsigned short m_count[0x3c / 2];   // 0x0
};

/* a valve's kind (zanim.h's: only the one made here) */
enum VALVE_TYPE {
    VALVE_TYPE_0,
    VALVE_TYPE_1
};

/* a named value (zanim.h's CValve: its lookup, its making and its value) */
class CValve {
public:
    static CValve *GetByName(const char *name);
    static CValve *Create(const char *name, unsigned int value, VALVE_TYPE type);

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
    unsigned int m_type : 8;   // 0x4
    unsigned int m_flags : 8;   // 0x4
};

/* a ZRDR value (zrdr.h's _zrdr, whose header this unit's CRdrArchive cannot meet: its fields and
 * DeleteChildren) */
enum ZRDR_TYPE {
    ZRDR_NULL = 0,
    ZRDR_INTEGER = 1,
    ZRDR_REAL = 2,
    ZRDR_STRING = 3,
    ZRDR_ARRAY = 4
};

struct _zrdr {
    unsigned int type : 8;   // 0x0
    unsigned int isclone : 1;   // 0x0
    unsigned int packed : 1;   // 0x0
    unsigned int nofree : 1;   // 0x0
    unsigned int unused : 5;   // 0x0
    unsigned int length : 16;   // 0x0
    union {
        float real;
        int integer;
        char *string;
        _zrdr *array;
    };   // 0x4

    void DeleteChildren();
};

/* the UI's variables (zui.h's CUIVariable: only the integer assignment) */
class CUIVariable : public _zrdr {
public:
    void UpdateDependents();
    /* an integer, the watchers told only when it changes (st_mpexit.cpp's) */
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

/* a temporary UI variable by name (ours) */
inline CUIVariable &UIVar(const char *name)
{
    return *theUIVarManager.Add(name, 0, UIVL_TEMPORARY);
}

/* the objectives rating (OVL1 0x1F3550): each kind's complete and total counts shown, then the
 * fractions weighed -- the primaries 0.7, the secondaries 0.23 and the hidden ones 0.07, a kind with
 * none giving its share to the primaries -- shown as a percentage and kept in a valve of the same
 * name; the weighed fraction returned (the two lesser weights in float locals: as constants in the
 * sum MW colours the fractions' registers the other way) */
extern "C" float func_001F3550(CObjectiveCounts *counts)
{
    UIVar("StatPrimaryObjectivesComplete") = counts->primary.complete;
    UIVar("StatPrimaryObjectivesTotal") = counts->primary.total;
    UIVar("StatSecondaryObjectivesComplete") = counts->secondary.complete;
    UIVar("StatSecondaryObjectivesTotal") = counts->secondary.total;
    UIVar("StatHiddenObjectivesComplete") = counts->third.complete;
    UIVar("StatHiddenObjectivesTotal") = counts->third.total;
    float primary = 0.7f;

    if (counts->secondary.total == 0)
        primary += 0.23f;
    if (counts->third.total == 0)
        primary += 0.07f;
    float second = 0.23f;
    float hidden = 0.07f;
    float pct = primary * counts->primary.fraction + second * counts->secondary.fraction +
                hidden * counts->third.fraction;
    int shown = Percent(pct);

    UIVar("StatObjectivesPct") = shown;
    CValve::Create("StatObjectivesPct", shown, VALVE_TYPE_1);
    return pct;
}

/* the stealth rating (OVL1 0x1F3290): 35 less the times seen, the stealth kills up to 30, 35 less
 * 35 for each alarm the "scoring_alarm" valve counts -- none under 45 seconds of mission -- as a
 * percentage; the stealth kills, the alarms, the enemy's shots and the rating shown.
 * Byte-exact (round 35; 48 off until round 34, 2 then): the first term counted down from a 35 in its
 * own variable, s3; the alarms numbered before the kills by a dead first definition ahead of them
 * (`int alarms = 0;`, the world's round-12 rule), s2 and s1; the sum a variable of its own, s3; the
 * final clamps ternaries (the target leaves their joins unpadded) -- and the kills' upper clamp a
 * TERNARY too, while the lower one stays an if: the clamp's join is then no block of its own, and
 * the alarms' `li s2,35` lands between the name's lui and the jal, the name's addiu in the delay
 * slot, as the target (as an if-statement MW put the li in the slot whatever its placement --
 * round 34's measurements, every one 2 off) */
extern "C" float func_001F3290(CSealStats **stats)
{
    int score = 35;

    score -= stats[0]->m_count[16];
    if (score < 0)
        score = 0;
    int alarms = 0;
    int kills = stats[0]->m_count[18];

    if (kills < 0)
        kills = 0;
    kills = kills > 30 ? 30 : kills;
    alarms = 35;
    CValve *alarm = CValve::GetByName("scoring_alarm");

    if (alarm)
        alarms -= 35 * alarm->m_value;
    if (alarms < 0)
        alarms = 0;
    int pct = alarms + (score + kills);

    if (theMission.m_timer < 45.0f)
        pct = 0;
    pct = pct < 0 ? 0 : pct;
    pct = pct > 100 ? 100 : pct;
    UIVar("StatStealthKills") = stats[0]->m_count[17];
    UIVar("StatAlarmsTriggered") = alarm ? alarm->m_value : 0;
    UIVar("StatEnemyShotsFired") = stats[0]->m_count[5];
    UIVar("StatStealthPct") = pct;
    return 0.01f * pct;
}


/* the accuracy rating (OVL1 0x1F2E40): the player's hits over his shots as a percentage, then the
 * team's -- every SEAL's stats summed, hits (2) over shots (4) and kills (6) -- weighed 1.3, less 1.5
 * for each of the player's counter 3, plus 1.5 for each of his counter 0, clamped to 0..100; the
 * player's accuracy, his kills, the team's kills and the rating shown, the rating returned as a
 * fraction (the name ours). Byte-exact (the states' round 41): the halfwords converted through int
 * locals (a plain unsigned short converts unsigned, bltz and all), the weighing three statements on
 * the team's own variable (one expression fuses into suba / madd) */
extern "C" float func_001F2E40(CSealStats **stats, int n)
{
    CZSealBody *player = ftsGetPlayer();
    int hit = player->m_590;
    int fired = player->m_594;
    float acc;

    if (fired > 0)
        acc = 100.0f * ((float)hit / (float)fired);
    else
        acc = 0.0f;
    int accuracy = Round(acc);
    int hits = 0;
    int shots = 0;
    int kills = 0;

    for (int i = 0; i < n; i++) {
        hits += stats[i]->m_count[2];
        shots += stats[i]->m_count[4];
        kills += stats[i]->m_count[6];
    }
    float team = 0.0f;

    if (shots > 0)
        team = 100.0f * ((float)hits / (float)shots);
    int c3 = stats[0]->m_count[3];
    int c0 = stats[0]->m_count[0];

    team *= 1.3f;
    team -= 1.5f * c3;
    team += 1.5f * c0;
    int pct = Round(team);

    pct = pct > 0 ? pct : 0;
    pct = pct < 100 ? pct : 100;
    UIVar("StatPlayerAccuracy") = accuracy;
    UIVar("StatPlayerKills") = stats[0]->m_count[6];
    UIVar("StatTeamKills") = kills;
    UIVar("StatAccuracyPct") = pct;
    return 0.01f * pct;
}

/* a mission taken into the career (OVL1 0x1F1FB0; SetRoundStatUiVars 0x1F21B0 calls it on
 * theMission +0x1950, the name ours): each percentage averaged in over the missions so far, the
 * count up one, the player's counters added.
 * OPEN, 0x1EC against 0x1F8 (scheduling): the operations are the target's, but the target runs
 * every percentage and quotient through $f1 (and the count's conversion through $f1 into $f3),
 * waiting a nop after each mtc1, where this spreads them over $f1-$f3 and schedules three fewer
 * slots. Tried: the percentages inline in the averages, as locals ahead (in a-e order, the count's
 * conversion before, between or after them), `f + 0.5f`, the count converted implicitly, the sum
 * divided as an int; round 33: all 720 orders of the five percentages with the count's conversion at
 * each place (every one 0x1EC: MW computes each at its use whatever the order -- the target's are
 * computed ahead, pe before pd, held in a2/t0/a3/a1), const locals, the percentages in an array with
 * an unrolled loop (0x228+), the constants in float locals or a defaulted parameter. The target's
 * count conversion is $f3, not deferred: its degree must fall under 32, which only an early
 * computation of the percentages gives. Round 34, all 0x1EC with each percentage still made
 * just before its use (one GPR, a1, for all five): each set through an inline's `int &` out-
 * parameter, a local struct's fields, a reference bound to one, the count read after them or
 * twice, and the five passed as by-value parameters of one inline doing the averages (a member or
 * a free function) -- MW forwards a single-use local's pure expression to its use through every
 * one of them.
 * Round 38: THE FORWARDING IS THE IR OPTIMISER'S EXPRESSION PROPAGATION (mweppc IroPropagate.c), WHICH
 * WORKS INSIDE ONE FLOW-GRAPH NODE: any control flow between the definitions and the averages stops
 * it. A one-pass `for` around the averages keeps the five percentages at their definitions (a2, t1,
 * t0, a3, a2) but leaves two blocks (0x1FC); `bool ok = true; ... if (ok) { the averages }` -- a
 * branch IRO folds only after the propagation (its EvaluateConditionals runs later in the same pass),
 * so the blocks merge again -- gives the target's size with n1 defined after pc (34 off: the
 * percentages' registers and n1's $f3 the target's; left, the target converts n+1 through $f1 into
 * $f3 and runs the first quotient through $f2/$f5 -- the scheduling the README's round-33 note
 * describes). An inline whose constant argument decides a `?:` (folded at inlining) or `if (Always())`
 * do not stop it. Not adopted: the flag is a reproduction's device, not a shape found in the
 * original -- whatever the original's control flow was (an assert, a mode parameter), it is the lead */
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

extern "C" {
void func_001F21B0(CObjectiveCounts *counts);   /* SetRoundStatUiVars */
}


/* the stats screen up: the objectives counted, the mission unloaded around the stats being set,
 * and -- unless the mission ends the game (popped) or the menu is skipped (popped and the load
 * state switched to) -- the reader archive, the menu, its anims, the UI's sound bank */
bool CExitState::Init()
{
    CObjectiveCounts counts;

    func_001F38F0(&counts, (const std::list<CMissionObjectiveCam *> *)theMission.m_objectives);
    theMission.ReOpen_Unload1();
    func_001F21B0(&counts);
    theMission.ReOpen_Unload2();
    if (theMission.m_valve_abort->m_value == 2) {
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
    m_menu->Tick(dT);
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
