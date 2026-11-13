/* st_exit.cpp -- the end of a mission: the stats screen (SOCOM 1's Apps/FTS/st_exit.cpp; OVL1
 * 0x1F1A00-0x1F3B1F). Defined in reverse of their image order (-inline deferred). */

#include "fts_state.h"

#include <iostream>

void zVid_Swap(bool flip, bool restore);   /* 0x3AFF30 */

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
    unsigned char _0[0xc8];
    unsigned int m_team;   // 0xc8 (zentity.h's CEntity's: 0x20000 the enemy)
    unsigned char _cc[0xe0 - 0xcc];
    unsigned int m_e0_0 : 12;   // 0xe0 (zentity.h's flags: m_blink_eyes ... m_include_in_targetlist)
    unsigned int m_isAlive : 1;   // 0xe0, bit 12 (zentity.h's)
    unsigned int m_e0_13 : 19;
    unsigned char _e4[0x58c - 0xe4];
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
    unsigned char _5ba[0x1044 - 0x5ba];
    float m_health;   // 0x1044 (zseal.h's)

    /* out of health (the name ours) */
    bool IsDown() const { return m_health <= 0.0f; }
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

    /* the walk begun, and whether it has ended (st_mpexit.cpp's; the teamwork rating's walk) */
    CEntityItr() : _it(GetEntityList().begin()), _iIdx(0) {}
    /* the walk begun at a position the caller takes (the two rating walks pass the list's begin(),
     * so its temporaries are made a pass ahead of Next()'s: the target's frame slots) */
    CEntityItr(std::list<CEntity *>::const_iterator i) : _it(i), _iIdx(0) {}
    bool More() const { return _it != GetEntityList().end(); }
    void Next()
    {
        do {
            ++_it;
            _iIdx++;
        } while (_it != GetEntityList().end() && !IsSeal());
    }

    std::list<CEntity *>::const_iterator _it;   // 0x0
    int _iIdx;   // 0x4
};

/* a fraction as a whole percentage, and a value rounded (the names ours) */
static inline int Round(float f) { return (int)(0.5f + f); }
static inline int Percent(float f) { return Round(100.0f * f); }

/* the round's ratings, each a percentage put in the UI's variables (SetRoundStatUiVars' four;
 * the names ours) -- what they read: */

/* a SEAL's stats (zseal.h's CSealStats: SOCOM II's halfword counters at the SEAL's +0x58C) */
class CSealStats {
public:
    void Reset();   /* OVL2 0x545200: every counter zeroed (SOCOM 1's Reset -- no `move v0,a0`, not a constructor) */

    unsigned short m_count[0x48 / 2];   // 0x0 (0x48 bytes: Reset, 0x545200, zeroes them all)
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
    CUIVariable &operator=(const char *s);   /* 0x3862A0 */
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
extern "C" float func_001F3290(CSealStats **stats, int)
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

float fabsf(float x);   /* 0x1B3620 */

/* the teamwork rating (OVL1 0x1F27C0; SetRoundStatUiVars' third, on the team it gathered -- the
 * player first -- and their stats): with fewer than three teammates down, a balance score -- 25 less
 * the kills' imbalance between the two pairs (the player's and the other) and up to 15 for twice the
 * player's counter 7 --, a health score -- 50 less 20 for each one down, 3 for each one hurt and 10
 * under a fifth (5 under a half) --, and an enemy score -- 25 less 10 for each enemy dead, 4 under a
 * fifth and 2 under a half, plus 10 for each the player captured --; none online; the wounded, the
 * downed, the captured and the rating shown, the rating returned as a fraction (the name ours).
 * Byte-exact since the game states' round 52: the walk `for (CEntityItr it(GetEntityList().begin());
 * it._it != GetEntityList().end(); it.Next())` -- the begin and the test in the loop's own text, a
 * pass shallower than Next()'s end() (MW's breadth-first expansion: a `for`'s init, increment and
 * test in that order each pass), so the slots read [begin-bt More-bt] [begin-cp Next-bt More-cp]
 * [Next-cp] as the target's; through More() 11 off, through Advance() 0x678. The history:
 * OPEN, 0x678 against 0x680 (written in the game states' round 47; everything but the walk's advance
 * and its temporaries is the target's): the down flags `int` locals set from a bool accessor (bool
 * locals are narrowed again at the sum), the lag's and the balance's clamps ternaries and the health's
 * and the penalty's ifs (an if's join takes an alignment nop the target's ternary joins do not), the
 * health 50 in its own statement, the walk's SEAL test on `&it.GetSeal()` (IsSeal() materialises the
 * bool). Left: the target's inlined Advance tests its SEAL materialised (`sltu; xori 1; bnez`), which
 * `!&GetSeal()` or `!IsSeal()` in Advance gives -- but then the out-of-line Advance 0x1F1F30
 * materialises it too (0x80 against 0x78; an inline Next() with it, `++it` over Advance and a while
 * loop measured alike); with it the rest is the iterator temporaries' frame slots: here Advance's
 * end() temporaries take the slots above the loop test's, in the target below them, `it` between
 * (for, while and an inline increment give the same order).
 * Round 48: an in-class inline Next() (`while (_it != GetEntityList().end() && !IsSeal())`) for
 * the walk, the out-of-line Advance left direct, gives 0x680 with only the frame slots off (11).
 * The four-byte slots, high to low (bt = a begin()/end() temporary, cp = the copy it is built into):
 * the target  begin-bt 156, More-bt 152, it 148, Next-bt 144, More-cp 140, Next-cp 136, then the
 *             bottom test's bt / cp 132 / 128;
 * for / while  begin-bt, Next-bt, More-bt, it, Next-cp, More-cp (the increment's ahead of the test's);
 * `if (it.More()) do { ... } while (it.More());`  begin-bt, More-bt, Next-bt, bottom-bt, it,
 *             More-cp, Next-cp, bottom-cp (the test first, but the bottom pair interleaved);
 * the test written in place (`it._it != GetEntityList().end()`)  More-bt 156, begin-bt 152,
 *             Next-bt 148, More-cp 144, it 140, Next-cp 136.
 * SetRoundStatUiVars (unwritten, its walk `count < 4 && More()` tested at the top only) holds
 * begin-bt 284, More-bt 280, it 276, More-cp 272, Next-bt 268, Next-cp 264.
 * Round 49: `it` has no slot of its own (both members scalarised: _it in a1, _iIdx dead), so the
 * slot named `it` above is begin()'s copy -- every slot is a temporary. With Next() the slots are
 * begin-bt 156, Next-bt 152, More-bt 148, begin-cp 144, Next-cp 140, More-cp 136: by inline depth
 * (the bt temporaries, then the cp ones), each depth in generation order (the increment before the
 * test). The target's [begin-bt, More-bt] [begin-cp, Next-bt] [More-cp, Next-cp] fits no one depth
 * per call: Next() through More() (0x6C0), a Step() wrapper over Next() (0x664) and More() through an
 * inline End() (0x6A4) all change the size. (C2DMessage_Q::Tick's iterator below every temporary,
 * round 48's pointer here, was a second web: fts_message.cpp, round 49 -- not this walk's case)
 * Round 50 (with SetRoundStatUiVars above, the same walk): igdump's syms number the slot objects in
 * the slots' order (@N ascending = address descending), and each chain's base temporary (bt) is made
 * one inline level before its copy (cp), level by level, a level's in generation order (a `for`: the
 * init, the increment, the test). BOTH targets then read as the Next() chain ONE LEVEL DEEPER than
 * the constructor's begin() and More()'s end(): here [begin-bt More-bt] [begin-cp Next-bt More-cp]
 * [Next-cp], SetRoundStatUiVars' [begin-bt More-bt] [begin-cp More-cp Next-bt] [Next-cp] -- where
 * this source gives [begin-bt Next-bt More-bt] [begin-cp Next-cp More-cp]. With Next() (`it.Next()`
 * for it.Advance() below) this function is at the size, 11 off, the slots alone; every deeper
 * spelling measured (a Step() wrapper, More() / AtEnd() / an End() inline in Next's test) either
 * trips the level-5 cut or lets MW share the end node in s7 */
extern "C" float func_001F27C0(CZSealBody **seals, CSealStats **stats, int n)
{
    int down1 = 0;
    int down2 = 0;
    int down3 = 0;

    if (seals[1])
        down1 = seals[1]->IsDown();
    if (seals[2])
        down2 = seals[2]->IsDown();
    if (seals[3])
        down3 = seals[3]->IsDown();
    int downed = down1 + down2 + down3;
    int wounded = 0;
    int pct;

    if (downed < 3) {
        float a = 0.5f * (stats[0]->m_count[6] + stats[1]->m_count[6]);
        float b = 0.5f * (stats[2]->m_count[6] + stats[3]->m_count[6]);
        int balance = 25;
        float sum = a + b;
        float diff = fabsf(a - b);
        float pen = 0.0f;

        if (sum > 0.0f)
            pen = 25.0f * ((100.0f * (diff / sum) - 25.0f) / 75.0f);
        int lag = 2 * stats[0]->m_count[7];

        lag = lag > 15 ? 15 : lag;
        pen -= lag;
        if (pen < 0.0f)
            pen = 0.0f;
        balance -= Round(pen);
        balance = balance < 0 ? 0 : balance;
        int health = 50;

        health -= 20 * downed;

        for (int i = 1; i < n; i++) {
            float h = seals[i]->m_health;

            if (h > 0.0f) {
                if (h < 1.0f)
                    wounded++;
                if (h < 1.0f)
                    health -= 3;
                if (h < 0.2f)
                    health -= 10;
                else if (h < 0.5f)
                    health -= 5;
            }
        }
        if (health < 0)
            health = 0;
        int enemy = 25;

        for (CEntityItr it(CEntityItr::GetEntityList().begin()); it._it != CEntityItr::GetEntityList().end();
             it.Next()) {
            if (&it.GetSeal()) {
                CZSealBody &seal = it.GetSeal();

                if (seal.m_team & 0x20000) {
                    if (!seal.m_isAlive)
                        enemy -= 10;
                    else if (seal.m_health < 0.2f)
                        enemy -= 4;
                    else if (seal.m_health < 0.5f)
                        enemy -= 2;
                }
            }
        }
        enemy += 10 * stats[0]->m_count[20];
        if (enemy < 0)
            enemy = 0;
        pct = enemy + (balance + health);
        if (theMission.m_18 == 4)
            pct = 0;
        pct = pct > 0 ? pct : 0;
        pct = pct < 100 ? pct : 100;
    } else {
        pct = 0;
    }
    UIVar("StatTeammembersWounded") = wounded;
    UIVar("StatTeammembersDowned") = downed;
    UIVar("StatEnemiesCaptured") = stats[0]->m_count[20];
    UIVar("StatTeamworkPct") = pct;
    return 0.01f * pct;
}

/* the round's stats into the UI's variables (OVL1 0x1F21B0; SOCOM 1's SetRoundStatUiVars, the
 * single-player half): the player and the SEALs of his team (bit 2) gathered, up to four, each
 * with its stats -- a missing one's a zeroed default --, the four ratings computed (objectives,
 * stealth, accuracy, teamwork), the career taken up in a single-player game (theMission's byte at
 * 0x18 2), the overall rating -- 45 % objectives, 20 % stealth, 10 % accuracy, 25 % teamwork,
 * at least 60 in that game -- shown and kept in a valve, and the time in the game as "hhh:mm:ss".
 * Byte-exact since the game states' round 52 (written in round 50, 6 off: the walk's frame slots).
 * THE SLOTS FOLLOW MW'S BREADTH-FIRST EXPANSION (the MSL headers' round 37): a pass per inline level,
 * each end()/begin() chain's base temporary made at its call's expansion and its copy a pass later,
 * so the target's Next() chain one pass deeper than begin's and the first test's is those two
 * written in the function's own text -- the walk begun through a constructor taking the list's
 * begin() and the end test `it._it != end()` in place -- with Next() unchanged; every way to deepen
 * Next() instead (below) moved the code. The walk is a do-while with the end test first (`count < 4` at the
 * bottom, jumping back to the one More() test at the top: a `for` / `while` with the count in its
 * condition is laid out test-at-the-bottom, `for (;;)` and `while (count < 4)` with the break give
 * this layout too); its step the in-class Next() with `!IsSeal()` (the target materialises the SEAL
 * test, sltu / xori / bnez, where the out-of-line Advance 0x1F1F30 does not); the time's digits by
 * `iSec -= q * c` (`%=` is a div: the target multiplies back), the first digit an if/else (a
 * ternary leaves one store), the overflow's "999:59:59" through MW's __memcpy intrinsic (the
 * array initialiser's own block copy, both loads first, from the literal: strcpy and memcpy stay
 * calls, a struct assignment pairs each load with its store), the career's `this` a local declared
 * ahead of ftsGetPlayer (computed in its delay slot, held in s0), the four default stats four
 * statements (an unrolled loop schedules the stores apart). Round 50's slots, high to low -- the
 * target begin-bt 284, More-bt 280, begin-cp 276, More-cp 272, Next-bt 268, Next-cp 264, here
 * begin-bt, More-bt, Next-bt, begin-cp, More-cp, Next-cp (@1405 @1411 @1417, @1422 @1425 @1428 in
 * igdump's syms: every walk's base temporary is made one inline level before its copy, so the
 * target's Next() chain sits one level deeper than the constructor's and More()'s). Tried: Next()
 * out of line (auto-inlined, dead-stripped), through More(), an End() / AtEnd() or a static IsEnd(it) / NotEnd(it) inline (each 0x614: any inline level around Next's end() lets MW share the end node in s7), behind a
 * Step() wrapper (the cut: 0x5C0, or the end node held in s7: 0x60C-0x614), the walk written in
 * place with a static step (0x600), the four loop forms above -- the order holds or the code moves.
 * Round 52: the constructor alone or the test alone in place are 8 and 10 off; both, by value or by
 * reference, byte-exact. The teamwork rating (above) was the same question, closed the same way */
extern "C" void func_001F21B0(CObjectiveCounts *counts)
{
    CSealStats none;

    none.Reset();
    CZSealBody *seals[4] = {0};
    CSealStats *stats[4] = {0};

    stats[0] = &none;
    stats[1] = &none;
    stats[2] = &none;
    stats[3] = &none;
    int n = 0;
    int count = 0;
    CEntityItr it(CEntityItr::GetEntityList().begin());

    do {
        if (!(it._it != CEntityItr::GetEntityList().end()))
            break;
        CZSealBody *seal = &it.GetSeal();

        if (seal) {
            if (seal == ftsGetPlayer() || seal->m_team & 2) {
                seals[count] = seal;
                stats[count] = (CSealStats *)&seal->m_58c;
                count++;
                n++;
            }
        }
        it.Next();
    } while (count < 4);
    float objectives = func_001F3550(counts);
    float stealth = func_001F3290(stats, n);
    float accuracy = func_001F2E40(stats, n);
    float teamwork = func_001F27C0(seals, stats, n);

    if (theMission.m_18 == 2) {
        CCareerStats *career = (CCareerStats *)((char *)&theMission + 0x1950);
        CZSealBody *player = ftsGetPlayer();
        int hit = player->m_590;
        int fired = player->m_594;
        float acc;

        if (fired > 0)
            acc = 100.0f * ((float)hit / (float)fired);
        else
            acc = 0.0f;
        career->AddMission(0.01f * Round(acc), accuracy, stealth, teamwork, objectives);
    }
    int total = Round(100.0f * (0.45f * objectives + 0.2f * stealth + 0.1f * accuracy + 0.25f * teamwork));

    if (theMission.m_18 == 2)
        total = total < 60 ? 60 : total;
    UIVar("StatOverallPct") = total;
    CValve::Create("StatOverallPct", total, VALVE_TYPE_1);
    char szTime[] = "000:00:00";
    int iSec = (int)theMission.m_timer;

    if (iSec < 3600000) {
        int h = iSec / 360000;

        if (h > 0)
            szTime[0] = '0' + h;
        else
            szTime[0] = ' ';
        iSec -= h * 360000;
        int th = iSec / 36000;

        szTime[1] = '0' + th;
        iSec -= th * 36000;
        int hr = iSec / 3600;

        szTime[2] = '0' + hr;
        iSec -= hr * 3600;
        int tm = iSec / 600;

        szTime[4] = '0' + tm;
        iSec -= tm * 600;
        int mn = iSec / 60;

        szTime[5] = '0' + mn;
        iSec -= mn * 60;
        int ts = iSec / 10;

        szTime[7] = '0' + ts;
        iSec -= ts * 10;
        szTime[8] = '0' + iSec;
    } else {
        Z_MEMCPY((void *)szTime, (void *)"999:59:59", sizeof(szTime));
    }
    UIVar("TimeInGame") = szTime;
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
 * original -- whatever the original's control flow was (an assert, a mode parameter), it is the lead.
 * Round 53, the same device over every order of the percentages, the count and the first quotient as a
 * float local (d= from 30 to 73: a, b, c, n1, d, e stays the best, 34 off), and the count's conversion
 * written in each average (a common subexpression: 0x200 with the device, 0x1E8 without). The GPRs say
 * the target numbers `n + 1` after `n * m_avg[0]` -- n1 is made after the first average's product --
 * and its order (the percentages' chains between the first quotient's cvt, div and add) reads as
 * latency-filling by MW's scheduler, so the IR order is not the instruction order. `#pragma
 * opt_propagation off` with n1 after pa is the target's size too (62 off). The flag is not needed to
 * hold a value: an int-sized enum local (`enum E { MAX = 0x7FFFFFFF }`, cast back to int at the sum, an
 * unsigned sum's conversion being 0x30C) gives the flag's numbers to the instruction with no control
 * flow -- IRO's propagation leaves enum-typed variables alone. So held, with the product and the first
 * sum as enum locals too, the best of all 360 orders is product, pa, sum, pb, pc, n1, pd, pe, d=19, 30
 * off at the size: not adopted either, no such enum being in evidence. */
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
    zVid_Swap(true, true);
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
