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

/* the reader's queries (zrdr.h's, which declares a _zrdr of its own: zanim.h's here) */
_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);
bool zrdr_findRfloat(const _zrdr *rdr, const char *tag, Rfloat *out);
bool zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n);
bool zrdr_tobool(const _zrdr *rdr, bool *out);
char *zrdr_findstring(const _zrdr *rdr, const char *tag);
bool zrdr_finduint(const _zrdr *rdr, const char *tag, unsigned int *out, int n);

extern "C" {
/* the anim's index'th command argument (SOCOM 1's CZAnimArgList::GetCmd(int); OVL1 0x2720E0) */
_zanim_cmd_hdr *func_002720E0(CZAnimArgList *list, int index);
/* the AI's mode put back to the one it saved, which is made 1 (OVL2 0x56D090, the SEAL lane's
 * row: SetAiMode with the byte at 0x3F5, then 1 stored there -- where SOCOM 1's CTHold::End called
 * SetAiMode with 1) */
void func_0056D090(CSealCtrlAi *ai);
/* the AI switched on or off, and its alarm (SOCOM 1's static CSealCtrlAi::SetAiEnabled(bool,
 * bool), which its CTStopAll::Tick calls so; OVL2 0x574830, the SEAL lane's row), and a static
 * SOCOM II added after it (OVL2 0x598840) */
void func_00574830(bool enabled, bool alarm);
void func_00598840(bool enabled);
/* the AI's stance set, and set for a time (OVL2 0x5631A0, 0x5631B0: the SEAL lane's rows, where
 * SOCOM 1's CTStance::Begin called CSealCtrlAi::SetStance) */
void func_005631A0(CSealCtrlAi *ai, SEAL_STANCE stance);
void func_005631B0(CSealCtrlAi *ai, SEAL_STANCE stance, bool b, float time);
/* CSealCtrl::ClearLook(AI_LOOK) (OVL2 0x600D40, seal_look.cpp's) */
void func_00600D40(CSealCtrl *ctrl, int look);
/* the script state's hold bit 3 set or cleared for the AI (OVL2 0x5F3FC0, seal_aistate.cpp's
 * range) and a CSealCtrlAi function the goto's end calls first (OVL2 0x5628F0) */
void func_005F3FC0(CAiSScript *s, CSealCtrlAi *ai, bool b);
void func_005628F0(CSealCtrlAi *ai);
/* the goto's move set up (OVL2 0x5DB8F0, this unit's: the AI, the command, a flag; whether it
 * began -- not written yet) */
bool func_005DB8F0(CSealCtrlAi *ai, void *cmd, int x);
/* a squad command by name, 0 for none (OVL2 0x58E0A0, the SEAL lane's row; a byte) */
FT_COMMAND func_0058E0A0(const char *name);
}

/* the commands' ids (SOCOM 1's ai_cmdid, 0x18 bytes: RegisterCommands stores every command's id
 * in the first word, which each Parse copies into its block's type; the third is the type of the
 * command argument that carries a task's SEAL) */
int ai_cmdid[6];

/* the command argument a task's SEAL comes in (the name ours): the SEAL at 0xC */
struct TASK_PARAM {
    _zanim_cmd_hdr hdr;   // 0x0
    unsigned char _4[0xc - 0x4];
    CZSealBody *m_seal;   // 0xc
};

/* the index'th SEAL argument of the running anim (SOCOM 1's taskFindParam(type, index), out of
 * line there; SOCOM II inlines it into every handler). The end of the arguments is a `break` on
 * `!(bool)p` -- materialised (sltu, xori) and laid out after the loop, as every inlined copy has
 * it; `if (!p) return 0;` branches inline -- and the counter is declared ahead of the list, which
 * gives the list s0 and the counter s1.
 * Still open in the handlers that use their block after the find: the target keeps the block in
 * s1, between the list and the counter (the find's type and index in s4/s3), where every spelling
 * tried gives it s4 or s0. Measured in round 9 (probes and ~40 real-unit variants): the order is
 * MW's numbering, not degree or use counts (20 extra temporaries or 16 extra uses of the block
 * never move it) -- params first, the handler's named locals in declaration order, then the named
 * variables of the inlined functions in expansion order, then temporaries; the highest number
 * takes s0, and the first-read non-parameter (here the list, by its null test) moves to the end.
 * `T *cmd = (T *)hdr;` is a variable of its own numbered just after the params (CTHold::Tick:
 * dT s5, block s4); a cast through an inline (`template <class T> T *TaskCmd(_zanim_cmd_hdr *h)
 * { return (T *)h; }`, a member returning `this`) makes the block the param's own register, which
 * the m_param read makes first-read: block s0, list s1, only those two swapped (CTGroup::End 5
 * instructions off); any use of hdr after the find gives s4. Written out by hand in a handler,
 * the target's order comes only with the block declared BETWEEN the find's counter and its list
 * -- the original numbers the block where an inlined variable declared between `int i` and the
 * list would be; no C++ shape found that does it. Tried and no better: the find `inline`; the list
 * first, from an inline accessor; the accessors through ((TASK_HDR *)hdr); the block declared after
 * the find, nested, as two locals, from TaskCmd after the find; the work after the find in an
 * inline member or a free inline taking the cast; the find taking the block, or a member with the
 * counter a parameter; GetCmd as a member; tools/perm.py (2 x 150 s over the find's declarations
 * and copies of type/index: best 8 of CTWait::Begin's 14, the block still s4) */
static _zanim_cmd_hdr *taskFindParam(unsigned int type, int index)
{
    int i;
    CZAnimArgList *list = ZAnim.m_CurAnim->m_arg_list;

    if (!list)
        return 0;
    for (i = 0;; i++) {
        _zanim_cmd_hdr *p = func_002720E0(list, i);

        if (!(bool)p)
            break;
        if (p->data_type == type && index-- == 0)
            return p;
    }
    return 0;
}

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

    TASK_PARAM *GetParam() const
    {
        _zanim_cmd_hdr *p = taskFindParam(ai_cmdid[2], m_param);

        return p ? (TASK_PARAM *)p : 0;
    }
    CZSealBody *GetSeal() const
    {
        TASK_PARAM *p = GetParam();

        return p ? p->m_seal : 0;
    }
    CSealCtrlAi *GetAi() const
    {
        CZSealBody *seal = GetSeal();

        return seal ? SealAi(seal) : 0;
    }
};

/* a block allocated and its header made: the type the registration left, the size (SOCOM II's
 * _calloc called without the file and line SOCOM 1's passed) */
template <class T> inline T *NewTask()
{
    T *cmd = (T *)_calloc(sizeof(T), 1, 0, 0);

    cmd->hdr.data_size = sizeof(T);
    cmd->hdr.data_type = ai_cmdid[0];
    return cmd;
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

    short m_type;   // 0x0
    short m_name;   // 0x2
    unsigned int m_id;   // 0x4: the entity's id once found
};

/* ai::INRANGE (SOCOM 1's CTRange): an entity within a range of the SEAL */
struct CTRange : TASK_HDR {
    float m_range2;   // 0x8: the range squared
    TASK_ENT m_ent;   // 0xc

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
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

    static void Begin(_zanim_cmd_hdr *hdr);
    static bool Tick(_zanim_cmd_hdr *hdr, float *dT);
    static void End(_zanim_cmd_hdr *hdr);
};

/* ai::EVENT (SOCOM 1's CTEvent) */
struct CTEvent : TASK_HDR {
    static void Begin(_zanim_cmd_hdr *hdr);
};

/* ai::DRAIN (SOCOM II's) */
struct CTDrain : TASK_HDR {
    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static void Begin(_zanim_cmd_hdr *hdr);
    static void End(_zanim_cmd_hdr *hdr);
};

/* ai::WAIT (SOCOM 1's CTWait): a wait drawn when it begins */
struct CTWait : TASK_HDR {
    TASK_TIMER m_wait;   // 0x8

    static _zanim_cmd_hdr *Parse(_zrdr *rdr);
    static void Begin(_zanim_cmd_hdr *hdr);
    /* a block made (inline: the period set, the timer drawn, and drawn again) */
    static CTWait *Create(const Rfloat &wait)
    {
        CTWait *cmd = NewTask<CTWait>();

        cmd->m_wait.Set(wait);
        cmd->m_wait.Reset();
        return cmd;
    }
};

/* ai::STANCE (SOCOM 1's CTStance): a stance taken for a time */
struct CTStance : TASK_HDR {
    SEAL_STANCE m_stance;   // 0x8 (a word; the callees take the byte)
    unsigned char _9[0xc - 0x9];
    SEAL_STANCE m_c;   // 0xc
    unsigned char _d[0x10 - 0xd];
    float m_time;   // 0x10
    float m_timer;   // 0x14

    static void Begin(_zanim_cmd_hdr *hdr);
    static void End(_zanim_cmd_hdr *hdr);
};

/* ai::MOVE (SOCOM 1's CTMove) */
struct CTMove : TASK_HDR {
    static void End(_zanim_cmd_hdr *hdr);
};

/* ai::MACRO (SOCOM 1's CTMacro): an anim of the set, by name */
struct CTMacro : TASK_HDR {
    CZAnim *m_anim;   // 0x8
    unsigned char _c[0x10 - 0xc];
    char m_name[16];   // 0x10

    static void Begin(_zanim_cmd_hdr *hdr);
};

/* ai::LOOKAT (SOCOM 1's CTLookAt) */
struct CTLookAt : TASK_HDR {
    /* (the names ours) */
    void SetLooking(unsigned int n) { m_looking = n; }

    Rfloat m_wait;   // 0x8
    float m_timer;   // 0x10
    unsigned char _14[0x18 - 0x14];
    unsigned char m_18_0 : 4;   // 0x18
    unsigned char m_looking : 4;   // 0x18 bits 4-7 (bit 4: the look to clear at the end)

    static void Begin(_zanim_cmd_hdr *hdr);
    static void End(_zanim_cmd_hdr *hdr);
};

/* ai::GROUP (SOCOM II's) */
struct CTGroup : TASK_HDR {
    unsigned char _8[0x14 - 0x8];
    unsigned int m_14;   // 0x14

    static void End(_zanim_cmd_hdr *hdr);
};

/* ai::GOTO (SOCOM 1's CTGoto) */
struct CTGoto : TASK_HDR {
    /* (the names ours) */
    void SetBit1(bool b) { m_bit1 = b; }

    unsigned char _8[0xc - 0x8];
    unsigned char m_bit0 : 1;   // 0xc bit 0
    unsigned char m_bit1 : 1;   // bit 1

    static void Begin(_zanim_cmd_hdr *hdr);
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
    CTDrain *cmd = (CTDrain *)hdr;
    CZSealBody *seal = cmd->GetSeal();
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? ((CSealCtrl *)seal->m_control)->GetSealCtrlAi() : 0;

    if (!seal || !ctrl)
        return;
}

/* (OVL2 0x5DD2A0) */
void CTDrain::End(_zanim_cmd_hdr *hdr)
{
    CTDrain *cmd = (CTDrain *)hdr;
    CZSealBody *seal = cmd->GetSeal();
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? ((CSealCtrl *)seal->m_control)->GetSealCtrlAi() : 0;

    if (!seal || !ctrl)
        return;
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

/* (OVL2 0x5DC800) */
_zanim_cmd_hdr *CTFireWeapon::Parse(_zrdr *rdr)
{
    CTFireWeapon *cmd = NewTask<CTFireWeapon>();

    cmd->m_item = CZWeapon::GetWeaponId(zrdr_findstring(rdr, "weapon"));
    cmd->SetFiring(false);
    return &cmd->hdr;
}

/* firing marked when the kit holds the weapon (OVL2 0x5DC6B0)
 * OPEN, 4 instructions: the AI's ternary: the target's call arm keeps the result in both arms (as SealAi gives GetAi); with the seal kept for the kit it takes the zero-first form here */
void CTFireWeapon::Begin(_zanim_cmd_hdr *hdr)
{
    CTFireWeapon *cmd = (CTFireWeapon *)hdr;
    CZSealBody *seal = cmd->GetSeal();
    CSealCtrlAi *ai = seal ? ((CSealCtrl *)seal->m_control)->GetSealCtrlAi() : 0;

    if (ai && seal->m_kit.HasItem((EQUIP_ITEM)cmd->m_item))
        cmd->SetFiring(true);
}

/* (OVL2 0x5DB730)
 * OPEN, 29 instructions: the command block's register: the target keeps it in s1, between the find's list (s0) and its counter (s2), the find's type and index in s4/s3; here the block takes s4 (see TASK_HDR), and the seal/controller/AI triple's registers */
void CTGoto::Begin(_zanim_cmd_hdr *hdr)
{
    CTGoto *cmd = (CTGoto *)hdr;
    CZSealBody *seal = cmd->GetSeal();
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? ((CSealCtrl *)seal->m_control)->GetSealCtrlAi() : 0;

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

/* (OVL2 0x5DB350)
 * OPEN, 29 instructions: the command block's register: the target keeps it in s1, between the find's list (s0) and its counter (s2), the find's type and index in s4/s3; here the block takes s4 (see TASK_HDR), and the seal/controller/AI triple's registers */
void CTGoto::End(_zanim_cmd_hdr *hdr)
{
    CTGoto *cmd = (CTGoto *)hdr;
    CZSealBody *seal = cmd->GetSeal();
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? ((CSealCtrl *)seal->m_control)->GetSealCtrlAi() : 0;

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

/* (OVL2 0x5D9430)
 * OPEN, 10 instructions: the command block's register: the target keeps it in s1, between the find's list (s0) and its counter (s2), the find's type and index in s4/s3; here the block takes s4 (see TASK_HDR) -- nothing else differs */
void CTGroup::End(_zanim_cmd_hdr *hdr)
{
    CTGroup *cmd = (CTGroup *)hdr;
    CSealCtrlAi *ai = cmd->GetAi();

    cmd->m_14 = 0;
}

/* the hold over: the AI's mode put back, unless it never began (OVL2 0x5D8E20)
 * OPEN, 10 instructions: the command block's register: the target keeps it in s1, between the find's list (s0) and its counter (s2), the find's type and index in s4/s3; here the block takes s4 (see TASK_HDR) -- nothing else differs */
void CTHold::End(_zanim_cmd_hdr *hdr)
{
    CTHold *cmd = (CTHold *)hdr;
    CSealCtrlAi *ai = cmd->GetAi();

    if (ai && cmd->m_timer != -1.0f)
        func_0056D090(ai);
}

/* the wait run down (OVL2 0x5D8F60)
 * OPEN, 12 instructions: the command block's register: the target keeps it in s1, between the find's list (s0) and its counter (s2), the find's type and index in s4/s3; here the block takes s4 (see TASK_HDR) */
bool CTHold::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    CTHold *cmd = (CTHold *)hdr;

    if (!cmd->GetAi()) {
        *dT = 0.0f;
        return true;
    }
    bool done = cmd->m_timer <= 0.0f;

    if (!done)
        cmd->m_timer -= *dT;
    *dT = 0.0f;
    return done;
}

/* the wait drawn, the AI held in mode 2 for it (OVL2 0x5D90E0)
 * OPEN, 14 instructions: the command block's register: the target keeps it in s1, between the find's list (s0) and its counter (s2), the find's type and index in s4/s3; here the block takes s4 (see TASK_HDR) */
void CTHold::Begin(_zanim_cmd_hdr *hdr)
{
    CTHold *cmd = (CTHold *)hdr;
    CSealCtrlAi *ai = cmd->GetAi();

    if (ai) {
        cmd->m_timer = cmd->m_wait;
        if (cmd->m_timer != 0.0f)
            ai->SetAiMode((AI_MODE)2);
    }
}

/* the look drawn (OVL2 0x5D7850)
 * OPEN, 31 instructions: the command block's register: the target keeps it in s1, between the find's list (s0) and its counter (s2), the find's type and index in s4/s3; here the block takes s4 (see TASK_HDR), and the seal/controller pair's registers */
void CTLookAt::Begin(_zanim_cmd_hdr *hdr)
{
    CTLookAt *cmd = (CTLookAt *)hdr;
    CZSealBody *seal = cmd->GetSeal();
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? ((CSealCtrl *)seal->m_control)->GetSealCtrlAi() : 0;

    if (!seal || !ctrl)
        return;
    cmd->m_timer = cmd->m_wait;
    cmd->SetLooking(cmd->m_looking & ~4);
}

/* the look cleared (OVL2 0x5D7290)
 * OPEN, 17 instructions: the command block's register: the target keeps it in s1, between the find's list (s0) and its counter (s2), the find's type and index in s4/s3; here the block takes s4 (see TASK_HDR), and the seal/controller pair's registers */
void CTLookAt::End(_zanim_cmd_hdr *hdr)
{
    CTLookAt *cmd = (CTLookAt *)hdr;
    CZSealBody *seal = cmd->GetSeal();
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? ((CSealCtrl *)seal->m_control)->GetSealCtrlAi() : 0;

    if (!seal || !ctrl)
        return;
    if (cmd->m_looking & 1)
        func_00600D40((CSealCtrl *)ctrl, 3);
}

/* the anim looked up by name (OVL2 0x5D7040)
 * OPEN, 11 instructions: the command block's register: the target keeps it in s1, between the find's list (s0) and its counter (s2), the find's type and index in s4/s3; here the block takes s4 (see TASK_HDR) */
void CTMacro::Begin(_zanim_cmd_hdr *hdr)
{
    CTMacro *cmd = (CTMacro *)hdr;

    if (cmd->GetAi())
        cmd->m_anim = ZAnim.GetAnim(cmd->m_name, 0);
}

/* (OVL2 0x5D5A20) */
void CTMove::End(_zanim_cmd_hdr *hdr)
{
    CTMove *cmd = (CTMove *)hdr;
    CZSealBody *seal = cmd->GetSeal();
    CEntityCtrl *ctrl = seal ? seal->m_control : 0;
    CSealCtrlAi *ai = seal ? ((CSealCtrl *)seal->m_control)->GetSealCtrlAi() : 0;

    if (!seal || !ctrl)
        return;
}

/* (OVL2 0x5D4080)
 * OPEN, 4 instructions: the target keeps the unused AI pointer, `b; move v1,v0` / `move v1,zero`, which MW drops here */
void CTPursue::End(_zanim_cmd_hdr *hdr)
{
    CTPursue *cmd = (CTPursue *)hdr;
    CSealCtrlAi *ai = cmd->GetAi();
}

/* running while the AI pursues (OVL2 0x5D4190)
 * OPEN, 2 instructions: 8 bytes short besides the block's register (not looked at closely) */
bool CTPursue::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    CTPursue *cmd = (CTPursue *)hdr;
    CSealCtrlAi *ai = cmd->GetAi();

    if (!ai) {
        *dT = 0.0f;
        return true;
    }
    bool r = ai->m_stateStack.Is(AI_STATE_SCRIPT);

    *dT = 0.0f;
    return r;
}

/* the pursuit pushed, its duration the command's (OVL2 0x5D4300)
 * OPEN, 10 instructions: the command block's register: the target keeps it in s1, between the find's list (s0) and its counter (s2), the find's type and index in s4/s3; here the block takes s4 (see TASK_HDR) */
void CTPursue::Begin(_zanim_cmd_hdr *hdr)
{
    CTPursue *cmd = (CTPursue *)hdr;
    CSealCtrlAi *ai = cmd->GetAi();

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

/* the stance taken, its timer started (OVL2 0x5D09F0)
 * OPEN, 13 instructions: the command block's register: the target keeps it in s1, between the find's list (s0) and its counter (s2), the find's type and index in s4/s3; here the block takes s4 (see TASK_HDR) */
void CTStance::Begin(_zanim_cmd_hdr *hdr)
{
    CTStance *cmd = (CTStance *)hdr;
    CSealCtrlAi *ai = cmd->GetAi();

    if (ai) {
        func_005631A0(ai, cmd->m_c);
        func_005631B0(ai, cmd->m_stance, true, CAiTimer::infinite);
        cmd->m_timer = cmd->m_time;
    }
}

/* back to standing, unless the stance was for no time or was prone (OVL2 0x5D08A0)
 * OPEN, 11 instructions: the command block's register: the target keeps it in s1, between the find's list (s0) and its counter (s2), the find's type and index in s4/s3; here the block takes s4 (see TASK_HDR) */
void CTStance::End(_zanim_cmd_hdr *hdr)
{
    CTStance *cmd = (CTStance *)hdr;
    CSealCtrlAi *ai = cmd->GetAi();

    if (ai && cmd->m_time != 0.0f && cmd->m_stance != STANCE_PRONE)
        func_005631B0(ai, STANCE_PRONE, true, 0.0f);
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

/* the AI switched on or off (OVL2 0x5CF800) */
bool CTStopAll::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
    CTStopAll *cmd = (CTStopAll *)hdr;
    bool enabled = !cmd->m_enable;

    func_00574830(enabled, !cmd->m_inactive);
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

/* (OVL2 0x5D2890) */
bool CTSelect::Tick(_zanim_cmd_hdr *hdr, float *dT)
{
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

/* the wait read, and drawn twice (OVL2 0x5CDF00; SOCOM 1's the same) */
_zanim_cmd_hdr *CTWait::Parse(_zrdr *rdr)
{
    CTWait *cmd = 0;
    Rfloat wait(0.0f);

    if (zrdr_findRfloat(rdr, "wait", &wait))
        cmd = Create(wait);
    return &cmd->hdr;
}

/* the wait drawn (OVL2 0x5CDDA0)
 * OPEN, 14 instructions: the command block's register: the target keeps it in s1, between the find's list (s0) and its counter (s2), the find's type and index in s4/s3; here the block takes s4 (see TASK_HDR) */
void CTWait::Begin(_zanim_cmd_hdr *hdr)
{
    CTWait *cmd = (CTWait *)hdr;
    CSealCtrlAi *ai = cmd->GetAi();

    cmd->m_wait.Reset();
}
