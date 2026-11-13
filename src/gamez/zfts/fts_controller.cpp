/* fts_controller.cpp -- the controller configurations: CCtrlrConfigs (SOCOM 1's fts_controller.cpp:
 * the pad layouts read from controller.rdr, the current one's button for each game function).
 * OVL1 from 0x2C5C90, after fts_net_persona.cpp. Only some of it so far: the loader (Init,
 * 0x2C5E70, with the copies its inlined layout constructor calls, 0x2C5FF0-0x2C60BF), the edited
 * layout's buttons, the pressures, the jump/action test, and SOCOM II's stick readers (the strafe
 * 0x2C6110, a function's button held 0x2C61E0, a stick's axis 0x2C6520, the pitch 0x2C65B0 and its
 * caller, the look 0x2C6290), and the chat line's USB keyboard (0x2C6C70, its last function
 * before the layouts' destructor), and the per-layout parser 0x2C67F0. Whole since round 53. Defined
 * in reverse of their image order (-inline deferred). */

#include <string>
#include <vector>

#include "../zrdr/zrdr.h"

/* a game function's button (SOCOM 1's PadBtnFunc::_) */
namespace PadBtnFunc {
enum _ {
    BTN_JUMP = 0,
    BTN_ACTION = 5
};
}

/* a pad button (a byte enum: copied with lb, compared and indexed with lbu), 16 for none */
enum PAD_BUTTON {
    PAD_BUTTON_0 = 0,
    PAD_BUTTON_NONE = 16
};

/* a button's state (SOCOM 1's KEY_STATE, a byte enum: copied with lb, compared zero-extended) */
enum KEY_STATE {
    KEY_UP = 0,
    KEY_FALLING = 1,
    KEY_DOWN = 2,
    KEY_RISING = 3
};

/* a byte enum's value as a char (ours: THE BYTE-ENUM FAMILY'S `lb; andi 0xff`, states round 26).
 * A use reading the stored enum through a temporary of this, built at each use, keeps the char's
 * load (lb, the constructor's copy) and its zero-extension (the conversion) apart: two uses build
 * two temporaries, whose loads MW merges and whose conversions it merges only after the peephole
 * pass that folds a single-use lb/andi pair into lbu has run -- so the pair survives, where one
 * read of the enum, any local, parameter, reference or cast folds into lbu
 * (build/scratch/states/p11.cpp's t3, p12.cpp's q1/q2b: `#pragma opt_common_subs off` shows the
 * two andi's apart, `#pragma peephole off` the pair kept) */
template <class E> class CByteEnum {
public:
    CByteEnum(E e) : m_v((char)e) {}
    operator E() const { return (E)(unsigned char)m_v; }

    char m_v;
};

/* a stick a layout gives a function, none, the left or the right (a byte; the names ours) */
enum STICK {
    STICK_NONE = 0,
    STICK_LEFT = 1,
    STICK_RIGHT = 2
};

/* the pad (only what this unit reads: the buttons' states and pressures, the sticks' positions --
 * raw and dilated, each stick's x then y) */
class CPad {
public:
    void SetButtonState(PAD_BUTTON button, KEY_STATE state);   /* 0x2DA160 */
    /* a button's state (the name ours: the look's, 0x2C6290: the argument's copy in the select's arm
     * keeps the select two blocks) */
    KEY_STATE GetButtonState(PAD_BUTTON button) const { return m_state[button]; }
    bool GetDoubleClick(PAD_BUTTON button);   /* 0x2DA1C0 */

    /* a button's pressure, none for no button (the name ours: the button arrives through an
     * inline taking the enum -- loaded with lb, narrowed inside) */
    float GetPressure(PAD_BUTTON btn) const
    {
        if (btn != PAD_BUTTON_NONE)
            return m_pressure[btn];
        return 0.0f;
    }

    unsigned char _0;
    KEY_STATE m_state[16];   // 0x1
    unsigned char _11[0x14 - 0x11];
    float m_pressure[16];   // 0x14
    unsigned char _54[0xd8 - 0x54];
    int m_port;   // 0xd8: the libpad2 port (zinput.h's)
    unsigned char _dc[0xe4 - 0xdc];
    unsigned char m_e4[256];   // 0xe4: the pad's read buffer (zinput.h's)
    unsigned char _1e4[0x210 - 0x1e4];
    float m_raw[2][2];   // 0x210
    float m_dilated[2][2];   // 0x220
    unsigned char _230[0x2a8 - 0x230];
    /* each stick's two flags in its CStickType (zinput.h's m_stickType[2] at 0x284: +0x24, +0x25),
     * set from the layout by SelectConfig (the names ours) */
    unsigned char m_leftFlagX;   // 0x2a8
    unsigned char m_leftFlagY;   // 0x2a9
    unsigned char _2aa[0x2d0 - 0x2aa];
    unsigned char m_rightFlagX;   // 0x2d0
    unsigned char m_rightFlagY;   // 0x2d1
};

/* the pads (zinput.h's) */
class CInput {
public:
    static CPad *m_pads[2];
};

/* a string that is never null (SOCOM 1's zutil.h inline over MSL's basic_string: the text
 * assigned, or the string emptied), made from a C string -- the layout's name (OVL1 0x2C6060, the
 * constructor's copy: the three words cleared, then the assignment inlined).
 * THE DEPTH IS FITTED, as fts_state.h's string is: the layout's constructor must be six high at
 * -inline level=5 for Init and the parser to inline it one level (calling this and the settings'
 * constructors), where include/string's default constructor is two high and this, written
 * plainly, three. fts_state.h measured the original's string four high (CMenuState() cut,
 * CCinematicState() not); the two forwarding levels below (Empty, Erase -- ours) stand in for
 * those two levels, compile to nothing, and leave the copy byte-exact (one level fewer and the
 * constructor inlines whole: 0x194 against 0x174) */
class CSaferStr : public std::string {
public:
    CSaferStr() {}
    CSaferStr(const char *sz) { *this = sz; }
    void operator=(const char *sz)
    {
        if (sz)
            assign(sz);
        else
            Empty();
    }
    void Empty() { Erase(); }
    void Erase() { erase(); }
};

/* a layout's settings, each a byte in a class of its own, cleared by its own constructor (SOCOM
 * 1's layout zeroed six bytes in place; SOCOM II's constructor calls three distinct one-byte
 * constructors -- 0x2C6050 three times, 0x2C6040 twice, 0x2C6030 once -- and the buttons' fill
 * 0x2C5FF0: four classes. Their names are ours): a stick (none, left or right) */
class CCfgStick {
public:
    CCfgStick() : m_stick(STICK_NONE) {}

    STICK m_stick;
};

/* a stick mask (bit 0 the left stick, bit 1 the right) */
class CCfgSticks {
public:
    CCfgSticks() : m_mask(0) {}

    unsigned char m_mask;
};

/* a flag */
class CCfgFlag {
public:
    CCfgFlag() : m_on(0) {}

    unsigned char m_on;
};

/* a game function's button, none at first */
class CCfgButton {
public:
    CCfgButton() : m_btn(PAD_BUTTON_NONE) {}

    PAD_BUTTON m_btn;
};

/* each game function's button (SOCOM 1's twelve, SOCOM II's thirteen): the implicit constructor's
 * copy 0x2C5FF0 is MW's array construction, a pointer loop to the 13th byte that is never
 * unrolled, the element's constructor inlined */
class CCfgButtons {
public:
    CCfgButton m_btn[13];
};

/* a layout: each game function's button (SOCOM 1's at 10), and SOCOM II's stick settings ahead of
 * them -- the look's stick (0xC), the strafe's (0xD), the pitch's two stick masks (0xF while
 * function 1's button is held, 0x10 otherwise) and whether that button holds the look (0x11) */
class CCtrlrConfig {
public:
    /* named, everything else at its default (the loader's dummy and the parser's `new`: too tall to
     * inline whole -- the name's constructor is -- so both callers call its members' constructors,
     * whose copies follow the loader, the first of them, in reverse of their first use) */
    CCtrlrConfig(const char *name) : m_name(name) {}

    PAD_BUTTON GetButton(int func) const { return m_btns.m_btn[func].m_btn; }
    /* the same, through a byte temporary (CByteEnum: each use its own) */
    CByteEnum<PAD_BUTTON> Button(int func) const { return m_btns.m_btn[func].m_btn; }
    /* a function's button's state on the pad, up for no button */
    KEY_STATE GetButtonState(int func, const CPad *pad) const
    {
        return m_btns.m_btn[func].m_btn != PAD_BUTTON_NONE ? pad->m_state[m_btns.m_btn[func].m_btn] : KEY_UP;
    }
    /* the same with the button held in a local, the pad's own accessor reading the state (the
     * look's, 0x2C6290) */
    KEY_STATE HeldButtonState(int func, const CPad *pad) const
    {
        PAD_BUTTON button = GetButton(func);

        return button != PAD_BUTTON_NONE ? pad->GetButtonState(button) : KEY_UP;
    }
    bool IsDown(int func, const CPad *pad) const { return HeldButtonState(func, pad) == KEY_DOWN; }
    KEY_STATE ButtonState(int func, const CPad *pad) const
    {
        return Button(func) != PAD_BUTTON_NONE ? pad->m_state[Button(func)] : KEY_UP;
    }
    /* whether a function's button is in a state (the name ours: 0x2C60C0's) */
    bool IsInState(int func, const CPad *pad, KEY_STATE state) const { return state == ButtonState(func, pad); }
    bool HoldsLook() const { return m_11.m_on; }

    CSaferStr m_name;   // 0x0: SelectConfig's key
    CCfgStick m_lookStick;   // 0xc
    CCfgStick m_strafeStick;   // 0xd
    CCfgStick m_e;   // 0xe: a stick (1 left, 2 right) SelectConfig flags at the pad (x)
    CCfgSticks m_pitchSticksHeld;   // 0xf: the pitch's sticks while function 1's button is down
    CCfgSticks m_pitchSticks;   // 0x10: bit 0 the left stick's y, bit 1 the right's
    CCfgFlag m_11;   // 0x11
    CCfgButtons m_btns;   // 0x12
};   // 0x20 bytes (the loader's and the parser's `new`)

class CCtrlrConfigs {
public:
    static void Init();
    static bool IsJumpAndActionShared();
    static float GetFwdBackStickPos(const CPad &pad);
    static void SelectConfig(const char *name);

    /* the layouts read from controller.rdr (SOCOM 1's static vector of them) */
    static std::vector<CCtrlrConfig *> m_configs;
};

/* the layouts (0x4415C8: the unit's initialiser 0x400730 clears its three words and registers its
 * destructor's copy 0x2C6ED0) */
std::vector<CCtrlrConfig *> CCtrlrConfigs::m_configs;

extern "C" int strcasecmp(const char *a, const char *b);

namespace {
/* the layout in use (SOCOM 1's) */
CCtrlrConfig *g_pCurrentConfig;
}

extern "C" {
extern CCtrlrConfig *data_004415B0;   /* (a second layout: the one being edited) */
}

/* the chat line's keyboard (only what the handler below reads or calls): a key read this frame
 * (zin_keyboard.cpp's KeyInput: the code, the shift key, the caps lock light), the network's
 * online flag, and the mission's message editor (theMission +0x1060, fts_msgeditor.cpp's) */
struct KeyInput {
    unsigned char code;   // 0x0: the USB keyboard's code
    int shift;   // 0x4
    int ctrl;   // 0x8
    int alt;   // 0xc
    int _10;   // 0x10
    int caps;   // 0x14
    int _18[2];
};

int GetKeyReadThisFrame(KeyInput *key);   /* 0x2DBD90 */

class CZNetwork {
public:
    unsigned char m_8;   // 0x0
    unsigned char m_9;   // 0x1: an online game
};

extern CZNetwork theNetwork;

class CMsgEditor {
public:
    void AddCharacter(char c);   /* 0x2C9D90 */
    void DeleteCharacter();   /* 0x2C9D00 */
    void Submit();   /* 0x2C9B80 */
    void Cancel();   /* 0x2C9CC0 */
};

class CMissionChat {
public:
    unsigned char _0[0x1060];
    CMsgEditor m_messages4;   // 0x1060: the chat line
};

extern CMissionChat theMission;

/* the keys typed into the chat line while online (SOCOM II's; OVL1 0x2C6C70, the name ours): a
 * letter (codes 4-29, upper case with shift or caps lock but not both), the digit row (30-39) and
 * the punctuation (45-56) with or without shift, the keypad's digits (89-99), space and the
 * keypad's four operators each typed; otherwise backspace (42), enter (40) and escape (41) delete,
 * send and drop -- each switch's cases in the reverse of their compare chain's order */
extern "C" void func_002C6C70()
{
    KeyInput key;

    if (!theNetwork.m_9)
        return;
    while (GetKeyReadThisFrame(&key)) {
        char c = 0;

        if (key.code >= 4 && key.code < 30) {
            c = key.code + 93;
            c = (key.shift != 0) ^ (key.caps != 0) ? c - 32 : c;
        } else if (key.code >= 30 && key.code < 40)
            c = (key.shift ? "!@#$%^&*()" : "1234567890")[key.code - 30];
        else if (key.code >= 45 && key.code < 57)
            c = (key.shift ? "_+{}|}:\"~<>?" : "-=[]\\];'`,./")[key.code - 45];
        else if (key.code >= 89 && key.code < 100)
            c = "1234567890."[key.code - 89];
        else {
            switch (key.code) {
            case 44:
                c = ' ';
                break;
            case 84:
                c = '/';
                break;
            case 85:
                c = '*';
                break;
            case 86:
                c = '-';
                break;
            case 87:
                c = '+';
                break;
            }
        }
        if (c)
            theMission.m_messages4.AddCharacter(c);
        else {
            switch (key.code) {
            case 42:
                theMission.m_messages4.DeleteCharacter();
                break;
            case 40:
                theMission.m_messages4.Submit();
                break;
            case 41:
                theMission.m_messages4.Cancel();
                break;
            }
        }
    }
}

namespace {
PAD_BUTTON ReadBtnName(const char *name);

/* the game functions' names in controller.rdr, by function (SOCOM 1's c_arrFuncNames, a local of
 * the unit's namespace; SOCOM II adds TACMAP) */
const char *c_arrFuncNames[13] = {
    "Action", "AimMode", "Fire", "FireMode", "Inventory", "Jump", "Reload", "StrafeLeft", "StrafeRight",
    "SwapWeapon1", "SwapWeapon2", "TeamCommand", "TACMAP"
};
}

/* one layout read from its rdr item and added to the layouts (SOCOM 1's Init's loop body, moved out
 * of the loader in SOCOM II; OVL1 0x2C67F0, the name the map's): its name, then each mapping's
 * Result and Input's Control -- a game function's button by name, else the stick settings: MoveLong
 * the look's stick (and an AimMode Off the held look), Pitch a stick mask for the held set, the
 * other or both by AimMode, Rotate and Strafe their sticks. The layout is pushed by reference, so
 * it lives in the frame and every store reloads it; the function index is narrowed to the byte
 * enum before ReadBtnName (the target's andi in the call's delay slot), the mask an int narrowed
 * at each OR.
 * Byte-exact since round 53 (written in round 20, 12 off). At the button store and the two mask ORs the target
 * colours the layout's reload a0 and the narrowed value v1, and for the ORs that means the numbering
 * byte < reload < mask, the loaded byte numbered ahead of the base it is loaded through -- only a
 * variable can be. An integer local is forwarded into the OR (any of them), a local of the byte enum
 * STICK is not (IRO's propagation leaves enum-typed variables alone, as in zRemoteClientConnectsCallback
 * 0x2BC620), and written `mask | sticks` the OR's operands are the target's. The button store's
 * base an rvalue pointer (the index then narrowed after the reload); `(unsigned char)mask` rather than
 * the plain int, which folds the narrowing away (0x478) */
extern "C" void func_002C67F0(_zrdr *config)
{
    const char *name = zrdr_findstring(config, "Name");
    CCtrlrConfig *pConfig = new CCtrlrConfig(name);

    CCtrlrConfigs::m_configs.push_back(pConfig);
    _zrdr *mappings = zrdr_findtag(config, "Mappings");
    int count = mappings ? (int)mappings->Count() : 0;

    for (int i = 0; i < count; i++) {
        _zrdr *mapping = mappings->Item(i);
        const char *result = zrdr_findstring(mapping, "Result");
        _zrdr *input = zrdr_findtag(mapping, "Input");
        const char *control = zrdr_findstring(input, "Control");

        if (!result || !control)
            continue;
        bool found = false;

        for (int func = 0; func < 13; func++) {
            if (!strcasecmp(result, c_arrFuncNames[func])) {
                PadBtnFunc::_ f = (PadBtnFunc::_)func;

                ((CCfgButton *)&pConfig->m_btns)[f].m_btn = ReadBtnName(control);
                found = true;
                break;
            }
        }
        if (found)
            continue;
        if (!strcasecmp(result, "MoveLong")) {
            if (!strcasecmp(control, "LeftStickVert"))
                pConfig->m_lookStick.m_stick = STICK_LEFT;
            else if (!strcasecmp(control, "RightStickVert"))
                pConfig->m_lookStick.m_stick = STICK_RIGHT;
            const char *aim = zrdr_findstring(input, "AimMode");

            if (aim && !strcasecmp(aim, "Off"))
                pConfig->m_11.m_on = 1;
        } else if (!strcasecmp(result, "Pitch")) {
            int mask = 0;

            if (!strcasecmp(control, "LeftStickVert"))
                mask = 1;
            else if (!strcasecmp(control, "RightStickVert"))
                mask = 2;
            bool hasAim = zrdr_findtag(input, "AimMode") != 0;
            const char *aim = zrdr_findstring(input, "AimMode");
            bool on = !aim || strcasecmp(aim, "Off");

            if (!hasAim || on) {
                STICK sticks = (STICK)pConfig->m_pitchSticksHeld.m_mask;

                pConfig->m_pitchSticksHeld.m_mask = (unsigned char)mask | sticks;
            }
            if (!hasAim || !on) {
                STICK sticks = (STICK)pConfig->m_pitchSticks.m_mask;

                pConfig->m_pitchSticks.m_mask = (unsigned char)mask | sticks;
            }
        } else if (!strcasecmp(result, "Rotate")) {
            if (!strcasecmp(control, "LeftStickHoriz"))
                pConfig->m_e.m_stick = STICK_LEFT;
            else if (!strcasecmp(control, "RightStickHoriz"))
                pConfig->m_e.m_stick = STICK_RIGHT;
        } else if (!strcasecmp(result, "Strafe")) {
            if (!strcasecmp(control, "LeftStickHoriz"))
                pConfig->m_strafeStick.m_stick = STICK_LEFT;
            else if (!strcasecmp(control, "RightStickHoriz"))
                pConfig->m_strafeStick.m_stick = STICK_RIGHT;
        }
    }
}

namespace {
/* a button by its controller.rdr name, none for an unknown one (SOCOM 1's ReadBtnName -- nested in
 * CCtrlrConfigs there, a namespace in SOCOM 1 --, the same chain with SOCOM II's Select and Pause
 * after X; OVL1 0x2C6670: the parser 0x2C67F0's) */
PAD_BUTTON ReadBtnName(const char *name)
{
    if (!strcasecmp(name, "Circle"))
        return (PAD_BUTTON)7;
    if (!strcasecmp(name, "L1"))
        return (PAD_BUTTON)12;
    if (!strcasecmp(name, "L2"))
        return (PAD_BUTTON)13;
    if (!strcasecmp(name, "LeftStickTap"))
        return (PAD_BUTTON)15;
    if (!strcasecmp(name, "R1"))
        return (PAD_BUTTON)10;
    if (!strcasecmp(name, "R2"))
        return (PAD_BUTTON)11;
    if (!strcasecmp(name, "RightStickTap"))
        return (PAD_BUTTON)14;
    if (!strcasecmp(name, "Square"))
        return (PAD_BUTTON)8;
    if (!strcasecmp(name, "X"))
        return (PAD_BUTTON)9;
    if (!strcasecmp(name, "Select"))
        return (PAD_BUTTON)1;
    if (!strcasecmp(name, "Pause"))
        return PAD_BUTTON_0;
    return PAD_BUTTON_NONE;
}
}

#pragma auto_inline off
/* the pitch from the sticks a layout gives it -- the held set while function 1's button is down
 * -- summed and clamped to -1..1 (OVL1 0x2C65B0: 0x2C6280's tail call, out of line, pinned) */
extern "C" float func_002C65B0(const CCtrlrConfig *config, const CPad *pad)
{
    const unsigned char *sticks;

    if (config->GetButtonState(1, pad) == KEY_DOWN)
        sticks = &config->m_pitchSticksHeld.m_mask;
    else
        sticks = &config->m_pitchSticks.m_mask;
    unsigned char mask = *sticks;
    float pos = 0.0f;

    if (mask & 1)
        pos += pad->m_dilated[0][1];
    if (mask & 2)
        pos += pad->m_dilated[1][1];
    if (pos < -1.0f)
        pos = -1.0f;
    else if (pos > 1.0f)
        pos = 1.0f;
    return pos;
}

/* a stick's position on one axis, raw or dilated, x negated (OVL1 0x2C6520: the strafe's call,
 * out of line, pinned; each case's ternary joins before its break, and the negation is a returned
 * ternary -- an if-assign leaves a nop before the return) */
extern "C" float func_002C6520(const CPad *pad, STICK stick, int axis, bool dilated)
{
    float pos = 0.0f;

    switch (stick) {
    case STICK_LEFT:
        pos = dilated ? pad->m_dilated[0][axis] : pad->m_raw[0][axis];
        break;
    case STICK_RIGHT:
        pos = dilated ? pad->m_dilated[1][axis] : pad->m_raw[1][axis];
        break;
    }
    return axis == 0 ? -pos : pos;
}

/* whether a function's button is held down (OVL1 0x2C61E0: the strafe's calls, out of line,
 * pinned) */
extern "C" bool func_002C61E0(const CCtrlrConfig *config, unsigned char func, const CPad *pad)
{
    return config->GetButtonState(func, pad) == KEY_DOWN;
}
#pragma auto_inline reset

/* a layout's stick read out of line (OVL1 0x2C6220, a draft's: `lb v0,0(a0)`) */
extern "C" STICK func_002C6220(const STICK *stick);

/* the forward and back stick (SOCOM 1's GetFwdBackStickPos, the name by its body; the rdr's MoveLong):
 * the layout's stick's raw y, unless function 1's button holds it (OVL1 0x2C6290). The test is `!A || !B` over two inline bools: the target materialises both negations
 * (sltu; xori and xori; sltiu; xori) with the true in the first branch's delay slot.
 * Byte-exact since round 53 (written in round 18, 6 off). The colouring needs config, look, button in that
 * order (v1, a1, a2): the button numbered before look's Linearize temporary, the layout after it --
 * so the button an inline's named local and the layout read from the global at each use, a common
 * subexpression (round 34 found those registers). What stopped it was the select: with a scalar
 * button IRO's RebuildCondExpressions turns the two arms into one ECOND, the zero in the beq's delay
 * slot, 0x10 shorter. CPad's accessor in the arm stops that: its by-value argument is copied in the
 * arm, a statement besides the value's assignment, and the rebuild leaves the arm alone (the copy
 * itself costs no code). The select in an inline returning KEY_STATE keeps the load a plain copy,
 * `lb`; written straight in the compare the load is `lbu`, one off. */
float CCtrlrConfigs::GetFwdBackStickPos(const CPad &pad)
{
    bool look = !g_pCurrentConfig->HoldsLook() || !g_pCurrentConfig->IsDown(1, &pad);

    if (look) {
        float pos = 0.0f;

        switch (g_pCurrentConfig->m_lookStick.m_stick) {
        case STICK_LEFT:
            pos = pad.m_raw[0][1];
            break;
        case STICK_RIGHT:
            pos = pad.m_raw[1][1];
            break;
        }
        return pos;
    }
    return 0.0f;
}

/* whether a function's button is down on the pad (OVL1 0x2C60C0: the button's lb/andi through its
 * byte temporary, KEY_DOWN's runtime narrowing -- `li a2,2` in the test's delay slot, `andi` at the
 * join -- through one of its own).
 * Byte-exact since round 53 (written in round 26, 2 off). The wanted state is an inline's by-value parameter
 * compared on the LEFT, `state == ButtonState(...)`, and its argument a conversion through
 * CByteEnum, not a plain constant: MW substitutes a plain constant (xori), but holds this one in a
 * temporary assigned ahead of the inline's body, in another flow node than the compare, where
 * propagation does not forward it. Written in place the constant's temporary on the compare's right
 * is 2 off (the andi's the other way round) and on its left it is built at the join and folds. */
extern "C" bool func_002C60C0(unsigned char func, const CPad *pad)
{
    const CCtrlrConfig *config = g_pCurrentConfig;

    return config->IsInState(func, pad, CByteEnum<KEY_STATE>(KEY_DOWN));
}

/* the strafe: the layout's stick's x when it has one, else its two buttons, left -1 and right 1
 * (OVL1 0x2C6110; the buttons' value through a result local -- returned straight, the 1.0f's
 * branch lands on the zero's nop) */
extern "C" float func_002C6110(const CPad *pad)
{
    const CCtrlrConfig *config = g_pCurrentConfig;

    if (func_002C6220(&config->m_strafeStick.m_stick) != STICK_NONE)
        return func_002C6520(pad, func_002C6220(&config->m_strafeStick.m_stick), 0, false);
    float pos;

    if (func_002C61E0(config, 7, pad))
        pos = -1.0f;
    else if (func_002C61E0(config, 8, pad))
        pos = 1.0f;
    else
        pos = 0.0f;
    return pos;
}

/* the layouts read from controller.rdr, the default made the current one -- SOCOM 1's Init, its
 * parse moved out to 0x2C67F0. Without a default SOCOM 1 made a dummy the current layout; SOCOM
 * II's makes the dummy, keeps nothing of it and copies the (null) current layout to the edited one
 * (OVL1 0x2C5E70: CMission::Init's call) */
void CCtrlrConfigs::Init()
{
    _zrdr *rdr = zrdr_read("controller.rdr", "data/common", 0);
    _zrdr *configs = zrdr_findtag(rdr, "ControllerConfigs");

    for (int i = 0; i < (configs ? (int)configs->Count() : 0); i++)
        func_002C67F0(configs->Item(i));
    zrdr_free((CRdrFile *)rdr);
    SelectConfig("Default");
    if (!g_pCurrentConfig) {
        new CCtrlrConfig("NoDefaultDummy");
        data_004415B0 = g_pCurrentConfig;
    }
}

/* the first pad's y flags set from the pitch's stick masks when function 1's button goes down (the
 * held set) or up (the other) (OVL1 0x2C5C90: the state's ternary in place -- through an inline
 * taking the pad its index's add takes the pad first --, the mask a char narrowed after the join,
 * the rising test's return its own block) */
extern "C" void func_002C5C90(CPad *pad)
{
    const CCtrlrConfig *config = g_pCurrentConfig;
    char mask;

    KEY_STATE state = config->Button(1) != PAD_BUTTON_NONE ? pad->m_state[config->Button(1)] : KEY_UP;

    if (state == KEY_FALLING)
        mask = config->m_pitchSticksHeld.m_mask;
    else {
        if (state != KEY_RISING)
            return;
        mask = config->m_pitchSticks.m_mask;
    }
    unsigned char m = mask;

    pad->m_leftFlagY = (m & 1) != 0;
    pad->m_rightFlagY = (m & 2) != 0;
}

/* a function's button's state set on a pad (OVL1 0x2C5D10: the layout read ahead of the pad's
 * test -- it loads in the test's delay slot; the button a local, passed raw and compared narrowed:
 * the lb/andi pair a second use gives) */
extern "C" void func_002C5D10(CPad *pad, unsigned char func, KEY_STATE state)
{
    const CCtrlrConfig *config = g_pCurrentConfig;

    if (pad) {
        PAD_BUTTON btn = config->GetButton(func);

        if (btn != PAD_BUTTON_NONE)
            pad->SetButtonState(btn, state);
    }
}

/* the layout of a name made the current one (and the edited one), the first pad's stick flags set
 * from it: x by the stick at 0xE, y by the pitch's sticks at 0x10 (OVL1 0x2C5D60; SOCOM 1's walk,
 * which stopped at the current one). Pinned: the loader calls it out of line */
#pragma auto_inline off
void CCtrlrConfigs::SelectConfig(const char *name)
{
    for (int i = 0; i < m_configs.size(); i++) {
        if (!strcasecmp(name, m_configs[i]->m_name.c_str())) {
            g_pCurrentConfig = m_configs[i];
            CCtrlrConfig *config = g_pCurrentConfig;
            CPad *pad = CInput::m_pads[0];

            pad->m_leftFlagX = config->m_e.m_stick == 1;
            pad->m_leftFlagY = config->m_pitchSticks.m_mask == 1;
            pad->m_rightFlagX = config->m_e.m_stick == 2;
            pad->m_rightFlagY = config->m_pitchSticks.m_mask == 2;
            data_004415B0 = g_pCurrentConfig;
            return;
        }
    }
}
#pragma auto_inline reset

/* a function's button's state on the pad, up for no button (OVL1 0x2C64E0: the button read at
 * each use through its byte temporary, the ternary's arms apart) */
extern "C" KEY_STATE func_002C64E0(unsigned char func, const CPad *pad)
{
    return g_pCurrentConfig->Button(func) != PAD_BUTTON_NONE ? pad->m_state[g_pCurrentConfig->Button(func)]
                                                             : KEY_UP;
}

/* whether a function's button was double-clicked on the pad (OVL1 0x2C6490: CPad::GetDoubleClick
 * through the layout's button) */
extern "C" bool func_002C6490(unsigned char func, CPad *pad)
{
    PAD_BUTTON btn = g_pCurrentConfig->GetButton(func);

    return btn != PAD_BUTTON_NONE ? pad->GetDoubleClick(btn) : false;
}

/* the edited layout's button for a function, set (OVL1 0x2C6460) */
extern "C" bool func_002C6460(unsigned char func, PAD_BUTTON btn)
{
    bool done = false;

    if (data_004415B0) {
        done = true;
        data_004415B0->m_btns.m_btn[func].m_btn = btn;
    }
    return done;
}

/* the edited layout's button for a function (OVL1 0x2C6430) */
extern "C" PAD_BUTTON func_002C6430(unsigned char func)
{
    PAD_BUTTON btn = PAD_BUTTON_0;

    if (data_004415B0)
        btn = data_004415B0->m_btns.m_btn[func].m_btn;
    return btn;
}

/* a function's button's pressure on the pad (OVL1 0x2C63F0).
 * Byte-exact now; the history, 0x38 against 0x40: the target loads the button with lb and narrows it with andi 0xff (a
 * byte enum's copy, then its compare), and leaves a nop after the 0.0f; every form here folds the
 * two into one lbu. Tried: the enum through an inline taking it (GetPressure), an enum local, a
 * signed char local converted at each use, the test inverted, a negative enumerator; round 8: a
 * switch on it, unsigned char and int locals, the body as an inline of its own, the button
 * through GetButton and then GetPressure. The same lb/andi pair opens 0x2C5C90, 0x2C5D10 and
 * 0x2C60C0 (CCtrlrConfigs' inlined GetButtonState, as in SOCOM 1's GetStrafeStickPos, whose
 * out-of-line GetButtonState folds to lbu), and 0x2C60C0 keeps an inline's constant KEY_DOWN as
 * `li a2,2; andi v1,a2,0xff` where every probe here folds it into xori: the target's inlining
 * kept its parameters' conversions, ours does not. Round 20 (the buttons now a one-byte class,
 * CCfgButton): a BY-VALUE copy of the element -- an accessor returning it, a parameter taking it,
 * a named local -- does load it with lb, but keeps the copy in a stack slot (`sb`, then `lbu` back)
 * where the target narrows the register (build/scratch/states/p2.cpp); an inline taking an int or
 * an unsigned char and narrowing it inside folds to lbu like the rest */
extern "C" float func_002C63F0(unsigned char func, const CPad *pad)
{
    if (g_pCurrentConfig->Button(func) != PAD_BUTTON_NONE)
        return pad->m_pressure[g_pCurrentConfig->Button(func)];
    return 0.0f;
}

/* whether jump and action are on one button (OVL1 0x2C63D0; SOCOM 1's, the same code: jump's
 * button through the accessor, expanded ahead, so it is loaded first and compared second) */
bool CCtrlrConfigs::IsJumpAndActionShared()
{
    return g_pCurrentConfig->m_btns.m_btn[PadBtnFunc::BTN_ACTION].m_btn ==
           g_pCurrentConfig->GetButton(PadBtnFunc::BTN_JUMP);
}

extern "C" int scePad2GetButtonInfo(int port, unsigned char *data, int n);   /* (libpad2: the pad read) */

/* function 2's button's pressure as the pad reports it, for the two buttons it is asked of (10 and
 * 9: the pad's bytes 29 and 26), none for any other (OVL1 0x2C6350: CSealCtrl::PreTick's; the
 * button read at each test through its byte temporary -- the lb/andi pair --, the result a local:
 * returned straight, the second call's branch lands on the zero's padding nop) */
extern "C" float func_002C6350(CPad *pad)
{
    float r;

    if (g_pCurrentConfig->Button(2) == 10)
        r = scePad2GetButtonInfo(pad->m_port, pad->m_e4, 29);
    else if (g_pCurrentConfig->Button(2) == 9)
        r = scePad2GetButtonInfo(pad->m_port, pad->m_e4, 26);
    else
        r = 0.0f;
    return r;
}

/* the pitch from the current layout's sticks (OVL1 0x2C6280) */
extern "C" float func_002C6280(const CPad *pad)
{
    return func_002C65B0(g_pCurrentConfig, pad);
}
