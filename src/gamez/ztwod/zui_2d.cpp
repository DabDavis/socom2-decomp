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
 * Round 22 (the HUD lane): C2DButton::DrawBackground, CScrollText::Tick, CTeleType's SetString and
 * TickZAnimCmd, CRenderableString::UpdateUIVar (and its thunk 0x37D150, emitted with CTeleType's
 * table), CWrappedText's parser, lookup and command tick, the radio strip's, the counter's and the
 * slider's command ticks, the counter's parser and Tick. MW's slots for a class with a secondary
 * base: the primary base's, then the secondary's part, then the class's own new virtuals and an
 * overrider's primary slot (CRenderableString: Update +0xA0, UpdateUIVar +0xA4).
 * Round 26: the unit's head -- CSecureText::SetString (0x36AFF0) and C2DClock::UpdateUIVar
 * (0x36B0A0), defined last; the inline copies before them (0x36AFA0-0x36AFEF, CSecureText's table's)
 * come with ~CSecureText (zui.cpp's 0x36AF30) and stay drafts.
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
void _free(void *p, const char *file, int line);

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


/* a UI variable's value written out as text into a buffer (zui_var.cpp's, 0x3860F0; no row
 * named yet) */
extern "C" char *func_003860F0(CUIVariable *var, char *buf, int size);

/* a UI variable's hook (SOCOM 1's DWARF; the vtable pointer first) */
class UIVarMixin {
public:
    UIVarMixin() { m_uivar = 0; }
    virtual ~UIVarMixin() { DetachUIVar(0); }
    void DetachUIVar(CUIVariable *var);
    /* the variable changed (an override takes a new primary slot after all the tables, so the
     * secondary tables' offsets stand) */
    virtual void UpdateUIVar(CUIVariable *var);

    CUIVariable *m_uivar;   // 0x4
};   // 8 bytes

/* a plain bitmap of the UI's (SOCOM 1's DWARF): it may pulse, and a string variable names its
 * texture */
class CPlainBmp : public C2DBitmap, public UIVarMixin {
public:
    /* grey, opaque, see-through, deaf to the pad, still (zui_2d.h's; out of line 0x20CD50) */
    CPlainBmp()
    {
        SetColor(128.0f, 128.0f, 128.0f);
        SetTrans(128.0f);
        SetIsTrans(true);
        SetCanHandleInput(false);
        m_pulsing = false;
        m_period = 1.0f;
    }
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
    CRenderableString();
    virtual ~CRenderableString();
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr);
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    virtual int GetZAnimCmdType(const char *name);
    /* its own slot after the tables (+0xA0; 0x37A630, empty -- SOCOM 1's Update, between Draw and
     * the destructor in both builds), then UpdateUIVar's primary slot (+0xA4, 0x379260) */
    virtual void Update();
    virtual void UpdateUIVar(CUIVariable *var);
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);

    C2DFont *m_alternateFont;   // 0xa0
    CPnt3D m_offset;   // 0xa4
    CPnt3D m_Color;   // 0xb0
    float m_height;   // 0xbc: a height (the wrapped list item's look sets it, as a float)
};   // 0xc0 bytes

/* wrapped text (SOCOM 1's CWrappedText; zui_2d.h's layout, 0x250 bytes), its destructor declared
 * first so that this unit does not take its table */
class CWrappedText : public CRenderableString {
public:
    virtual ~CWrappedText();
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr);
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    virtual int GetZAnimCmdType(const char *name);

    /* the width it wraps to (out of line in the list item's look, 0x36B320) */
    void SetWidth(float w) { m_width = w; }

    float m_width;   // 0xc0: the width it wraps to (the list item's look sets 300)
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

/* the font (SOCOM II's C2DFont: c2dfont.h, after the CSaferStr it holds) -- the renderable string
 * owns its alternate font */
#include "c2dfont.h"

/* a teletype: text typed out, its cursor a plain bitmap (SOCOM 1's CTeleType; SOCOM II's 0x1B0
 * bytes: the text a string at 0xC0, the cursor at 0x100 -- zui_2d.h's layout; the counters'
 * names ours, by what SetString clears: SOCOM 1's from 0x3AC) */
class CTeleType : public CRenderableString {
public:
    virtual ~CTeleType();
    virtual int GetZAnimCmdType(const char *name);
    virtual void Tick(float dT);
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    virtual void SetString(const char *s, bool translate);

    CSaferStr m_text;   // 0xc0
    int m_cc;   // 0xcc
    int m_len;   // 0xd0: the text's length
    int m_typed;   // 0xd4: the characters shown
    int m_d8;   // 0xd8
    unsigned char _dc[0xe4 - 0xdc];
    int m_shown;   // 0xe4: the lines the window shows
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
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
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

/* a button's action (zui_spec.h's ActionItem, 0x10 bytes: its destructor zui_cleanup.cpp's,
 * 0x3890C0; the type and the key state byte enums) */
class ActionItem {
public:
    /* zui_spec.h's: a button action on the falling edge of no button (the list items build theirs) */
    ActionItem() : Type(0), Animation(0), Argument(0), State(1), Event(16) {}
    ~ActionItem();
    void Run() const;

    unsigned char Type;   // 0x0: ACTION_TYPE
    char *Animation;   // 0x4: the animation it starts
    char *Argument;   // 0x8: handed to it, if any
    unsigned char State;   // 0xc: KEY_STATE
    unsigned char Event;   // 0xd: the pad button or the event (a union in zui_spec.h)
};

/* the animations (zanim.h's CZAnimMain and CZAnim: only what an action calls; ZAnim OVL1 bss
 * 0x414BB0) */
class CZAnim {
public:
    CZAnim *ActivateArg(int type, ...);
    void Activate(CZAnim *parent);
    /* the animation's name table: a name added, its index; a name by index (the button's
     * commands keep their caption or valve so) */
    int AddName(const char *name);
    const char *GetName(int index);
};

class CZAnimMain {
public:
    CZAnim *GetAnim(const char *name, const char *set);

    unsigned char _0[0x38];
    CZAnim *m_CurAnim;   // 0x38: the animation being parsed or run (zanim.h's)
};

/* a valve (zanim.h's CValve: only the button's VALVE command's reach -- the valve by name, made if
 * new, and its value, 16 bits at 4) */
enum VALVE_TYPE { VALVE_ANY = 0, VALVE_PERM = 1, VALVE_TEMP = 2, VALVE_PERSIST = 3 };

class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
    unsigned int m_type : 8;
    unsigned int m_flags : 8;
};

const char *zrdr_findstring(const _zrdr *rdr, const char *tag);

extern CZAnimMain ZAnim;

/* a button (SOCOM 1's DWARF: : C3StateButton; SOCOM II's 0x440 bytes with a UIVarMixin at 0xC0,
 * its background a plain bitmap at 0x1A0) */
class C2DButton : public C3StateButton, public UIVarMixin {
public:
    virtual ~C2DButton();
    /* slot 26: the background's alpha, a fraction of 128 (0x37C2C0) */
    virtual void SetBackgroundAlpha(float a) { m_background.SetTrans(128.0f * a); }
    virtual void DrawBackground(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void SetCanHandleInput(bool val);
    void ShowState(ButtonState *state);
    virtual void Tick(float dT);
    virtual void SetActive();
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
    float m_pressTime;   // 0x24c: the press's time left, ticked down; zeroed when the normal state is shown
    ActionItem m_actions[30];   // 0x250: the button's actions (SOCOM 1's _actions)
    int m_numActions;   // 0x430
    unsigned char _434[0x440 - 0x434];
} __attribute__((aligned(16)));   // 0x440 bytes

/* a check box: a button with a check mark (SOCOM 1's CCheckBox; SOCOM II's 0x520 bytes: two
 * actions at 0x454 and 0x464, the check a plain bitmap at 0x480) */
class CCheckBox : public C2DButton {
public:
    virtual ~CCheckBox();
    /* the check box ignores its variable (0x37C3E0, empty; reached through the thunk 0x37D0C0) */
    virtual void UpdateUIVar(CUIVariable *var) {}
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual bool HandlePad(const CPad *pad, int port);

    bool m_checked;   // 0x440
    /* the check's place against the button's and its size (the names ours) */
    int m_checkX;   // 0x444
    int m_checkY;   // 0x448
    int m_checkW;   // 0x44c
    int m_checkH;   // 0x450
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

    /* the strip's two refreshes after a new choice (SOCOM II's: SOCOM 1 walked the buttons inline;
     * 0x370D10, the name ours, and 0x370DB0, no row named yet) */
    void ShowChoice();

    int m_choice;   // 0x440
} __attribute__((aligned(16)));   // 0x450 bytes

extern "C" void func_00370DB0(CRadioStrip *strip);

/* a counting string (SOCOM 1's C2DCounter; zui_2d.h's layout: 0xD8 bytes), its destructor declared
 * first for the same reason */
class C2DCounter : public CRenderableString {
public:
    virtual ~C2DCounter();
    virtual void Tick(float dT);
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr);
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    virtual int GetZAnimCmdType(const char *name);

    float m_start;   // 0xc0
    bool m_counting;   // 0xc4
    float m_end;   // 0xc8
    int m_value;   // 0xcc
    float m_time;   // 0xd0: the clock, advanced by Tick; Start takes it as the start
    int m_last;   // 0xd4
};   // 0xd8 bytes

/* a clock (SOCOM 1's C2DClock; zui_2d.h's layout: 0xC0 bytes), its destructor declared first so
 * that this unit does not take its table (0x407190, its mixin's 0x407220) */
class C2DClock : public CRenderableString {
public:
    virtual ~C2DClock();
    virtual void UpdateUIVar(CUIVariable *var);
};

/* a password's stars (SOCOM II's CSecureText; zui_2d.h's layout: 0xC0 bytes); its table 0x406FA0
 * goes with its destructor, zui.cpp's 0x36AF30 */
class CSecureText : public CRenderableString {
public:
    virtual ~CSecureText();
    virtual void SetString(const char *s, bool translate);
};

/* a UTF-8 character's length in bytes (zutil's; the string not advanced) */
int UTF8CharLen(const char *&s);

/* whether a text pointer is null, and whether it has a character left (the secure text's walk;
 * the names ours) */
inline bool SecureNull(const char *s) { return !s; }
inline bool SecureMore(const char *s) { return !SecureNull(s) && *s; }

/* a button showing a spec of its own (SOCOM 1's CHandlerButton; SOCOM II's 0x440 bytes, zui_2d.h's
 * layout). It keeps C2DButton's UpdateUIVar, so its vtable (0x407700) is one of the three that take
 * the button's thunk 0x37D0E0 -- defining its destructor here is what emits that thunk, since
 * C2DButton's own table belongs with ~C2DButton's unit and CCheckBox overrides the slot */
class CHandlerButton : public C2DButton {
public:
    virtual ~CHandlerButton();
    virtual bool HandlePad(const CPad *pad, int port);
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

/* the teletype's typing noise (SOCOM II's, the name ours: 1 in the image; CTeleType::Tick reads it,
 * TickZAnimCmd's finish clears it around each step it types out at once) */
namespace {
bool g_bTtyNoise = true;
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

/* the background loaded with a texture, the caption's scale (inlines' out-of-line copies after
 * SetNormal, matched as drafts) */
extern "C" void func_0036E840(C2DButton *button, zdb::CTexHandle *tex);
extern "C" void func_0036E830(C2DButton *button, float scale);

/* slot 18: the state at 0xFC shown, when the button takes input, and every action of type 1 on
 * event 2 run (SOCOM 1's C2DButton::SetActive; the activation written out -- through the check box's
 * ActionItem::Run its copy of the item takes a register more) */
void C2DButton::SetActive()
{
    if (CanHandleInput()) {
        m_state = &m_pressed;
        func_0036E840(this, m_pressed.m_tex);
        func_0036E830(this, m_pressed.m_scale);
        SetCaptionStyle(m_pressed.m_color, m_pressed.m_pulse, m_pressed.m_fadeTime, m_pressed.m_fadeMin,
                        m_pressed.m_fadeMax);
        m_background.SetPulse(m_pressed.m_period);
        for (int i = 0; i < m_numActions; i++)
            if (m_actions[i].Type == 1 && m_actions[i].Event == 2) {
                CZAnim *anim = ZAnim.GetAnim(m_actions[i].Animation, 0);

                if (anim) {
                    if (m_actions[i].Argument)
                        anim->ActivateArg(1, 8, m_actions[i].Argument, 0);
                    else
                        anim->Activate(0);
                }
            }
    }
}

/* slot 12: the caption and the background ticked, and a press timed out -- the state at 0xFC shown
 * when the button takes input, the disabled one when not (SOCOM 1's C2DButton::Tick) */
void C2DButton::Tick(float dT)
{
    _strCaption.Tick(dT);
    m_background.Tick(dT);
    if (m_pressTime > 0.0f) {
        if (m_pressTime > dT)
            m_pressTime -= dT;
        else {
            m_pressTime = 0.0f;
            if (CanHandleInput()) {
                m_state = &m_pressed;
                func_0036E840(this, m_pressed.m_tex);
                func_0036E830(this, m_pressed.m_scale);
                SetCaptionStyle(m_pressed.m_color, m_pressed.m_pulse, m_pressed.m_fadeTime,
                                m_pressed.m_fadeMin, m_pressed.m_fadeMax);
                m_background.SetPulse(m_pressed.m_period);
            } else {
                m_state = &m_disabled;
                func_0036E840(this, m_disabled.m_tex);
                func_0036E830(this, m_disabled.m_scale);
                SetCaptionStyle(m_disabled.m_color, m_disabled.m_pulse, m_disabled.m_fadeTime,
                                m_disabled.m_fadeMin, m_disabled.m_fadeMax);
                m_background.SetPulse(m_disabled.m_period);
            }
        }
    }
}

/* slot 21 (C2DButton's table +0x54, as in every button's): the normal state shown, when the
 * button takes input -- the background's texture, the caption's scale and style, the pulse (SOCOM
 * 1's SetNormal; the s1:vtable pass had put SetCanHandleInput's name here, a slot early) */
void C2DButton::SetNormal()
{
    if (CanHandleInput()) {
        m_pressTime = 0;
        m_state = &m_normal;
        func_0036E840(this, m_normal.m_tex);
        func_0036E830(this, m_normal.m_scale);
        SetCaptionStyle(m_normal.m_color, m_normal.m_pulse, m_normal.m_fadeTime, m_normal.m_fadeMin,
                        m_normal.m_fadeMax);
        m_background.SetPulse(m_normal.m_period);
    }
}

/* the caption's style as SetCaptionStyle sets it, for ShowState, which inlines what SetNormal
 * calls out of line (the four helpers are pinned for SetNormal; the name ours) */
static inline void ApplyCaptionStyle(C2DString &caption, const CPnt3D &color, bool pulse, float time,
                                     float min, float max)
{
    caption.SetColor(color.x, color.y, color.z);
    caption.SetTrans(100.0f);
    caption.m_Type = pulse ? STRING_PULSE : STRING_NORMAL;
    caption.m_fadeTime = time;
    caption.m_fadeMax = max;
    caption.m_fadeMin = min;
}

/* the background's pulse as CPlainBmp::SetPulse sets it (its period a by-value parameter: loaded
 * once), for ShowState (the name ours) */
static inline void ApplyPulse(CPlainBmp &bmp, float period)
{
    if (period == 0.0f) {
        bmp.SetTrans(128.0f);
        bmp.m_pulsing = false;
    } else {
        bmp.m_pulsing = true;
        bmp.m_period = period;
    }
}

/* a state shown and made the current one: the background's texture, the caption's scale and
 * style, the background's pulse -- SetNormal's body with its four helpers inlined (0x3773F0; the
 * disabled state through SetCanHandleInput, which calls it out of line: pinned; the name ours) */
#pragma auto_inline off
void C2DButton::ShowState(ButtonState *state)
{
    m_state = state;
    m_background.Load(0.0f, 0.0f, _fWidth, _fHeight, state->m_tex);
    _strCaption.m_scale = state->m_scale;
    ApplyCaptionStyle(_strCaption, state->m_color, state->m_pulse, state->m_fadeTime,
                      state->m_fadeMin, state->m_fadeMax);
    ApplyPulse(m_background, state->m_period);
}
#pragma auto_inline reset

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
        ShowState(&m_disabled);
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

/* CAPTION: the caption taken from the animation's names (SOCOM 1's); VALVE (SOCOM II's): the
 * valve's value printed as the caption, "0" when the name is empty -- into a buffer on the stack,
 * which the caption pointer is left holding, as the target does. Both done at once; anything else
 * not the button's */
bool C2DButton::TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done)
{
    C2DAnimCmdHdr *cmd = (C2DAnimCmdHdr *)hdr;
    CZAnim *anim = ZAnim.m_CurAnim;

    switch (cmd->m_type - C2D::GetZAnimCmdType(0)) {
    case 1: {
        const char *s = anim->GetName(cmd->m_arg);

        _szCaption = (char *)s;
        if (s) {
            _strCaption.SetString(s, true);
            _strCaption.UpdatePos();
        }
        return true;
    }
    case 2: {
        char buf[64] = "0";
        const char *name = anim->GetName(cmd->m_arg);

        if (name && *name)
            sprintf(buf, "%d", CValve::Create(name, VALVE_PERM)->m_value);
        _szCaption = buf;
        _strCaption.SetString(buf, true);
        _strCaption.UpdatePos();
        return true;
    }
    default:
        return false;
    }
}

/* CAPTION or VALVE as a command, the name added to the animation's and its index kept (SOCOM 1's
 * took CAPTION alone); anything else none -- the base's parser is not asked */
C2DAnimCmdHdr *C2DButton::ParseZAnimCmd(_zrdr *rdr)
{
    C2DAnimCmdHdr *cmd;
    const char *caption = zrdr_findstring(rdr, "CAPTION");
    const char *valve = zrdr_findstring(rdr, "VALVE");

    if (caption) {
        cmd = (C2DAnimCmdHdr *)_calloc(1, sizeof(C2DAnimCmdHdr), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(C2DAnimCmdHdr);
        cmd->m_type = GetZAnimCmdType("CAPTION");
        cmd->m_arg = ZAnim.m_CurAnim->AddName(caption);
    } else if (valve) {
        cmd = (C2DAnimCmdHdr *)_calloc(1, sizeof(C2DAnimCmdHdr), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(C2DAnimCmdHdr);
        cmd->m_type = GetZAnimCmdType("VALVE");
        cmd->m_arg = ZAnim.m_CurAnim->AddName(valve);
    } else
        cmd = 0;
    return cmd;
}

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
            ShowChoice();
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

/* Continue (1) restarts the pause; scroll up/down (2, 3) move the window a line once all is typed;
 * finish (4) types the rest at once, quietly, and goes on to the renderable string's commands as
 * any other does (it falls through). Only Continue and the base report a command handled */
bool CTeleType::TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done)
{
    C2DAnimCmdHdr *cmd = (C2DAnimCmdHdr *)hdr;
    bool handled = false;

    switch (cmd->m_type - CRenderableString::GetZAnimCmdType(0)) {
    case 1:
        m_1a0 = 0;
        handled = true;
        break;
    case 2:
        if (!(m_typed < m_len)) {
            int lines = m_numLines;

            if ((lines - m_1a8) * GetSpacing() > m_shown)
                m_1a8++;
        }
        break;
    case 3:
        if (!(m_typed < m_len) && m_1a8 > 0)
            m_1a8--;
        break;
    case 4:
        while (m_typed < m_len) {
            g_bTtyNoise = false;
            Tick(100000.0f);
            g_bTtyNoise = true;
        }
    default:
        handled = CRenderableString::TickZAnimCmd(hdr, dT, done);
        break;
    }
    return handled;
}

/* START and FINISH as commands of their own, their argument zeroed; anything else the renderable
 * string's (SOCOM 1's two, its slots moved) */
C2DAnimCmdHdr *C2DCounter::ParseZAnimCmd(_zrdr *rdr)
{
    C2DAnimCmdHdr *cmd;

    if (zrdr_findtag(rdr, "START")) {
        cmd = (C2DAnimCmdHdr *)_calloc(1, sizeof(C2DAnimCmdHdr), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(C2DAnimCmdHdr);
        cmd->m_type = GetZAnimCmdType("START");
        cmd->m_arg = 0;
    } else if (zrdr_findtag(rdr, "FINISH")) {
        cmd = (C2DAnimCmdHdr *)_calloc(1, sizeof(C2DAnimCmdHdr), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(C2DAnimCmdHdr);
        cmd->m_type = GetZAnimCmdType("FINISH");
        cmd->m_arg = 0;
    } else
        cmd = CRenderableString::ParseZAnimCmd(rdr);
    return cmd;
}

void PlayCountSound();

/* the clock run on to the count's end at the counter speed; past the start the value shown is
 * the target in proportion to the time, the whole once the end is reached -- each new one shown
 * with the count's tick (SOCOM 1's C2DCounter::Tick, its slots moved) */
void C2DCounter::Tick(float dT)
{
    float end = m_counting ? m_start + m_end : m_end;

    if (m_time <= end) {
        int value;

        m_time += dT * g_fCounterSpeedFactor;
        if (m_time < m_start)
            return;
        if (m_time < end) {
            float span = m_counting ? m_end : end - m_start;

            value = (int)(m_value * ((m_time - m_start) / span));
        } else
            value = m_value;
        if (m_last != value) {
            m_last = value;

            char buf[16] = {0};

            sprintf(buf, "%d", value);
            SetString(buf, true);
            Update();
            PlayCountSound();
        }
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
        m_start = m_time;
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

/* the map control (SOCOM 1's CUIMapCtrl, a C2D: its destructor and command lookup here -- the
 * destructor its key function, so this unit takes its table 0x407260) */
class CUIMapCtrl : public C2D {
public:
    virtual ~CUIMapCtrl();
    virtual int GetZAnimCmdType(const char *name);

    ActionItem m_actions[30];   // 0x18
    int m_numActions;   // 0x1f8 (the destructor zeroes it)
};

/* the children deleted, each taken off the list first, the actions forgotten (SOCOM 1's, its count
 * at 0x1FC there). The walk's iterator is assigned from begin() and end() taken once (it lives in
 * s0); the erase's answer is ASSIGNED to a next declared without it -- initialised from it, MW
 * builds it in place and the copy through the frame is lost -- and kept until the child is gone */
CUIMapCtrl::~CUIMapCtrl()
{
    iterator i;
    iterator e = end();

    for (i = begin(); i != e;) {
        C2D *p = *i;
        iterator next;

        next = erase(i);
        delete p;
        i = next;
    }
    m_numActions = 0;
}

/* the map control's commands: panning and the two zooms, after the UI's own (SOCOM 1's; SOCOM
 * II's counts four) */
int CUIMapCtrl::GetZAnimCmdType(const char *name)
{
    if (!name)
        return C2D::GetZAnimCmdType(name) + 4;
    if (!*name)
        return 0;
    if (!strcasecmp("Pan", name))
        return C2D::GetZAnimCmdType(name) + 1;
    if (!strcasecmp("ZOOM_IN", name))
        return C2D::GetZAnimCmdType(name) + 2;
    if (!strcasecmp("ZOOM_OUT", name))
        return C2D::GetZAnimCmdType(name) + 3;
    return C2D::GetZAnimCmdType(name);
}

/* an .rdr built in memory (zrdr.h's CRdrEditor: only the three node adders the list items call) */
class CRdrEditor {
public:
    bool push(const char *name);
    bool pop();
    bool addintnode(int i);
    bool addrealnode(float f);
    bool addstringnode(const char *s);
};

/* strtod (MSL's; include/stdlib.h has no declaration of it) */
extern "C" double strtod(const char *s, char **end);

class CUIListBox;

/* a list box's column (SOCOM 1's ColumnData: only what the items read -- its kind, 0 an image,
 * and the image's size) */
class ColumnData {
public:
    unsigned char m_type;   // 0x0
    int m_x;   // 0x4: the column's offset
    int m_width;   // 0x8
    int m_height;   // 0xc
    int m_y;   // 0x10
    unsigned char m_justify;   // 0x14: the string's 0x6A
    float m_scale;   // 0x18: the string look's scale when not 0
};

/* a list box's item's look (SOCOM 1's CListItemTypeImpl, zui_2d.cpp's). SOCOM II's table (0x4070F0)
 * has 18 slots to SOCOM 1's 13, SOCOM 1's in its order with five of SOCOM II's among them: the
 * look's transparency after GetString (4), a bool after VisualGratification (13), the item writer
 * (14), the line count where SOCOM 1's IsEnabled was (15: the wrapped text's says how many lines it
 * wraps to) and two size setters (16, 17: the wrapped text's height and width). The empty and
 * constant defaults are inline (their copies, 0x36BA70, 0x36B5D0, 0x36B200, 0x36BDC0, 0x36BA60,
 * 0x36BA20, 0x36B9D0, come with the table); the destructor and the writer are its own functions */
class CListItemTypeImpl {
public:
    virtual ~CListItemTypeImpl();
    virtual void DrawPrep(const CUIListBox &box, int x, int y, int idx) = 0;
    virtual void Finish() {}
    virtual const char *GetString() = 0;
    virtual float GetTrans() = 0;
    virtual void PrepListItem(const std::vector<ColumnData *> &cols, C2DFont *font, float scale,
                              zdb::CTextureRelocMgr *mgr) = 0;
    virtual void SetFont(C2DFont *font) = 0;
    virtual void SetPos(int x, int y) = 0;
    virtual void SetScale(float scale) = 0;
    virtual void SetColor(int r, int g, int b) = 0;
    virtual float SetSequenceTime(float start, float length) { return start; }
    virtual void Tick(float dT) {}
    virtual void VisualGratification(zdb::CTextureRelocMgr *mgr, float depth) = 0;
    virtual bool IsReady() { return true; }
    virtual void AddToRdr(CRdrEditor *ed);
    virtual int GetNumLines() { return 1; }
    virtual void SetHeight(int h) {}
    virtual void SetWidth(int w) {}
};

/* a list box's item (SOCOM 1's ListItem, 0x30 bytes in SOCOM II: its two actions, its text, its
 * look at 0x28 -- then the vtable pointer, at 0x2C, where its first virtual is declared: the array
 * look calls the four through `lw t9,44(item)`). The table, 0x407240, has four slots, each
 * forwarding to the look: the item writer (0x36B1D0: the `jr ra` after it, which no form of it gave,
 * is CListItemTypeImpl's empty writer, a function of its own at 0x36B1F0, the map's row split), the
 * line count, the height and the width. The destructor is not virtual and inline (the array look's
 * inlines it) */
class ListItem {
public:
    ListItem(const char *s);
    ListItem(_zrdr *rdr, const std::vector<ColumnData *> &cols);
    ListItem(_zrdr *rdr, const std::vector<ColumnData *> &cols, C2DFont *font, float scale);
    ~ListItem()
    {
        if (m_text)
            _free(m_text, 0, 0);
        m_text = 0;
        delete m_impl;
    }

    void MakeTimedText();
    void MakeWrappedText();
    void SetString(const char *s);
    void SetFont(C2DFont *font);
    void SetScale(float scale);
    void SetPos(int x, int y);
    void PrepListItem(const std::vector<ColumnData *> &cols, C2DFont *font, float scale,
                      zdb::CTextureRelocMgr *mgr);
    void DrawPrep(const CUIListBox &box, int x, int y, int idx);

    /* the look's, when it has one (inlined into the array look's walks) */
    void Finish()
    {
        if (m_impl)
            m_impl->Finish();
    }
    const char *GetString()
    {
        if (m_impl)
            return m_impl->GetString();
        return "";
    }
    float GetTrans()
    {
        if (m_impl)
            return m_impl->GetTrans();
        return 128.0f;
    }
    void SetColor(int r, int g, int b)
    {
        if (m_impl)
            m_impl->SetColor(r, g, b);
    }
    float SetSequenceTime(float start, float length)
    {
        if (m_impl)
            return m_impl->SetSequenceTime(start, length);
        return start;
    }
    void Tick(float dT)
    {
        if (m_impl)
            m_impl->Tick(dT);
    }
    void VisualGratification(zdb::CTextureRelocMgr *mgr, float depth)
    {
        if (m_impl)
            m_impl->VisualGratification(mgr, depth);
    }

    ActionItem m_action;   // 0x0
    ActionItem m_action2;   // 0x10
    char *m_text;   // 0x20
    bool m_24;   // 0x24
    CListItemTypeImpl *m_impl;   // 0x28

    virtual void AddToRdr(CRdrEditor *ed);
    virtual int GetNumLines();
    virtual void SetHeight(int h);
    virtual void SetWidth(int w);
};

/* the lines the item takes: its look's say, or one without one (SOCOM II's slot 15 where SOCOM 1's
 * IsEnabled was -- the wrapped text's override returns the lines it wraps to, the default 1; the
 * name ours by it) */
int ListItem::GetNumLines()
{
    if (m_impl)
        return m_impl->GetNumLines();
    return 1;
}

/* the look's height and width (slots 2 and 3, forwarded; the names ours) */
void ListItem::SetHeight(int h)
{
    if (m_impl)
        m_impl->SetHeight(h);
}

void ListItem::SetWidth(int w)
{
    if (m_impl)
        m_impl->SetWidth(w);
}

/* the item written into an .rdr: nothing, for a look without text (SOCOM II's; the name ours) */
void CListItemTypeImpl::AddToRdr(CRdrEditor *ed)
{
}

CListItemTypeImpl::~CListItemTypeImpl()
{
}

/* the item written into an .rdr: its look's say (tail-called; SOCOM II's, the name ours) */
void ListItem::AddToRdr(CRdrEditor *ed)
{
    m_impl->AddToRdr(ed);
}

/* the list box as its items read it (SOCOM 1's CUIListBox: the highlighted row, its opacity and
 * its pulse -- what DrawPrep gives the row it draws; the rest stood in) */
class CUIListBox {
public:
    /* the row's opacity, pulse time and limits: the highlighted row's own, the others' fixed */
    int GetAlpha(int idx) const { return m_highlighted == idx ? m_highlightAlpha : 80; }
    float GetPulseTime(int idx) const { return m_highlighted == idx ? m_pulseTime : 0.0f; }
    float GetPulseMax(int idx) const { return m_highlighted == idx ? m_pulseMax : 0.0f; }
    float GetPulseMin(int idx) const { return m_highlighted == idx ? m_pulseMin : 0.0f; }

    unsigned char _0[0xd4];
    std::vector<ColumnData *> m_columns;   // 0xd4
    int m_highlightAlpha;   // 0xe0
    unsigned char _e4[0x130 - 0xe4];
    CPnt3D m_color;   // 0x130: the rows' colour
    unsigned char _13c[0x154 - 0x13c];
    CPnt3D m_highlightColor;   // 0x154
    unsigned char _160[0x174 - 0x160];
    float m_pulseMin;   // 0x174
    float m_pulseMax;   // 0x178
    float m_pulseTime;   // 0x17c
    unsigned char _180[0x194 - 0x180];
    int m_highlighted;   // 0x194
};

namespace {
/* a list item's text look (SOCOM 1's CListItemStringTypeImpl, the unit's anonymous namespace,
 * table 0x407B40): SOCOM 1's embedded string, a renderable one in SOCOM II at 8, drawn through a
 * pointer at 4 (the wrapped text's look points it at its own); 0xCC bytes. The timed and the
 * wrapped text inherit from it */
class CListItemStringTypeImpl : public CListItemTypeImpl {
public:
    CListItemStringTypeImpl()
    {
        SetColored(false);
        SetString(&m_text);
    }
    /* the look with its text formatted in (SOCOM II's: ListItem's constructors new it, the format
     * skipped with the construction when the allocation fails) */
    CListItemStringTypeImpl(const char *s)
    {
        SetColored(false);
        SetString(&m_text);
        m_str->Format(s);
    }
    /* the flag SetColor sets (an inline of its own: it is what makes the derived looks'
     * constructors expand past inline_max_size, inlined one level in the makers, and ListItem's
     * setters too large for -inline auto) */
    void SetColored(bool on) { m_colored = on; }

    virtual void DrawPrep(const CUIListBox &box, int x, int y, int idx);
    virtual const char *GetString()
    {
        const char *s = m_str->m_Ustring;

        return s && *s ? s : m_str->m_string + m_str->m_firstCharPos;
    }
    virtual float GetTrans() { return m_str->m_color[3]; }
    virtual void PrepListItem(const std::vector<ColumnData *> &cols, C2DFont *font, float scale,
                              zdb::CTextureRelocMgr *mgr)
    {
    }
    virtual void SetFont(C2DFont *font) { m_str->SetFont(font); }
    virtual void SetPos(int x, int y) { m_str->SetPos(x, y); }
    virtual void SetScale(float scale) { m_str->m_scale = scale; }
    virtual void SetColor(int r, int g, int b)
    {
        m_colored = true;
        m_str->SetColor(r, g, b);
    }
    virtual void Tick(float dT);
    virtual void VisualGratification(zdb::CTextureRelocMgr *mgr, float depth)
    {
        m_str->C2DString::Draw(mgr, 0, (int)depth);
    }
    virtual void AddToRdr(CRdrEditor *ed);

    /* the string drawn (the wrapped text's look draws its own) */
    void SetString(CRenderableString *s) { m_str = s; }

    CRenderableString *m_str;   // 0x4
    CRenderableString m_text;   // 0x8
    bool m_colored;   // 0xc8: SetColor was called
};

/* a list item's text shown from a time on (SOCOM 1's CListItemTimedTextTypeImpl, table 0x4070A0;
 * 0xD4 bytes): drawn once its time has come, a count sound as it does */
class CListItemTimedTextTypeImpl : public CListItemStringTypeImpl {
public:
    CListItemTimedTextTypeImpl()
    {
        m_start = 0.0f;
        m_time = 0.0f;
    }

    virtual void Finish() { m_start = 0.0f; }
    virtual float SetSequenceTime(float start, float length)
    {
        m_start = start;
        return start + length;
    }
    /* SOCOM 1's: the sound once, as the time passes the start */
    virtual void Tick(float dT)
    {
        bool started = m_time > m_start;

        m_time += dT;
        if (!started && m_time > m_start)
            PlayCountSound();
    }
    virtual void VisualGratification(zdb::CTextureRelocMgr *mgr, float depth)
    {
        if (!(m_time < m_start))
            m_str->C2DString::Draw(mgr, 0, (int)depth);
    }
    virtual void AddToRdr(CRdrEditor *ed) {}

    float m_start;   // 0xcc
    float m_time;   // 0xd0
};

/* a list item's text wrapped to a width (SOCOM II's, table 0x407140; 0x320 bytes): the string type
 * drawing a wrapped text of its own, 300 wide at first */
class CListItemWrappedTextTypeImpl : public CListItemStringTypeImpl {
public:
    CListItemWrappedTextTypeImpl()
    {
        m_wrapped.SetWidth(300.0f);
        SetString(&m_wrapped);
    }

    virtual void VisualGratification(zdb::CTextureRelocMgr *mgr, float depth);
    virtual int GetNumLines()
    {
        if (m_wrapped.m_numLines <= 0)
            return 1;
        return m_wrapped.m_numLines;
    }
    virtual void SetHeight(int h) { m_wrapped.m_height = h; }
    virtual void SetWidth(int w) { m_wrapped.m_width = w; }

    int m_cc;   // 0xcc
    CWrappedText m_wrapped;   // 0xd0
};

/* a list item's picture (SOCOM 1's CListItemImageTypeImpl, table 0x407050; 0xC0 bytes): a plain
 * bitmap of the column's size, its texture named by the item's text */
class CListItemImageTypeImpl : public CListItemTypeImpl {
public:
    CListItemImageTypeImpl(int w, int h, zdb::CTexHandle *tex)
    {
        m_width = w;
        m_height = h;
        m_tex = tex;
        Reload();
    }

    /* the bitmap loaded at the look's size from its texture (the array look's PrepListItem calls it
     * again on the new look) */
    void Reload() { m_bmp.Load(0.0f, 0.0f, m_width, m_height, m_tex); }

    virtual void DrawPrep(const CUIListBox &box, int x, int y, int idx) {}
    virtual const char *GetString() { return ""; }
    virtual float GetTrans() { return m_bmp.GetTrans(); }
    virtual void PrepListItem(const std::vector<ColumnData *> &cols, C2DFont *font, float scale,
                              zdb::CTextureRelocMgr *mgr)
    {
    }
    virtual void SetFont(C2DFont *font) {}
    virtual void SetPos(int x, int y) { m_bmp.SetPos(x, y, x + m_width, y + m_height); }
    virtual void SetScale(float scale) {}
    virtual void SetColor(int r, int g, int b) { m_bmp.SetColor(r, g, b); }
    /* the bitmap at the depth, drawn through an identity matrix */
    virtual void VisualGratification(zdb::CTextureRelocMgr *mgr, float depth)
    {
        CMatrix m;

        m.m_matrix[3][0] = 0.0f;
        m.m_matrix[3][1] = 0.0f;
        m.m_matrix[3][2] = depth;
        m_bmp.Draw(m, mgr);
    }
    virtual void AddToRdr(CRdrEditor *ed) {}

    int m_4;   // 0x4
    CPlainBmp m_bmp;   // 0x10
    int m_width;   // 0xb0
    int m_height;   // 0xb4
    zdb::CTexHandle *m_tex;   // 0xb8
};

/* a list item of several columns (SOCOM 1's CListItemArrayTypeImpl, table 0x407B90; 0x14 bytes): an
 * item per column out of the reader's array of strings, each slot walking them -- through the
 * items' own looks where the item would only forward, through the items where they do more */
class CListItemArrayTypeImpl : public CListItemTypeImpl {
public:
    CListItemArrayTypeImpl(_zrdr *rdr, const std::vector<ColumnData *> &cols);
    CListItemArrayTypeImpl(_zrdr *rdr, const std::vector<ColumnData *> &cols, C2DFont *font,
                           float scale);
    virtual ~CListItemArrayTypeImpl()
    {
        for (int i = 0; i < m_items.size(); i++)
            delete m_items[i];
    }

    virtual void DrawPrep(const CUIListBox &box, int x, int y, int idx);
    virtual void Finish()
    {
        for (int i = 0; i < m_items.size(); i++)
            m_items[i]->Finish();
    }
    virtual const char *GetString() { return m_items[0]->GetString(); }
    virtual float GetTrans() { return m_items[0]->GetTrans(); }
    virtual void PrepListItem(const std::vector<ColumnData *> &cols, C2DFont *font, float scale,
                              zdb::CTextureRelocMgr *mgr);
    virtual void SetFont(C2DFont *font)
    {
        for (int i = 0; i < m_items.size(); i++)
            m_items[i]->SetFont(font);
    }
    virtual void SetPos(int x, int y) {}
    virtual void SetScale(float scale)
    {
        for (int i = 0; i < m_items.size(); i++)
            m_items[i]->SetScale(scale);
    }
    virtual void SetColor(int r, int g, int b)
    {
        for (int i = 0; i < m_items.size(); i++)
            m_items[i]->SetColor(r, g, b);
    }
    virtual float SetSequenceTime(float start, float length)
    {
        for (int i = 0; i < m_items.size(); i++)
            start = m_items[i]->SetSequenceTime(start, length);
        return start;
    }
    virtual void Tick(float dT)
    {
        for (int i = 0; i < m_items.size(); i++)
            m_items[i]->Tick(dT);
    }
    virtual void VisualGratification(zdb::CTextureRelocMgr *mgr, float depth)
    {
        for (int i = 0; i < m_items.size(); i++)
            m_items[i]->VisualGratification(mgr, depth);
    }
    virtual bool IsReady() { return m_ready; }
    virtual void AddToRdr(CRdrEditor *ed)
    {
        ed->push(0);
        for (int i = 0; i < m_items.size(); i++)
            m_items[i]->AddToRdr(ed);
        ed->pop();
    }
    /* the most lines any column takes */
    virtual int GetNumLines()
    {
        int n = 1;

        for (int i = 0; i < m_items.size(); i++) {
            if (n < m_items[i]->GetNumLines())
                n = m_items[i]->GetNumLines();
        }
        return n;
    }
    virtual void SetHeight(int h)
    {
        for (int i = 0; i < m_items.size(); i++)
            m_items[i]->SetHeight(h);
    }
    virtual void SetWidth(int w)
    {
        for (int i = 0; i < m_items.size(); i++)
            m_items[i]->SetWidth(w);
    }

    std::vector<ListItem *> m_items;   // 0x4
    bool m_ready;   // 0x10: every column's item ready
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

/* a renderable string: no alternate font, twice the size, white, not moved (SOCOM 1's; out of line
 * for the specs and the keyboard, inlined into the list item's text look) */
CRenderableString::CRenderableString()
{
    m_alternateFont = 0;
    m_height = 0.0f;
    m_scale = 2.0f;
    SetColor(255.0f, 255.0f, 255.0f);
    m_offset = CPnt3D::zero;
}

/* the alternate font deleted with the string (called out of line everywhere: the font's own
 * destructor chain is too tall for -inline auto) */
CRenderableString::~CRenderableString()
{
    if (m_alternateFont)
        delete m_alternateFont;
}

/* the wrapped text drawn at the depth, moved by its offset to whole pixels (SOCOM II's) */
void CListItemWrappedTextTypeImpl::VisualGratification(zdb::CTextureRelocMgr *mgr, float depth)
{
    CMatrix m;

    float x = (int)(m_wrapped.m_posx + m_wrapped.m_socom2_7c);
    float y = (int)(m_wrapped.m_posy + m_wrapped.m_socom2_80);

    m.m_matrix[3][0] = x;
    m.m_matrix[3][1] = y;
    m.m_matrix[3][2] = depth;
    m_wrapped.Draw(m, mgr);
}

/* a pulse's time and limits set on a string (ours: DrawPrep's stores, its arguments taken last
 * first) */
static inline void SetPulse(C2DString *s, float time, float max, float min)
{
    s->m_fadeTime = time;
    s->m_fadeMax = max;
    s->m_fadeMin = min;
}

/* the row placed and lit: the highlighted one at the box's opacity and pulsing, if the box pulses,
 * the others at 80 and still (SOCOM 1's DrawPrep, the pulse SOCOM II's) */
void CListItemStringTypeImpl::DrawPrep(const CUIListBox &box, int x, int y, int idx)
{
    m_str->SetPos(x, y);
    m_str->Update();
    m_str->SetTrans(box.GetAlpha(idx));
    if (box.GetPulseTime(idx) != 0.0f) {
        m_str->m_Type = STRING_PULSE;
        SetPulse(m_str, box.GetPulseTime(idx), box.GetPulseMax(idx), box.GetPulseMin(idx));
    } else
        m_str->m_Type = STRING_NORMAL;
}

/* a reader node as the array look reads it (zrdr.h's _zrdr: its type -- 3 a string, 4 an array --,
 * an array's count at 2 and its 8-byte elements, or a string's text, through the word at 4) */
struct ZRdrNode {
    unsigned char type;   // 0x0
    unsigned short count;   // 0x2
    union {
        ZRdrNode *array;   // 0x4
        const char *str;
    };
};

static inline unsigned int RdrCount(const ZRdrNode *r) { return r->type == 4 ? r->count : 0; }
static inline const char *RdrString(const ZRdrNode *r) { return r->type == 3 ? r->str : 0; }
static inline ZRdrNode *RdrElem(const ZRdrNode *r, unsigned int i)
{
    return i < RdrCount(r) ? &r->array[i] : 0;
}

/* an item per string of the reader's array, as many as there are columns (SOCOM 1's; ready while
 * every item is).
 * OPEN, 10 instructions, all in the prologue: the target builds CListItemTypeImpl's table address,
 * stores it, then builds this class's and stores it (one register, v0, serialised); here both
 * addresses are built up front in v1/v0 (as the string look's constructor 0x36B330 does in the
 * target too). The rest -- the reader's count clamped to the columns (RdrCount evaluated twice),
 * the walk, the push_back through the pod insert -- is byte-exact */
CListItemArrayTypeImpl::CListItemArrayTypeImpl(_zrdr *rdr, const std::vector<ColumnData *> &cols)
{
    const ZRdrNode *r = (const ZRdrNode *)rdr;
    int n = RdrCount(r) < cols.size() ? RdrCount(r) : cols.size();

    m_ready = true;
    for (int i = 0; i < n; i++) {
        ZRdrNode *e = RdrElem(r, i);

        if (e && e->type == 3) {
            ListItem *item = new ListItem(RdrString(e));

            if (!item->m_24)
                m_ready = false;
            item->SetString(RdrString(e));
            m_items.push_back(item);
        }
    }
}

/* each column's item set up: a picture column's look made a bitmap of the column's size, its
 * texture named by the item's text; the others given the font and scale (SOCOM 1's) */
void CListItemArrayTypeImpl::PrepListItem(const std::vector<ColumnData *> &cols, C2DFont *font,
                                          float scale, zdb::CTextureRelocMgr *mgr)
{
    for (int i = 0; i < m_items.size(); i++) {
        ListItem *item = m_items[i];
        ColumnData *col = cols[i];

        if (item) {
            if (col->m_type == 0) {
                int w, h;
                zdb::CTexHandle *tex;

                h = col->m_height;
                w = col->m_width;
                tex = PrepTexture(item->GetString(), mgr);

                delete item->m_impl;

                CListItemImageTypeImpl *img = new CListItemImageTypeImpl(w, h, tex);

                img->Reload();
                item->m_impl = img;
            } else {
                item->SetFont(font);
                item->SetScale(scale);
            }
        }
    }
}

/* each column's item placed and its text lit as the box's rows are (SOCOM 1's; the text looks only,
 * picked out with a dynamic_cast) */
void CListItemArrayTypeImpl::DrawPrep(const CUIListBox &box, int x, int y, int idx)
{
    for (int i = 0; i < m_items.size(); i++) {
        ListItem *item = m_items[i];

        item->SetPos(x + box.m_columns[i]->m_x, y + box.m_columns[i]->m_y);

        const CPnt3D *color = box.m_highlighted == idx ? &box.m_highlightColor : &box.m_color;
        CListItemStringTypeImpl *str = dynamic_cast<CListItemStringTypeImpl *>(item->m_impl);

        if (str) {
            bool ready = IsReady();

            if (!str->m_colored) {
                str->m_str->SetColor(color->x, color->y, color->z);
                str->m_str->Update();
            }
            if (box.GetPulseTime(idx) != 0.0f && ready) {
                str->m_str->m_Type = STRING_PULSE;
                SetPulse(str->m_str, box.GetPulseTime(idx), box.GetPulseMax(idx), box.GetPulseMin(idx));
            } else {
                str->m_str->SetTrans(box.GetAlpha(idx));
                str->m_str->m_Type = STRING_NORMAL;
            }
            str->m_str->m_socom2_6a = box.m_columns[i]->m_justify;
            if (box.m_columns[i]->m_scale != 0.0f)
                str->SetScale(box.m_columns[i]->m_scale);
        }
    }
}

/* an item of one text (SOCOM II's: the array look's constructor news one per column), its look a
 * text look holding it */
ListItem::ListItem(const char *s) : m_text(0), m_24(true), m_impl(new CListItemStringTypeImpl(s))
{
}

/* an item from a reader: an array's columns as an array look (SOCOM 1's constructor) */
ListItem::ListItem(_zrdr *rdr, const std::vector<ColumnData *> &cols)
    : m_text(0), m_24(true), m_impl(0)
{
    const ZRdrNode *r = (const ZRdrNode *)rdr;

    if (r && RdrCount(r))
        m_impl = new CListItemArrayTypeImpl(rdr, cols);
}

/* an item from a reader in a font and scale: a string as a text look, an array as an array look,
 * the item not ready while its columns are not (SOCOM II's) */
ListItem::ListItem(_zrdr *rdr, const std::vector<ColumnData *> &cols, C2DFont *font, float scale)
    : m_text(0), m_24(true), m_impl(0)
{
    const ZRdrNode *r = (const ZRdrNode *)rdr;

    if (r->type == 3)
        m_impl = new CListItemStringTypeImpl(RdrString(r));
    else if (RdrCount(r)) {
        m_impl = new CListItemArrayTypeImpl(rdr, cols, font, scale);
        if (!m_impl->IsReady())
            m_24 = false;
    }
}

/* the row drawn: a text look made first when the item has none (SOCOM II's; the name ours) */
void ListItem::DrawPrep(const CUIListBox &box, int x, int y, int idx)
{
    if (!m_impl)
        m_impl = new CListItemStringTypeImpl;
    m_impl->DrawPrep(box, x, y, idx);
}

/* the look's font, scale and place, a text look made first when the item has none (SOCOM II's;
 * out of line in the array look's walks; the names ours) */
void ListItem::SetFont(C2DFont *font)
{
    if (!m_impl)
        m_impl = new CListItemStringTypeImpl;
    m_impl->SetFont(font);
}

void ListItem::SetScale(float scale)
{
    if (!m_impl)
        m_impl = new CListItemStringTypeImpl;
    m_impl->SetScale(scale);
}

void ListItem::SetPos(int x, int y)
{
    if (!m_impl)
        m_impl = new CListItemStringTypeImpl;
    m_impl->SetPos(x, y);
}

/* the item's text: a text look made when it has none, the text formatted into its string (SOCOM
 * II's: the look taken as a text look; the name ours) */
void ListItem::SetString(const char *s)
{
    if (!m_impl)
        m_impl = new CListItemStringTypeImpl;
    ((CListItemStringTypeImpl *)m_impl)->m_str->Format(s);
}

/* the look set up for the columns, then the font and scale given (SOCOM 1's PrepListItem through
 * the item; the name ours) */
void ListItem::PrepListItem(const std::vector<ColumnData *> &cols, C2DFont *font, float scale,
                            zdb::CTextureRelocMgr *mgr)
{
    if (m_impl)
        m_impl->PrepListItem(cols, font, scale, mgr);
    SetFont(font);
    SetScale(scale);
}

/* the item's look a timed text (SOCOM II's: out of line in ListItem's constructor, the look's
 * constructor inlined one level; the name ours) */
void ListItem::MakeTimedText()
{
    m_impl = new CListItemTimedTextTypeImpl;
}

/* the item's look a wrapped text (likewise; the name ours) */
void ListItem::MakeWrappedText()
{
    m_impl = new CListItemWrappedTextTypeImpl;
}

/* the teletype's destructor (implicit: the cursor, the text, then the renderable string's;
 * reached only through the thunk 0x37D030) */
CTeleType::~CTeleType()
{
}

/* the strip's buttons shown, the chosen one active and the others normal (SOCOM II's; called out of
 * line by the command tick and the pad's handler: pinned; the end declared before the walk's
 * iterator, or the two trade saved registers) */
#pragma auto_inline off
void CRadioStrip::ShowChoice()
{
    int n = 0;

    for (iterator e = end(), i = begin(); i != e; ++i, ++n) {
        C2D *button = *i;

        if (button) {
            if (n == m_choice)
                button->SetActive();
            else
                button->SetNormal();
        }
    }
}
#pragma auto_inline reset

/* the render pipe as a 2D visual reads it: the opacity stack (zrndr_pipe.cpp's CPipe, OVL1 bss
 * 0x4887C0; its 0x720 bytes stood in) */
class CPipe {
public:
    unsigned char _0[0x668];
    float *m_opacity_stack;   // 0x668
    int m_opacity_stack_size;   // 0x66c
    int m_opacity_stack_index;   // 0x670
    unsigned char _674[0x720 - 0x674];
};

extern CPipe thePipe;

/* a 2D object drawn by the world's render pipe (SOCOM 1's C2DVisual, a zdb::CVisual made from a
 * C2D: zanim_menu.h's, the object at 0x68; its destructor declared first so that this unit does
 * not take its table) */
class C2DVisual {
public:
    virtual ~C2DVisual();
    virtual void Render(CMatrix *m);

    unsigned char _4[0x68 - 4];
    C2D *m_2d;   // 0x68
    zdb::CTextureRelocMgr *m_mgr;   // 0x6c
};

/* the object drawn through the matrix, faded to the pipe's opacity first when it is not opaque (a
 * fade within a hundredth of the end drawn opaque: SOCOM 1's C2DVisual::Render; the scale applied
 * in place, `*=`, keeps the opacity in the argument's register) */
void C2DVisual::Render(CMatrix *m)
{
    float opacity = thePipe.m_opacity_stack[thePipe.m_opacity_stack_index];

    if (opacity < 1.0f) {
        if (opacity == 0.99f)
            opacity = 1.0f;
        opacity *= 128.0f;
        m_2d->SetTrans(opacity);
    }
    m_2d->Draw(*m, m_mgr);
}

/* the string drawn at its offset through the matrix, placed again (and its own slot told,
 * +0xA0) when the position has moved (SOCOM 1's CRenderableString::Draw, slot 2) */
void CRenderableString::Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr)
{
    CPnt3D p = m_offset;

    m.Transform(&p, 1);
    if (p.x != (int)(m_posx + m_socom2_7c) || p.y != (int)(m_posy + m_socom2_80)) {
        SetPos((int)p.x, (int)p.y);
        Update();
    }
    C2DString::Draw(mgr, 0, (int)p.z);
}

/* the pad as the buttons read it: each button's key state (zinput.h's CPad::m_state from
 * 0x1; 2 a press, 1 a release) */
class CPad {
public:
    unsigned char m_0;   // 0x0
    unsigned char m_state[16];   // 0x1
};

/* an action's animation started, handed its argument if it has one (the check box's two arms:
 * an inline member whose copy of `this` -- the item, a variable of its own -- stays in a saved
 * register across the lookup, the first read folded into the box's offset; read through `this`
 * itself MW folds every read) */
inline void ActionItem::Run() const
{
    const ActionItem *item = this;
    CZAnim *anim = ZAnim.GetAnim(item->Animation, 0);

    if (anim) {
        if (item->Argument)
            anim->ActivateArg(1, 8, item->Argument, 0);
        else
            anim->Activate(0);
    }
}

/* a release of button 9 (SOCOM 1's button 27) toggles the check, running the action of the state
 * left -- the second when it was checked, the first when not (SOCOM 1's CCheckBox::HandlePad) */
bool CCheckBox::HandlePad(const CPad *pad, int port)
{
    if (!CanHandleInput())
        return false;
    if (pad->m_state[9] == 1) {
        if (m_checked) {
            m_checked = false;
            m_action1.Run();
        } else {
            m_checked = true;
            m_action0.Run();
        }
        return true;
    }
    return false;
}

/* the check box drawn: the button, then, when checked, its check placed by the matrix at its
 * offset and drawn with no matrix of its own (SOCOM 1's CCheckBox::Draw, slot 2) */
void CCheckBox::Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr)
{
    C3StateButton::Draw(m, mgr);
    if (m_checked) {
        CMatrix mat;
        CPnt3D p = CPnt3D::zero;

        m.Transform(&p, 1);
        p.x += m_checkX;
        p.y += m_checkY;
        m_check.SetPos((int)p.x, (int)p.y, (int)(p.x + m_checkW), (int)(p.y + m_checkH));
        m_check.Draw(mat, mgr);
    }
}

/* the button's destructor (implicit: the caption, then C2D's). Defined here, it brings the class's
 * table, and with it Tick's inline copy (0x37C600): once C2DButton overrides Tick, no other table
 * this unit emits takes it */
C3StateButton::~C3StateButton()
{
}

/* the button drawn: its background (slot 28), then its caption, placed by the matrix at its
 * offset (SOCOM 1's C3StateButton::Draw, slot 2) */
void C3StateButton::Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr)
{
    if (IsOn()) {
        DrawBackground(m, mgr);
        if (_szCaption) {
            CPnt3D p = CPnt3D::zero;

            m.Transform(&p, 1);
            _strCaption.SetPos((int)(p.x + _iTextX), (int)(p.y + _iTextY));
            _strCaption.Draw(mgr, 0, (int)p.z);
        }
    }
}

/* a press of button 9 (SOCOM 1's button 27) shows the button pressed and hands the pad to its
 * children, the first to take it ending the walk -- taken either way (SOCOM 1's, slot 15).
 * OPEN, 11 instructions: two saved registers swapped -- the target keeps this (then the end,
 * this + 4) in s3 and the walk's node in s2, MW here the other way round; the code is otherwise
 * the target's, the `!` of the child's answer materialised included. Tried: end declared first,
 * end in the loop test (13), an assigned iterator (0x110), a while walk (11), the base deleter's
 * iterator (11), the walk in an inline helper (0xF4) */
bool CHandlerButton::HandlePad(const CPad *pad, int port)
{
    if (!CanHandleInput())
        return false;
    if (size() && pad->m_state[9] == 2) {
        Slot27();
        iterator i = begin();
        iterator e = end();
        for (; i != e; ++i)
            if ((*i)->HandlePad(pad, port))
                return true;
        return true;
    }
    return false;
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

/* the variable's seconds as minutes and seconds (SOCOM 1's C2DClock::UpdateUIVar, the same code:
 * the value read as a real or an integer, "%02d:%02d" set through the table's SetString, the
 * extent measured again) */
void C2DClock::UpdateUIVar(CUIVariable *var)
{
    if (UIVarMixin::m_uivar && UIVarMixin::m_uivar == var) {
        char buf[800];
        int secs = 0;
        const CUIVariable *v = var;

        buf[0] = 0;
        if (v->m_type == 2)
            secs = (int)v->Real();
        else if (v->m_type == 1)
            secs = v->Int();
        sprintf(buf, "%02d:%02d", secs / 60, secs % 60);
        SetString(buf, true);
        UpdatePos();
    }
}

/* a star for every character of the text (a UTF-8 character's bytes one star), set untranslated
 * (slot 26 of CSecureText's table, the image's first function of the unit: defined last). The
 * walk's test is an inline bool over an inline `!p` (the target materialises `!t` with sltu/xori,
 * then the whole test into v1); the walk is a copy of the text declared after the count, the stars
 * indexed by the count, and each character's pointer copied for UTF8CharLen's reference after the
 * star is stored (each other order or a walking star pointer moves the saved registers) */
void CSecureText::SetString(const char *s, bool translate)
{
    if (s) {
        char stars[256];
        int n = 0;
        const char *t = s;

        while (SecureMore(t)) {
            stars[n] = '*';
            n++;
            const char *c = t;
            t += UTF8CharLen(c);
        }
        stars[n] = 0;
        C2DString::SetString(stars, false);
    }
}
