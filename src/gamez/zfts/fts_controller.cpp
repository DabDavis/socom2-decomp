/* fts_controller.cpp -- the controller configurations: CCtrlrConfigs (SOCOM 1's fts_controller.cpp:
 * the pad layouts read from controller.rdr, the current one's button for each game function).
 * OVL1 from 0x2C5C90, after fts_net_persona.cpp. Only some of it so far. Defined in reverse of
 * their image order (-inline deferred). */

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

/* the pad (only the pressures this unit reads) */
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

    unsigned char _0[0x14];
    float m_pressure[16];   // 0x14
};

/* a layout: each game function's button (SOCOM 1's at 10) */
class CCtrlrConfig {
public:
    PAD_BUTTON GetButton(int func) const { return m_btns[func]; }

    unsigned char _0[0x12];
    PAD_BUTTON m_btns[16];   // 0x12
};

class CCtrlrConfigs {
public:
    static bool IsJumpAndActionShared();
};

namespace {
/* the layout in use (SOCOM 1's) */
CCtrlrConfig *g_pCurrentConfig;
}

extern "C" {
extern CCtrlrConfig *data_004415B0;   /* (a second layout: the one being edited) */
void func_002C65B0(CCtrlrConfig *config, int arg);
}

/* the edited layout's button for a function, set (OVL1 0x2C6460) */
extern "C" bool func_002C6460(unsigned char func, PAD_BUTTON btn)
{
    bool done = false;

    if (data_004415B0) {
        done = true;
        data_004415B0->m_btns[func] = btn;
    }
    return done;
}

/* the edited layout's button for a function (OVL1 0x2C6430) */
extern "C" PAD_BUTTON func_002C6430(unsigned char func)
{
    PAD_BUTTON btn = PAD_BUTTON_0;

    if (data_004415B0)
        btn = data_004415B0->m_btns[func];
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
    return pad->GetPressure(g_pCurrentConfig->m_btns[func]);
}

/* whether jump and action are on one button (OVL1 0x2C63D0; SOCOM 1's, the same code: jump's
 * button through the accessor, expanded ahead, so it is loaded first and compared second) */
bool CCtrlrConfigs::IsJumpAndActionShared()
{
    return g_pCurrentConfig->m_btns[PadBtnFunc::BTN_ACTION] ==
           g_pCurrentConfig->GetButton(PadBtnFunc::BTN_JUMP);
}

/* (OVL1 0x2C6280) */
extern "C" void func_002C6280(int arg)
{
    func_002C65B0(g_pCurrentConfig, arg);
}
