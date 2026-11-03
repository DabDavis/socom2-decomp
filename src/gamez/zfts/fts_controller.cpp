/* fts_controller.cpp -- the controller configurations: CCtrlrConfigs (SOCOM 1's fts_controller.cpp:
 * the pad layouts read from controller.rdr, the current one's button for each game function).
 * OVL1 from 0x2C5C90, after fts_net_persona.cpp. Only some of it so far: the loader (Init,
 * 0x2C5E70, with the copies its inlined layout constructor calls, 0x2C5FF0-0x2C60BF), the edited
 * layout's buttons, the pressures, the jump/action test, and SOCOM II's stick readers (the strafe
 * 0x2C6110, a function's button held 0x2C61E0, a stick's axis 0x2C6520, the pitch 0x2C65B0 and its
 * caller, the look 0x2C6290 open). Not yet: the per-layout parser 0x2C67F0 and its button-name
 * reader 0x2C6670. Defined in reverse of their image order (-inline deferred). */

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
    /* a function's button's state on the pad, up for no button */
    KEY_STATE GetButtonState(int func, const CPad *pad) const
    {
        return m_btns.m_btn[func].m_btn != PAD_BUTTON_NONE ? pad->m_state[m_btns.m_btn[func].m_btn] : KEY_UP;
    }
    bool IsDown(int func, const CPad *pad) const { return GetButtonState(func, pad) == KEY_DOWN; }
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
    static void SelectConfig(const char *name);

    /* the layouts read from controller.rdr (SOCOM 1's static vector of them) */
    static std::vector<CCtrlrConfig *> m_configs;
};

extern "C" int strcasecmp(const char *a, const char *b);

namespace {
/* the layout in use (SOCOM 1's) */
CCtrlrConfig *g_pCurrentConfig;
}

extern "C" {
extern CCtrlrConfig *data_004415B0;   /* (a second layout: the one being edited) */
}

/* one layout read from its rdr item and added to the layouts (SOCOM II's, out of the loader's
 * loop; OVL1 0x2C67F0, not written yet) */
extern "C" void func_002C67F0(_zrdr *config);

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

/* the look's pitch: the layout's stick's raw y, unless function 1's button holds the look (OVL1
 * 0x2C6290). The test is `!A || !B` over two inline bools: the target materialises both negations
 * (sltu; xori and xori; sltiu; xori) with the true in the first branch's delay slot.
 * OPEN, 6 instructions: the layout's pointer and its button trade registers (the target holds the
 * layout in v1 and the button in a2, this the other way round). Tried: the global read at each use
 * (worse: a1/a2), `bool look;` declared first, HoldsLook as `!= 0`, IsDown through a button local or
 * an if-assigned state (both 0x10 shorter) */
extern "C" float func_002C6290(const CPad *pad)
{
    const CCtrlrConfig *config = g_pCurrentConfig;
    bool look = !config->HoldsLook() || !config->IsDown(1, pad);

    if (look) {
        float pos = 0.0f;

        switch (config->m_lookStick.m_stick) {
        case STICK_LEFT:
            pos = pad->m_raw[0][1];
            break;
        case STICK_RIGHT:
            pos = pad->m_raw[1][1];
            break;
        }
        return pos;
    }
    return 0.0f;
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
 * OPEN, 0x38 against 0x40: the target loads the button with lb and narrows it with andi 0xff (a
 * byte enum's copy, then its compare), and leaves a nop after the 0.0f; every form here folds the
 * two into one lbu. Tried: the enum through an inline taking it (GetPressure), an enum local, a
 * signed char local converted at each use, the test inverted, a negative enumerator; round 8: a
 * switch on it, unsigned char and int locals, the body as an inline of its own, the button
 * through GetButton and then GetPressure. The same lb/andi pair opens 0x2C5C90, 0x2C5D10 and
 * 0x2C60C0 (CCtrlrConfigs' inlined GetButtonState, as in SOCOM 1's GetStrafeStickPos, whose
 * out-of-line GetButtonState folds to lbu), and 0x2C60C0 keeps an inline's constant KEY_DOWN as
 * `li a2,2; andi v1,a2,0xff` where every probe here folds it into xori: the target's inlining
 * kept its parameters' conversions, ours does not */
extern "C" float func_002C63F0(unsigned char func, const CPad *pad)
{
    return pad->GetPressure(g_pCurrentConfig->m_btns.m_btn[func].m_btn);
}

/* whether jump and action are on one button (OVL1 0x2C63D0; SOCOM 1's, the same code: jump's
 * button through the accessor, expanded ahead, so it is loaded first and compared second) */
bool CCtrlrConfigs::IsJumpAndActionShared()
{
    return g_pCurrentConfig->m_btns.m_btn[PadBtnFunc::BTN_ACTION].m_btn ==
           g_pCurrentConfig->GetButton(PadBtnFunc::BTN_JUMP);
}

extern "C" int func_001BDA78(int port, unsigned char *data, int n);   /* (libpad2: the pad read) */

/* function 2's button's pressure as the pad reports it, for the two buttons it is asked of (10 and
 * 9: the pad's bytes 29 and 26), none for any other (OVL1 0x2C6350: CSealCtrl::PreTick's).
 * OPEN, 1 instruction (0x7C against 0x80): the byte-enum family's lb/andi pair -- the target loads
 * the button with lb and narrows it with andi 0xff before the first compare, this folds both into
 * one lbu (the tests' order and the two calls are the target's) */
extern "C" float func_002C6350(CPad *pad)
{
    PAD_BUTTON btn = g_pCurrentConfig->GetButton(2);

    if (btn == 10)
        return func_001BDA78(pad->m_port, pad->m_e4, 29);
    if (btn == 9)
        return func_001BDA78(pad->m_port, pad->m_e4, 26);
    return 0.0f;
}

/* the pitch from the current layout's sticks (OVL1 0x2C6280) */
extern "C" float func_002C6280(const CPad *pad)
{
    return func_002C65B0(g_pCurrentConfig, pad);
}
