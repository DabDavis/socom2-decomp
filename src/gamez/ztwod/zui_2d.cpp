/* zui_2d.cpp -- the UI's 2D controls (the press beta's asserts name the unit, zui_2d.cpp; SOCOM 1
 * kept it beside zui.h, in gamez/zui -- here with the 2D family until the UI has a directory of its
 * own). Written so far: CHProgressBar::Tick (0x3719E0), CPlainBmp's Tick, Draw and
 * SetMapOffset (OVL1 0x378FC0-0x37925B, 0x37C180) and the two speed setters (0x37B980, 0x37B990),
 * in SOCOM 1's order; the command lookups (GetZAnimCmdType) of the button, the renderable string,
 * the pop-up, the slider, the radio strip, the counter and the teletype, and the slider's and the
 * radio strip's parsers (ParseZAnimCmd); in the unit's tail, C2DButton::SetBackgroundAlpha (0x37C2C0) and
 * C3StateButton::Tick (0x37C600), inline virtuals' copies, and the check box's and the teletype's
 * implicit destructors (0x37C2F0, 0x37C620), each reached only through this unit's this-adjusting
 * thunks (0x37D0D0, 0x37D030: MW names them @192@__dt__9CCheckBoxFv and @152@__dt__9CTeleTypeFv).
 * The classes' layouts are zui_2d.h's (the UI lane's), redeclared here with the virtuals these
 * functions need. Defined in reverse of their image order (-inline deferred). */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "ztwod.h"

/* a ZAnim command's header (zanim.h's _zanim_cmd_hdr, SOCOM 1's DWARF) */
struct _zanim_cmd_hdr {
    unsigned int data_type : 16;   // 0x0
    unsigned int quad_align : 1;   // 0x0
    unsigned int timeless : 1;   // 0x0
    unsigned int data_size : 14;   // 0x0
};   // 0x4 bytes

/* a 2D object's command (zanim_menu.h's C2DAnimCmdHdr: SOCOM II's 16 bytes, the type the object's
 * GetZAnimCmdType gave at 6, an argument at 8) */
class C2DAnimCmdHdr {
public:
    _zanim_cmd_hdr header;   // 0x0
    unsigned short node_ref_index;   // 0x4
    unsigned char m_type;   // 0x6
    unsigned short m_arg;   // 0x8
    unsigned char _a[0x10 - 0xa];
};   // 0x10 bytes

_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);
bool zrdr_findint(const _zrdr *rdr, const char *tag, int *out, int n);
void *_calloc(unsigned int n, unsigned int size, const char *file, int line);

/* a ZRDR value as the UI variable's base: only the member the variable's setter calls (zrdr.h's
 * _zrdr; empty here, laid at offset 0) */
struct _zrdr {
    void DeleteChildren();
};

/* a UI variable as the plain bitmap reads it: its type (3 a string) and the string */
class CUIVariable : public _zrdr {
public:
    void UpdateDependents();
    /* a real set, the watchers told, unless it already holds it (zui.h's operator=(float)) */
    void SetReal(float f)
    {
        if (m_type != 2 || m_real != f) {
            DeleteChildren();
            m_type = 2;
            m_real = f;
            UpdateDependents();
        }
    }

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
    void SetPulse(float period);

    bool m_pulsing;   // 0x88
    float m_period;   // 0x8c
    float m_curtrans;   // 0x90
} __attribute__((aligned(16)));   // 0xa0 bytes

/* a string a spec configures (SOCOM 1's DWARF: : C2DString, UIVarMixin; 0xC0 bytes, zui_2d.h's
 * layout; its destructor 0x37A640 out of line) */
class CRenderableString : public C2DString, public UIVarMixin {
public:
    virtual ~CRenderableString();
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr);
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    virtual int GetZAnimCmdType(const char *name);
    /* its own slot after the tables (+0xA0; 0x37A630, empty -- SOCOM 1's Update, between Draw and
     * the destructor in both builds), then UpdateUIVar's primary slot (+0xA4, 0x379260) */
    virtual void Update();
    virtual void UpdateUIVar(CUIVariable *var);

    unsigned char _a0[0xc0 - 0xa0];
};   // 0xc0 bytes

/* wrapped text (SOCOM 1's CWrappedText; zui_2d.h's layout, 0x250 bytes), its destructor declared
 * first so that this unit does not take its table */
class CWrappedText : public CRenderableString {
public:
    virtual ~CWrappedText();
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr);
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    virtual int GetZAnimCmdType(const char *name);

    int m_c0;   // 0xc0
    int m_shown;   // 0xc4: the lines the window shows (zui_2d.h's m_line, -1 at first)
    int m_c8;   // 0xc8
    float m_offset;   // 0xcc: the scroll, in the string's spacing
    unsigned char _d0[0x250 - 0xd0];
};   // 0x250 bytes

/* wrapped text scrolling by itself (SOCOM 1's CScrollText; SOCOM II's 0x270 bytes, zui_2d.h's
 * layout: the line to show at 0x258, the scroll position, a float, at 0x260) */
class CScrollText : public CWrappedText {
public:
    virtual ~CScrollText();
    virtual void Tick(float dT);

    int m_250;   // 0x250
    bool m_254;   // 0x254
    int m_line;   // 0x258
    int m_25c;   // 0x25c
    float m_scroll;   // 0x260
};   // 0x270 bytes

/* a string whose destructor runs MSL's as a base's (passing 0: SOCOM 1's CSaferStr) */
class CSaferStr : public std::string {
public:
    CSaferStr();
    /* never null: no text empties it (SOCOM 1's zutil.h; nothing returned, as 0x22DCC0) */
    void operator=(const char *sz)
    {
        if (sz)
            assign(sz);
        else
            erase();
    }
};

/* a teletype: text typed out, its cursor a plain bitmap (SOCOM 1's CTeleType; SOCOM II's 0x1B0
 * bytes: the text a string at 0xC0, the cursor at 0x100 -- zui_2d.h's layout; the counters'
 * names ours, by what SetString clears: SOCOM 1's from 0x3AC) */
class CTeleType : public CRenderableString {
public:
    virtual ~CTeleType();
    virtual int GetZAnimCmdType(const char *name);
    virtual void SetString(const char *s, bool translate);

    CSaferStr m_text;   // 0xc0
    int m_cc;   // 0xcc
    int m_len;   // 0xd0: the text's length
    int m_typed;   // 0xd4: the characters shown
    int m_d8;   // 0xd8
    unsigned char _dc[0xe8 - 0xdc];
    int m_e8;   // 0xe8
    unsigned char _ec[0xf0 - 0xec];
    int m_f0;   // 0xf0
    unsigned char _f4[0x100 - 0xf4];
    CPlainBmp m_cursor;   // 0x100
    int m_1a0;   // 0x1a0
    int m_1a4;   // 0x1a4
    int m_1a8;   // 0x1a8
} __attribute__((aligned(16)));   // 0x1b0 bytes

const char *TranslateString(const char *s);

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
    void SetCaptionStyle(const CPnt3D &color, bool pulse, float time, float min, float max);

    char *_szCaption;   // 0x18
    C2DString _strCaption;   // 0x1c
    int _iTextX;   // 0xb4
    int _iTextY;   // 0xb8
    bool _bCaptionCentered;   // 0xbc
};   // 0xc0 bytes

/* a button's look in one state (0x34 bytes; its destructor 0x36E650, a string at 0 destroyed, is
 * the button's -- a plain stand-in here, the fields by what SetNormal hands on; the names ours) */
struct ButtonState {
    unsigned char _0[0xc];   // 0x0: a string
    zdb::CTexHandle *m_tex;   // 0xc: the background's texture
    float m_fadeMin;   // 0x10
    float m_fadeMax;   // 0x14
    bool m_pulse;   // 0x18: the caption pulses
    float m_period;   // 0x1c: the background's pulse
    float m_fadeTime;   // 0x20
    CPnt3D m_color;   // 0x24: the caption's
    float m_scale;   // 0x30: the caption's
};   // 0x34 bytes

/* a button (SOCOM 1's DWARF: : C3StateButton; SOCOM II's 0x440 bytes with a UIVarMixin at 0xC0,
 * its background a plain bitmap at 0x1A0) */
class C2DButton : public C3StateButton, public UIVarMixin {
public:
    virtual ~C2DButton();
    /* slot 26: the background's alpha, a fraction of 128 (0x37C2C0) */
    virtual void SetBackgroundAlpha(float a) { m_background.SetTrans(128.0f * a); }
    virtual void DrawBackground(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void SetCanHandleInput(bool val);
    virtual void SetNormal();
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr);
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    virtual int GetZAnimCmdType(const char *name);
    virtual void UpdateUIVar(CUIVariable *var);

    /* the four states (ButtonState, 0x34 bytes each: its destructor 0x36E650; the names ours) and
     * the one shown */
    ButtonState m_active;   // 0xc8
    ButtonState m_pressed;   // 0xfc
    ButtonState m_normal;   // 0x130
    ButtonState m_disabled;   // 0x164
    ButtonState *m_state;   // 0x198
    unsigned char _19c[0x1a0 - 0x19c];
    CPlainBmp m_background;   // 0x1a0
    float _fWidth;   // 0x240 (SOCOM 1's names, 0x30 later)
    float _fHeight;   // 0x244
    unsigned char _248[0x24c - 0x248];
    int m_24c;   // 0x24c: zeroed when the normal state is shown
    unsigned char _250[0x440 - 0x250];
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
    /* the check box ignores its variable (0x37C3E0, empty; reached through the thunk 0x37D0C0) */
    virtual void UpdateUIVar(CUIVariable *var) {}

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
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr);
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    virtual int GetZAnimCmdType(const char *name);

    float m_value;   // 0x440
    unsigned char _444[0x450 - 0x444];
    float m_center;   // 0x450: the variable's value at the slider's middle
    unsigned char _454[0x460 - 0x454];
    CPlainBmp m_knob;   // 0x460
    int m_intervals;   // 0x500
} __attribute__((aligned(16)));   // 0x510 bytes

/* a row of radio buttons (SOCOM 1's CRadioStrip; zui_2d.h's layout: 0x450 bytes, the choice at 0x440).
 * Its destructor is declared first so that this unit, which defines only the command lookup, does
 * not take its table */
class CRadioStrip : public C2DButton {
public:
    virtual ~CRadioStrip();
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr);
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    virtual int GetZAnimCmdType(const char *name);

    int m_choice;   // 0x440
} __attribute__((aligned(16)));   // 0x450 bytes

/* the strip's two refreshes after a new choice (SOCOM II's: SOCOM 1 walked the buttons inline;
 * 0x370D10 and 0x370DB0, no rows named yet) */
extern "C" void func_00370D10(CRadioStrip *strip);
extern "C" void func_00370DB0(CRadioStrip *strip);

/* a counting string (SOCOM 1's C2DCounter; zui_2d.h's layout: 0xD8 bytes), its destructor declared
 * first for the same reason */
class C2DCounter : public CRenderableString {
public:
    virtual ~C2DCounter();
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    virtual int GetZAnimCmdType(const char *name);

    float m_start;   // 0xc0
    bool m_counting;   // 0xc4
    float m_end;   // 0xc8
    int m_value;   // 0xcc
    float m_rate;   // 0xd0: what Start puts at 0xC0 (a float: lwc1/swc1)
    int m_last;   // 0xd4
};   // 0xd8 bytes

/* a button showing a spec of its own (SOCOM 1's CHandlerButton; SOCOM II's 0x440 bytes, zui_2d.h's
 * layout). It keeps C2DButton's UpdateUIVar, so its vtable (0x407700) is one of the three that take
 * the button's thunk 0x37D0E0 -- defining its destructor here is what emits that thunk, since
 * C2DButton's own table belongs with ~C2DButton's unit and CCheckBox overrides the slot */
class CHandlerButton : public C2DButton {
public:
    virtual ~CHandlerButton();
} __attribute__((aligned(16)));   // 0x440 bytes

/* the pop-up dialog as its command-type lookup sees it (slot 24 of its vtable, 0x36D970; the
 * renderable string's (0x36FC10) and the button's (0x3767C0) are declared with their classes
 * above). Each answers its own commands as the UI's count plus an index, and an empty name
 * with 0 */
class CPopUpDialog : public C2D {
public:
    /* declared, not defined: stand-ins whose implicit destructors would supersede the drafts */
    virtual ~CPopUpDialog();
    virtual void Tick(float dT);
    virtual int GetZAnimCmdType(const char *name);

    /* the parts its Tick ticks (SOCOM II's two members, through their tables; SOCOM 1 ticked the
     * pointer alone) -- C2D stand-ins, their real classes unknown */
    unsigned char _18[0x32c - 0x18];
    C2D *m_32c;   // 0x32c
    unsigned char _330[0x340 - 0x330];
    C2D m_340;   // 0x340
    unsigned char _358[0x790 - 0x358];
    C2D m_790;   // 0x790
};


/* the text from the string's own variable, written out and formatted in, then brought up to date
 * (SOCOM 1's CRenderableString::UpdateUIVar; reached through the thunk 0x37D150) */
void CRenderableString::UpdateUIVar(CUIVariable *var)
{
    char buf[10500] = {0};

    if (UIVarMixin::m_uivar && UIVarMixin::m_uivar == var) {
        Format(func_003860F0(UIVarMixin::m_uivar, buf, 10499));
        Update();
    }
}

/* the text to type out (looked up first when asked): the string shown emptied, the counters
 * cleared and the text kept, then the string brought up to date -- nothing to type, the length
 * and the count zeroed (SOCOM 1's CTeleType::SetString, sprintf'ing into a buffer; SOCOM II's
 * takes C2DString's translate flag) */
void CTeleType::SetString(const char *s, bool translate)
{
    if (translate)
        s = TranslateString(s);
    C2DString::SetString("", true);
    if (!s || !*s) {
        m_len = 0;
        m_typed = 0;
        return;
    }
    m_typed = 0;
    m_d8 = 0;
    m_e8 = 0;
    m_f0 = 0;
    m_len = strlen(s);
    m_cc = 0;
    m_text = s;
    m_1a8 = 0;
    Update();
}

/* the text scrolled toward its line, 30 a second, snapped once within a step (SOCOM 1's
 * CScrollText::Tick). The step computed after the distance (computed first, the constant 30 takes
 * the scratch register m_scroll's load wants) and the test `!(speed <= |diff|)` (as `|diff| <
 * speed` MW branches on c.lt with the other polarity) */
void CScrollText::Tick(float dT)
{
    float pos = m_line * GetSpacing();
    float diff = pos - m_scroll;
    float speed = 30.0f * dT;

    if (!(speed <= (float)fabs(diff)))
        m_scroll = pos;
    else {
        if (diff < 0.0f)
            speed = -speed;
        m_scroll += speed;
    }
}

/* the background drawn, first placed under a centred caption: centred on the button's own
 * width, as wide as half the button and as tall as it (SOCOM 1's C2DButton::DrawBackground) */
void C2DButton::DrawBackground(const CMatrix &m, zdb::CTextureRelocMgr *mgr)
{
    if (_bCaptionCentered)
        m_background.SetPos(-(m_background.m_iWidth / 2), 0, (int)(_fWidth / 2.0f), (int)_fHeight);
    m_background.Draw(m, mgr);
}

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

/* slot 12: the dialog's two parts ticked through their tables, then the part it points to
 * (SOCOM 1's Tick ticked only that one) */
void CPopUpDialog::Tick(float dT)
{
    m_340.Tick(dT);
    m_790.Tick(dT);
    if (m_32c)
        m_32c->Tick(dT);
}

/* a state applied: the background, the caption's style and the pulse (0x3773F0, unwritten) */
extern "C" void func_003773F0(C2DButton *button, ButtonState *state);
/* the background loaded with a texture, the caption's scale (inlines' out-of-line copies after
 * SetNormal, matched as drafts) */
extern "C" void func_0036E840(C2DButton *button, zdb::CTexHandle *tex);
extern "C" void func_0036E830(C2DButton *button, float scale);

/* slot 21 (C2DButton's table +0x54, as in every button's): the normal state shown, when the
 * button takes input -- the background's texture, the caption's scale and style, the pulse (SOCOM
 * 1's SetNormal; the s1:vtable pass had put SetCanHandleInput's name here, a slot early) */
void C2DButton::SetNormal()
{
    if (CanHandleInput()) {
        m_24c = 0;
        m_state = &m_normal;
        func_0036E840(this, m_normal.m_tex);
        func_0036E830(this, m_normal.m_scale);
        SetCaptionStyle(m_normal.m_color, m_normal.m_pulse, m_normal.m_fadeTime, m_normal.m_fadeMin,
                        m_normal.m_fadeMax);
        m_background.SetPulse(m_normal.m_period);
    }
}

/* slot 19 (C2DButton's table +0x4C, as in every button's): input on or off -- on, the normal
 * state shown unless the button is pressed or active; off, the disabled state (SOCOM 1's
 * SetCanHandleInput, which loaded the disabled bitmap itself) */
void C2DButton::SetCanHandleInput(bool val)
{
    C2D::SetCanHandleInput(val);
    if (val) {
        if (m_state != &m_pressed && m_state != &m_active)
            SetNormal();
    } else
        func_003773F0(this, &m_disabled);
}

/* the caption's colour, its alpha 100, and its pulse: whether, how fast, between what (the name ours;
 * SOCOM 1's SetCanHandleInput set the same fields inline). It and SetPulse are inlines' out-of-line
 * copies in the target, after their first caller SetNormal (0x36E6B0), which calls the four in
 * reverse of their order there: pinned, so SetNormal calls them as the target does */
#pragma auto_inline off
void C3StateButton::SetCaptionStyle(const CPnt3D &color, bool pulse, float time, float min, float max)
{
    _strCaption.SetColor(color.x, color.y, color.z);
    _strCaption.SetTrans(100.0f);
    _strCaption.m_Type = pulse ? STRING_PULSE : STRING_NORMAL;
    _strCaption.m_fadeTime = time;
    _strCaption.m_fadeMax = max;
    _strCaption.m_fadeMin = min;
}

/* pulsing with a period, or (0) steady and opaque (SOCOM 1's SetPulse, out of line at 0x38A810
 * there) */
void CPlainBmp::SetPulse(float period)
{
    if (period == 0.0f) {
        SetTrans(128.0f);
        m_pulsing = false;
    } else {
        m_pulsing = true;
        m_period = period;
    }
}
#pragma auto_inline reset

/* SETVALUE's value as a command (SOCOM 1's; SOCOM II's record is 16 bytes and keeps the value as a
 * halfword), anything else the button's */
C2DAnimCmdHdr *CSliderBar::ParseZAnimCmd(_zrdr *rdr)
{
    C2DAnimCmdHdr *cmd;

    if (zrdr_findtag(rdr, "SETVALUE")) {
        int value = 0;

        zrdr_findint(rdr, "SETVALUE", &value, 1);
        cmd = (C2DAnimCmdHdr *)_calloc(1, sizeof(C2DAnimCmdHdr), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(C2DAnimCmdHdr);
        cmd->m_type = GetZAnimCmdType("SETVALUE");
        cmd->m_arg = value;
    } else
        cmd = C2DButton::ParseZAnimCmd(rdr);
    return cmd;
}

/* the counter's commands after the renderable string's two: Start and Finish */
int C2DCounter::GetZAnimCmdType(const char *name)
{
    if (!name)
        return CRenderableString::GetZAnimCmdType(0) + 3;
    if (!*name)
        return 0;
    if (!strcasecmp("Start", name))
        return CRenderableString::GetZAnimCmdType(0) + 1;
    if (!strcasecmp("Finish", name))
        return CRenderableString::GetZAnimCmdType(0) + 2;
    return CRenderableString::GetZAnimCmdType(name);
}

/* SELECTION: a choice within the strip's buttons taken and shown, the command done at once;
 * anything else the button's (SOCOM 1's; its choice a byte at 7, SOCOM II's the halfword at 8) */
bool CRadioStrip::TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done)
{
    C2DAnimCmdHdr *cmd = (C2DAnimCmdHdr *)hdr;
    bool handled;

    switch (cmd->m_type - C2DButton::GetZAnimCmdType(0)) {
    case 1: {
        int choice = cmd->m_arg;

        if (choice >= 0 && choice <= size() - 1) {
            m_choice = choice;
            func_00370D10(this);
            func_00370DB0(this);
        }
        *done = true;
        handled = true;
        break;
    }
    default:
        handled = C2DButton::TickZAnimCmd(hdr, dT, done);
        break;
    }
    return handled;
}

/* SETVALUE: the value, a percentage, taken and handed to the slider's variable, the command done;
 * anything else the button's (the halfword converted as an unsigned int, the target's sequence;
 * divided in place, `v /= 100`, so the quotient keeps the dividend's register) */
bool CSliderBar::TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done)
{
    C2DAnimCmdHdr *cmd = (C2DAnimCmdHdr *)hdr;

    switch (cmd->m_type - C2DButton::GetZAnimCmdType(0)) {
    case 1: {
        CUIVariable *var;
        float v = (unsigned int)cmd->m_arg;

        v /= 100.0f;
        m_value = v;
        var = UIVarMixin::m_uivar;
        if (var)
            var->SetReal(m_value);
        *done = true;
        return true;
    }
    default:
        return C2DButton::TickZAnimCmd(hdr, dT, done);
    }
}

/* Start: the count's start taken from 0xD0; Finish: the count stopped at its value, shown and the
 * string brought up to date, the command done; anything else the renderable string's (SOCOM 1's
 * shape, its slots moved). Each case returns true straight: Start's true is the 1 the compare
 * chain left in v0, Finish's `li v0,1` serves *done too; the base's result goes back unmasked */
bool C2DCounter::TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done)
{
    C2DAnimCmdHdr *cmd = (C2DAnimCmdHdr *)hdr;

    switch (cmd->m_type - CRenderableString::GetZAnimCmdType(0)) {
    case 1:
        m_start = m_rate;
        return true;
    case 2: {
        m_counting = false;
        m_end = 0.0f;
        m_last = m_value;

        char buf[16] = {0};

        sprintf(buf, "%d", m_value);
        SetString(buf, true);
        Update();
        *done = true;
        return true;
    }
    default:
        return CRenderableString::TickZAnimCmd(hdr, dT, done);
    }
}

/* Scroll_Up a line while above the top, Scroll_Down a line while the lines below the window
 * last; anything else the renderable string's (SOCOM II's; the compare chain tests the cases in
 * reverse, the bool through a local -- the base's result masked, true assigned) */
bool CWrappedText::TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done)
{
    C2DAnimCmdHdr *cmd = (C2DAnimCmdHdr *)hdr;
    bool handled;

    switch (cmd->m_type - CRenderableString::GetZAnimCmdType(0)) {
    case 1:
        if (m_offset > 0.0f)
            m_offset -= GetSpacing();
        handled = true;
        break;
    case 2: {
        int lines = m_numLines;

        if (m_offset < lines * GetSpacing() - m_shown)
            m_offset += GetSpacing();
        handled = true;
        break;
    }
    default:
        handled = CRenderableString::TickZAnimCmd(hdr, dT, done);
        break;
    }
    return handled;
}

/* SCROLL_UP and SCROLL_DOWN as commands of their own (no argument), anything else the renderable
 * string's (the wrapped text's: slot 24 of CWrappedText's table 0x4079E0, which SOCOM 1's vtable
 * names gave CRenderableString -- its own table holds 0x36FE40 there, the base called here) */
C2DAnimCmdHdr *CWrappedText::ParseZAnimCmd(_zrdr *rdr)
{
    C2DAnimCmdHdr *cmd;

    if (zrdr_findtag(rdr, "SCROLL_UP")) {
        cmd = (C2DAnimCmdHdr *)_calloc(1, sizeof(C2DAnimCmdHdr), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(C2DAnimCmdHdr);
        cmd->m_type = GetZAnimCmdType("SCROLL_UP");
    } else if (zrdr_findtag(rdr, "SCROLL_DOWN")) {
        cmd = (C2DAnimCmdHdr *)_calloc(1, sizeof(C2DAnimCmdHdr), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(C2DAnimCmdHdr);
        cmd->m_type = GetZAnimCmdType("SCROLL_DOWN");
    } else
        cmd = CRenderableString::ParseZAnimCmd(rdr);
    return cmd;
}

/* the wrapped text's commands after the renderable string's two: Scroll_Up and Scroll_Down */
int CWrappedText::GetZAnimCmdType(const char *name)
{
    if (!name)
        return CRenderableString::GetZAnimCmdType(0) + 3;
    if (!*name)
        return 0;
    if (!strcasecmp("Scroll_Up", name))
        return CRenderableString::GetZAnimCmdType(0) + 1;
    if (!strcasecmp("Scroll_Down", name))
        return CRenderableString::GetZAnimCmdType(0) + 2;
    return CRenderableString::GetZAnimCmdType(name);
}

/* the teletype's commands after the renderable string's two: Continue, scroll_up, scroll_down and
 * finish (SOCOM 1's slot; each base's count taken with a null name, the base's lookup inlined) */
int CTeleType::GetZAnimCmdType(const char *name)
{
    if (!name)
        return CRenderableString::GetZAnimCmdType(0) + 5;
    if (!*name)
        return 0;
    if (!strcasecmp("Continue", name))
        return CRenderableString::GetZAnimCmdType(0) + 1;
    if (!strcasecmp("scroll_up", name))
        return CRenderableString::GetZAnimCmdType(0) + 2;
    if (!strcasecmp("scroll_down", name))
        return CRenderableString::GetZAnimCmdType(0) + 3;
    if (!strcasecmp("finish", name))
        return CRenderableString::GetZAnimCmdType(0) + 4;
    return CRenderableString::GetZAnimCmdType(name);
}

/* the slider's command after the button's two: SetValue (its count, as SOCOM 1's, two) */
int CSliderBar::GetZAnimCmdType(const char *name)
{
    if (!name)
        return C2DButton::GetZAnimCmdType(0) + 2;
    if (!*name)
        return 0;
    if (!strcasecmp("SetValue", name))
        return C2DButton::GetZAnimCmdType(0) + 1;
    return C2DButton::GetZAnimCmdType(name);
}

/* SELECTION's INDEX as a command (the slider's twin, INDEX read inside the SELECTION tag),
 * anything else the button's */
C2DAnimCmdHdr *CRadioStrip::ParseZAnimCmd(_zrdr *rdr)
{
    C2DAnimCmdHdr *cmd;
    int index = 0;
    _zrdr *sel = zrdr_findtag(rdr, "SELECTION");

    if (sel) {
        zrdr_findint(sel, "INDEX", &index, 1);
        cmd = (C2DAnimCmdHdr *)_calloc(1, sizeof(C2DAnimCmdHdr), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(C2DAnimCmdHdr);
        cmd->m_type = GetZAnimCmdType("SELECTION");
        cmd->m_arg = index;
    } else
        cmd = C2DButton::ParseZAnimCmd(rdr);
    return cmd;
}

/* the radio strip's command after the button's two: Selection (its count two) */
int CRadioStrip::GetZAnimCmdType(const char *name)
{
    if (!name)
        return C2DButton::GetZAnimCmdType(0) + 2;
    if (!*name)
        return 0;
    if (!strcasecmp("Selection", name))
        return C2DButton::GetZAnimCmdType(0) + 1;
    return C2DButton::GetZAnimCmdType(name);
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

/* an .rdr built in memory (zrdr.h's CRdrEditor: only the three node adders the list items call) */
class CRdrEditor {
public:
    bool addintnode(int i);
    bool addrealnode(float f);
    bool addstringnode(const char *s);
};

/* strtod (MSL's; include/stdlib.h has no declaration of it) */
extern "C" double strtod(const char *s, char **end);

/* a list box's item's look (SOCOM 1's CListItemTypeImpl, zui_2d.cpp's: its own Tick, IsEnabled,
 * Finish and the like, empty virtuals there; only the slots ListItem forwards to placed, the rest
 * stand-ins. SOCOM II's table has 18 slots to SOCOM 1's 13: slot 11 is the Tick, slot 14 the
 * SOCOM II's item writer) */
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
    virtual void AddToRdr(CRdrEditor *ed);
    virtual bool IsEnabled();
};

/* a list box's item (SOCOM 1's ListItem: its look at 0x28; SOCOM II's vtable 0x407240 has four
 * slots, each forwarding to the look -- slot 0, 0x36B1D0, the item writer: the `jr ra` after it,
 * which no form of it gave, is CListItemTypeImpl's empty writer, a function of its own at
 * 0x36B1F0 (its vtable's slot 14), the map's row split; slots 2 and 3 are m2c's drafts) */
class ListItem {
public:
    virtual void AddToRdr(CRdrEditor *ed);
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

/* the item written into an .rdr: nothing, for a look without text (SOCOM II's; the name ours) */
void CListItemTypeImpl::AddToRdr(CRdrEditor *ed)
{
}

/* the item written into an .rdr: its look's say (tail-called; SOCOM II's, the name ours) */
void ListItem::AddToRdr(CRdrEditor *ed)
{
    m_impl->AddToRdr(ed);
}

namespace {
/* a list item's text look (SOCOM 1's CListItemStringTypeImpl, the unit's anonymous namespace; its
 * string a pointer at 4 in SOCOM II, an embedded C2DString in SOCOM 1; the wrapped-text look
 * inherits its Tick and writer) */
class CListItemStringTypeImpl : public CListItemTypeImpl {
public:
    virtual void Tick(float dT);
    virtual void AddToRdr(CRdrEditor *ed);

    C2DString *m_str;   // 0x4
};
}

/* the text written as a number when it is all one: an int node when strtol reads all of it, else
 * a real node when strtod does, else a string node (SOCOM II's; the name ours) */
void CListItemStringTypeImpl::AddToRdr(CRdrEditor *ed)
{
    int len;
    const char *s = m_str->m_string + m_str->m_firstCharPos;

    len = strlen(s);
    char *end;
    long i = strtol(s, &end, 10);

    if (len == end - s) {
        ed->addintnode(i);
        return;
    }

    float f = strtod(s, &end);

    if (len == end - s) {
        ed->addrealnode(f);
        return;
    }
    ed->addstringnode(m_str->m_string + m_str->m_firstCharPos);
}

/* the text's tick (SOCOM 1's CListItemStringTypeImpl::Tick) */
void CListItemStringTypeImpl::Tick(float dT)
{
    m_str->Tick(dT);
}

/* the teletype's destructor (implicit: the cursor, the text, then the renderable string's;
 * reached only through the thunk 0x37D030) */
CTeleType::~CTeleType()
{
}

/* the handler button's destructor (implicit: the button's; reached through the thunk 0x37D050) */
CHandlerButton::~CHandlerButton()
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
