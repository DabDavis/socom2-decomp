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
bool zrdr_findint(const _zrdr *rdr, const char *tag, int *out, int n);
bool zrdr_findPNT2D(const _zrdr *rdr, const char *tag, PNT2D *out);
bool zrdr_isitemtype(const _zrdr *rdr, int index, int type);
bool zrdr_findbool(const _zrdr *rdr, const char *tag, bool *out);
bool zrdr_findPNT3D(const _zrdr *rdr, const char *tag, PNT3D *out);
bool zrdr_toINT(const _zrdr *rdr, int *out, int n);
/* an anim type by name, -1 for none (anim.cpp's, OVL2 0x5E5000) */
AnimType FindAnimType(const char *name);
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
/* and CAiMaps::GetRegion(const char *, CAiMapLoc &), a region by name with its cell (OVL2 0x52D9F0) */
const CAiMapRegion *GetRegion__7CAiMapsFPCcR9CAiMapLoc(CAiMaps *maps, const char *name, CAiMapLoc &loc);
/* and GetLocation(const char *, CAiMapLoc &), a line's cell by name (OVL2 0x52DB70) */
bool GetLocation__7CAiMapsFPCcR9CAiMapLoc(CAiMaps *maps, const char *name, CAiMapLoc &loc);
/* the anim's root node, not made when missing (OVL1 0x2703D0, the engine core's row: SOCOM 1's
 * GetRootNode with a flag), and a node's extension and its parent (zdb::CNode's inline accessors'
 * out-of-line copies, OVL2 0x5AA610 and 0x5AA600 -- seal_env.cpp's spelling) */
zdb::CNode *func_002703D0(CZAnim *anim, bool create);
zdb::CNodeEx *func_005AA610(zdb::CNode *node);
zdb::CNode *func_005AA600(zdb::CNode *node);
}

/* a region kind's name (SOCOM 1's CAiMap::region::GetTypeName, out of line there -- 0x2B9AA0, its
 * unit's tail -- and inlined here; ai.h's region::TYPE names only ANY, so the kinds are numbers:
 * the target's case blocks run 0, 1, 6, 2-5, default) */
inline const char *RegionTypeName(CAiMap::region::TYPE type)
{
    switch (type) {
    case 0:
        return "DEFAULT";
    case 1:
        return "EXTRACT_ZONE";
    case 6:
        return "GUARD";
    case 2:
        return "SAFE_HOUSE";
    case 3:
        return "WORM_HOLE";
    case 4:
        return "ROUTER";
    case 5:
        return "HUD_VISIBLE";
    }
    return "UNKNOWN";
}

/* a region kind by its name, 0 for none (inlined in both builds; the name ours) */
inline CAiMap::region::TYPE RegionTypeByName(const char *name)
{
    for (int i = 1; i < 7; i++)
        if (!strcasecmp(name, RegionTypeName((CAiMap::region::TYPE)i)))
            return (CAiMap::region::TYPE)i;
    return (CAiMap::region::TYPE)0;
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
    static void End(_zanim_cmd_hdr *hdr);    /* (RegisterCommands' handlers, unwritten) */
    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
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

    /* the entity if it is of a type (inline; the name ours): the `&&` and the select stay a value --
     * sltu, beqz, xori/sltiu, movz -- only inside an inline that holds Get's switch; written in the
     * caller after Get (a local, a bool, an int, a type-test inline) MW folds them back into branches
     * (CTTurret::Tick's and CTSignal::Tick's three) */
    CEntity *GetTyped(int t)
    {
        CEntity *e = Get();

        return e && e->m_type == t ? e : 0;
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

/* an anim name a task holds: the unit's none, then the running anim's name for it (inline: its
 * `this` kept in s0 across AddName; the names ours) */
struct TASK_ANIMNAME {
    void Set(const char *name)
    {
        m_name = data_00650AA8;
        if (name && ZAnim.m_CurAnim)
            m_name = ZAnim.m_CurAnim->AddName(name);
    }
    /* the name back from the running anim, none when unset (inline; the name ours: the anim read
     * here, so the first use shares the caller's earlier load and a later one reloads it) */
    const char *Get() const
    {
        const char *s = 0;

        if (m_name != data_00650AA8)
            s = AnimName(ZAnim.m_CurAnim, m_name);
        return s;
    }
    /* an anim's name of an index, none without the anim (inline: its parameter a variable of its
     * own, copied into a0 before the test) */
    static const char *AnimName(CZAnim *anim, int i)
    {
        return anim ? anim->GetName(i) : 0;
    }

    /* (unsigned: the tick's compares read it with lhu) */
    unsigned short m_name;
};

/* ai::CREATE (SOCOM II's): an entity cloned from another under a new name (mode 2: the tag "clone"
 * and its two names -- the new name first, then the entity copied) or destroyed (4: "destroy" and its
 * name); mode 1 makes the vehicle entry the block carries at 0x10 -- the names ours but the tags' */
struct CTCreate : TASK_HDR {
    /* a block made (inline: its arguments read before the allocation, right to left) */
    static CTCreate *Create(const char *src, const char *dst, short mode)
    {
        CTCreate *c = NewTask<CTCreate>();

        c->m_src.Set(src);
        c->m_dst.Set(dst);
        c->m_mode = mode;
        return c;
    }

    unsigned short m_mode;   // 0x8: flags 1, 2, 4 (the tick tests each with lhu)
    TASK_ANIMNAME m_src;   // 0xa
    TASK_ANIMNAME m_dst;   // 0xc
    unsigned char _e[0x10 - 0xe];
    unsigned char m_entry[0x160];   // 0x10: a CVehicleRdrEntry (0x160 bytes); 0x170 bytes in all

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);    /* (RegisterCommands' handlers, unwritten) */
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
};

/* ai::SIGNAL (SOCOM 1's CTSignal): an AI signal sent, to an entity or the SEAL */
struct CTSignal : TASK_HDR {
    /* a signal's name, AISIG_UNKNOWN past the seven (inline: SOCOM 1's had six -- its
     * GetSignalByName's switch) and a name's signal, 0 for none or no match (SOCOM 1's
     * GetSignalByName, out of line there, inlined in Parse here; the first ours) */
    static const char *GetSignalName(AI_SIGNAL s)
    {
        switch (s) {
        case 0:
            return "AISIG_UNKNOWN";
        case 1:
            return "AISIG_ANIMCOMPLETE";
        case 2:
            return "AISIG_BEGINCRITICAL";
        case 3:
            return "AISIG_ENDCRITICAL";
        case 4:
            return "AISIG_SUSPEND";
        case 5:
            return "AISIG_RELEASE";
        case 6:
            return "AISIG_AUTOSTOP";
        }
        return "AISIG_UNKNOWN";
    }
    static AI_SIGNAL GetSignalByName(const char *name)
    {
        if (name) {
            for (unsigned int i = 0; i < 7; i++)
                if (!strcasecmp(name, GetSignalName((AI_SIGNAL)i)))
                    return (AI_SIGNAL)i;
        }
        return (AI_SIGNAL)0;
    }

    int m_signal;   // 0x8: an AI_SIGNAL (a word)
    TASK_ENT m_ent;   // 0xc: the tag "entity"
    /* (0x14: the entity name's room, rounded to words -- allocated, never written since the name
     * became an anim name: SOCOM 1 copied the name here) */

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
};

/* ai::SETANIMSET (SOCOM 1's CTSetAnimSet, the name now a name of the running anim's): the SEAL's
 * anim set changed -- to the bored one (flag 2), or a named one; flag 4 the default's, and an
 * entity it applies to (the names ours but the tags') */
struct CTSetAnimSet : TASK_HDR {
    unsigned int m_flags;   // 0x8
    short m_set;   // 0xc: the tag "name", as a name of the running anim's
    unsigned char _e[0x10 - 0xe];
    TASK_ENT m_ent;   // 0x10: the tag "entity"

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);    /* (RegisterCommands' handlers, unwritten) */
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
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
    }    /* (RegisterCommands' handlers, unwritten) */
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
};

/* ai::INSTATE (SOCOM 1's CTInState): an entity's AI in a state, or with it on its stack */
struct CTInState : TASK_HDR {
    unsigned int m_state : 31;   // 0x8: an AI_STATE
    unsigned char m_stack : 1;   // 0xb bit 7: anywhere on the state stack (the tag "stack")
    TASK_ENT m_ent;   // 0xc

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
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
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
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

    /* a block for a region's cell (inline; the name ours: its arguments are bound ahead of the
     * allocation -- the flags' 1 is live across the header's mask, which takes a0 in the target --
     * where a block filled after NewTask colours them the other way round) */
    static CTPaint *Create(const CAiMapLoc &loc, unsigned short value, unsigned int flags, unsigned char mode)
    {
        CTPaint *cmd = NewTask<CTPaint>();

        cmd->m_loc = loc;
        cmd->m_value = value;
        cmd->m_flags = flags;
        cmd->m_mode = mode;
        cmd->m_13 = 0;
        return cmd;
    }
    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
};

/* ai::MAP (SOCOM 1's CTMap): the AI maps changed -- the kinds and fields its Parse (0x5D69F0) sets
 * from the tags (the names ours); ai::PARAM and ai::STATE (SOCOM 1's CTParam, CTState): their blocks
 * not laid out yet -- only the handlers RegisterCommands hands on (unwritten) */
struct CTMap : TASK_HDR {
    unsigned char m_kind;   // 0x8: 1 an action cleared, 2 a region's type, 3-5 a line's visibility, type, id
    unsigned char _9[0xc - 0x9];
    int m_value;   // 0xc
    CAiMapLoc m_loc;   // 0x10: the region's or the line's cell

    /* a block of a kind (inline: the kind bound ahead of the allocation, live across the header's
     * mask -- stored after NewTask the constants colour the other way round; the name ours) */
    static CTMap *Create(unsigned char kind)
    {
        CTMap *cmd = NewTask<CTMap>();

        cmd->m_kind = kind;
        return cmd;
    }
    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
};

struct CTParam : TASK_HDR {
    unsigned char _8[0x10 - 0x8];
    TASK_ENT m_ent;   // 0x10: an entity whose SEAL is tested in the task's SEAL's place (the tick's)

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
};

extern "C" {
/* the parameter's test on a SEAL and its AI (OVL2 0x5D4770, this unit's, unwritten: the int and
 * float ai::test instances' caller; the name a placeholder) */
bool func_005D4770(CZSealBody *seal, CSealCtrlAi *ai, CTParam *cmd);
}

struct CTState : TASK_HDR {
    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
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

    /* (the names ours: Parse's first stores, their values not folded -- an inline's parameters) */
    void SetKind(int kind) { m_kind = kind; }
    void SetBlocking(bool b) { m_blocking = b; }

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
    static void End(_zanim_cmd_hdr *hdr);    /* (RegisterCommands' handlers, unwritten) */
    static void Begin(_zanim_cmd_hdr *hdr);
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
    static CTLookAt *Create(const char *name);    /* (RegisterCommands' handlers, unwritten) */
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
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
    static void End(_zanim_cmd_hdr *hdr);    /* (RegisterCommands' handlers, unwritten) */
    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static void Begin(_zanim_cmd_hdr *hdr);
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
    static void End(_zanim_cmd_hdr *hdr);    /* (RegisterCommands' handlers, unwritten) */
    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
};

/* ai::FIREWEAPON (SOCOM 1's CTFireWeapon) */
struct CTFireWeapon : TASK_HDR {
    void SetFiring(bool b) { m_firing = b; }

    int m_item;   // 0x8: an EQUIP_ITEM (a word: the kit's test takes its byte)
    unsigned char m_firing : 1;   // 0xc bit 0 (the name ours)
    /* bits 1 and 3: an aim point or an aim cell given (Tick reads them; nothing read so far sets them
     * -- the names ours) */
    unsigned char m_hasPos : 1;
    unsigned char m_c_2 : 1;
    unsigned char m_hasCell : 1;
    unsigned char m_c_4 : 4;
    unsigned char _d[0x10 - 0xd];
    CAiMapLoc m_cell;   // 0x10
    CPnt3D m_pos;   // 0x14
    unsigned char _20[0x2c - 0x20];   // 0x2c bytes

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static void Begin(_zanim_cmd_hdr *hdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
};

/* the deploy state (kind 4: seal_ai.cpp's CAiSDeploy) as a fire task fills it -- where to fire, a
 * point it clears and the weapon (the names ours) */
struct TASK_DEPLOY : CAiState {
    CPnt3D m_pos;   // 0x58
    CPnt3D m_64;   // 0x64
    int m_item;   // 0x70
};

extern "C" {
/* the AI's target held (OVL2 0x60A850, ai_target.cpp's: CAiTarget's, the target at 0x67C) */
bool func_0060A850(void *target);
}

/* a fire mode as an AI takes it: SOCOM 1's CAiWepHint (the 8 bytes at a CTFireMode's 0x8, handed
 * whole to CSealCtrlAi::ChooseWeapon by Begin -- OVL2 0x563470, the SEALs' row; zseal.h declares
 * the class, kit.h its statics; the field names ours, after Parse's tags). The fields Parse
 * clears through a setter keep MW's unfolded inserts (`andi zero,3`, `andi zero,1` shared by bits 8
 * and 9, the 18-bit field's dsll32/dsrl32 of zero), the ones it sets straight fold to constants */
class CAiWepHint {
public:
    void SetMode(unsigned int m) { m_mode = m; }
    void SetBit8(bool b) { m_bit8 = b; }
    void SetHasWeapon(bool b) { m_hasWeapon = b; }
    void SetHigh(unsigned int n) { m_high = n; }

    unsigned int m_mode : 2;   // 0x8 bits 0-1
    unsigned int m_2 : 2;   // bits 2-3 (2)
    unsigned int m_auto : 2;   // bits 4-5 (1; "fullauto" 3)
    unsigned int m_burst : 2;   // bits 6-7 (1; "burst" 3)
    unsigned int m_bit8 : 1;   // 0x9 bit 0
    unsigned int m_hasWeapon : 1;   // bit 1: "weapon" given
    unsigned int m_weapon : 4;   // bits 2-5: "weapon"
    unsigned int m_high : 18;   // bits 14-31 (new in SOCOM II)
    unsigned int m_c;   // 0xc
};

/* ai::FIREMODE (SOCOM 1's CTFireMode) */
struct CTFireMode : TASK_HDR {
    CAiWepHint m_fire;   // 0x8
    PNT2D m_trigger;   // 0x10: "trigger" (0.1, 0.8 by default)
    unsigned char m_hasTrigger : 1;   // 0x18 bit 0: "trigger" given
    unsigned char _19[0x1c - 0x19];   // 0x1c bytes

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static void Begin(_zanim_cmd_hdr *hdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
};


/* the AI's two trigger times as one struct (zseal.h's m_734, m_73c, m_744 and m_748: Rfloats and what
 * they rolled), set and rolled through an inline member called on a REFERENCE local -- its address
 * kept in one saved register across the rolls' rand() calls, every field at an offset from it (the
 * target's s2); called through the cast pointer, MW hoists each field's address into a register of
 * its own (s1-s3) instead; the rolls Rfloat's operator (its locals pad the joins). The names ours */
struct TASK_AITRIGGER {
    void Set(float b, float a)
    {
        m_b = b;
        m_a = a;
        Roll();
    }
    void Roll()
    {
        m_first = m_a;
        m_sum = m_first + m_b;
    }

    Rfloat m_a;   // 0x734
    Rfloat m_b;   // 0x73c
    float m_sum;   // 0x744
    float m_first;   // 0x748
};

/* the AI's bits 16 and 17 set through a bool (SOCOM 1's CTFireMode::Begin calls SetHoldFire(false)
 * where SOCOM II clears both: the `andi zero,1` the two share; the names ours) */
inline void SetAiBit16(CSealCtrlAi *ai, bool b)
{
    ai->m_aibit16 = b;
}
inline void SetAiBit17(CSealCtrlAi *ai, bool b)
{
    ai->m_aibit17 = b;
}

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

    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);    /* (RegisterCommands' handlers, unwritten) */
    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
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
    }    /* (RegisterCommands' handlers, unwritten) */
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
};

/* ai::PLAY (SOCOM 1's CTPlay): anims played, their types at the block's end (the names ours) */
struct CTPlay : TASK_HDR {
    Rfloat m_8;   // 0x8
    Rfloat m_10;   // 0x10
    unsigned char _18[0x20 - 0x18];
    signed char m_count;   // 0x20: the anims (SOCOM 1's a bitfield at 0x20 bit 1)
    unsigned char _21;
    unsigned char m_22;   // 0x22: 11 when made, else the tag "stance" (0-2)
    unsigned char m_skip_end : 1;   // 0x23 bit 0: the tag "skip_end"
    unsigned char m_blocking : 1;   // bit 1: the tag "blocking"
    unsigned char _23_2 : 6;
    TASK_ENT m_ent;   // 0x24: the tag "entity"
    unsigned char _2c[0x30 - 0x2c];
    unsigned int m_anims[1];   // 0x30: m_count of them (SOCOM 1's at 0x24)

    static CTPlay *Create(int count, unsigned int *anims, float a, float b);
    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    /* (RegisterCommands' handlers, unwritten) */
    static void Begin(_zanim_cmd_hdr *hdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
    static void End(_zanim_cmd_hdr *hdr);
};

/* the pursuit state as the task sets it (CAiSPursue, seal_ai.cpp's: its duration at 0x58) */
class CAiSPursueTask : public CAiState {
public:
    float m_duration;   // 0x58
};

/* the commands registered with ZAnim under their names, each id left in ai_cmdid[0] (every Parse copies
 * it into its block) but ai::SELECT's, kept in ai_cmdid[2] -- the type the handlers' find looks for
 * (OVL2 0x5DE320; SOCOM 1's, without its ai::ACTION, ai::DOOR and ai::SPECIAL; CMission::Init calls it) */
bool CSealCtrlAi::RegisterCommands()
{
    ai_cmdid[0] = ZAnim.AddCmd("ai::COMM", CTComm::Parse, 0, CTComm::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::CREATE", CTCreate::Parse, 0, CTCreate::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::DEBUG", CTDebug::Parse, 0, CTDebug::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::DRAIN", CTDrain::Parse, CTDrain::Begin, CTDrain::Tick, CTDrain::End);
    ai_cmdid[0] = ZAnim.AddCmd("ai::EVENT", CTEvent::Parse, CTEvent::Begin, CTEvent::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::FIREMODE", CTFireMode::Parse, CTFireMode::Begin, CTFireMode::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::FIREWEAPON", CTFireWeapon::Parse, CTFireWeapon::Begin, CTFireWeapon::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::GOTO", CTGoto::Parse, CTGoto::Begin, CTGoto::Tick, CTGoto::End);
    ai_cmdid[0] = ZAnim.AddCmd("ai::GROUP", CTGroup::Parse, CTGroup::Begin, CTGroup::Tick, CTGroup::End);
    ai_cmdid[0] = ZAnim.AddCmd("ai::HOLD", CTHold::Parse, CTHold::Begin, CTHold::Tick, CTHold::End);
    ai_cmdid[0] = ZAnim.AddCmd("ai::INSTATE", CTInState::Parse, 0, CTInState::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::INVIEW", CTInView::Parse, 0, CTInView::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::LOOKAT", CTLookAt::Parse, CTLookAt::Begin, CTLookAt::Tick, CTLookAt::End);
    ai_cmdid[0] = ZAnim.AddCmd("ai::MACRO", CTMacro::Parse, CTMacro::Begin, CTMacro::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::MAP", CTMap::Parse, 0, CTMap::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::MOVE", CTMove::Parse, CTMove::Begin, CTMove::Tick, CTMove::End);
    ai_cmdid[0] = ZAnim.AddCmd("ai::PAINT", CTPaint::Parse, 0, CTPaint::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::PARAM", CTParam::Parse, 0, CTParam::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::PURSUE", CTPursue::Parse, CTPursue::Begin, CTPursue::Tick, CTPursue::End);
    ai_cmdid[0] = ZAnim.AddCmd("ai::ANIMATE", CTPlay::Parse, CTPlay::Begin, CTPlay::Tick, CTPlay::End);
    ai_cmdid[0] = ZAnim.AddCmd("ai::INRANGE", CTRange::Parse, 0, CTRange::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::INREGION", CTRegion::Parse, 0, CTRegion::Tick, 0);
    ai_cmdid[2] = ZAnim.AddCmd("ai::SELECT", CTSelect::Parse, 0, CTSelect::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::SETMODE", CTSetMode::Parse, 0, CTSetMode::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::SETANIMSET", CTSetAnimSet::Parse, 0, CTSetAnimSet::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::SIGNAL", CTSignal::Parse, 0, CTSignal::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::SOUND", CTSound::Parse, CTSound::Begin, CTSound::Tick, CTSound::End);
    ai_cmdid[0] = ZAnim.AddCmd("ai::STANCE", CTStance::Parse, CTStance::Begin, CTStance::Tick, CTStance::End);
    ai_cmdid[0] = ZAnim.AddCmd("ai::STATE", CTState::Parse, 0, CTState::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::STOPALL", CTStopAll::Parse, 0, CTStopAll::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::TEAM", CTTeam::Parse, 0, CTTeam::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::TURRET", CTTurret::Parse, 0, CTTurret::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::VALVE", CTValve::Parse, 0, CTValve::Tick, 0);
    ai_cmdid[0] = ZAnim.AddCmd("ai::WAIT", CTWait::Parse, CTWait::Begin, CTWait::Tick, 0);
    return true;
}

/* whether the AI drives a SEAL, the controller tested in a ternary (zseal.h's IsAi presets a bool:
 * the target's both arms narrow the result -- the IsPlayer shape; the name ours) */
inline bool TaskIsAi(CZSealBody *seal)
{
    return seal->m_control ? ((CSealCtrl *)seal->m_control)->IsAi() : false;
}

/* a squad command issued to a team's unit, the team's AI SEALs put to sleep (28) or woken (27) first
 * (OVL2 0x5DDFA0; SOCOM 1's, without its cover and hold cases). The command is held twice, as the
 * FT_COMMAND IssueCommand takes and as the int the switch tests: one lbu, coalesced into one register
 * that neither use masks (an int alone masks at the call, an FT_COMMAND alone in the switch; any of
 * seven spellings of the pair matches). The walk's element is re-assigned through its ternary, `e =
 * e && ... ? e : 0`: a named variable of two definitions keeps its declaration's number, after the
 * command's and before the SEAL's ternary temporary -- initialised from the ternary, the element is
 * the ternary's temporary, numbered after the SEAL's, and the two trade s3/s4 (round 42, replaying
 * MW's colouring with the element relabelled). The iterator declared outside the loop is the
 * target's (kept in the frame, each step through $f0); inside the for it is a register (8 regs) */
bool CTComm::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTComm);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;
    FT_COMMAND command = (FT_COMMAND)cmd->m_cmd;
    int c = command;
    CEntityList::const_iterator it;

    if (c) {
        switch (c) {
        case 27:
        case 28:
            for (it = CEntity::GetEntityList().begin(); it != CEntity::GetEntityList().end(); ++it) {
                CEntity *e = *it;

                e = e && e->m_type == CEntity::ENT_TYPE_SEAL ? e : 0;
                CZSealBody *s = (CZSealBody *)e;

                if (s && (1 << cmd->m_team) & s->m_team && TaskIsAi(s))
                    SealAi(s)->SetSleeping(c == 28);
            }
            break;
        }
        CSealUnit *unit = CSealUnit::GetUnitByTeam(cmd->m_team);

        if (unit)
            unit->IssueCommand(seal, command, 0, false);
    }
    *dT = 0.0f;
    return true;
}

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

/* a reader's item as zrdr.h's _zrdr::Item has it, the index unsigned (zanim.h's _zrdr, which this
 * unit sees, takes an int, and an int index is strength-reduced in a loop: CTPlay::Parse's walk
 * recomputes `i << 3` each pass; the name ours) */
inline _zrdr *TaskItem(_zrdr *rdr, unsigned int i)
{
    return i < rdr->Count() ? &rdr->array[i] : 0;
}

/* the AI maps changed: "clear_action" (kind 1); a "region" by "name" given a "set_type" (kind 2);
 * else a "line" by "name" given "set_visible", "set_type" or "set_id" (kinds 3-5) -- the line's
 * test compares the tag with the region's null result (`==`, not `=`: the line branch runs only
 * when there is no line, and reads the null region) (OVL2 0x5D69F0; SOCOM 1's) */
_zanim_cmd_hdr *CTMap::Parse(_zrdr *rdr)
{
    CTMap *cmd = 0;
    bool clear;

    if (zrdr_findbool(rdr, "clear_action", &clear)) {
        cmd = Create(1);
    } else {
        _zrdr *r;

        if ((r = zrdr_findtag(rdr, "region")) != 0) {
            CAiMapLoc loc;

            if (GetRegion__7CAiMapsFPCcR9CAiMapLoc(&AiMaps, zrdr_findstring(r, "name"), loc)) {
                const char *type = zrdr_findstring(r, "set_type");

                if (type) {
                    CAiMap::region::TYPE t = RegionTypeByName(type);

                    cmd = Create(2);
                    cmd->m_loc = loc;
                    cmd->m_value = t;
                }
            }
        } else if (r == zrdr_findtag(rdr, "line")) {
            CAiMapLoc loc;

            if (GetLocation__7CAiMapsFPCcR9CAiMapLoc(&AiMaps, zrdr_findstring(r, "name"), loc)) {
                bool visible;
                int value;

                if (zrdr_findbool(r, "set_visible", &visible)) {
                    cmd = Create(3);
                    cmd->m_loc = loc;
                    cmd->m_value = visible;
                } else if (zrdr_findint(r, "set_type", &value, 1)) {
                    cmd = Create(4);
                    cmd->m_loc = loc;
                    cmd->m_value = value;
                } else if (zrdr_findint(r, "set_id", &value, 1)) {
                    cmd = Create(5);
                    cmd->m_loc = loc;
                    cmd->m_value = value;
                }
            }
        }
    }
    return &cmd->hdr;
}

/* the action a node offers, up its tree (CNodeAction::GetAction(node, true) inlined one level, as
 * seal_env.cpp's ActionOf: the accessors and the recursion called) */
static inline CNodeAction *TaskActionOf(zdb::CNode *node)
{
    CNodeAction *a = 0;

    if (node) {
        a = dynamic_cast<CNodeAction *>(func_005AA610(node));
        if (!a)
            a = CNodeAction::GetAction(func_005AA600(node), true);
    }
    return a;
}

/* the AI maps changed, done at once: the running anim's node action's AI map action marked invalid
 * and its valve set to 99 (kind 1), a region's type set (2), a line's visibility, type or id set
 * (3-5, new in SOCOM II) (OVL2 0x5D6770; SOCOM 1's, the line kinds added) */
bool CTMap::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    CTMap *cmd = (CTMap *)hdr;

    switch (cmd->m_kind) {
    case 1: {
        CNodeAction *na = TaskActionOf(func_002703D0(ZAnim.m_CurAnim, false));

        if (na) {
            CAiMap::action *act = 0;
            CAiMapLoc loc;

            if (AiMaps.GetActionById(na->m_id, loc, &act)) {
                act->SetTypeInvalid(1);
                na->m_valve->SetValue(99);
            }
        }
        break;
    }
    case 2: {
        const CAiMapRegion *r = GetRegion__7CAiMapsFRC9CAiMapLoc(&AiMaps, cmd->m_loc);

        if (r)
            SetRegionType__7CAiMapsFPCQ26CAiMap6regionQ36CAiMap6region4TYPE(&AiMaps, r, (unsigned char)cmd->m_value);
        break;
    }
    case 4:
    case 3:
    case 5: {
        CAiMap *map = AiMaps.GetMapByIndex(cmd->m_loc.m_mapid);
        CAiMap::line *l = map ? map->m_lineTable.Find(cmd->m_loc) : 0;

        if (l) {
            switch (cmd->m_kind) {
            case 4:
                l->m_type = cmd->m_value;
                break;
            case 3:
                l->m_unk15 = cmd->m_value != 0;
                break;
            case 5:
                l->m_id = cmd->m_value;
                break;
            }
        }
        break;
    }
    }
    *dT = 0.0f;
    return true;
}

/* anims played: the "anim" list's types (-1 for a name none has), a "stance" (0-2 kept in m_22),
 * "skip_end", the "prewait" and "postwait" periods, "blocking", an "entity" by the running anim's
 * name and the AI's index "ai" (OVL2 0x5D3C80; SOCOM 1's grown -- the stance, skip_end and the
 * entity new in SOCOM II) */
_zanim_cmd_hdr *CTPlay::Parse(_zrdr *rdr)
{
    _zrdr *anims = zrdr_findtag(rdr, "anim");

    if (!anims)
        return 0;
    int count = anims->Count();
    CTPlay *cmd = Create(count, 0, 0.0f, 0.0f);

    for (int i = 0; i < count; i++) {
        const char *name = TaskItem(anims, i)->String();

        cmd->m_anims[i] = name ? (int)FindAnimType(name) : -1;
    }
    const char *stance = zrdr_findstring(rdr, "stance");

    if (stance)
        switch (sealai::get_stance(stance)) {
        case STANCE_STAND:
            cmd->m_22 = 0;
            break;
        case STANCE_CROUCH:
            cmd->m_22 = 1;
            break;
        case STANCE_PRONE:
            cmd->m_22 = 2;
            break;
        }
    cmd->m_skip_end = zrdr_findtag(rdr, "skip_end") != 0;
    zrdr_findRfloat(rdr, "prewait", &cmd->m_8);
    zrdr_findRfloat(rdr, "postwait", &cmd->m_10);
    if (zrdr_findtag(rdr, "blocking"))
        cmd->m_blocking = 1;
    const char *ent = zrdr_findstring(rdr, "entity");

    cmd->m_ent.m_name = 0;
    cmd->m_ent.m_id = CEntity::invalidId;
    cmd->m_ent.m_type = 0;
    cmd->m_ent.SetName(ent);
    cmd->m_param = 0;
    zrdr_finduint(rdr, "ai", (unsigned int *)&cmd->m_param, 1);
    return &cmd->hdr;
}

/* a region painted: the "region" named and its cell, then a "soil" (mode 1, flag bit 2), a
 * "boundary" tile under 3 (mode 2, bit 3) or a "reg_type" (mode 3, the flags cleared) (OVL2
 * 0x5D57C0; SOCOM 1's, its GetTypeName inlined) */
_zanim_cmd_hdr *CTPaint::Parse(_zrdr *rdr)
{
    CTPaint *cmd = 0;
    const char *name = zrdr_findstring(rdr, "region");
    CAiMapLoc loc;

    if (name && GetRegion__7CAiMapsFPCcR9CAiMapLoc(&AiMaps, name, loc)) {
        cmd = CTPaint::Create(loc, 0, 1, 0);
    }
    if (cmd) {
        const char *soil = zrdr_findstring(rdr, "soil");
        const char *type = zrdr_findstring(rdr, "reg_type");
        unsigned int tile = 0;

        if (soil) {
            cmd->m_mode = 1;
            cmd->m_value = CAiMap::GetSoilByName(soil);
            cmd->m_flags |= 4;
        } else if (zrdr_finduint(rdr, "boundary", &tile, 1) && tile < 3) {
            cmd->m_mode = 2;
            cmd->m_value = tile;
            cmd->m_flags |= 8;
        } else if (type) {
            cmd->m_mode = 3;
            cmd->m_value = RegionTypeByName(type);
            cmd->m_flags = 0;
        }
    }
    return &cmd->hdr;
}

/* a move from the tags: "node" (its names, kind 1), else "relseal" (a SEAL's index, kind 3 -- 4 with a
 * "dir") or "self" (kind 2); "pos" (or "ofs"), "offset" (or "dist"), "dir", "blocking" and the AI's
 * index "ai" (OVL2 0x5D64B0; SOCOM 1's, but _calloc's file and line) */
_zanim_cmd_hdr *CTMove::Parse(_zrdr *rdr)
{
    CTMove *cmd = NewTask<CTMove>();

    cmd->SetKind(0);
    cmd->m_pos = CPnt3D::zero;
    cmd->m_dist = 0.0f;
    cmd->m_dir = CPnt3D::unit_nz;
    cmd->m_node = 0;
    cmd->SetBlocking(false);

    const char *node = zrdr_findstring(rdr, "node");

    if (node) {
        cmd->m_node = ZAnim.m_CurAnim->LoadNodeNames(0, node, 0, 1);
        cmd->m_kind = 1;
    } else {
        _zrdr *rel = zrdr_findtag(rdr, "relseal");

        if (rel) {
            zrdr_toINT(rel, &cmd->m_node, 1);
            cmd->m_kind = 3;
        } else if (zrdr_findtag(rdr, "self"))
            cmd->m_kind = 2;
    }
    if (!zrdr_findPNT3D(rdr, "pos", &cmd->m_pos))
        zrdr_findPNT3D(rdr, "ofs", &cmd->m_pos);
    if (!zrdr_findreal(rdr, "offset", &cmd->m_dist, 1))
        zrdr_findreal(rdr, "dist", &cmd->m_dist, 1);
    if (zrdr_findPNT3D(rdr, "dir", &cmd->m_dir))
        switch (cmd->m_kind) {
        case 3:
            cmd->m_kind = 4;
            break;
        }
    if (zrdr_findtag(rdr, "blocking"))
        cmd->m_blocking = 1;
    cmd->m_param = 0;
    zrdr_finduint(rdr, "ai", (unsigned int *)&cmd->m_param, 1);
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

/* a signal by name -- the tag "signal", else the first item when neither tag is there, else for an
 * empty block AISIG_ANIMCOMPLETE -- and an entity; none for signal 0 (OVL2 0x5D1B70; SOCOM 1's,
 * with a seventh signal and the entity an anim name). The header's type is left unset, as SOCOM 1's
 * was. The entity through a reference local (or a pointer) keeps its address in s0 across AddName,
 * the target's; written `cmd->m_ent.` MW folds it into the block's offsets (CTSetAnimSet::Parse's
 * shape); the block declared after the name's fallbacks takes its zero in the loop test's slot; the
 * found signal tested `!= 0` keeps the re-mask */
_zanim_cmd_hdr *CTSignal::Parse(_zrdr *rdr)
{
    const char *name = zrdr_findstring(rdr, "signal");
    const char *ent = zrdr_findstring(rdr, "entity");

    if (!name && !ent && zrdr_isitemtype(rdr, 0, 3))
        name = rdr->Item(0)->String();
    if (!name && !rdr->Count())
        name = "AISIG_ANIMCOMPLETE";
    CTSignal *cmd = 0;
    AI_SIGNAL sig = GetSignalByName(name);

    if (sig != 0) {
        unsigned int size = TaskAlign(TaskStrLen(ent)) + 20;

        cmd = (CTSignal *)_calloc(size, 1, 0, 0);
        cmd->hdr.data_size = size;
        cmd->m_signal = sig;
        TASK_ENT &e = cmd->m_ent;

        e.m_name = 0;
        e.m_id = CEntity::invalidId;
        e.m_type = 0;
        e.SetName(ent);
    }
    return &cmd->hdr;
}

/* the signal handed to an AI -- the task's SEAL's, or with an entity named that SEAL's (found
 * three times over, for the SEAL, its controller and its AI: GetTyped's materialised type test
 * each time) -- with the running anim; done at once (OVL2 0x5D16E0; SOCOM 1's, the entity then by
 * its name). The SEAL and its controller declared ahead of the find's locals (the SEAL a copy of the
 * find's) give the target's numbering: the block s2, the SEAL the counter's s3, the controller the
 * list's s1 (one of them alone swaps the other pair); the type read through a const view, so Get's
 * switch reloads it */
bool CTSignal::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    CZSealBody *dest;
    CEntityCtrl *ctrl;
    TASK_SEAL(CTSignal);
    dest = seal;
    ctrl = dest ? dest->m_control : 0;
    CSealCtrlAi *ai = dest ? SealAi(dest) : 0;

    if (((const TASK_ENT *)&cmd->m_ent)->m_type) {
        dest = (CZSealBody *)cmd->m_ent.GetTyped(CEntity::ENT_TYPE_SEAL);
        CZSealBody *s = (CZSealBody *)cmd->m_ent.GetTyped(CEntity::ENT_TYPE_SEAL);

        ctrl = s ? s->m_control : 0;
        s = (CZSealBody *)cmd->m_ent.GetTyped(CEntity::ENT_TYPE_SEAL);
        ai = s ? SealAi(s) : 0;
    }
    if (!dest || !ctrl || !ai) {
        *dT = 0.0f;
        return true;
    }
    ai->AnimSignal(ZAnim.m_CurAnim, (AI_SIGNAL)cmd->m_signal);
    *dT = 0.0f;
    return true;
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
 * OPEN, 0x274 against 0x27C: the target ends a block after the anim's load and the preset `r`
 * (`lw a0; daddiu s0,zero,1; nop`) -- nothing branches to the switch it pads for -- so the case
 * compares load their constants fresh (`li v1,1`) where MW here shares the 1 with `r` (`li a2,1`),
 * and it leaves the id ternary's delay slot empty (`beqz v0; nop`); CTRange::Tick (byte-exact
 * through the same TASK_ENT::Get) has neither; `r` before or after the AI, assigned apart, no AI
 * local: no change. The turret's type test, materialised in the target (sltu; beqz; lbu; xori 3;
 * sltiu; movz -- the `&&` as a value, then the select), is TASK_ENT::GetTyped's (round 37: inside
 * the inline that holds Get's switch it stays a value; in the caller every spelling folded it into
 * branches -- build/scratch/kit-r36/v_turret*.py) */
bool CTTurret::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTTurret);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;
    bool r = true;
    CTurret *turret = (CTurret *)cmd->m_ent.GetTyped(CEntity::ENT_TYPE_TURRET);

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

/* an entity cloned ("clone": the source's name, then the new one) or destroyed ("destroy": its
 * name) (OVL2 0x5DDD60; new in SOCOM II: the name assigned inside the test, which tests v0 with
 * the copy in the delay slot) */
_zanim_cmd_hdr *CTCreate::Parse(_zrdr *rdr)
{
    CTCreate *cmd = 0;
    _zrdr *clone = zrdr_findtag(rdr, "clone");

    if (clone && zrdr_isitemtype(clone, 0, 3) && zrdr_isitemtype(clone, 1, 3))
        cmd = Create(clone->Item(0)->String(), clone->Item(1)->String(), 2);
    else {
        const char *name;

        if ((name = zrdr_findstring(rdr, "destroy")) != 0)
            cmd = Create(name, 0, 4);
    }
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

/* an AI event made as the task says -- its kind, range (75 unless given) and duration (0.15: both
 * statics, OVL2 data 0x650AB0 and 0x650AB8 -- a literal is built in registers), at its point or a
 * node's transform of it -- the SEAL its owner (OVL2 0x5DCE70; SOCOM 1's) */
bool CTEvent::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    static float duration = 0.15f;
    static float range = 75.0f;
    CAiEvent *e;   /* declared ahead of the find: the target's block s1, the event the counter's s2 */
    TASK_SEAL(CTEvent);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    e = CAiEvent::Acquire();

    e->m_duration = cmd->m_hasDuration ? cmd->m_duration : duration;
    float r = cmd->m_hasRange ? cmd->m_range : range;

    e->m_rangeSq = r * r;
    e->m_visible = (bool)cmd->m_vis;
    CPnt3D pos = CPnt3D::zero;

    if (cmd->m_hasNode) {
        zdb::CNode *node = ZAnim.m_CurAnim->GetNode(cmd->m_nodes, false);

        if (node) {
            CMatrix m;

            node->BuildMTW(m);
            m.Transform(&cmd->m_pos, &pos, 1);
        }
    } else
        pos = cmd->m_pos;
    e->m_pos = pos;
    if (seal)
        e->m_owner = seal->m_id;
    e->m_type = (CAiEvent::EVENT)cmd->m_type;
    CAiEvent::Create(e);
    return true;
}

/* a fire mode: its defaults (mode 2, single auto and burst steps, the trigger 0.1 to 0.8), a
 * weapon slot, the trigger, full auto or burst (OVL2 0x5DCC00; SOCOM 1's, the 18-bit field new) */
_zanim_cmd_hdr *CTFireMode::Parse(_zrdr *rdr)
{
    CTFireMode *cmd = NewTask<CTFireMode>();
    int weapon;
    PNT2D trigger;

    cmd->m_trigger.x = 0.1f;
    cmd->m_trigger.y = 0.8f;
    cmd->m_fire.SetMode(0);
    cmd->m_fire.m_2 = 2;
    cmd->m_fire.m_auto = 1;
    cmd->m_fire.m_burst = 1;
    cmd->m_fire.SetBit8(false);
    cmd->m_fire.m_c = 0;
    cmd->m_fire.SetHasWeapon(false);
    cmd->m_fire.SetHigh(0);
    if (zrdr_findint(rdr, "weapon", &weapon, 1)) {
        cmd->m_fire.m_weapon = weapon;
        cmd->m_fire.m_hasWeapon = 1;
    }
    if (zrdr_findPNT2D(rdr, "trigger", &trigger)) {
        cmd->m_hasTrigger = 1;
        cmd->m_trigger = trigger;
    }
    if (zrdr_findtag(rdr, "fullauto"))
        cmd->m_fire.m_auto = 3;
    else if (zrdr_findtag(rdr, "burst"))
        cmd->m_fire.m_burst = 3;
    return &cmd->hdr;
}

/* the fire mode given the AI's weapon choice; with a trigger its two trigger times (zseal.h's m_734
 * and m_73c, Rfloats) set to the trigger's ends and rolled, as an attack's reset rolls them (0x565B80);
 * the AI's bits 16 and 17 cleared (OVL2 0x5DC9D0; SOCOM 1's, its fire-mode switch to the kit gone) */
void CTFireMode::Begin(_zanim_cmd_hdr *hdr)
{
    TASK_SEAL(CTFireMode);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (ai) {
        ai->ChooseWeapon(cmd->m_fire);
        if (cmd->m_hasTrigger) {
            TASK_AITRIGGER &t = *(TASK_AITRIGGER *)ai->m_734;

            t.Set(cmd->m_trigger.x, cmd->m_trigger.y);
        }
        SetAiBit16(ai, false);
        SetAiBit17(ai, false);
    }
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

/* firing once: the mark dropped, the aim the task's point, its cell's centre or the AI's held
 * target, and a deploy state pushed to fire the weapon there (OVL2 0x5DC3C0; SOCOM 1's -- as there,
 * with no aim found and no target held the push goes ahead on the mark's true). The mark tested
 * `== 1` (andi, xori, sltiu into the bool's saved register); the bool declared ahead of the find's
 * locals takes the counter's register and leaves the block s2 (declared after them, the two swap) */
bool CTFireWeapon::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    bool ok;
    TASK_SEAL(CTFireWeapon);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (!ai) {
        *dT = 0.0f;
        return true;
    }
    ok = cmd->m_firing == 1;

    if (ok) {
        CPnt3D pos;

        cmd->SetFiring(false);
        if (cmd->m_hasPos) {
            pos = cmd->m_pos;
            ok = true;
        } else if (cmd->m_hasCell)
            ok = AiMaps.GetCellCenter(cmd->m_cell, &pos) != 0;
        else if (func_0060A850(ai->m_67c)) {
            CEntity *e = *(CEntity **)ai->m_67c;
            const CPnt3D *p = e ? &e->m_posW : 0;

            pos = *p;
            ok = true;
        }
        if (ok && ai->PushAiState(AI_STATE_DEPLOY, 0)) {
            CAiState *s = ai->GetAiState();
            TASK_DEPLOY *d = s && s->m_state == AI_STATE_DEPLOY ? (TASK_DEPLOY *)s : 0;

            d->SetFlags(1, 0);
            d->m_pos = pos;
            d->m_64 = CPnt3D::zero;
            d->m_item = cmd->m_item;
            d->m_active = true;
        }
    }
    *dT = 0.0f;
    return true;
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

/* a vector of blocks freed with the game's _free when it goes (fts_ovl2statics.cpp's static
 * s_taskBlocks registers this destructor; the class and its names the game states'): out of line
 * here, between CTGroup's tick and end (OVL2 0x5D9540) -- the blocks freed from the back, the storage
 * traded away through FreeVector (its vector(), clear, swap and ~vector called: 0x5D96F0, 0x5D96E0,
 * 0x5D96A0, 0x5D9630), then the base's teardown */
class CMallocBlocks : public std::vector<void *> {
public:
    ~CMallocBlocks();
};

CMallocBlocks::~CMallocBlocks()
{
    while (size()) {
        _free(back(), 0, 0);
        pop_back();
    }
    FreeVector<std::vector<void *> >(*this);
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

/* a remote SEAL's AI state as its network state carries it (the low byte of the word
 * CSealCtrl::GetAiNetState sends -- copied out by memcpy; the names ours) */
struct TASK_NETSTATE {
    unsigned char m_state;   // 0x0: an AI_STATE
    unsigned char _1[3];
};

/* a SEAL's AI, or 0 (inline: its by-value parameter keeps the tested value, v0, for the call
 * where an assignment's variable would be read; the name ours) */
static inline CSealCtrlAi *TaskAiOf(CZSealBody *s)
{
    return s ? SealAi(s) : 0;
}

/* whether the SEAL (the task's, or the entity named, when it is a SEAL) is in a state: its AI's
 * current state, or anywhere on its stack (the stack's bits); a remote SEAL's by its network state
 * (OVL2 0x5D8840; SOCOM 1's, the remote test new; the time left as it is). The entity's SEAL
 * assigned in TaskAiOf's argument (v0 tested and used, the copy in the delay slot); the type read
 * through a const view, so Get's switch reloads it; the network state declared before the word
 * (the later-declared 4-byte local takes the lower slot) */
bool CTInState::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTInState);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (((const TASK_ENT *)&cmd->m_ent)->m_type)
        ai = TaskAiOf(seal = (CZSealBody *)cmd->m_ent.GetTyped(CEntity::ENT_TYPE_SEAL));
    bool r = false;

    if (seal) {
        if (ai) {
            if (!cmd->m_stack)
                r = ai->AiStateIs((AI_STATE)cmd->m_state);
            else
                r = (ai->m_stateStack.m_10 & (1 << (AI_STATE)cmd->m_state)) != 0;
        } else if (seal->IsRemote()) {
            TASK_NETSTATE ns;
            unsigned int v = ((CSealCtrl *)seal->m_control)->GetAiNetState();

            memcpy(&ns, &v, sizeof(ns));
            r = cmd->m_state == ns.m_state;
        }
    }
    return r;
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

/* a script state's hold, false for none (inline: the hold's own bool inline converted in the arm --
 * `sltu` / `move v0,zero` -- and the result masked at the assignment) */
inline bool TaskScriptHeld(const CAiSScript *script)
{
    return script ? script->Held() : false;
}

/* a SEAL's select (inline: its result a temporary, numbered after the SEAL's) */
inline void *TaskSelect(CZSealBody *seal)
{
    return seal->m_select;
}

/* the anim run for the SEAL, its script state pushed with it (the AI's bit 6 held set meanwhile, the
 * old script's hold carried over) -- done at once (OVL2 0x5D6DC0; the select read into a local ahead
 * of GetNode, the bit read through an inline bool: both as the target). Closed in round 42 by MW's
 * colouring replayed with the vregs relabelled: the target needs the select numbered after the SEAL's
 * ternary temporary (the SEAL then s4, the select s1, the anim s5) -- the select through an inline
 * accessor is one; a named local is numbered first. The hold: an inline bool whose true arm is the
 * hold's own bool inline (`m_hold != 0` or `(bool)m_hold` alike) and whose false arm is `false` --
 * `0` there, or the inline returning unsigned char or int, leaves the conversion at the join */
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
        void *select = TaskSelect(seal);

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

/* the parameter tested on the task's SEAL, or with an entity named on its SEAL, with its AI -- done
 * at once (OVL2 0x5D4510; SOCOM 1's, its test inlined there, out of line here: 0x5D4770) */
bool CTParam::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTParam);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;

    if (((const TASK_ENT *)&cmd->m_ent)->m_type)
        seal = (CZSealBody *)cmd->m_ent.GetTyped(CEntity::ENT_TYPE_SEAL);
    ai = seal ? SealAi(seal) : 0;
    bool r = func_005D4770(seal, ai, cmd);

    *dT = 0.0f;
    return r;
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
    /* an entry's vehicle made and an entity put back in the pool (OVL1 0x2B28D0, 0x2B2310: the
     * states' rows; ai::CREATE's tick calls both) */
    CEntity *LoadVehicle(CVehicleRdrEntry *entry, bool b);
    void RecycleEntity(CEntity *e);

    unsigned char _0[0x1814];
    /* the vehicle file's entries (OVL1 bss 0x437CF4: an entity's m_vehicle_index indexes them) */
    CVehicleRdr *m_vehicles;   // 0x1814
};
extern CMission theMission;

extern "C" {
/* a SEAL's health at a damage location scaled, its death handed on at none (OVL2 0x5A5790:
 * seal_env.cpp's, unnamed) */
void func_005A5790(CZSealBody *seal, unsigned char loc, float scale);
}

/* an entity if it is a SEAL, none else -- and one by name (inline: tested as branches, outside
 * Get's switch; the names ours) */
static inline CEntity *TypedEntity(CEntity *e)
{
    return e && e->m_type == CEntity::ENT_TYPE_SEAL ? e : 0;
}
static inline CEntity *TypedEntity(const char *name)
{
    return TypedEntity(CEntity::GetEntityList().GetEntity(name));
}

/* an entity made from another's vehicle entry under a new name, a SEAL of the AI's side joined to
 * no squad and its brain started (mode 2), or a SEAL killed, switched off and put back in the pool
 * (mode 4), the running anim refreshed after either; mode 1 makes the block's own entry's vehicle
 * (Parse never sets it) -- done at once, dT untouched (OVL2 0x5DD990; SOCOM II's command, the AI
 * asked for and unused).
 * OPEN, 0x3BC against 0x3CC (6 structural, 29 register-only): where the anim's name is read with the
 * running anim the caller already holds (the clone's source, the destroyed SEAL's name) the target
 * copies s0 into a0 and tests a0 (`move a0,s0; beqz a0`, a nop padding the join), MW here tests s0
 * and moves it in the call's delay slot -- the anim read in Get, passed to a static inline, an if in
 * place of the ternary, `anim` declared at the top: all alike (build/scratch/kit/v_create1.py);
 * and the registers: the target's clone source s3, new name s2, made SEAL s1 (the block's), the
 * destroyed SEAL s1 and its node s2 -- here s2, s1, s2, s2, s1 (`tools/igreplay.py --want` over the
 * five found no numbering in its 200 s budget: the copy's node is missing from this graph) */
bool CTCreate::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    TASK_SEAL(CTCreate);
    CSealCtrlAi *ai = seal ? SealAi(seal) : 0;
    CZAnim *anim = ZAnim.m_CurAnim;

    if (cmd->m_mode & 1)
        theMission.LoadVehicle((CVehicleRdrEntry *)cmd->m_entry, false);
    else if (cmd->m_mode & 2) {
        const char *from = cmd->m_dst.Get();
        const char *name = cmd->m_src.Get();
        CEntity *e = name ? TypedEntity(from) : 0;

        if (e) {
            CVehicleRdr *rdr = theMission.m_vehicles;
            unsigned int i = e->m_vehicle_index;
            CVehicleRdrEntry *entry = i < rdr->size() ? &(*rdr)[i] : 0;

            if (entry) {
                CZSealBody *s = (CZSealBody *)TypedEntity(theMission.LoadVehicle(entry, false));

                if (s) {
                    s->m_bit13 = 1;
                    s->m_node->SetName(name);
                    s->SetTeamMask(1);
                    InitalizeSealUnit__9CSealUnitFP10CZSealBodyP9CSealUnit(s, 0);
                    SealAi(s)->InitAiBrain();
                    anim->Refresh(false);
                }
            }
        }
    } else if (cmd->m_mode & 4) {
        CZSealBody *s = (CZSealBody *)TypedEntity(cmd->m_src.Get());

        if (s) {
            zdb::CNode *node = s->GetNode();

            func_005A5790(s, 0, 0.0f);
            node->SetActive(false);
            theMission.RecycleEntity(s);
            anim->Refresh(false);
        }
    }
    return true;
}

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
