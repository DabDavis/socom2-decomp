/* zui_2d.cpp -- the UI's 2D controls (the press beta's asserts name the unit, zui_2d.cpp; SOCOM 1
 * kept it beside zui.h, in gamez/zui -- here with the 2D family until the UI has a directory of its
 * own). Written so far: CHProgressBar::Tick (0x3719E0), CPlainBmp's Tick, Draw and
 * SetMapOffset (OVL1 0x378FC0-0x37925B, 0x37C180) and the two speed setters (0x37B980, 0x37B990),
 * in SOCOM 1's order; in the unit's tail, C2DButton::SetBackgroundAlpha (0x37C2C0) and
 * C3StateButton::Tick (0x37C600), inline virtuals' copies, and the check box's and the teletype's
 * implicit destructors (0x37C2F0, 0x37C620), each reached only through this unit's this-adjusting
 * thunks (0x37D0D0, 0x37D030: MW names them @192@__dt__9CCheckBoxFv and @152@__dt__9CTeleTypeFv).
 * The classes' layouts are zui_2d.h's (the UI lane's), redeclared here with the virtuals these
 * functions need. Defined in reverse of their image order (-inline deferred). */

#include <string.h>
#include <string>

#include "ztwod.h"

/* a UI variable as the plain bitmap reads it: its type (3 a string) and the string */
class CUIVariable {
public:
    bool IsString() const { return m_type == 3; }
    const char *GetString() const { return m_type == 3 ? m_str : 0; }
    /* the value as a number (zrdr.h's _zrdr::Real and Int: 2 a real, 1 an integer) */
    float Real() const
    {
        if (m_type == 2)
            return m_real;
        else if (m_type == 1)
            return (float)m_int;
        else
            return 0.0f;
    }
    int Int() const
    {
        if (m_type == 1)
            return m_int;
        else if (m_type == 2)
            return (int)m_real;
        else
            return 0;
    }

    unsigned char m_type;   // 0x0
    union {
        const char *m_str;   // 0x4
        float m_real;
        int m_int;
    };
};

/* the texture handle's name, first (zdb's) */
namespace zdb {
class CTexHandle {
public:
    const char *m_name;   // 0x0
};
}

zdb::CTexHandle *PrepTexture(const char *name, zdb::CTextureRelocMgr *mgr);

extern "C" void func_00385D90(class UIVarMixin *mixin, CUIVariable *var);

/* a UI variable's value written out as text into a buffer (zui_var.cpp's, 0x3860F0; no row
 * named yet) */
extern "C" char *func_003860F0(CUIVariable *var, char *buf, int size);

/* a UI variable's hook (SOCOM 1's DWARF; the vtable pointer first) */
class UIVarMixin {
public:
    virtual ~UIVarMixin() { func_00385D90(this, 0); }
    /* the variable changed (an override takes a new primary slot after all the tables, so the
     * secondary tables' offsets stand) */
    virtual void UpdateUIVar(CUIVariable *var);

    CUIVariable *m_uivar;   // 0x4
};   // 8 bytes

/* a plain bitmap of the UI's (SOCOM 1's DWARF): it may pulse, and a string variable names its
 * texture */
class CPlainBmp : public C2DBitmap, public UIVarMixin {
public:
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void Tick(float dT);
    virtual void SetMapOffset(int x, int y);

    bool m_pulsing;   // 0x88
    float m_period;   // 0x8c
    float m_curtrans;   // 0x90
} __attribute__((aligned(16)));   // 0xa0 bytes

/* a string a spec configures (SOCOM 1's DWARF: : C2DString, UIVarMixin; 0xC0 bytes, zui_2d.h's
 * layout; its destructor 0x37A640 out of line) */
class CRenderableString : public C2DString, public UIVarMixin {
public:
    virtual ~CRenderableString();
    virtual int GetZAnimCmdType(const char *name);

    unsigned char _a0[0xc0 - 0xa0];
};   // 0xc0 bytes

/* a string whose destructor runs MSL's as a base's (passing 0: SOCOM 1's CSaferStr) */
class CSaferStr : public std::string {
public:
    CSaferStr();
};

/* a teletype: text typed out, its cursor a plain bitmap (SOCOM 1's CTeleType; SOCOM II's 0x1B0
 * bytes: the text a string at 0xC0, the cursor at 0x100 -- zui_2d.h's layout) */
class CTeleType : public CRenderableString {
public:
    virtual ~CTeleType();

    CSaferStr m_text;   // 0xc0
    unsigned char _cc[0x100 - 0xcc];
    CPlainBmp m_cursor;   // 0x100
    int m_1a0;   // 0x1a0
} __attribute__((aligned(16)));   // 0x1b0 bytes

/* a button's caption and states (SOCOM 1's DWARF: : C2D; SOCOM II's 0xC0 bytes, its vtable 29
 * slots: C2D's 26 and three new, DrawBackground the last) */
class C3StateButton : public C2D {
public:
    virtual ~C3StateButton();
    /* slot 12: the caption ticks with the button (0x37C600) */
    virtual void Tick(float dT) { _strCaption.Tick(dT); }
    virtual void SetBackgroundAlpha(float a);
    virtual void Slot27();
    virtual void DrawBackground(const CMatrix &m, zdb::CTextureRelocMgr *mgr);

    char *_szCaption;   // 0x18
    C2DString _strCaption;   // 0x1c
    int _iTextX;   // 0xb4
    int _iTextY;   // 0xb8
    bool _bCaptionCentered;   // 0xbc
};   // 0xc0 bytes

/* a button (SOCOM 1's DWARF: : C3StateButton; SOCOM II's 0x440 bytes with a UIVarMixin at 0xC0,
 * its background a plain bitmap at 0x1A0) */
class C2DButton : public C3StateButton, public UIVarMixin {
public:
    virtual ~C2DButton();
    /* slot 26: the background's alpha, a fraction of 128 (0x37C2C0) */
    virtual void SetBackgroundAlpha(float a) { m_background.SetTrans(128.0f * a); }
    virtual int GetZAnimCmdType(const char *name);
    virtual void UpdateUIVar(CUIVariable *var);

    unsigned char _c8[0x1a0 - 0xc8];
    CPlainBmp m_background;   // 0x1a0
    unsigned char _240[0x440 - 0x240];
} __attribute__((aligned(16)));   // 0x440 bytes

/* a button's action (zui_spec.h's ActionItem, 0x10 bytes: its destructor zui_cleanup.cpp's,
 * 0x3890C0) */
class ActionItem {
public:
    ~ActionItem();

    unsigned char _0[0x10];
};

/* a check box: a button with a check mark (SOCOM 1's CCheckBox; SOCOM II's 0x520 bytes: two
 * actions at 0x454 and 0x464, the check a plain bitmap at 0x480) */
class CCheckBox : public C2DButton {
public:
    virtual ~CCheckBox();

    bool m_checked;   // 0x440
    unsigned char _441[0x454 - 0x441];
    ActionItem m_action0;   // 0x454
    ActionItem m_action1;   // 0x464
    unsigned char _474[0x480 - 0x474];
    CPlainBmp m_check;   // 0x480
} __attribute__((aligned(16)));   // 0x520 bytes

/* a horizontal progress bar: a plain bitmap filled to its variable's value (SOCOM 1's
 * CHProgressBar; SOCOM II's 0xC0 bytes, a string at 0xAC -- its destructor destroys one --
 * zui_2d.h's layout; the names ours) */
class CHProgressBar : public CPlainBmp {
public:
    virtual ~CHProgressBar();
    virtual void Tick(float dT);
    virtual void UpdateUIVar(CUIVariable *var);

    float m_fill;   // 0xa0: 0 to 1
    float _a4;
    float m_time;   // 0xa8: the seconds a full bar takes
    CSaferStr m_str;   // 0xac
} __attribute__((aligned(16)));   // 0xc0 bytes

/* a slider: a button with a knob (SOCOM 1's CSliderBar; SOCOM II's 0x510 bytes, the knob a plain
 * bitmap at 0x460 -- zui_2d.h's layout) */
class CSliderBar : public C2DButton {
public:
    virtual ~CSliderBar();
    virtual void UpdateUIVar(CUIVariable *var);

    float m_value;   // 0x440
    unsigned char _444[0x450 - 0x444];
    float m_center;   // 0x450: the variable's value at the slider's middle
    unsigned char _454[0x460 - 0x454];
    CPlainBmp m_knob;   // 0x460
    int m_intervals;   // 0x500
} __attribute__((aligned(16)));   // 0x510 bytes

/* the pop-up dialog as its command-type lookup sees it (slot 24 of its vtable, 0x36D970; the
 * renderable string's (0x36FC10) and the button's (0x3767C0) are declared with their classes
 * above). Each answers its own commands as the UI's count plus an index, and an empty name
 * with 0 */
class CPopUpDialog : public C2D {
public:
    /* declared, not defined: stand-ins whose implicit destructors would supersede the drafts */
    virtual ~CPopUpDialog();
    virtual int GetZAnimCmdType(const char *name);
};


/* the caption from the button's own variable, as text (SOCOM 1's C2DButton::UpdateUIVar; reached
 * through the thunk 0x37D0E0) */
void C2DButton::UpdateUIVar(CUIVariable *var)
{
    char buf[10500] = {0};

    if (UIVarMixin::m_uivar && UIVarMixin::m_uivar == var) {
        char *caption = func_003860F0(UIVarMixin::m_uivar, buf, 10499);

        _szCaption = caption;
        if (caption) {
            _strCaption.SetString(caption, true);
            _strCaption.UpdatePos();
        }
    }
}

/* the slider set from a real: below the centre value the left half in proportion, above it the
 * right half (SOCOM 1's; reached through the thunk 0x37D080). Read through a const view: the
 * inline accessor then reuses the type byte the test loaded */
void CSliderBar::UpdateUIVar(CUIVariable *var)
{
    const CUIVariable *uivar = var;

    if (uivar->m_type == 2) {
        float v = uivar->Real();

        if (v < m_center)
            m_value = 0.5f * (v / m_center);
        else
            m_value = 0.5f + 0.5f * ((v - m_center) / (1.0f - m_center));
    }
}

/* the bar's time from a number (a real, or an integer made whole), and the bar emptied (SOCOM
 * 1's; reached through the thunk 0x37D0A0) */
void CHProgressBar::UpdateUIVar(CUIVariable *var)
{
    const CUIVariable *uivar = var;

    if (uivar->m_type == 2)
        m_time = uivar->Real();
    else if (uivar->m_type == 1)
        m_time = uivar->Int();
    m_fill = 0.0f;
}

/* slot 12: filled at its rate, to full */
void CHProgressBar::Tick(float dT)
{
    m_fill += dT / m_time;
    if (!(m_fill <= 1.0f))
        m_fill = 1.0f;
}

/* the speeds the counters count and the teletypes type at (SOCOM 1's statics, its symbol table's
 * names: C2DCounter::Tick and CTeleType::Tick read them; the menus' ZAnim commands and
 * CGameMenu::LoadFromDesign set them through the two setters, 0x37B990 and 0x37B980) */
namespace {
float g_fCounterSpeedFactor = 1.0f;
float g_fTtySpeedFactor = 1.0f;
}

void UiSetCounterSpeedFactor(float f)
{
    g_fCounterSpeedFactor = f;
}

void UiSetTtySpeedFactor(float f)
{
    g_fTtySpeedFactor = f;
}

/* slot 2: a string variable naming another texture loads it in the same rectangle first */
void CPlainBmp::Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr)
{
    if (UIVarMixin::m_uivar && UIVarMixin::m_uivar->IsString()) {
        const char *name = UIVarMixin::m_uivar->GetString();

        if (strcasecmp(name, m_pTexHandle ? m_pTexHandle->m_name : "")) {
            zdb::CTexHandle *tex = PrepTexture(name, mgr);

            if (tex)
                Load(m_x, m_y, m_x + m_iWidth, m_y + m_iHeight, tex);
        }
    }
    C2DBitmap::Draw(m, mgr);
}

/* slot 12: a pulsing bitmap's alpha rides a sine, once round per period, between 0 and 128 */
void CPlainBmp::Tick(float dT)
{
    if (m_pulsing) {
        m_curtrans += 6.2831855f * (dT / m_period);
        if (!(m_curtrans <= 6.2831855f))
            m_curtrans -= 6.2831855f;
        if (m_curtrans < 0.0f)
            m_curtrans += 6.2831855f;
        SetTrans(64.0f + 64.0f * sinf(m_curtrans));
    }
}

/* slot 25: moved, keeping its size */
void CPlainBmp::SetMapOffset(int x, int y)
{
    SetPos(x, y, x + m_iWidth, y + m_iHeight);
}

/* the button's commands: Caption and Valve (SOCOM II added Valve: SOCOM 1's answered Caption only) */
int C2DButton::GetZAnimCmdType(const char *name)
{
    if (!name)
        return C2D::GetZAnimCmdType(name) + 3;
    if (!*name)
        return 0;
    if (!strcasecmp("Caption", name))
        return C2D::GetZAnimCmdType(name) + 1;
    if (!strcasecmp("Valve", name))
        return C2D::GetZAnimCmdType(name) + 2;
    return C2D::GetZAnimCmdType(name);
}

/* the renderable string's commands, the button's two */
int CRenderableString::GetZAnimCmdType(const char *name)
{
    if (!name)
        return C2D::GetZAnimCmdType(name) + 3;
    if (!*name)
        return 0;
    if (!strcasecmp("Caption", name))
        return C2D::GetZAnimCmdType(name) + 1;
    if (!strcasecmp("Valve", name))
        return C2D::GetZAnimCmdType(name) + 2;
    return C2D::GetZAnimCmdType(name);
}

/* the dialog's commands: Caption and Background (its count, as the image has it, one) */
int CPopUpDialog::GetZAnimCmdType(const char *name)
{
    if (!name)
        return C2D::GetZAnimCmdType(name) + 1;
    if (!*name)
        return 0;
    if (!strcasecmp("Caption", name))
        return C2D::GetZAnimCmdType(name) + 1;
    if (!strcasecmp("Background", name))
        return C2D::GetZAnimCmdType(name) + 2;
    return C2D::GetZAnimCmdType(name);
}

/* a list box's item's look (SOCOM 1's CListItemTypeImpl, zui_2d.cpp's: its own Tick, IsEnabled,
 * Finish and the like, empty virtuals there; only the slots ListItem forwards to placed, the rest
 * stand-ins) */
class CListItemTypeImpl {
public:
    virtual void _v0();
    virtual void _v1();
    virtual void _v2();
    virtual void _v3();
    virtual void _v4();
    virtual void _v5();
    virtual void _v6();
    virtual void _v7();
    virtual void _v8();
    virtual void _v9();
    virtual void _v10();
    virtual void _v11();
    virtual void _v12();
    virtual void _v13();
    virtual void Tick(float dT);
    virtual bool IsEnabled();
};

/* a list box's item (SOCOM 1's ListItem: its look at 0x28; SOCOM II's vtable 0x407240 has four
 * slots, each forwarding to the look -- slot 0, 0x36B1D0, still open: a tail call followed by a
 * dead `jr ra` that no form tried gives (a plain call, `return` of it, do/while(0), switch, an
 * always-true if); slots 2 and 3 are m2c's drafts) */
class ListItem {
public:
    virtual void Tick(float dT);
    virtual bool IsEnabled();

    unsigned char _4[0x28 - 0x4];
    CListItemTypeImpl *m_impl;   // 0x28
};

/* whether the item is enabled: its look's say, or yes without one (SOCOM 1's
 * CListItemTypeImpl::IsEnabled returns true; the name ours by it) */
bool ListItem::IsEnabled()
{
    if (m_impl)
        return m_impl->IsEnabled();
    return true;
}

/* the teletype's destructor (implicit: the cursor, the text, then the renderable string's;
 * reached only through the thunk 0x37D030) */
CTeleType::~CTeleType()
{
}

/* the slider's destructor (implicit: the knob, then the button's; reached through the thunk
 * 0x37D090) */
CSliderBar::~CSliderBar()
{
}

/* the progress bar's destructor (implicit: the string, then the plain bitmap's, inline; reached
 * through the thunk 0x37D0B0) */
CHProgressBar::~CHProgressBar()
{
}

/* the check box's destructor (its implicit one: the check, the two actions, then the button's;
 * reached only through the thunk 0x37D0D0, the UIVarMixin's table's) */
CCheckBox::~CCheckBox()
{
}
