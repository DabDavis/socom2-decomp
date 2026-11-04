/* seal_tasks.cpp -- the AI's ZAnim task commands, ai::WAIT to ai::COMM (SOCOM 1's unit,
 * __sinit_seal_tasks.cpp): each command a class of static handlers (Parse, Begin, Tick, End) that
 * CSealCtrlAi::RegisterCommands (0x5DE320) hands to CZAnimMain::AddCmd under its name, a command
 * without its own Begin or End registering the one before it (as SOCOM 1's does). OVL2
 * 0x5CDBD0-~0x5DEEBF, in SOCOM 1's order (the commands in reverse alphabetical order, WAIT first,
 * then RegisterCommands); SOCOM II dropped ai::ACTION, ai::DOOR and ai::SPECIAL and added ai::CREATE,
 * ai::DRAIN, ai::GROUP and ai::TURRET (the class names of those four ours, after the commands), and
 * inlines SOCOM 1's taskFindParam. The handlers' rows are named off SOCOM II's own registration
 * table. Defined in reverse of their image order (-inline deferred). */

#include "../zkit/zkit.h"
#include "../zanim/zanim.h"
#include "../zsys/zsys.h"

namespace ai {
/* a parameter's test, as ai::PARAM's tick makes it: not equal, equal, greater, less, at least, at most
 * -- 0 and anything past 6 true (the names ours: SOCOM 1's DWARF has no such enum) */
enum TEST_OP {
    TEST_NONE = 0, TEST_NE = 1, TEST_EQ = 2, TEST_GT = 3, TEST_LT = 4, TEST_GE = 5, TEST_LE = 6,
    TEST_LAST = 255
};

/* a value tested against another (SOCOM 1's MW 2.4 inlined it into CTParam::Tick, a 7-way table
 * each time; MW 3.0 inlines no template, so SOCOM II calls the int and float instances, OVL2
 * 0x5DECB0 and 0x5DED70, from 0x5D4770 -- the unit's template tail; the name ours) */
template <class T>
bool test(const T &a, TEST_OP op, const T &b)
{
    switch (op) {
    case TEST_NE:
        return a != b;
    case TEST_EQ:
        return a == b;
    case TEST_GT:
        return a > b;
    case TEST_LT:
        return a < b;
    case TEST_GE:
        return a >= b;
    case TEST_LE:
        return a <= b;
    }
    return true;
}
}

/* the two instances, explicit until their caller (0x5D4770, ai::PARAM's tick) is written */
template bool ai::test<int>(const int &, ai::TEST_OP, const int &);
template bool ai::test<float>(const float &, ai::TEST_OP, const float &);
/* and ai::_max over bytes (OVL2 0x5DEE70: 0x5D4770 calls it on a byte it computed and the controller's
 * byte at 0x5D0, its result read with lbu -- the copy after the two tests, as their tail) */
template const unsigned char &ai::_max<unsigned char>(const unsigned char &, const unsigned char &);

/* the reader's queries (zrdr.h's, which declares a _zrdr of its own: zanim.h's here) */
_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);
bool zrdr_findRfloat(const _zrdr *rdr, const char *tag, Rfloat *out);
bool zrdr_toRfloat(const _zrdr *rdr, Rfloat *out);
bool zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n);
bool zrdr_tobool(const _zrdr *rdr, bool *out);
char *zrdr_findstring(const _zrdr *rdr, const char *tag);
bool zrdr_isitemtype(const _zrdr *rdr, int index, int type);
bool zrdr_findbool(const _zrdr *rdr, const char *tag, bool *out);
bool zrdr_findPNT3D(const _zrdr *rdr, const char *tag, PNT3D *out);
/* a team block's teams as a mask (fts_player.cpp's, OVL1 0x2B34A0) */
unsigned int ParseTeamBlock(_zrdr *rdr);
/* the same, OVL2's copy (vehicle_rdr.cpp's, OVL2 0x533C10) */
unsigned int ParseTeams(_zrdr *rdr);
bool zrdr_finduint(const _zrdr *rdr, const char *tag, unsigned int *out, int n);
/* a tag's values as a team mask (SOCOM 1's ftmask_create; OVL2 0x5B8190, seal_ex.cpp's) */
unsigned int ftmask_create(_zrdr *rdr);
/* a state and a stance by their names (OVL2 0x575410, 0x575100: seal_ai.cpp's) */
namespace sealai {
AI_STATE get_state(const char *name);
SEAL_STANCE get_stance(const char *name);
/* a mode by its name, 1 for none (OVL2 0x575790, seal_ai.cpp's) */
AI_MODE get_mode(const char *name);
}
/* a state by name or alias (below) */
AI_STATE get_state_fixup(const char *name);

/* a turret as the task reads it (zveh.h's CTurret, which this unit cannot include -- its zrdr.h
 * declares a _zrdr of its own): the flag word at 0x3A4 (bit 5 switched on) */
class CTurret;
struct TASK_TURRET {
    bool GetFlag(unsigned int mask) const { return (m_flags & mask) != 0; }

    unsigned char _0[0x3a4];
    unsigned int m_flags;   // 0x3a4
};

extern "C" {
/* whether one of the teams is in the region (OVL2 0x56D530, seal_ai.cpp's) */
bool func_0056D530(const CAiMap::region *r, unsigned int team);
/* the turret's target in sight (OVL1 0x3A7FA0, veh_turret.cpp's) and the turret switched on or off
 * (OVL1 0x3A9930, CTurret::SetActive) */
bool func_003A7FA0(CTurret *t);
void SetActive__7CTurretFb(CTurret *t, bool on);
/* a node of an anim's by a reader's tag (OVL1 0x26E2D0, zanim_anim.cpp's) */
int func_0026E2D0(CZAnim *anim, _zrdr *rdr);
/* the AI's mode put back to the one it saved, which is made 1 (OVL2 0x56D090, the SEAL lane's
 * row: SetAiMode with the byte at 0x3F5, then 1 stored there -- where SOCOM 1's CTHold::End called
 * SetAiMode with 1) */
void RestoreAiMode__11CSealCtrlAiFv(CSealCtrlAi *ai);
/* the AI switched on or off, and its alarm (SOCOM 1's static CSealCtrlAi::SetAiEnabled(bool,
 * bool), which its CTStopAll::Tick calls so; OVL2 0x574830, the SEAL lane's row), and a static
 * SOCOM II added after it (OVL2 0x598840) */
void SetAiEnabled__11CSealCtrlAiFbb(bool enabled, bool alarm);
void func_00598840(bool enabled);
/* the AI's stance set, and set for a time (OVL2 0x5631A0, 0x5631B0: the SEAL lane's rows, where
 * SOCOM 1's CTStance::Begin called CSealCtrlAi::SetStance) */
void SetStance__11CSealCtrlAiF11SEAL_STANCE(CSealCtrlAi *ai, SEAL_STANCE stance);
void SetDesiredStance__11CSealCtrlAiF11SEAL_STANCEfb(CSealCtrlAi *ai, SEAL_STANCE stance, bool b, float time);
/* CSealCtrl::ClearLook(AI_LOOK) (OVL2 0x600D40, seal_look.cpp's) */
void ClearLook__9CSealCtrlF7AI_LOOK(CSealCtrl *ctrl, int look);
/* the script state's hold bit 3 set or cleared for the AI (OVL2 0x5F3FC0, seal_aistate.cpp's
 * range) and a CSealCtrlAi function the goto's end calls first (OVL2 0x5628F0) */
void func_005F3FC0(CAiSScript *s, CSealCtrlAi *ai, bool b);
void func_005628F0(CSealCtrlAi *ai);
/* the AI's byte at 0x5BC, out of line (OVL2 0x555890: `lb` there, the caller masks it), and whether its route is done -- a path
 * at 0x508 in state 4, or 1 to 3 (OVL2 0x5629B0): the SEAL lane's rows, CTGoto::Tick's */
unsigned char func_00555890(CSealCtrlAi *ai);
bool func_005629B0(CSealCtrlAi *ai);
/* a squad's member set up again (CTTeam::Tick calls it after CSealCtrlAi::UpdateEventMask, OVL2 0x572D70,
 * as SOCOM 1's does -- respelled from its placeholder, the kit lane round 25)
 * (CSealUnit::InitalizeSealUnit, OVL2 0x5EC9F0: seal_unit.cpp's, which zseal.h's CSealUnit does not
 * declare) */
void InitalizeSealUnit__9CSealUnitFP10CZSealBodyP9CSealUnit(CZSealBody *seal, CSealUnit *unit);
/* the goto's move set up (OVL2 0x5DB8F0, this unit's: the AI, the command, a flag; whether it
 * began -- not written yet) */
bool func_005DB8F0(CSealCtrlAi *ai, void *cmd, int x);
/* a squad command by name, 0 for none (OVL2 0x58E0A0, the SEAL lane's row; a byte) */
FT_COMMAND func_0058E0A0(const char *name);
/* CAiMaps::GetRegion(const CAiMapLoc &), Tile(const CAiMap::region *, unsigned int, unsigned int)
 * and SetRegionType(const CAiMap::region *, CAiMap::region::TYPE) (OVL2 0x52D950, 0x52A6B0,
 * 0x529C00, the AI core's: zseal.h's CAiMaps stub, which this unit sees, declares none of them,
 * and ai_maps.h's CAiMaps cannot sit beside it) */
struct CAiMapRegion;
const CAiMapRegion *GetRegion__7CAiMapsFRC9CAiMapLoc(CAiMaps *maps, const CAiMapLoc &loc);
void Tile__7CAiMapsFPCQ26CAiMap6regionUiUi(CAiMaps *maps, const CAiMapRegion *r, unsigned int value, unsigned int flags);
bool SetRegionType__7CAiMapsFPCQ26CAiMap6regionQ36CAiMap6region4TYPE(CAiMaps *maps, const CAiMapRegion *r, unsigned char type);
}

/* the AI's parameters ai::PARAM tests and sets, in its parser's order (the names its tags'; the
 * enum ours, a byte) */
enum AI_PARAM {
    PARAM_NONE = 0, PARAM_COURAGE, PARAM_HEALTH, PARAM_SPEED, PARAM_ACCURACY, PARAM_SLEEP,
    PARAM_STANCE, PARAM_AWARE, PARAM_AWARE_DECAY, PARAM_INSIDE, PARAM_OUTSIDE, PARAM_WEAPON_UP,
    PARAM_SPRAY, PARAM_DOCORPSE, PARAM_DEEPSLEEPER, PARAM_NOFADE, PARAM_VISRANGE, PARAM_THROTTLE,
    PARAM_ROLE, PARAM_SLEEP_REGION, PARAM_AWARE_COUNT, PARAM_CANCHAT, PARAM_REDSMOKE,
    PARAM_LAST = 255
};

/* a parameter's name, "param_none" for none (OVL2 0x5DEB10: CTParam::Parse matches its tag against
 * each in turn, where SOCOM 1's get_param(const char *) compared the tag with each name; the unit's
 * first function, ahead of RegisterCommands. The default preset in a local and each case returning:
 * the preset lands in the range test's delay slot and every case, the last too, branches to the
 * shared return -- assigning in the cases drops the last branch; the name ours) */
const char *get_param_name(AI_PARAM p)
{
    const char *s = "param_none";

    switch (p) {
    case PARAM_COURAGE:
        return "courage";
    case PARAM_HEALTH:
        return "health";
    case PARAM_SPEED:
        return "speed";
    case PARAM_ACCURACY:
        return "accuracy";
    case PARAM_SLEEP:
        return "sleep";
    case PARAM_STANCE:
        return "stance";
    case PARAM_AWARE:
        return "aware";
    case PARAM_AWARE_DECAY:
        return "aware_decay";
    case PARAM_INSIDE:
        return "inside";
    case PARAM_OUTSIDE:
        return "outside";
    case PARAM_WEAPON_UP:
        return "weapon_up";
    case PARAM_SPRAY:
        return "spray";
    case PARAM_DOCORPSE:
        return "docorpse";
    case PARAM_DEEPSLEEPER:
        return "deepsleeper";
    case PARAM_NOFADE:
        return "nofade";
    case PARAM_VISRANGE:
        return "visrange";
    case PARAM_THROTTLE:
        return "throttle";
    case PARAM_ROLE:
        return "role";
    case PARAM_SLEEP_REGION:
        return "sleep_region";
    case PARAM_AWARE_COUNT:
        return "aware_count";
    case PARAM_CANCHAT:
        return "canchat";
    case PARAM_REDSMOKE:
        return "redsmoke";
    }
    return s;
}

/* the commands' ids (SOCOM 1's ai_cmdid, 0x18 bytes: RegisterCommands stores every command's id
 * in the first word, which each Parse copies into its block's type; the third is the type of the
 * command argument that carries a task's SEAL) */
int ai_cmdid[6];

/* the command argument a task's SEAL comes in (the name ours): the SEAL at 0xC */
struct TASK_PARAM {
    _zanim_cmd_hdr hdr;   // 0x0
    unsigned char _4[0x8 - 0x4];
    CSealCtrlAi *m_ai;   // 0x8: the SEAL's AI controller, when it has one (CTSelect::Create)
    CZSealBody *m_seal;   // 0xc
};

/* the SEAL's AI controller, if its controller is one (an inline of its own: the ternary around it
 * keeps the result in both arms) */
inline CSealCtrlAi *SealAi(CZSealBody *seal)
{
    return ((CSealCtrl *)seal->m_control)->GetSealCtrlAi();
}

/* a task command's block: its header, then which SEAL argument it acts on (the names ours) */
struct TASK_HDR {
    _zanim_cmd_hdr hdr;   // 0x0
    int m_param;   // 0x4
};

/* THE HANDLERS' FIND: the index'th SEAL argument of the running anim (SOCOM 1's taskFindParam(type,
 * index), out of line there; SOCOM II has it in every handler), then its SEAL, WRITTEN OUT in the
 * handler (the kit lane, round 23: 15 handlers byte-exact, open since round 8). The handler
 * declares the find's type, index and counter as its OWN locals, then its block (`cmd`), then the
 * list: MW numbers named locals in declaration order, and the list, read first by its null test,
 * moves to the end -- type s4, index s3, counter s2, block s1, list s0, the target's registers in
 * every handler that uses its block after the find (README "MW's saved-register order is a
 * NUMBERING"). As an inline (rounds 8-22: `static taskFindParam` under inline TASK_HDR::GetParam/
 * GetSeal/GetAi) the find's variables are numbered after EVERY named local of the handler, its
 * block included, which took s4 in all thirteen; an outer inline's locals are numbered before the
 * inlines it expands too (a whole-handler inline, its local block copy-propagation-proof, gave s4),
 * so no inline shape numbers the block after the counter. The rest reproduces what the inline
 * compiled to: its returns are the stores to `found` and the jumps to the label, the end of the
 * arguments the `!(bool)p` materialised (sltu, xori) through a bool of its own (tested straight in
 * the `if` MW branches on p), each `return p` a block of its own (`b` to the join: a `goto` alone
 * folds into the compare). Then the argument's cast and its SEAL, as the inline's GetParam/GetSeal
 * did; a handler adds its controller or AI. A macro: the original's spelling is unknown -- this is
 * the one shape found that numbers the block between the counter and the list. */
#define TASK_SEAL(T)                                                        \
    unsigned int type = ai_cmdid[2];                                        \
    int index = ((TASK_HDR *)hdr)->m_param;                                 \
    int i;                                                                  \
    T *cmd = (T *)hdr;                                                      \
    CZAnimArgList *list = ZAnim.m_CurAnim->m_arg_list;                      \
    _zanim_cmd_hdr *found;                                                  \
                                                                            \
    if (!list) {                                                            \
        found = 0;                                                          \
        goto found_it;                                                      \
    }                                                                       \
    for (i = 0;; i++) {                                                     \
        _zanim_cmd_hdr *p = list->GetCmd(i);                                \
        bool end = !(bool)p;                                                \
                                                                            \
        if (end)                                                            \
            break;                                                          \
        if (p->data_type == type && index-- == 0) {                         \
            found = p;                                                      \
            goto found_it;                                                  \
        }                                                                   \
    }                                                                       \
    found = 0;                                                              \
found_it:                                                                   \
    TASK_PARAM *param = found ? (TASK_PARAM *)found : 0;                    \
    CZSealBody *seal = param ? param->m_seal : 0

/* a block allocated and its header made: the type the registration left, the size (SOCOM II's
 * _calloc called without the file and line SOCOM 1's passed) */
template <class T> inline T *NewTask()
{
    T *cmd = (T *)_calloc(sizeof(T), 1, 0, 0);

    cmd->hdr.data_size = sizeof(T);
    cmd->hdr.data_type = ai_cmdid[0];
    return cmd;
}

/* a name's length, 0 for none (inline;
 * the names ours) */
inline unsigned int TaskStrLen(const char *s)
{
    return s ? strlen(s) : 0;
}

/* a name a block carries at its end, sized to it (its Set an inline member, the name's address a
 * local of its own: the target computes it once, ahead of the test, and stores the empty name
 * through it -- written m_str in place MW folds it into each arm; the names ours) */
struct TASK_NAME {
    unsigned int Set(const char *s)
    {
        char *d = m_str;

        if (s) {
            strcpy(d, s);
            return strlen(s);
        }
        *d = 0;
        return 0;
    }

    char m_str[16];
};

/* a size rounded up to words (inline; the name ours) */
inline unsigned int TaskAlign(unsigned int n)
{
    if (n & 3)
        n += 4 - (n & 3);
    return n;
}

/* a period and its countdown as the tasks hold it: SOCOM 1's CAiTimer (its DWARF: Rfloat m_period,
 * float m_timer; 0xc bytes), its inlines ours -- aitimer.h's CAiTimer spells the period as two
 * floats. Set's `this` stays in a register across rand() (CTWait::Parse) */
struct TASK_TIMER {
    /* SOCOM 1's Rfloat's operator float, through a result local and the random factor's own */
    float Period() const
    {
        float f;

        if (m_period.m_range == 0.0f)
            f = m_period.m_min;
        else {
            float r = 4.656613e-10f * rand();
            f = m_period.m_min + m_period.m_range * r;
        }
        return f;
    }
    void Reset() { m_timer = Period(); }
    /* the countdown run down by dt and drawn again when it ends, whether it did -- an infinite one
     * never does (inline: its by-value dt is read ahead of the test, CTWait::Tick 0x5CDBD0) */
    bool Update(float dt)
    {
        bool done = false;

        if (m_timer != CAiTimer::infinite) {
            m_timer -= dt;
            done = m_timer != CAiTimer::infinite && m_timer <= 0.0f;
            if (done)
                Reset();
        }
        return done;
    }
    void Set(const Rfloat &p)
    {
        m_period = p;
        Reset();
    }

    Rfloat m_period;   // 0x0
    float m_timer;   // 0x8
};

/* ai::SOUND (SOCOM 1's CTSound): a sound played on the SEAL */
struct CTSound : TASK_HDR {
    /* the bits set through bool setters (true `daddiu`, false `andi zero,1` and its shift; the
     * names ours) */
    void SetStarted(bool b) { m_started = b; }
    void SetBit7(bool b) { m_bit7 = b; }

    unsigned char m_8_0 : 7;   // 0x8
    unsigned char m_bit7 : 1;   // 0x8 bit 7
    unsigned char m_started : 1;   // 0x9 bit 0
    unsigned char m_9_1 : 7;
    unsigned char _a[0xc - 0xa];
    CSndInstance *m_inst;   // 0xc: the sound playing

    static void Begin(_zanim_cmd_hdr *hdr);
    static void End(_zanim_cmd_hdr *hdr);
};

/* ai::SELECT (SOCOM 1's CTSelect) */
struct CTSelect : TASK_HDR {
    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
    static TASK_PARAM *Create(CZSealBody *seal);
};

/* ai::SETMODE (SOCOM 1's CTSetMode, its mode a word there): the AI's mode set, or for none the AI
 * put to sleep -- "always", or while it has not respawned unless "respawn_only" (the tags';
 * SOCOM II's mode a 16-bit field, the two tags bits 16 and 17 of the same word) */
struct CTSetMode : TASK_HDR {
    enum { MODE = 0xFFFF, ALWAYS = 0x10000, RESPAWN_ONLY = 0x20000 };

    void SetMode(AI_MODE mode)
    {
        m_flags &= ~MODE;
        m_flags |= mode;
    }

    unsigned int m_flags;   // 0x8: the AI_MODE (the reader's first item, 0 to sleep), the tags

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
};

/* ai::STOPALL (SOCOM 1's CTStopAll): every AI switched on or off */
struct CTStopAll : TASK_HDR {
    unsigned char m_enable : 1;   // 0x8 bit 0 (the reader's own value)
    unsigned char m_inactive : 1;   // bit 1 (the tag "inactive")

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
    /* a block made (inline: its arguments read before the allocation) */
    static CTStopAll *Create(bool enable, bool inactive)
    {
        CTStopAll *cmd = NewTask<CTStopAll>();

        cmd->m_enable = enable;
        cmd->m_inactive = inactive;
        return cmd;
    }
};

/* an entity a task names (the names ours): 1 once the name is the running anim's */
struct TASK_ENT {
    /* the name added to the running anim's (inline: its `this` stays in a register across the
     * call) */
    void SetName(const char *name)
    {
        if (name && ZAnim.m_CurAnim) {
            m_name = ZAnim.m_CurAnim->AddName(name);
            m_type = 1;
        }
    }

    /* the entity: by the name until it is found, then by its id -- unless it carries bit 13 (new in
     * SOCOM II), when the name is asked again each time; none for no name (inline; the name ours) */
    enum TYPE { TYPE_NONE = 0, TYPE_NAME = 1, TYPE_ID = 2 };

    CEntity *Get()
    {
        CZAnim *anim = ZAnim.m_CurAnim;
        CEntity *e = 0;

        switch (m_type) {
        case TYPE_NONE:
            break;
        case TYPE_NAME: {
            e = CEntity::GetEntityList().GetEntity(anim ? anim->GetName(m_name) : 0);
            m_id = e ? e->m_id : CEntity::invalidId;

            /* preset, then cleared under two materialised tests: `else byId = byId;` keeps the
             * inner test a branch */
            bool byId = true;
            bool gone = !(bool)e;

            if (!gone) {
                bool off = !(bool)e->m_bit13;

                if (!off)
                    byId = false;
                else
                    byId = byId;
            }
            m_type = byId ? TYPE_ID : TYPE_NAME;
            break;
        }
        case TYPE_ID:
            e = CEntity::GetEntityList().GetEntityById(m_id);
            break;
        }
        return e;
    }

    unsigned short m_type;   // 0x0: 0 none, 1 by the name, 2 by the id
    unsigned short m_name;   // 0x2
    unsigned int m_id;   // 0x4: the entity's id once found
};

/* ai::INRANGE (SOCOM 1's CTRange): an entity within a range of the SEAL */
struct CTRange : TASK_HDR {
    float m_range2;   // 0x8: the range squared
    TASK_ENT m_ent;   // 0xc

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
    /* a block made (inline: the range read before the allocation) */
    static CTRange *Create(float range, const char *name)
    {
        CTRange *cmd = NewTask<CTRange>();

        cmd->m_range2 = range * range;
        cmd->m_ent.m_name = 0;
        cmd->m_ent.m_id = CEntity::invalidId;
        cmd->m_ent.m_type = 0;
        cmd->m_ent.SetName(name);
        return cmd;
    }
};

/* the AI maps as the tasks read them (ai_maps.h's CAiMaps, which this unit cannot see beside zseal.h's):
 * the cookie ReadMapSet moves on at 0xDC */
struct TASK_AIMAPS {
    unsigned char _0[0xdc];
    unsigned char m_cookie;   // 0xdc
};

/* a pickup's contents as the tasks read them: the weapon at 0xC (the names ours) */
struct TASK_PICKUPDATA {
    unsigned char _0[0xc];
    CZWeapon *m_weapon;   // 0xc
};

/* ai::INREGION (SOCOM 1's CTRegion, rewritten): a region of the AI maps holding an entity (kind 1),
 * one of a team's (2), an item (3) or a database node (4) -- what is tested the first tag of
 * "entity", "weapon", "dbnode" and "team" given (the names ours but the tags') */
struct CTRegion : TASK_HDR {
    /* the region, looked up again when the maps have been read since (inline; the name ours) */
    bool Fresh()
    {
        unsigned char cookie = ((TASK_AIMAPS *)&AiMaps)->m_cookie;
        bool same = cookie == m_cookie;

        m_cookie = cookie;
        return same;
    }
    const CAiMap::region *GetRegion()
    {
        const CAiMap::region *r;

        if (!m_region || !Fresh())
            r = m_region = AiMaps.GetRegion(m_loc);
        else
            r = m_region;
        return r;
    }

    CAiMapLoc m_loc;   // 0x8: the tag "region"'s cell
    const CAiMap::region *m_region;   // 0xc: the region, once looked up
    union {
        TASK_ENT m_ent;   // 0x10: kind 1
        unsigned int m_team;   // kind 2: a team mask
        int m_weapon;   // kind 3: an EQUIP_ITEM
        int m_node;   // kind 4: a node of the running anim's
    };
    unsigned char m_cookie;   // 0x18: the AI maps' cookie when the region was looked up
    unsigned char m_kind;   // 0x19

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
};

/* the anim set a task names when it names none (OVL2 data 0x650AA8, 0xFFFF: CTSetAnimSet's and
 * CTCreate's handlers read it as a word; the name ours) */
extern int data_00650AA8;

/* ai::SETANIMSET (SOCOM 1's CTSetAnimSet, the name now a name of the running anim's): the SEAL's
 * anim set changed -- to the bored one (flag 2), or a named one; flag 4 the default's, and an
 * entity it applies to (the names ours but the tags') */
struct CTSetAnimSet : TASK_HDR {
    unsigned int m_flags;   // 0x8
    short m_set;   // 0xc: the tag "name", as a name of the running anim's
    unsigned char _e[0x10 - 0xe];
    TASK_ENT m_ent;   // 0x10: the tag "entity"

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
};

/* ai::TURRET (SOCOM II's): a turret entity tested for a target (flag 1) or being active (2), or set
 * active (4, the value at 0xc) -- the names ours but the tags' */
struct CTTurret : TASK_HDR {
    unsigned int m_flags;   // 0x8
    int m_active;   // 0xc: the tag "set_active"'s value
    TASK_ENT m_ent;   // 0x10: the tag "name"

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
    /* a block made for a name (inline, as CTRange's: the entity's `this` kept across AddName) */
    static CTTurret *Create(const char *name)
    {
        CTTurret *cmd = NewTask<CTTurret>();

        cmd->m_ent.m_name = 0;
        cmd->m_ent.m_id = CEntity::invalidId;
        cmd->m_ent.m_type = 0;
        cmd->m_ent.SetName(name);
        return cmd;
    }
};

/* ai::INVIEW (SOCOM 1's CTInView): a viewer entity seeing a target entity, or any of a team's, with
 * an item in hand (the names ours but the tags') */
struct CTInView : TASK_HDR {
    int m_equip;   // 0x8: the tag "equip", an EQUIP_ITEM (0 for none)
    unsigned int m_team;   // 0xc: the tag "team"'s mask
    TASK_ENT m_viewer;   // 0x10: the tag "viewer"
    TASK_ENT m_target;   // 0x18: the tag "target"

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    /* a block made (inline: its arguments read before the allocation) */
    static CTInView *Create(const char *viewer, const char *target, const char *equip, unsigned int team)
    {
        CTInView *cmd = NewTask<CTInView>();

        cmd->m_viewer.m_name = 0;
        cmd->m_viewer.m_id = CEntity::invalidId;
        cmd->m_viewer.m_type = 0;
        cmd->m_viewer.SetName(viewer);
        cmd->m_target.m_name = 0;
        cmd->m_target.m_id = CEntity::invalidId;
        cmd->m_target.m_type = 0;
        cmd->m_target.SetName(target);
        cmd->m_equip = equip ? CZWeapon::GetWeaponId(equip) : 0;
        cmd->m_team = team;
        return cmd;
    }
};

/* ai::INSTATE (SOCOM 1's CTInState): an entity's AI in a state, or with it on its stack */
struct CTInState : TASK_HDR {
    unsigned int m_state : 31;   // 0x8: an AI_STATE
    unsigned char m_stack : 1;   // 0xb bit 7: anywhere on the state stack (the tag "stack")
    TASK_ENT m_ent;   // 0xc

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    /* a block made (inline: its arguments read before the allocation) */
    static CTInState *Create(AI_STATE state, bool stack, const char *name)
    {
        CTInState *cmd = NewTask<CTInState>();

        cmd->m_state = state;
        cmd->m_stack = stack;
        cmd->m_ent.m_name = 0;
        cmd->m_ent.m_id = CEntity::invalidId;
        cmd->m_ent.m_type = 0;
        cmd->m_ent.SetName(name);
        return cmd;
    }
};

/* ai::TEAM (SOCOM 1's CTTeam): the SEAL joins, leaves or tests a team mask */
struct CTTeam : TASK_HDR {
    unsigned int m_mask;   // 0x8: the teams (the tag's values)
    int m_op;   // 0xc: 0 join, 1 leave, 2 ismember

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
    /* a block made (inline: its arguments read before the allocation) */
    static CTTeam *Create(signed char op, _zrdr *tag)
    {
        CTTeam *cmd = NewTask<CTTeam>();

        cmd->m_op = -1;
        cmd->m_mask = 0;
        cmd->m_op = op;
        cmd->m_mask = ftmask_create(tag);
        return cmd;
    }
};

/* ai::PURSUE (SOCOM 1's CTPursue): the pursuit state pushed for a while */
struct CTPursue : TASK_HDR {
    float m_duration;   // 0x8

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static void Begin(_zanim_cmd_hdr *hdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
    static void End(_zanim_cmd_hdr *hdr);
};

/* ai::HOLD (SOCOM 1's CTHold): a wait, the AI's mode 2 while it runs */
struct CTHold : TASK_HDR {
    Rfloat m_wait;   // 0x8
    float m_timer;   // 0x10: -1 until begun

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static void Begin(_zanim_cmd_hdr *hdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
    static void End(_zanim_cmd_hdr *hdr);
    /* a block made: a wait of -1 (inline) */
    static CTHold *Create()
    {
        CTHold *cmd = NewTask<CTHold>();

        cmd->m_wait = -1.0f;
        cmd->m_timer = 0.0f;
        return cmd;
    }
};

/* ai::EVENT (SOCOM 1's CTEvent) */
struct CTEvent : TASK_HDR {
    /* the bits set through bool setters (the names ours) */
    void SetVis(bool b) { m_vis = b; }
    void SetRange(bool b) { m_hasRange = b; }
    void SetDuration(bool b) { m_hasDuration = b; }
    void SetNode(bool b) { m_hasNode = b; }

    CPnt3D m_pos;   // 0x8: the tag "pos"
    float m_range;   // 0x14: the tag "range"
    float m_duration;   // 0x18: the tag "duration"
    int m_nodes;   // 0x1c: the tag "node"'s names, the running anim's
    short m_type;   // 0x20: a CAiEvent::EVENT, the tag "type"
    unsigned char m_vis : 1;   // 0x22 bit 0: the tag "vis"
    unsigned char m_hasRange : 1;   // bit 1
    unsigned char m_hasDuration : 1;   // bit 2
    unsigned char m_hasNode : 1;   // bit 3
    unsigned char _22 : 4;
    unsigned char _23;   // 0x24 bytes

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static void Begin(_zanim_cmd_hdr *hdr);
};

/* ai::DEBUG (SOCOM 1's CTDebug): a message, "DEBUG on"/"DEBUG off" for the words on and off (the
 * mode 1 or 0, else -1) -- the block sized to it, its type left unset (the names ours) */
struct CTDebug : TASK_HDR {
    int m_mode;   // 0x8
    TASK_NAME m_text;   // 0xc

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
    /* a block made for a text, or none (inline) */
    static CTDebug *Create(const char *text)
    {
        if (!text)
            return 0;
        unsigned int size = TaskAlign(TaskStrLen(text)) + 16;
        CTDebug *cmd = (CTDebug *)_calloc(size, 1, 0, 0);

        cmd->hdr.data_size = size;
        cmd->m_text.Set(text);
        return cmd;
    }
};

/* ai::DRAIN (SOCOM II's) */
struct CTDrain : TASK_HDR {
    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static void Begin(_zanim_cmd_hdr *hdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
    static void End(_zanim_cmd_hdr *hdr);
};

/* ai::WAIT (SOCOM 1's CTWait): a wait drawn when it begins */
struct CTWait : TASK_HDR {
    TASK_TIMER m_wait;   // 0x8

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static void Begin(_zanim_cmd_hdr *hdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
    /* a block made (inline: the period set, the timer drawn, and drawn again) */
    static CTWait *Create(const Rfloat &wait)
    {
        CTWait *cmd = NewTask<CTWait>();

        cmd->m_wait.Set(wait);
        cmd->m_wait.Reset();
        return cmd;
    }
};

/* ai::STANCE (SOCOM 1's CTStance): a stance taken, and a peek (left -1, up 0, right 1, none 2) for a
 * time -- words the parser stores sign-extended from byte-enum locals, the callees taking the byte
 * (the names ours but the tags') */
struct CTStance : TASK_HDR {
    int m_peek;   // 0x8: the tag "peek"
    int m_stance;   // 0xc: the tag "stance"
    float m_time;   // 0x10: the tag "delay"
    float m_timer;   // 0x14

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static void Begin(_zanim_cmd_hdr *hdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
    static void End(_zanim_cmd_hdr *hdr);
    /* a block made (inline: its by-value arguments read before the allocation) */
    static CTStance *Create(SEAL_STANCE stance, SEAL_STANCE peek, float time)
    {
        CTStance *cmd = NewTask<CTStance>();

        cmd->m_time = time;
        cmd->m_peek = peek;
        cmd->m_stance = stance;
        return cmd;
    }
};

/* ai::PAINT (SOCOM 1's CTPaint): a region of the AI maps given a soil, a tile or a type -- the
 * region's cell, the flags (bit 2 a soil, bit 3 a tile), the value and the mode (1 soil, 2 tile, 3
 * a type) its Parse (0x5D57C0) sets; the names ours */
struct CTPaint : TASK_HDR {
    CAiMapLoc m_loc;   // 0x8
    unsigned int m_flags;   // 0xc
    unsigned short m_value;   // 0x10
    unsigned char m_mode;   // 0x12
    unsigned char m_13;   // 0x13

    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
};

/* ai::MOVE (SOCOM 1's CTMove): a move to a point, a node, the SEAL itself or relative to it -- the
 * kinds and fields its Parse (0x5D64B0) sets from the tags (the names ours but the tags') */
struct CTMove : TASK_HDR {
    CPnt3D m_pos;   // 0x8: the tag "pos" (or "ofs")
    CPnt3D m_dir;   // 0x14: the tag "dir"
    float m_dist;   // 0x20: the tag "offset" (or "dist")
    int m_node;   // 0x24: the tag "node"'s names, or "relseal"'s value
    unsigned char m_kind : 4;   // 0x28: 1 node, 2 self, 3 relseal, 4 relseal with a dir
    unsigned char m_28_4 : 1;
    unsigned char m_blocking : 1;   // bit 5: the tag "blocking"
    unsigned char m_28_6 : 2;

    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
    static void End(_zanim_cmd_hdr *hdr);
};

/* ai::MACRO (SOCOM 1's CTMacro): an anim of the set, by name -- the block sized to the name */
struct CTMacro : TASK_HDR {
    /* (the name ours) */
    void SetBlocking(bool b) { m_blocking = b; }

    CZAnim *m_anim;   // 0x8
    unsigned char m_blocking : 1;   // 0xc bit 0: the tag "blocking"
    unsigned char _c1 : 7;
    unsigned char _d[0x10 - 0xd];
    TASK_NAME m_name;   // 0x10

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static void Begin(_zanim_cmd_hdr *hdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
};

/* ai::LOOKAT (SOCOM 1's CTLookAt) */
struct CTLookAt : TASK_HDR {
    /* (the names ours) */
    void SetLooking(unsigned int n) { m_looking = n; }

    Rfloat m_wait;   // 0x8: the tag "wait"
    float m_timer;   // 0x10
    float m_rate;   // 0x14: the tag "rate" (0.85 unless given)
    unsigned char m_kind : 4;   // 0x18: what is looked at -- 2 the entity (the names ours)
    unsigned char m_looking : 4;   // 0x18 bits 4-7 (bit 4: the look to clear at the end)
    unsigned int m_flags : 16;   // 0x19: the tag "flags", look flags by name (lw/sw: a word's bits)
    unsigned int m_18_24 : 8;
    unsigned char _1c[0x28 - 0x1c];
    TASK_ENT m_ent;   // 0x28: the tag "entity"

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static void Begin(_zanim_cmd_hdr *hdr);
    static void End(_zanim_cmd_hdr *hdr);
    static CTLookAt *Create(const char *name);
};

/* a scripted AI group (ai_group.cpp's class, OVL2 0x60C190-0x60EDFF: its Find by id, 0x60E460) */
class CAiGroup {
public:
    static CAiGroup *Find(int id);
};

/* ai::GROUP (SOCOM II's) */
struct CTGroup : TASK_HDR {
    unsigned char _8[0x14 - 0x8];
    unsigned int m_14;   // 0x14: the group's id (CAiGroup::Find's), 0 once ended

    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
    static void End(_zanim_cmd_hdr *hdr);
};

/* ai::GOTO (SOCOM 1's CTGoto) */
struct CTGoto : TASK_HDR {
    /* (the names ours) */
    void SetBit1(bool b) { m_bit1 = b; }

    unsigned char _8[0xc - 0x8];
    unsigned char m_bit0 : 1;   // 0xc bit 0
    unsigned char m_bit1 : 1;   // bit 1
    unsigned char _c2 : 6;
    unsigned char _d;
    unsigned char m_done : 1;   // 0xe bit 0: the tick done at once

    static void Begin(_zanim_cmd_hdr *hdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
    static void End(_zanim_cmd_hdr *hdr);
};

/* ai::FIREWEAPON (SOCOM 1's CTFireWeapon) */
struct CTFireWeapon : TASK_HDR {
    void SetFiring(bool b) { m_firing = b; }

    int m_item;   // 0x8: an EQUIP_ITEM (a word: the kit's test takes its byte)
    unsigned char m_firing : 1;   // 0xc bit 0 (the name ours)
    unsigned char _d[0x2c - 0xd];   // 0x2c bytes

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static void Begin(_zanim_cmd_hdr *hdr);
};

/* ai::FIREMODE (SOCOM 1's CTFireMode) */
struct CTFireMode : TASK_HDR {
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
};

/* a valve command's data as a task holds it (SOCOM 1's VALVE_CMD_DATA, zvalve/valve.h's, which this
 * unit does not include): the valve set before the compare and cleared after (OVL1 0x353FD0) */
struct VALVE_CMD_DATA {
    CValve *m_valve;   // 0x0
    unsigned char _4[0xc - 0x4];

    bool Compare();
};

/* ai::VALVE (SOCOM 1's CTValve): an AI valve compared -- the valve the SEAL's AI holds, a word its
 * Parse stores, narrowed to the byte enum at the call (`lbu`; the names ours) */
struct CTValve : TASK_HDR {
    VALVE_CMD_DATA m_data;   // 0x8
    int m_valve;   // 0x14: a SEAL_VALVE

    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
};

/* ai::COMM (SOCOM 1's CTComm): a squad command to a team */
struct CTComm : TASK_HDR {
    unsigned int m_team;   // 0x8
    unsigned short m_cmd;   // 0xc

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    /* a block made (inline: its arguments read before the allocation) */
    static CTComm *Create(unsigned int team, FT_COMMAND cmd)
    {
        CTComm *c = NewTask<CTComm>();

        c->m_team = team;
        c->m_cmd = cmd;
        return c;
    }
};

/* ai::PLAY (SOCOM 1's CTPlay): anims played, their types at the block's end (the names ours) */
struct CTPlay : TASK_HDR {
    Rfloat m_8;   // 0x8
    Rfloat m_10;   // 0x10
    unsigned char _18[0x20 - 0x18];
    signed char m_count;   // 0x20: the anims (SOCOM 1's a bitfield at 0x20 bit 1)
    unsigned char _21;
    unsigned char m_22;   // 0x22: 11 when made
    unsigned char _23[0x30 - 0x23];
    unsigned int m_anims[1];   // 0x30: m_count of them (SOCOM 1's at 0x24)

    static CTPlay *Create(int count, unsigned int *anims, float a, float b);
};

/* the pursuit state as the task sets it (CAiSPursue, seal_ai.cpp's: its duration at 0x58) */
class CAiSPursueTask : public CAiState {
public:
    float m_duration;   // 0x58
};

/* a squad command to a team, when the command is known (OVL2 0x5DE250) */
_zanim_cmd_hdr *CTComm::Parse(_zrdr *rdr)
{
    CTComm *cmd = 0;
    unsigned int team = 0;

    if (zrdr_finduint(rdr, "team", &team, 1)) {
        FT_COMMAND c = func_0058E0A0(zrdr_findstring(rdr, "cmd"));

        if (c != CMD_UNKNOWN)
            cmd = Create(team, c);
    }
    return &cmd->hdr;
}

/* (OVL2 0x5DD4F0) */
void CTDrain::Begin(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTDrain);
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? ((CSealCtrl *)seal->m_control)->GetSealCtrlAi() : 0;

    if (!seal || !ctrl)
        return;
}

/* done at once (OVL2 0x5DD3C0) */
bool CTDrain::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTDrain);
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? ((CSealCtrl *)seal->m_control)->GetSealCtrlAi() : 0;

    if (!seal || !ctrl) {
        *dT = 0.0f;
        return true;
    }
    *dT = 0.0f;
    return true;
}

/* (OVL2 0x5DD2A0) */
void CTDrain::End(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTDrain);
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? ((CSealCtrl *)seal->m_control)->GetSealCtrlAi() : 0;

    if (!seal || !ctrl)
        return;
}

/* (OVL2 0x5D7170: the block 0x14 bytes and the name's, rounded to words) */
_zanim_cmd_hdr *CTMacro::Parse(_zrdr *rdr)
{
    CTMacro *cmd = 0;
    const char *name = zrdr_findstring(rdr, "anim");
    bool blocking = zrdr_findtag(rdr, "blocking") != 0;

    if (name) {
        unsigned int size = TaskAlign(TaskStrLen(name)) + 20;

        cmd = (CTMacro *)_calloc(size, 1, 0, 0);
        cmd->hdr.data_size = size;
        cmd->hdr.data_type = ai_cmdid[0];
        cmd->m_anim = 0;
        cmd->SetBlocking(blocking);
        cmd->m_name.Set(name);
    }
    return &cmd->hdr;
}

/* (OVL2 0x5D0B40: the peek a byte-enum local, 2 when there is none) */
_zanim_cmd_hdr *CTStance::Parse(_zrdr *rdr)
{
    float delay = 0.0f;
    const char *stance = zrdr_findstring(rdr, "stance");
    const char *peek = zrdr_findstring(rdr, "peek");

    zrdr_findreal(rdr, "delay", &delay, 1);
    SEAL_STANCE st = sealai::get_stance(stance);
    SEAL_STANCE pk = STANCE_PRONE;

    if (peek) {
        if (!strcmp(peek, "left"))
            pk = STANCE_NONE;
        else if (!strcmp(peek, "right"))
            pk = STANCE_CROUCH;
        else if (!strcmp(peek, "up"))
            pk = STANCE_STAND;
    }
    return &Create(st, pk, delay)->hdr;
}

/* (OVL2 0x5D2310) */
_zanim_cmd_hdr *CTSetAnimSet::Parse(_zrdr *rdr)
{
    CTSetAnimSet *cmd = 0;
    const char *name = zrdr_findstring(rdr, "name");

    if (name) {
        cmd = NewTask<CTSetAnimSet>();
        cmd->m_flags = 0;
        if (strstr(name, "bored"))
            cmd->m_flags |= 2;
        else {
            cmd->m_set = data_00650AA8;
            if (name && ZAnim.m_CurAnim)
                cmd->m_set = ZAnim.m_CurAnim->AddName(name);
        }
        if (zrdr_findtag(rdr, "default"))
            cmd->m_flags |= 4;
        const char *ent = zrdr_findstring(rdr, "entity");

        cmd->m_ent.m_name = 0;
        cmd->m_ent.m_id = CEntity::invalidId;
        cmd->m_ent.m_type = 0;
        cmd->m_ent.SetName(ent);
    }
    return &cmd->hdr;
}

/* (OVL2 0x5CE8E0) */
_zanim_cmd_hdr *CTTurret::Parse(_zrdr *rdr)
{
    CTTurret *cmd = 0;
    const char *name = zrdr_findstring(rdr, "name");

    if (name) {
        bool active = false;

        cmd = Create(name);
        cmd->m_flags = 0;
        cmd->m_active = 0;
        if (zrdr_findtag(rdr, "has_target"))
            cmd->m_flags |= 1;
        else if (zrdr_findtag(rdr, "is_active"))
            cmd->m_flags |= 2;
        else if (zrdr_findbool(rdr, "set_active", &active)) {
            cmd->m_flags |= 4;
            cmd->m_active = active;
        }
    }
    return &cmd->hdr;
}

/* the turret named: whether it has a target in sight, whether it is switched on, or switched on
 * or off (OVL2 0x5CE660; the SEAL's AI asked for and unused).
 * OPEN, 0x274 against 0x27C: two things. (1) The target ends a block after the anim's load and the
 * preset `r` (`lw a0; daddiu s0,zero,1; nop`), the entity's switch a block of its own, and leaves
 * the id ternary's delay slot empty (`beqz v0; nop`) -- CTRange::Tick (byte-exact through the same
 * TASK_ENT::Get) has neither; `r` before or after the AI, assigned apart, no AI local: no change.
 * (2) The turret's type test is materialised in the target (sltu; beqz; lbu; xori 3; sltiu; movz
 * v0,zero,v1 -- the `&&` as a value, then the select): an int local takes the form with an andi
 * more, a bool local, a bool inline (`== true` compares it), `(bool)e &&` and a type-test inline
 * all fold back into branches (build/scratch/kit/v_turret*.py) */
bool CTTurret::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTTurret);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;
    bool r = true;
    CEntity *e = cmd->m_ent.Get();
    CTurret *turret = e && e->m_type == CEntity::ENT_TYPE_TURRET ? (CTurret *)e : 0;

    if (turret) {
        if (cmd->m_flags & 1)
            r = func_003A7FA0(turret);
        else if (cmd->m_flags & 2)
            r = ((TASK_TURRET *)turret)->GetFlag(0x20);
        else if (cmd->m_flags & 4) {
            SetActive__7CTurretFb(turret, cmd->m_active != 0);
            r = true;
        }
    }
    *dT = 0.0f;
    return r;
}

/* a viewer and a target or a team (OVL2 0x5D8690) */
_zanim_cmd_hdr *CTInView::Parse(_zrdr *rdr)
{
    const char *viewer = zrdr_findstring(rdr, "viewer");
    const char *target = zrdr_findstring(rdr, "target");
    const char *equip = zrdr_findstring(rdr, "equip");
    unsigned int team = ParseTeamBlock(zrdr_findtag(rdr, "team"));

    if (viewer && (target || team))
        return &Create(viewer, target, equip, team)->hdr;
    return 0;
}

/* an AI event's type, place and parameters (OVL2 0x5DD0E0) */
_zanim_cmd_hdr *CTEvent::Parse(_zrdr *rdr)
{
    CTEvent *cmd = NewTask<CTEvent>();

    cmd->m_pos = CPnt3D::zero;
    cmd->m_type = CAiEvent::AIE_GENERIC;
    cmd->m_type = CAiEvent::GetEventFromString(zrdr_findstring(rdr, "type"), CAiEvent::AIE_GENERIC);
    zrdr_findPNT3D(rdr, "pos", &cmd->m_pos);
    const char *node = zrdr_findstring(rdr, "node");

    if (node) {
        cmd->m_nodes = ZAnim.m_CurAnim->LoadNodeNames(0, node, 0, 1);
        cmd->SetNode(true);
    }
    if (zrdr_findreal(rdr, "range", &cmd->m_range, 1))
        cmd->SetRange(true);
    if (zrdr_findreal(rdr, "duration", &cmd->m_duration, 1))
        cmd->SetDuration(true);
    if (zrdr_findtag(rdr, "vis"))
        cmd->SetVis(true);
    return &cmd->hdr;
}

/* the first item a text, on or off; an integer gives none (OVL2 0x5DD7C0) */
_zanim_cmd_hdr *CTDebug::Parse(_zrdr *rdr)
{
    CTDebug *cmd = 0;
    _zrdr *item = rdr->Item(0);

    if (item) {
        const char *text = 0;
        int mode = -1;

        switch (item->type) {
        case 3:
            text = item->String();
            if (!strcasecmp(text, "on"))
                mode = 1;
            else if (!strcasecmp(text, "off"))
                mode = 0;
            break;
        case 1:
            return 0;
        }
        if (mode != -1)
            text = mode ? "DEBUG on" : "DEBUG off";
        cmd = Create(text);
        if (cmd)
            cmd->m_mode = mode;
    }
    return &cmd->hdr;
}

/* the AI's debug switched by the mode (OVL2 0x5DD620: SOCOM 1's called SetDebug(true) for 1 and
 * SetDebug(false) for 0, then the AI's Debug(text) -- SOCOM II stores 0 in the bit both ways and
 * prints nothing, the debug build's code gone) */
bool CTDebug::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTDebug);
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (!seal || !ctrl) {
        *dT = 0.0f;
        return true;
    }
    if (ai) {
        if (cmd->m_mode == 1)
            ai->m_debug = false;
        else if (cmd->m_mode == 0)
            ai->m_debug = false;
    }
    return true;
}

/* (OVL2 0x5DD610) */
_zanim_cmd_hdr *CTDrain::Parse(_zrdr *rdr)
{
    return 0;
}

/* (OVL2 0x5DD0D0) */
void CTEvent::Begin(_zanim_cmd_hdr *hdr)
{
}

/* done at once, the time kept unless there is no AI (OVL2 0x5DC8A0) */
bool CTFireMode::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTFireMode);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (!ai) {
        *dT = 0.0f;
        return true;
    }
    return true;
}

/* (OVL2 0x5DC800) */
_zanim_cmd_hdr *CTFireWeapon::Parse(_zrdr *rdr)
{
    CTFireWeapon *cmd = NewTask<CTFireWeapon>();

    cmd->m_item = CZWeapon::GetWeaponId(zrdr_findstring(rdr, "weapon"));
    cmd->SetFiring(false);
    return &cmd->hdr;
}

/* firing marked when the kit holds the weapon (OVL2 0x5DC6B0) */
void CTFireWeapon::Begin(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTFireWeapon);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (ai && seal->m_kit.HasItem((EQUIP_ITEM)cmd->m_item))
        cmd->SetFiring(true);
}

/* (OVL2 0x5DB730: the find written out, TASK_SEAL; the AI's ternary through SealAi keeps the call's
 * result in its own arm) */
void CTGoto::Begin(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTGoto);
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (!seal || !ctrl || !ai)
        return;
    cmd->SetBit1(false);
    if (func_005DB8F0(ai, cmd, 0)) {
        CAiState *s = ai->GetAiState();
        CAiSScript *script = s && s->m_state == AI_STATE_SCRIPT ? (CAiSScript *)s : 0;

        if (script)
            func_005F3FC0(script, ai, true);
    }
}

/* running until the AI's route is done, the AI in mode 3: the move set up again for a script state
 * in mode 2 (OVL2 0x5DB510) */
bool CTGoto::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTGoto);
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (!seal || !ctrl || !ai) {
        *dT = 0.0f;
        return true;
    }
    bool done = cmd->m_done;

    if (!done) {
        ai->SetAiMode((AI_MODE)3);
        CAiState *s = ai->GetAiState();
        CAiSScript *script = s && s->m_state == AI_STATE_SCRIPT ? (CAiSScript *)s : 0;

        if (script && func_00555890(ai) == 2) {
            func_005DB8F0(ai, cmd, 1);
            func_005F3FC0(script, ai, true);
        }
        bool r = func_005629B0(ai);

        done = cmd->m_bit0 ? r : true;
    }
    *dT = 0.0f;
    return done;
}

/* (OVL2 0x5DB350: as Begin) */
void CTGoto::End(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTGoto);
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (!seal || !ctrl || !ai)
        return;
    cmd->SetBit1(true);
    if (ai && cmd->m_bit0) {
        func_005628F0(ai);
        CAiState *s = ai->GetAiState();
        CAiSScript *script = s && s->m_state == AI_STATE_SCRIPT ? (CAiSScript *)s : 0;

        if (script)
            func_005F3FC0(script, ai, false);
    }
}

/* running until the group is gone (OVL2 0x5D9710) */
bool CTGroup::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTGroup);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;
    bool done = false;

    if (!CAiGroup::Find(cmd->m_14))
        done = true;
    *dT = 0.0f;
    return done;
}

/* (OVL2 0x5D9430) */
void CTGroup::End(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTGroup);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    cmd->m_14 = 0;
}

/* the hold over: the AI's mode put back, unless it never began (OVL2 0x5D8E20) */
void CTHold::End(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTHold);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (ai && cmd->m_timer != -1.0f)
        RestoreAiMode__11CSealCtrlAiFv(ai);
}

/* the wait run down (OVL2 0x5D8F60) */
bool CTHold::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTHold);

    if (!(seal ? SealAi(seal) : 0)) {
        *dT = 0.0f;
        return true;
    }
    bool done = cmd->m_timer <= 0.0f;

    if (!done)
        cmd->m_timer -= *dT;
    *dT = 0.0f;
    return done;
}

/* a state by name, its own or one of the task scripts' aliases, 0 for none (SOCOM 1's
 * get_state_fixup, the next function after CTInState::Parse there too: SOCOM II added
 * "restrained"; "path reco" gives the reload, as SOCOM 1's did) (OVL2 0x5D8CC0) */
AI_STATE get_state_fixup(const char *name)
{
    AI_STATE state = sealai::get_state(name);

    if (name && state == AI_STATE_UNKNOWN) {
        if (!strcasecmp(name, "following"))
            state = AI_STATE_FOLLOW;
        else if (!strcasecmp(name, "avoiding"))
            state = AI_STATE_AVOID;
        else if (!strcasecmp(name, "breaching"))
            state = AI_STATE_BREACH;
        else if (!strcasecmp(name, "deploying"))
            state = AI_STATE_DEPLOY;
        else if (!strcasecmp(name, "holding"))
            state = AI_STATE_HOLD;
        else if (!strcasecmp(name, "path reco"))
            state = AI_STATE_RELOAD;
        else if (!strcasecmp(name, "reloading"))
            state = AI_STATE_RELOAD;
        else if (!strcasecmp(name, "rushing"))
            state = AI_STATE_RUSH;
        else if (!strcasecmp(name, "restrained"))
            state = AI_STATE_HOSTAGE;
    }
    return state;
}

/* the entity, the state by name and whether the whole stack counts (OVL2 0x5D8B80) */
_zanim_cmd_hdr *CTInState::Parse(_zrdr *rdr)
{
    char *name = zrdr_findstring(rdr, "entity");
    char *statename = zrdr_findstring(rdr, "state");
    /* declared ahead of the flag (its saved register), compared with 0 (the andi before the
     * test) */
    AI_STATE state;
    bool stack = zrdr_findtag(rdr, "stack") != 0;

    state = get_state_fixup(statename);
    if (state != 0)
        return &Create(state, stack, name)->hdr;
    return 0;
}

/* the wait as a tagged range, the reader's own range, or its bool (true holds for good: -1)
 * (OVL2 0x5D9270) */
_zanim_cmd_hdr *CTHold::Parse(_zrdr *rdr)
{
    Rfloat wait(0.0f);
    CTHold *cmd = 0;
    bool forever = false;

    if (zrdr_findRfloat(rdr, "wait", &wait)) {
        cmd = Create();
        cmd->m_wait = wait;
    } else if (zrdr_toRfloat(rdr, &wait)) {
        cmd = Create();
        cmd->m_wait = wait;
    } else if (zrdr_tobool(rdr, &forever)) {
        cmd = Create();
        cmd->m_wait = forever ? -1.0f : 1.0f;
    }
    return &cmd->hdr;
}

/* the wait drawn, the AI held in mode 2 for it (OVL2 0x5D90E0) */
void CTHold::Begin(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTHold);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (ai) {
        cmd->m_timer = cmd->m_wait;
        if (cmd->m_timer != 0.0f)
            ai->SetAiMode((AI_MODE)2);
    }
}

/* a block made for an entity's name or none (OVL2 0x5D7E70: out of line, CTLookAt::Parse's first
 * call; SOCOM 1's Parse built it inline and copied the name to the block's end -- SOCOM II still
 * sizes the block for it but adds the name to the running anim's names; the name ours) */
CTLookAt *CTLookAt::Create(const char *name)
{
    unsigned int size = TaskAlign(TaskStrLen(name)) + 48;
    CTLookAt *cmd = (CTLookAt *)_calloc(size, 1, 0, 0);

    cmd->hdr.data_size = size;
    cmd->hdr.data_type = ai_cmdid[0];
    cmd->m_kind = name ? 2 : 0;
    cmd->m_flags = 1;
    cmd->m_rate = 0.85f;
    cmd->m_timer = CAiTimer::infinite;
    cmd->m_wait = CAiTimer::infinite;
    cmd->SetLooking(0);
    cmd->m_ent.m_name = 0;
    cmd->m_ent.m_id = CEntity::invalidId;
    cmd->m_ent.m_type = 0;
    cmd->m_ent.SetName(name);
    return cmd;
}

/* the look drawn (OVL2 0x5D7850: the bit cleared in place gives the byte mask `andi 0xfb`; through
 * the int setter it is `li -5; and`) */
void CTLookAt::Begin(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTLookAt);
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? ((CSealCtrl *)seal->m_control)->GetSealCtrlAi() : 0;

    if (!seal || !ctrl)
        return;
    cmd->m_timer = cmd->m_wait;
    cmd->m_looking &= ~4;
}

/* the look cleared (OVL2 0x5D7290) */
void CTLookAt::End(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTLookAt);
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? ((CSealCtrl *)seal->m_control)->GetSealCtrlAi() : 0;

    if (!seal || !ctrl)
        return;
    if (cmd->m_looking & 1)
        ClearLook__9CSealCtrlF7AI_LOOK((CSealCtrl *)ctrl, 3);
}

/* the AI's bit 6 (an inline bool: its read is not merged with the set's load of the same byte) */
inline bool TaskAiBit6(const CSealCtrlAi *ai)
{
    return ai->m_aibit6;
}

/* a script state's hold, false for none (inline: its value masked at the assignment) */
inline bool TaskScriptHeld(const CAiSScript *script)
{
    return script ? script->m_hold : false;
}

/* the anim run for the SEAL, its script state pushed with it (the AI's bit 6 held set meanwhile, the
 * old script's hold carried over) -- done at once (OVL2 0x5D6DC0; the select read into a local ahead
 * of GetNode, the bit read through an inline bool: both as the target)
 * OPEN, 8 bytes short: the old hold's bool is masked into its saved register at the join in the target
 * (`sltu v0` / `move v0,zero`, then `andi s4,v0,0xff`), straight into it here (`sltu s4`) -- through an
 * inline, an if-assign, an unsigned char, the anim's and run's order, the select declared at the top:
 * none moves it; and the SEAL takes s1 where the target colours it after the select and the AI (s4) */
bool CTMacro::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTMacro);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (!ai) {
        *dT = 0.0f;
        return true;
    }
    CZAnim *anim = cmd->m_anim;
    CZAnim *run = 0;

    if (anim) {
        void *select = seal->m_select;

        run = anim->ActivateArg(1, 1, select, 3, seal->GetNode(), 0);
    }
    if (run) {
        bool bit6 = TaskAiBit6(ai);

        ai->m_aibit6 = true;
        CAiState *s = ai->GetAiState();
        CAiSScript *script = s && s->m_state == AI_STATE_SCRIPT ? (CAiSScript *)s : 0;
        bool hold = TaskScriptHeld(script);

        if (ai->PushAiState(AI_STATE_SCRIPT, 5)) {
            s = ai->GetAiState();
            script = s && s->m_state == AI_STATE_SCRIPT ? (CAiSScript *)s : 0;
            script->m_58 = (unsigned int)run;
            script->SetHold(hold);
        }
        ai->m_aibit6 = bit6;
    }
    *dT = 0.0f;
    return true;
}

/* the anim looked up by name (OVL2 0x5D7040) */
void CTMacro::Begin(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTMacro);

    if (seal ? SealAi(seal) : 0)
        cmd->m_anim = ZAnim.GetAnim(cmd->m_name.m_str, 0);
}

/* running, when blocking, while the AI moves to its goal (OVL2 0x5D5B40) */
bool CTMove::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTMove);
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (!seal || !ctrl) {
        *dT = 0.0f;
        return true;
    }
    bool done = true;

    if (cmd->m_blocking && ai) {
        CSealCtrl *sc = (CSealCtrl *)ctrl;

        done = !(sc->m_goal_type == GOAL_MOVE && (sc->m_goal_dir_valid || sc->m_goal_pos_valid) &&
                 ai->m_3f4 == 5);
    }
    *dT = 0.0f;
    return done;
}

/* (OVL2 0x5D5A20) */
void CTMove::End(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTMove);
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? ((CSealCtrl *)seal->m_control)->GetSealCtrlAi() : 0;

    if (!seal || !ctrl)
        return;
}

/* the region painted: a soil or a tile through Tile (modes 1 and 2, when the cell has a region), a
 * type through SetRegionType (mode 3) -- done at once (OVL2 0x5D5700; SOCOM 1's, the same code) */
bool CTPaint::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    CTPaint *cmd = (CTPaint *)hdr;
    const CAiMapRegion *r = GetRegion__7CAiMapsFRC9CAiMapLoc(&AiMaps, cmd->m_loc);

    switch (cmd->m_mode) {
    case 1:
    case 2:
        if (r)
            Tile__7CAiMapsFPCQ26CAiMap6regionUiUi(&AiMaps, r, cmd->m_value, cmd->m_flags);
        break;
    case 3:
        SetRegionType__7CAiMapsFPCQ26CAiMap6regionQ36CAiMap6region4TYPE(&AiMaps, r, (unsigned char)cmd->m_value);
        break;
    }
    *dT = 0.0f;
    return true;
}

/* a block made for a count of anims, copied when given (OVL2 0x5D3F00: SOCOM 1's CTPlay::Create, out
 * of line in both, called from the same place in CTPlay::Parse -- SOCOM II's anims at 0x30, the
 * count a byte at 0x20, 11 at 0x22) */
CTPlay *CTPlay::Create(int count, unsigned int *anims, float a, float b)
{
    unsigned int size = (count - 1) * 4 + sizeof(CTPlay);
    CTPlay *cmd = (CTPlay *)_calloc(size, 1, 0, 0);

    cmd->hdr.data_size = size;
    cmd->hdr.data_type = ai_cmdid[0];
    cmd->hdr.quad_align = true;
    cmd->m_count = count;
    if (anims) {
        for (int i = 0; i < count; i++)
            cmd->m_anims[i] = anims[i];
    }
    cmd->m_22 = 11;
    cmd->m_8 = a;
    cmd->m_10 = b;
    return cmd;
}

/* (OVL2 0x5D4080: the AI tested and nothing done -- the test keeps the ternary's moves into v1 that
 * an unused AI loses, its branch to the epilogue removed after allocation) */
void CTPursue::End(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTPursue);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (!ai)
        return;
}

/* running while the AI pursues (OVL2 0x5D4190): the script-state test is the controller's AiStateIs
 * (zseal.h: the stack's const GetState called -- the kit lane, round 21) */
bool CTPursue::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTPursue);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (!ai) {
        *dT = 0.0f;
        return true;
    }
    bool r = ai->AiStateIs(AI_STATE_SCRIPT);

    *dT = 0.0f;
    return r;
}

/* the pursuit pushed, its duration the command's (OVL2 0x5D4300) */
void CTPursue::Begin(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTPursue);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (ai && ai->PushAiState(AI_STATE_PURSUE, 0)) {
        CAiState *s = ai->GetAiState();
        CAiSPursueTask *p = s && s->m_state == AI_STATE_PURSUE ? (CAiSPursueTask *)s : 0;

        p->m_duration = cmd->m_duration;
        p->SetActive(true);
    }
}

/* (OVL2 0x5D4480) */
_zanim_cmd_hdr *CTPursue::Parse(_zrdr *rdr)
{
    CTPursue *cmd = NewTask<CTPursue>();

    cmd->m_duration = -1.0f;
    zrdr_findreal(rdr, "duration", &cmd->m_duration, 1);
    return &cmd->hdr;
}

/* the stance taken, its timer started (OVL2 0x5D09F0) */
void CTStance::Begin(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTStance);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (ai) {
        SetStance__11CSealCtrlAiF11SEAL_STANCE(ai, (SEAL_STANCE)cmd->m_stance);
        SetDesiredStance__11CSealCtrlAiF11SEAL_STANCEfb(ai, (SEAL_STANCE)cmd->m_peek, true, CAiTimer::infinite);
        cmd->m_timer = cmd->m_time;
    }
}

/* back to standing, unless the stance was for no time or was prone (OVL2 0x5D08A0) */
void CTStance::End(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTStance);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (ai && cmd->m_time != 0.0f && cmd->m_peek != STANCE_PRONE)
        SetDesiredStance__11CSealCtrlAiF11SEAL_STANCEfb(ai, STANCE_PRONE, true, 0.0f);
}

/* the stance's time run down (OVL2 0x5D0730) */
bool CTStance::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTStance);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (!ai) {
        *dT = 0.0f;
        return true;
    }
    cmd->m_timer -= *dT;
    *dT = 0.0f;
    return cmd->m_timer <= 0.0f;
}

/* the entity named and the range, 1000 unless given (OVL2 0x5CF6F0) */
_zanim_cmd_hdr *CTRange::Parse(_zrdr *rdr)
{
    CTRange *cmd = 0;
    char *name = zrdr_findstring(rdr, "entity");
    float range = 1000.0f;

    zrdr_findreal(rdr, "range", &range, 1);
    if (name)
        cmd = Create(range, name);
    return &cmd->hdr;
}

/* the entity within the range of the SEAL (OVL2 0x5CF480; SOCOM 1's) */
bool CTRange::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTRange);
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (!seal || !ctrl) {
        *dT = 0.0f;
        return true;
    }
    CEntity *e = cmd->m_ent.Get();

    if (!seal || !e)
        return false;
    CPnt3D d = seal->m_posW - e->m_posW;

    return d.x * d.x + d.y * d.y + d.z * d.z <= cmd->m_range2;
}

/* whether the region holds what the task names: a pickup of the item, the entity, one of the teams
 * (0x56D530, seal_ai.cpp's) or the node (OVL2 0x5CED60; SOCOM 1's tested an entity, an item in
 * hand or a pickup's).
 * OPEN, 0x514 against 0x524: the shape is the target's (the region cached by the maps' cookie, its
 * fetch the fall-through), but (1) the saved registers -- the target's block s0, list s1, counter
 * s2, index s3, type s4, `r` s4 and the region s5 after the find; here the block s4, the find's
 * four a place lower, `r` s3 -- and (2) the kind's dispatch: the target loads each case constant
 * into v0 in its own compare (two `nop` delay slots), here 2 and 1 sit in a2/a1 and are reused by
 * the entity's switch in case 1 (enum case labels there: no change) -- CTTurret::Tick's open is the
 * same constant sharing */
bool CTRegion::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTRegion);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;
    bool r = false;
    const CAiMap::region *region = cmd->GetRegion();

    if (!region)
        return false;
    switch (cmd->m_kind) {
    case 3: {
        CPickup::iterator it = CPickup::GetFirst();
        CPickup *p;

        while (!r && (p = CPickup::GetNext(it, CPickup::PU_NONE)) != 0) {
            void *a = p->GetData4567();
            void *b = p->GetData123();
            CZWeapon *w = a ? ((TASK_PICKUPDATA *)a)->m_weapon : b ? ((TASK_PICKUPDATA *)b)->m_weapon : 0;

            if (w && w->m_ID == cmd->m_weapon) {
                CAiMapLoc loc;

                if (AiMaps.GetLocation(p->GetPosW(), loc))
                    r = region->Contains(loc);
            }
        }
        break;
    }
    case 1: {
        CEntity *e = cmd->m_ent.Get();

        if (e)
            r = region->Contains(e->m_mapLoc);
        break;
    }
    case 2:
        r = func_0056D530(region, cmd->m_team);
        break;
    case 4: {
        zdb::CNode *node = ZAnim.m_CurAnim->GetNode(cmd->m_node, false);
        CAiMapLoc loc;

        if (node && AiMaps.GetLocation((const CPnt3D *)node->m_matrix.m_matrix[3], loc))
            r = region->Contains(loc);
        break;
    }
    }
    return r;
}

/* the region by name, then what it is to hold -- a block for none freed (OVL2 0x5CF290; SOCOM 1's
 * tested an entity's name and an item) */
_zanim_cmd_hdr *CTRegion::Parse(_zrdr *rdr)
{
    CTRegion *cmd = 0;
    char *entity = zrdr_findstring(rdr, "entity");
    char *region = zrdr_findstring(rdr, "region");
    char *weapon = zrdr_findstring(rdr, "weapon");
    _zrdr *dbnode = zrdr_findtag(rdr, "dbnode");
    unsigned int team = ParseTeams(zrdr_findtag(rdr, "team"));
    CAiMapLoc loc;

    if (AiMaps.GetRegion(region, loc)) {
        cmd = NewTask<CTRegion>();
        cmd->m_loc = loc;
        cmd->m_region = 0;
        cmd->m_cookie = 0;
        if (entity) {
            cmd->m_ent.m_name = 0;
            cmd->m_ent.m_id = CEntity::invalidId;
            cmd->m_ent.m_type = 0;
            cmd->m_ent.SetName(entity);
            cmd->m_kind = 1;
        } else if (weapon) {
            cmd->m_weapon = CZWeapon::GetWeaponId(weapon);
            cmd->m_kind = 3;
        } else if (dbnode) {
            cmd->m_node = func_0026E2D0(ZAnim.m_CurAnim, dbnode);
            cmd->m_kind = 4;
        } else if (team) {
            cmd->m_team = team;
            cmd->m_kind = 2;
        } else {
            _free(cmd, 0, 0);
            cmd = 0;
        }
    }
    return &cmd->hdr;
}

/* the mission (fts_state.h's CMission: only the team valves' refresh, OVL1 0x2B23A0) and theMission
 * (OVL1 bss 0x4364E0) */
class CMission {
public:
    void RefreshTeamValves();
};
extern CMission theMission;

/* the SEAL joins or leaves the teams, its AI's events, its squad and the team valves brought up to
 * it; ismember does nothing here (OVL2 0x5CEA40; SOCOM 1's, which updated the AI's events only --
 * its team read once ahead of the switch, as SOCOM 1's too: the load in the first compare's delay slot)
 * OPEN, 20 instructions: the block and the find's counter swap saved registers (the target's block
 * s2, counter s3, the SEAL sharing the counter's; here the block s3, the counter and the SEAL s2) --
 * the target colours the block before the SEAL where every other handler's allocation is numbering
 * order; the find's five declarations in all 60 orders (build/scratch/kit/v_team2.py, best 17), the
 * controller/AI order, the team local first, a second block pointer after the AI: none moves it */
bool CTTeam::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTTeam);
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (!seal || !ctrl) {
        *dT = 0.0f;
        return true;
    }
    unsigned int team = seal->m_team;

    switch (cmd->m_op) {
    case 0:
        seal->SetTeamMask(team | cmd->m_mask);
        if (ai)
            ai->UpdateEventMask();
        InitalizeSealUnit__9CSealUnitFP10CZSealBodyP9CSealUnit(seal, 0);
        theMission.RefreshTeamValves();
        break;
    case 1:
        seal->SetTeamMask(team & ~cmd->m_mask);
        if (ai)
            ai->UpdateEventMask();
        InitalizeSealUnit__9CSealUnitFP10CZSealBodyP9CSealUnit(seal, 0);
        theMission.RefreshTeamValves();
        break;
    case 2:
        break;
    }
    *dT = 0.0f;
    return true;
}

/* join, leave or ismember, the tag's values the mask (OVL2 0x5CEC40) */
_zanim_cmd_hdr *CTTeam::Parse(_zrdr *rdr)
{
    _zrdr *tag;
    signed char op = -1;

    if ((tag = zrdr_findtag(rdr, "join")) != 0)
        op = 0;
    else if ((tag = zrdr_findtag(rdr, "leave")) != 0)
        op = 1;
    else if ((tag = zrdr_findtag(rdr, "ismember")) != 0)
        op = 2;
    if (op != -1)
        return &Create(op, tag)->hdr;
    return 0;
}

/* the AI switched on or off (OVL2 0x5CF800) */
bool CTStopAll::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    CTStopAll *cmd = (CTStopAll *)hdr;
    bool enabled = !cmd->m_enable;

    SetAiEnabled__11CSealCtrlAiFbb(enabled, !cmd->m_inactive);
    func_00598840(enabled);
    return true;
}

/* (OVL2 0x5CF860) */
_zanim_cmd_hdr *CTStopAll::Parse(_zrdr *rdr)
{
    bool inactive = false;
    bool enable = true;

    zrdr_tobool(rdr, &enable);
    if (zrdr_findtag(rdr, "inactive"))
        inactive = true;
    return &Create(enable, inactive)->hdr;
}

/* the SEAL of a name, by the entity list: the first item a string (OVL2 0x5D28A0; Create
 * inlined) */
_zanim_cmd_hdr *CTSelect::Parse(_zrdr *rdr)
{
    TASK_PARAM *p = 0;

    if (zrdr_isitemtype(rdr, 0, 3)) {
        CEntity *e = CEntity::GetEntityList().GetEntity(rdr->Item(0)->String());
        CZSealBody *seal = e && e->m_type == CEntity::ENT_TYPE_SEAL ? (CZSealBody *)e : 0;

        if (seal)
            p = Create(seal);
    }
    return &p->hdr;
}

/* (OVL2 0x5D2890) */
bool CTSelect::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    return true;
}

/* a SEAL's task argument made: the block typed as the SEAL arguments are (ai_cmdid[2]), the SEAL
 * and its AI (SOCOM 1's CTSelect::Create, which PostCreateSeal calls as SOCOM II's does; OVL2
 * 0x5D29F0, after CTSelect's Parse as there) */
TASK_PARAM *CTSelect::Create(CZSealBody *seal)
{
    TASK_PARAM *p = (TASK_PARAM *)_calloc(sizeof(TASK_PARAM), 1, 0, 0);

    p->hdr.data_size = sizeof(TASK_PARAM);
    p->hdr.data_type = ai_cmdid[2];
    p->m_seal = seal;
    p->m_ai = seal ? SealAi(seal) : 0;
    if (seal)
        p->hdr.data_type = ai_cmdid[2];
    return p;
}

/* a reader's item a string, or 0 (inline; the name ours) */
inline char *TaskItemString(_zrdr *rdr, int i)
{
    if (rdr && rdr->Count() > i) {
        _zrdr *item = rdr->Item(i);

        if (item && item->type == 3)
            return item->String();
    }
    return 0;
}

/* the mode by the first item's name, then the items after it as the tags (OVL2 0x5D2630; SOCOM 1's
 * read only the mode) */
_zanim_cmd_hdr *CTSetMode::Parse(_zrdr *rdr)
{
    CTSetMode *cmd = 0;
    char *name = TaskItemString(rdr, 0);

    if (name) {
        cmd = NewTask<CTSetMode>();
        cmd->SetMode((AI_MODE)1);
        cmd->SetMode(sealai::get_mode(name));

        int i = 1;
        char *s;

        while ((s = TaskItemString(rdr, i++)) != 0) {
            if (!strcasecmp(s, "always"))
                cmd->m_flags |= CTSetMode::ALWAYS;
            else if (!strcasecmp(s, "respawn_only"))
                cmd->m_flags |= CTSetMode::RESPAWN_ONLY;
        }
        cmd->m_param = 0;
        zrdr_finduint(rdr, "ai", (unsigned int *)&cmd->m_param, 1);
    }
    return &cmd->hdr;
}

/* the AI's mode set, or for none the AI put to sleep (OVL2 0x5D2480; SOCOM 1's slept for mode 0,
 * and without an AI sent the SEAL to a goal for mode 2 -- SOCOM II dropped that) */
bool CTSetMode::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTSetMode);
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (!seal || !ctrl) {
        *dT = 0.0f;
        return true;
    }
    AI_MODE mode = (AI_MODE)(cmd->m_flags & CTSetMode::MODE);

    if (ai) {
        bool respawned = ai->m_has_respawned ? true : false;

        if (mode == 0) {
            if ((!respawned && !(cmd->m_flags & CTSetMode::RESPAWN_ONLY)) || (cmd->m_flags & CTSetMode::ALWAYS))
                ai->SetSleeping(true);
        } else
            ai->SetAiMode(mode);
    }
    return true;
}

/* the sound let finish (OVL2 0x5D1210) */
void CTSound::End(_zanim_cmd_hdr *hdr)
{
    CTSound *cmd = (CTSound *)hdr;

    if (cmd->m_inst)
        cmd->m_inst->TerminateOnFinish();
    cmd->m_inst = 0;
}

/* (OVL2 0x5D1250) */
void CTSound::Begin(_zanim_cmd_hdr *hdr)
{
    CTSound *cmd = (CTSound *)hdr;

    cmd->SetStarted(true);
    cmd->SetBit7(false);
    cmd->m_inst = 0;
}

/* the valve the AI holds compared, done at once (OVL2 0x5CE040; SOCOM 1's the same) */
bool CTValve::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTValve);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (!ai) {
        *dT = 0.0f;
        return true;
    }
    cmd->m_data.m_valve = ai->GetValve((SEAL_VALVE)cmd->m_valve);
    bool r = cmd->m_data.Compare();

    cmd->m_data.m_valve = 0;
    return r;
}

/* the wait read, and drawn twice (OVL2 0x5CDF00; SOCOM 1's the same) */
_zanim_cmd_hdr *CTWait::Parse(_zrdr *rdr)
{
    CTWait *cmd = 0;
    Rfloat wait(0.0f);

    if (zrdr_findRfloat(rdr, "wait", &wait))
        cmd = Create(wait);
    return &cmd->hdr;
}

/* the wait run down, drawn again when it ends (OVL2 0x5CDBD0; SOCOM 1's the same but for the timer's
 * compares, two materialised bools there) */
bool CTWait::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTWait);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;
    bool done = cmd->m_wait.Update(*dT);

    *dT = 0.0f;
    return done;
}

/* the wait drawn (OVL2 0x5CDDA0) */
void CTWait::Begin(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTWait);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    cmd->m_wait.Reset();
}
