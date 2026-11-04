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
 * functions need. Defined in reverse of their image order (-inline deferred). Its initialiser,
 * 0x4036C0, builds only <iostream>'s two init objects, as SOCOM 1's __sinit_zui_2d.cpp. */

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "ztwod.h"
#include "../zvid/zvid.h"

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
    unsigned char _a[0xc - 0xa];
    unsigned short m_stringid;   // 0xc: a string table id (the renderable string's caption)
    unsigned char _e[0x10 - 0xe];
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
    /* a value written out as text into a buffer of that size (zui.h's static, zui_var.cpp's
     * 0x3860F0) */
    static char *Format(_zrdr *value, char *buf, int size);
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



/* a UI variable's hook (SOCOM 1's DWARF; the vtable pointer first) */
class UIVarMixin {
public:
    UIVarMixin() { m_uivar = 0; }
    virtual ~UIVarMixin() { DetachUIVar(0); }
    void DetachUIVar(CUIVariable *var);
    void AttachUIVar(CUIVariable *var);
    /* watched and applied at once (ours: the LoadSpecs' pair, the update through the mixin's own
     * table) */
    void WatchUIVar(CUIVariable *var)
    {
        AttachUIVar(var);
        UpdateUIVar(var);
    }
    /* the variable changed (an override takes a new primary slot after all the tables, so the
     * secondary tables' offsets stand) */
    virtual void UpdateUIVar(CUIVariable *var);

    CUIVariable *m_uivar;   // 0x4
};   // 8 bytes

/* the UI variables (zui.h's: the manager makes or finds one by name, with its value and lifetime)
 * and the specs' variable block (zui_spec.h's CUIVariableSpec: the name, the value, the longevity at
 * 0x28, a byte enum) -- what the LoadSpecs read */
enum UIVAR_LONGEVITY {
    UIVL_DORMANT = 0,
    UIVL_TEMPORARY = 1,
    UIVL_PERMANENT = 2,
    UIVL_PERSISTENT = 3
};

class CUIVarManager {
public:
    CUIVariable *Add(const char *name, _zrdr *value, UIVAR_LONGEVITY longevity);
};
extern CUIVarManager theUIVarManager;

class CUIVariableSpec {
public:
    char *m_name;   // 0x0
    _zrdr *m_value;   // 0x4
    unsigned char _8[0x28 - 0x8];
    UIVAR_LONGEVITY m_longevity;   // 0x28
};

/* a dialog object's spec (zui_spec.h's CDlgObjSpec: its variable block at 4 and the inline that
 * makes the variable) */
class CDlgObjSpec {
public:
    virtual ~CDlgObjSpec();
    /* (zui_spec.h's slots 0x0C and 0x10: the map control's LoadSpec makes its children so) */
    virtual CDlgObjSpec *Copy() = 0;
    virtual C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const = 0;
    CUIVariable *GetUIVar() const
    {
        return m_var ? theUIVarManager.Add(m_var->m_name, m_var->m_value, m_var->m_longevity) : 0;
    }

    CUIVariableSpec *m_var;   // 0x4
};

/* a text's spec and a teletype's (zui_spec.h's CTextSpec, 0x34 bytes, and CTtySpec: INTERVAL,
 * MAXLENGTH, SPACING, PULSERATE, MAXHEIGHT and the cursor's two flags) */
class CTextSpec;   // after CSaferStr, below
class CTtySpec;

/* a wrapped text's spec (zui_spec.h's CWrapSpec: WIDTH, SPACING, HEIGHT, DISPLAY_ONE_LINE, the two
 * arrows' textures, offsets and sizes, AUTOSCROLL) */
struct IPNT2D {
    int x, y;
};

class CWrapSpec;   // after CSaferStr, below

/* an image's spec (zui_spec.h's CImageSpec: the file name's string, the size, the pulse) */
class CImageSpec : public CDlgObjSpec {
public:
    unsigned char _8[0x14 - 0x8];
    int m_sizex;   // 0x14: the size (zui_spec.h's IPNT2D)
    int m_sizey;   // 0x18
    bool m_pulse;   // 0x1c
    float m_pulseRate;   // 0x20
};

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
    void LoadSpec(const CImageSpec &spec, zdb::CTexHandle *tex);

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
    void LoadSpec(const CTextSpec &spec);

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

    void LoadSpec(const CWrapSpec &spec, zdb::CTextureRelocMgr *mgr);

    float m_width;   // 0xc0: the width it wraps to (the list item's look sets 300)
    int m_shown;   // 0xc4: the lines the window shows (zui_2d.h's m_line, -1 at first)
    int m_c8;   // 0xc8
    float m_offset;   // 0xcc: the scroll, in the string's spacing
    bool m_autoScroll;   // 0xd0: the spec's AUTOSCROLL
    zdb::CTexHandle *m_downTex;   // 0xd4: DOWN_ARROW's texture
    zdb::CTexHandle *m_upTex;   // 0xd8: UP_ARROW's texture
    IPNT2D m_downOffset;   // 0xdc
    IPNT2D m_downSize;   // 0xe4
    IPNT2D m_upOffset;   // 0xec
    IPNT2D m_upSize;   // 0xf4
    CPlainBmp m_upArrow;   // 0x100 (the destructor 0x379D90 destroys it after the down arrow)
    CPlainBmp m_downArrow;   // 0x1a0
    bool m_displayOneLine;   // 0x240: DISPLAY_ONE_LINE
    unsigned char _241[0x250 - 0x241];
};   // 0x250 bytes

/* wrapped text scrolling by itself (SOCOM 1's CScrollText; SOCOM II's 0x270 bytes, zui_2d.h's
 * layout: the line to show at 0x258, the scroll position, a float, at 0x260) */
class CScrollSpec;

class CScrollText : public CWrappedText {
public:
    void LoadSpec(const CScrollSpec &spec);
    virtual ~CScrollText();
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
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
    /* inline (zonline.h's, SOCOM 1's): the renderable string's parser builds one on the stack */
    CSaferStr() {}
    /* never null: no text empties it (SOCOM 1's zutil.h; nothing returned, as 0x22DCC0) */
    void operator=(const char *sz)
    {
        if (sz)
            assign(sz);
        else
            erase();
    }
};

/* a progress bar's spec (zui_spec.h's CHProgressBarSpec: an image's, then a string -- SOCOM 1's at
 * 0x20, SOCOM II's at 0x28; the name ours) */
class CHProgressBarSpec : public CImageSpec {
public:
    unsigned char _24[0x28 - 0x24];
    CSaferStr m_text;   // 0x28
};

class CTextSpec : public CDlgObjSpec {
public:
    /* the colour by value (ours: the target copies it through a return slot on the stack) */
    CPnt3D GetColor() const { return m_color; }

    CSaferStr m_caption;   // 0x8: CAPTION
    float m_scale;   // 0x14: SCALE
    bool m_hasFont;   // 0x18: a FONT named
    bool m_hcentered;   // 0x19: HCENTERED
    bool m_rightJustified;   // 0x1a: RIGHT_JUSTIFIED
    CSaferStr m_font;   // 0x1c: FONT
    CPnt3D m_color;   // 0x28: COLOR
};

class CTtySpec : public CTextSpec {
public:
    float m_interval;   // 0x34
    float m_maxLength;   // 0x38
    float m_spacing;   // 0x3c
    float m_pulseRate;   // 0x40
    int m_maxHeight;   // 0x44
    bool m_noCursor;   // 0x48
    bool m_fadeCursor;   // 0x49
};

class CWrapSpec : public CTextSpec {
public:
    float m_width;   // 0x34
    int m_spacing;   // 0x38
    float m_height;   // 0x3c
    bool m_displayOneLine;   // 0x40
    CSaferStr m_upTexture;   // 0x44
    CSaferStr m_downTexture;   // 0x50
    IPNT2D m_upOffset;   // 0x5c
    IPNT2D m_upSize;   // 0x64
    IPNT2D m_downOffset;   // 0x6c
    IPNT2D m_downSize;   // 0x74
    bool m_autoScroll;   // 0x7c
};

class ActionItem;

/* a button's spec (zui_spec.h's CButtonSpec, 0x118 bytes: only its size here) and a check box's
 * (CCheckBoxSpec: the two actions, the check's place and size, its texture's name) */
class CButtonSpec : public CDlgObjSpec {
public:
    unsigned char _8[0x118 - 0x8];
};

class CCheckBoxSpec : public CButtonSpec {
public:
    ActionItem *m_onCheck;   // 0x118: ONCHECK
    ActionItem *m_onUncheck;   // 0x11c: ONUNCHECK
    IPNT2D m_checkOffset;   // 0x120
    IPNT2D m_checkSize;   // 0x128
    CSaferStr m_checkTexture;   // 0x130
};

/* a slider's spec (zui_spec.h's CSliderBarSpec: the knob's texture name and size, the centre value,
 * the intervals) */
class CSliderBarSpec : public CButtonSpec {
public:
    CSaferStr m_knobTexture;   // 0x118
    IPNT2D m_knobSize;   // 0x124
    float m_centerValue;   // 0x12c
    int m_numIntervals;   // 0x130
};

/* the radio strip's and the handler button's specs (zui_spec.h's: buttons' specs with fields of their
 * own this unit does not read) and the scrolling text's (a wrapped text's) */
class CRadioStripSpec : public CButtonSpec {
};

class CHandlerButtonSpec : public CButtonSpec {
};

class CScrollSpec : public CWrapSpec {
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
    void LoadSpec(const CTtySpec &spec);

    CSaferStr m_text;   // 0xc0
    int m_cc;   // 0xcc
    int m_len;   // 0xd0: the text's length
    int m_typed;   // 0xd4: the characters shown
    int m_d8;   // 0xd8
    float m_interval;   // 0xdc: the spec's INTERVAL
    float m_maxLength;   // 0xe0: MAXLENGTH (100000 when none)
    int m_shown;   // 0xe4: the lines the window shows (the spec's MAXHEIGHT)
    float m_e8;   // 0xe8: a float (LoadSpec's 0.0f takes the FP temporary the target numbers there)
    float m_pulseRate;   // 0xec: the cursor's PULSERATE (1 when none)
    float m_f0;   // 0xf0
    unsigned char _f4[0x100 - 0xf4];
    CPlainBmp m_cursor;   // 0x100
    int m_1a0;   // 0x1a0
    bool m_showCursor;   // 0x1a4: no NOCURSOR
    bool m_fadeCursor;   // 0x1a5: FADECURSOR
    int m_1a8;   // 0x1a8
} __attribute__((aligned(16)));   // 0x1b0 bytes

const char *TranslateString(const char *s);

/* a button's caption and states (SOCOM 1's DWARF: : C2D; SOCOM II's 0xC0 bytes, its vtable 29
 * slots: C2D's 26 and three new, DrawBackground the last) */
class C3StateButton : public C2D {
public:
    /* inline (C2DButton's constructor inlines it, as SOCOM 1's does): no caption, the text at
     * (10, 5), white, at scale 1.5 */
    C3StateButton() : _szCaption(0), _iTextX(10), _iTextY(5), _bCaptionCentered(false)
    {
        _strCaption.SetColor(255.0f, 255.0f, 255.0f);
        _strCaption.m_scale = 1.5f;
    }
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
    /* inline (C2DButton's constructor builds its four so, as SOCOM 1's: grey caption, the scale
     * left alone) */
    ButtonState()
        : m_tex(0), m_fadeMin(0.0f), m_fadeMax(0.0f), m_pulse(false), m_period(0.0f), m_fadeTime(0.0f),
          m_color(128.0f, 128.0f, 128.0f)
    {
    }

    CSaferStr m_texName;   // 0x0: the state's image (SOCOM 1's _TexName)
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
    bool Start() const;
    ActionItem &operator=(const ActionItem &a);   // zui.cpp's, 0x369FA0

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
    /* the animation's nodes looked up again; a node by its reference index (the map control's
     * zooms) */
    void Refresh(bool force);
    zdb::CNode *GetNode(int index, bool create);
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

/* what a valve's change tells its callbacks (zanim.h's) */
enum VALVE_STATE { VALVE_SAME = 0, VALVE_ON = 1, VALVE_OFF = 2 };

class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);
    void MakeCallbacks(VALVE_STATE state);
    /* the value set, its callbacks told when it changed (the README's shape: both builds inline
     * it -- an int parameter, the state compared against an enumerator) */
    void SetValue(int value)
    {
        VALVE_STATE state = VALVE_SAME;

        if (m_value != value)
            state = value == 0 ? VALVE_OFF : VALVE_ON;
        m_value = value;
        if (state != VALVE_SAME)
            MakeCallbacks(state);
    }

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
    C2DButton();
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
    void LoadSpec(const CButtonSpec &spec);   // 0x376BD0, not written

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
    C2DFont *m_alternateFont;   // 0x248 (SOCOM 1's, at 0x218 there): deleted by the destructor
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
    void LoadSpec(const CCheckBoxSpec &spec, zdb::CTexHandle *tex);

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
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void Tick(float dT);
    virtual void UpdateUIVar(CUIVariable *var);
    /* a slot of its own, +0x84 (zui_2d.h's) */
    virtual void LoadSpec(const CHProgressBarSpec &spec, zdb::CTexHandle *tex);

    float m_fill;   // 0xa0: 0 to 1
    int m_width;   // 0xa4: the full bar's width (Draw converts it)
    float m_time;   // 0xa8: the seconds a full bar takes
    CSaferStr m_str;   // 0xac
} __attribute__((aligned(16)));   // 0xc0 bytes

/* a slider: a button with a knob (SOCOM 1's CSliderBar; SOCOM II's 0x510 bytes, the knob a plain
 * bitmap at 0x460 -- zui_2d.h's layout) */
class CSliderBar : public C2DButton {
public:
    virtual ~CSliderBar();
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void UpdateUIVar(CUIVariable *var);
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr);
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    virtual int GetZAnimCmdType(const char *name);
    void LoadSpec(const CSliderBarSpec &spec, zdb::CTexHandle *tex);

    float m_value;   // 0x440
    zdb::CTexHandle *m_knobTex;   // 0x444: the knob's texture
    int m_knobW;   // 0x448: the knob's size
    int m_knobH;   // 0x44c
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

    virtual bool HandlePad(const CPad *pad, int port);
    /* the strip's two refreshes after a new choice: its buttons shown (SOCOM II's, 0x370D10, the name
     * ours) and its check boxes set (SOCOM 1's PressActive, 0x370DB0) */
    void ShowChoice();
    void PressActive();
    void LoadSpec(const CRadioStripSpec *spec, zdb::CTexHandle *tex);

    int m_choice;   // 0x440
    bool m_444;   // 0x444: set by LoadSpec (SOCOM 1's at 0x414)
} __attribute__((aligned(16)));   // 0x450 bytes


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
    void LoadSpec(const CHandlerButtonSpec &spec);
} __attribute__((aligned(16)));   // 0x440 bytes

/* the pop-up dialog as its command-type lookup sees it (slot 24 of its vtable, 0x36D970; the
 * renderable string's (0x36FC10) and the button's (0x3767C0) are declared with their classes
 * above). Each answers its own commands as the UI's count plus an index, and an empty name
 * with 0 */
class CPopUpDialog : public CPlainBmp {
public:
    /* declared, not defined: stand-ins whose implicit destructors would supersede the drafts */
    virtual ~CPopUpDialog();
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void Tick(float dT);
    virtual bool HandlePad(const CPad *pad, int port);
    virtual int GetZAnimCmdType(const char *name);

    /* a plain bitmap (Draw inlines CPlainBmp's), its text a 2D object at 0xB0 placed at an offset;
     * the parts its Tick ticks (SOCOM II's two members, through their tables; SOCOM 1 ticked the
     * pointer alone), each with an offset after it, drawn when 0x330 is set; the 2D object drawn
     * instead of it all when 0x32C points at one -- C2D stand-ins, their real classes unknown */
    unsigned char _a0[0xb0 - 0xa0];
    C2DString m_text;   // 0xb0 (a string: HandlePad empties it through slot 26)
    unsigned char _148[0x304 - 0x148];
    IPNT2D m_textOffset;   // 0x304
    unsigned char _30c[0x32c - 0x30c];
    C2D *m_32c;   // 0x32c
    CValve *m_330;   // 0x330: the valve the answer is set in (the two buttons drawn only with one)
    int m_choice;   // 0x334: the button chosen, 1 or 2
    unsigned char _338;
    bool m_339;   // 0x339: the cancel button (6) answers 66
    unsigned char _33a[0x340 - 0x33a];
    C2DButton m_340;   // 0x340: the first button
    IPNT2D m_340Offset;   // 0x780
    unsigned char _788[0x790 - 0x788];
    C2DButton m_790;   // 0x790: the second
    IPNT2D m_790Offset;   // 0xbd0
    unsigned char _bd8[0xbe0 - 0xbd8];
    bool m_keep;   // 0xbe0: the pop-up stays up after an answer
};


/* the text from the string's own variable, written out and formatted in, then brought up to date
 * (SOCOM 1's CRenderableString::UpdateUIVar; reached through the thunk 0x37D150) */
void CRenderableString::UpdateUIVar(CUIVariable *var)
{
    char buf[10500] = {0};

    if (UIVarMixin::m_uivar && UIVarMixin::m_uivar == var) {
        Format(CUIVariable::Format(UIVarMixin::m_uivar, buf, 10499));
        Update();
    }
}

/* set up from its spec (SOCOM 1's CTeleType::LoadSpec, which inlined the string's): the string's
 * own first, then the typing's interval and length (100000 when none), the window's height, the
 * spacing, the text emptied, the cursor's pulse (1 when none), the cursor bitmap untextured and
 * opaque, and its two flags */
void CTeleType::LoadSpec(const CTtySpec &spec)
{
    CRenderableString::LoadSpec(spec);
    m_interval = spec.m_interval;
    m_e8 = 0.0f;
    m_f0 = 0.0f;
    m_maxLength = spec.m_maxLength;
    m_shown = spec.m_maxHeight;
    m_height = spec.m_spacing;
    if (!m_maxLength)
        m_maxLength = 100000.0f;
    C2DString::SetString("", true);
    m_pulseRate = spec.m_pulseRate;
    if (m_pulseRate == 0.0f)
        m_pulseRate = 1.0f;
    m_cursor.SetHasTexture(false);
    m_cursor.SetTrans(128.0f);
    m_showCursor = !spec.m_noCursor;
    m_fadeCursor = spec.m_fadeCursor;
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

/* slot 2: the 2D object it points at drawn instead, if any; else the bitmap (CPlainBmp's Draw
 * inlined), its two parts at their offsets from the corner the matrix places when they are shown,
 * and its text at its offset (SOCOM 1's CPopUpDialog::Draw).
 * OPEN, 22 instructions, all in the two parts' block: the target keeps &p.y, &p.z and the three
 * translation cells' addresses in s6, s5, s4, s3, s1 (p.z's address numbered before the matrix's)
 * and loads p.z with the other loads before any store, for both parts; here p.z's address comes
 * after two of the matrix's and its load sinks to its store. The text's block and everything else
 * is the target's. Tried: the cells assigned straight (interleaved stores), a CPnt3D built by its
 * constructor (y and z swapped: arguments right to left), the parts as an array of 0x450-byte
 * {object, offset} walked by a loop (not unrolled, two registers more), an inline setter taking the
 * three floats (23) */
void CPopUpDialog::Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr)
{
    CPnt3D p = CPnt3D::zero;

    m.Transform(&p, 1);
    if (m_32c) {
        m_32c->Draw(m, mgr);
        return;
    }
    CPlainBmp::Draw(m, mgr);
    if (m_330) {
        CMatrix mat;
        CPnt3D pos;

        pos.x = p.x + m_340Offset.x;
        pos.y = p.y + m_340Offset.y;
        pos.z = p.z;
        mat(3, 0) = pos.x;
        mat(3, 1) = pos.y;
        mat(3, 2) = pos.z;
        m_340.Draw(mat, mgr);
        pos.x = p.x + m_790Offset.x;
        pos.y = p.y + m_790Offset.y;
        pos.z = p.z;
        mat(3, 0) = pos.x;
        mat(3, 1) = pos.y;
        mat(3, 2) = pos.z;
        m_790.Draw(mat, mgr);
    }
    CPnt3D pos;

    pos.x = p.x + m_textOffset.x;
    pos.y = p.y + m_textOffset.y;
    pos.z = p.z;
    CMatrix mat;

    mat(3, 0) = pos.x;
    mat(3, 1) = pos.y;
    mat(3, 2) = pos.z;
    m_text.Draw(mat, mgr);
}

/* slot 2: the wrapped lines' strings laid out from the corner the matrix places, side by side
 * along a row until the width is passed (that string then starts the next row), the rows
 * scrolled up by the scroll position and faded at the window's edges -- out of sight beyond a
 * row past either, in proportion within it (SOCOM 1's CScrollText::Draw; SOCOM II places each
 * string with SetPos) */
void CScrollText::Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr)
{
    CPnt3D p = CPnt3D::zero;

    m.Transform(&p, 1);
    int line = 0;
    float bottom = p.y + m_25c * GetSpacing();
    float x = 0.0f;
    iterator e = end();

    for (iterator i = begin(); i != e;) {
        float y = p.y + line * GetSpacing() - m_scroll;
        C2DString *str = (C2DString *)*i;

        str->SetPos((int)(p.x + x), (int)y);
        if (!(x <= m_width)) {
            x = 0.0f;
            line++;
            continue;
        }
        ++i;
        x += str->m_length;
        if (y < p.y) {
            if (y < p.y - GetSpacing())
                str->SetTrans(0.0f);
            else {
                y -= p.y - GetSpacing();
                str->SetTrans(100.0f * (y / GetSpacing()));
            }
        } else if (!(y <= bottom - GetSpacing())) {
            if (!(y <= bottom))
                str->SetTrans(0.0f);
            else
                str->SetTrans(100.0f * ((bottom - y) / GetSpacing()));
        } else
            str->SetTrans(100.0f);
        str->Draw(mgr, 0, 0);
    }
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
        char *caption = CUIVariable::Format(UIVarMixin::m_uivar, buf, 10499);

        _szCaption = caption;
        if (caption) {
            _strCaption.SetString(caption, true);
            _strCaption.UpdatePos();
        }
    }
}

/* the slider drawn: the button, then its knob, placed by the matrix and along the width by the
 * value, drawn with no matrix of its own (SOCOM 1's CSliderBar::Draw, slot 2: the button's Draw
 * inlined, the corner transformed out of line in SOCOM II) */
void CSliderBar::Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr)
{
    CPnt3D p = CPnt3D::zero;

    m.Transform(&p, 1);
    CMatrix mat;

    C3StateButton::Draw(m, mgr);
    int y = (int)p.y;
    int x = (int)(p.x + m_value * _fWidth);

    m_knob.Load(x, y, x + m_knobW, y + m_knobH, m_knobTex);
    m_knob.Draw(mat, mgr);
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
/* set up from its spec (SOCOM 1's CHProgressBar::LoadSpec, 0x38B890 there): the plain bitmap's own
 * (the pulse, the spec's variable watched, the bitmap loaded), the variable watched again, the
 * full width the spec's, empty, and the spec's text kept */
void CHProgressBar::LoadSpec(const CHProgressBarSpec &spec, zdb::CTexHandle *tex)
{
    CPlainBmp::LoadSpec(spec, tex);
    CUIVariable *var = spec.GetUIVar();

    if (var)
        WatchUIVar(var);
    m_width = spec.m_sizex;
    m_fill = 0.0f;
    m_str = spec.m_text.c_str();
}

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

/* set up from an image spec and its texture (SOCOM 1's, 0x390D40 there): the pulse, the spec's
 * variable made and watched, the bitmap loaded at the origin in the spec's size */
void CPlainBmp::LoadSpec(const CImageSpec &spec, zdb::CTexHandle *tex)
{
    m_pulsing = spec.m_pulse;
    m_period = spec.m_pulseRate;
    if (m_pulsing)
        m_curtrans = 0.0f;
    CUIVariable *var = spec.GetUIVar();

    if (var)
        WatchUIVar(var);
    Load(0.0f, 0.0f, spec.m_sizex, spec.m_sizey, tex);
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

/* built with no size, font, press or actions: the bases', the four states' and the background's
 * inline constructors (SOCOM 1's C2DButton(), 0x38FA10 there; CButtonSpec::CreateDisplayObj's) */
C2DButton::C2DButton() : _fWidth(0.0f), _fHeight(0.0f), m_alternateFont(0), m_pressTime(0.0f), m_numActions(0)
{
}

/* the children deleted, each taken off the list first (~CUIMapCtrl's walk, under a test of the
 * list's size), the alternate font deleted and the actions forgotten (SOCOM 1's ~C2DButton, 0x38F780
 * there: the same body), then the members and the bases */
C2DButton::~C2DButton()
{
    if (size()) {
        iterator i;
        iterator e = end();

        for (i = begin(); i != e;) {
            C2D *p = *i;
            iterator next;

            next = erase(i);
            delete p;
            i = next;
        }
    }
    if (m_alternateFont)
        delete m_alternateFont;
    m_numActions = 0;
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

/* the caption: from the string table when the command carries an id, else the animation's name,
 * then the string updated -- done at once; VALVE: the valve's value printed, "0" when the name is
 * empty (as the button's); anything else not the string's */
bool CRenderableString::TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done)
{
    C2DAnimCmdHdr *cmd = (C2DAnimCmdHdr *)hdr;
    CZAnim *anim = ZAnim.m_CurAnim;

    switch (cmd->m_type - C2D::GetZAnimCmdType(0)) {
    case 1:
        if (cmd->m_stringid)
            SetStringId(cmd->m_stringid);
        else
            SetString(anim->GetName(cmd->m_arg), true);
        Update();
        return *done = true;
    case 2: {
        char buf[64] = "0";
        const char *name = anim->GetName(cmd->m_arg);

        if (name && *name)
            sprintf(buf, "%d", CValve::Create(name, VALVE_PERM)->m_value);
        SetString(buf, true);
        return true;
    }
    default:
        return false;
    }
}

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
            PressActive();
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

/* set up from its spec and its knob's texture (SOCOM 1's CSliderBar::LoadSpec, 0x38C0A0 there): the
 * button's own first, the knob untextured when there is no texture, loaded white and opaque at the
 * origin in its size, the centre value and the intervals, the spec's variable watched, and the
 * slider brought to its variable's value */
void CSliderBar::LoadSpec(const CSliderBarSpec &spec, zdb::CTexHandle *tex)
{
    C2DButton::LoadSpec(spec);
    if (!tex)
        m_knob.SetHasTexture(false);
    m_knobTex = tex;
    m_knobW = spec.m_knobSize.x;
    m_knobH = spec.m_knobSize.y;
    m_knob.Load(0.0f, 0.0f, m_knobW, m_knobH, m_knobTex);
    m_knob.SetColor(255.0f, 255.0f, 255.0f);
    m_knob.SetTrans(128.0f);
    m_center = spec.m_centerValue;
    m_intervals = spec.m_numIntervals;
    CUIVariable *var = spec.GetUIVar();

    if (var)
        WatchUIVar(var);
    if (UIVarMixin::m_uivar)
        UpdateUIVar(UIVarMixin::m_uivar);
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
_zrdr *FetchMissionFontsRdr();

/* set up from its spec (SOCOM 1's CRenderableString::LoadSpec, 0x390FB0 there): the scale and the
 * caption, the justification, the colour, a font of its own when the spec names one (made and
 * loaded from the mission's fonts), deaf to the pad, the spec's variable watched */
void CRenderableString::LoadSpec(const CTextSpec &spec)
{
    m_scale = spec.m_scale;
    SetString(spec.m_caption.c_str(), true);
    m_isCentered = spec.m_hcentered;
    m_socom2_6a = spec.m_rightJustified;
    m_Color = spec.GetColor();
    SetColor(m_Color.x, m_Color.y, m_Color.z);
    if (spec.m_hasFont) {
        const char *name = spec.m_font.c_str();

        m_alternateFont = new C2DFont;
        m_alternateFont->Load(FetchMissionFontsRdr(), name);
        SetFont(m_alternateFont);
    }
    SetCanHandleInput(false);
    CUIVariable *var = spec.GetUIVar();

    if (var)
        WatchUIVar(var);
}

/* set up from its spec (SOCOM 1's CWrappedText::LoadSpec): the width, the spacing and the window's
 * height first, then the string's own, the arrows' textures made, their places, the scroll and
 * the arrows pulsing once a second, and the one-line display */
void CWrappedText::LoadSpec(const CWrapSpec &spec, zdb::CTextureRelocMgr *mgr)
{
    m_width = spec.m_width;
    m_height = spec.m_spacing;
    m_shown = (int)spec.m_height;
    CRenderableString::LoadSpec(spec);
    m_upTex = PrepTexture(spec.m_upTexture.c_str(), mgr);
    m_downTex = PrepTexture(spec.m_downTexture.c_str(), mgr);
    m_downOffset = spec.m_downOffset;
    m_downSize = spec.m_downSize;
    m_upOffset = spec.m_upOffset;
    m_upSize = spec.m_upSize;
    m_autoScroll = spec.m_autoScroll;
    m_downArrow.m_pulsing = true;   // SetPulse(1.0f)'s arm, written out (SetPulse itself stays a call)
    m_downArrow.m_period = 1.0f;
    m_upArrow.m_pulsing = true;
    m_upArrow.m_period = 1.0f;
    m_displayOneLine = spec.m_displayOneLine;
}

/* the lines deleted, each taken off the list first (SOCOM 1's ~CWrappedText, 0x391480 there: the
 * same walk, ~CUIMapCtrl's shape), then the arrows and the renderable string */
CWrappedText::~CWrappedText()
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
}

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
class CUIMapCtrlAnimCmdHdr;

/* a design's object (zui_spec.h's CDesignDlgObj, 0x28 bytes: its spec and its place, then the
 * opacity and two strings) */
class CDesignDlgObj {
public:
    CDlgObjSpec *m_spec;   // 0x0: SPEC
    int m_xpos;   // 0x4: XPOS
    int m_ypos;   // 0x8: YPOS
    unsigned char _c[0x28 - 0xc];
};   // 0x28 bytes

/* the map control's spec (zui_spec.h's CUIMapCtrlSpec, 0x30 bytes: its corners, its elements and
 * its animations) */
class CUIMapCtrlSpec : public CDlgObjSpec {
public:
    virtual ~CUIMapCtrlSpec();

    IPNT2D m_upperLeft;   // 0x8: UPPERLEFT
    IPNT2D m_lowerRight;   // 0x10: LOWERRIGHT
    std::vector<CDesignDlgObj> m_elements;   // 0x18: ELEMENTS
    std::vector<ActionItem> m_animations;   // 0x24: ANIMATIONS
};   // 0x30 bytes

/* set when a menu object takes actions (zui.cpp's global, SOCOM 1's g_hasAnims at gp-25344 there;
 * OVL1 bss 0x49EAA0) */
extern unsigned char g_hasAnims;

class CUIMapCtrl : public C2D {
public:
    CUIMapCtrl();
    virtual ~CUIMapCtrl();
    /* the rectangle, the actions and the children from the spec (SOCOM 1's, the library unused) */
    void LoadSpec(const CUIMapCtrlSpec &spec, C2DFont *font, zdb::CAssetLib *lib,
                  zdb::CTextureRelocMgr *mgr);
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr);
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    virtual int GetZAnimCmdType(const char *name);
    /* the frame time kept for the pad's zooms (SOCOM II's: slot +0x38 of the table, C2D's Tick) */
    virtual void Tick(float dT);
    /* a pan step (SOCOM 1's OnPan, 0x36CA30) and the two zooms towards a node (SOCOM II's,
     * 0x36C3B0 and 0x36BFF0: the scale first, the node, the time -- the call's argument order;
     * the names ours) */
    void OnPan(const CUIMapCtrlAnimCmdHdr &cmd, float dT) const;
    void ZoomIn(float scale, zdb::CNode *node, float dT);
    void ZoomOut(float scale, zdb::CNode *node, float dT);
    virtual bool HandlePad(const CPad *pad, int port);
    /* whether a screen point lies on one of the children's bitmaps, the map placed at pos and
     * scaled (SOCOM 1's IsCovered, 0x386400: OnPan and the zooms test their corners with it) */
    bool IsCovered(float x, float y, const CPnt3D &pos, float scale) const;
    /* all four corners covered (ours: ZoomOut's two tests, each corner's x converted once; the
     * answer a bool and-ed a corner at a time -- the target re-tests it at each step, where one `&&`
     * chain branches straight through) */
    bool AllCovered(const CPnt3D &pos, float scale) const
    {
        float l = m_left;
        float r = m_right;
        bool c = IsCovered(l, m_top, pos, scale);

        c = c && IsCovered(r, m_top, pos, scale);
        c = c && IsCovered(l, m_bottom, pos, scale);
        c = c && IsCovered(r, m_bottom, pos, scale);
        return c;
    }
    /* the zoomed-out position held against the side the four corners' covered flags leave (SOCOM
     * II's: ZoomOut's, 0x36D510; the name ours) */
    void AnchorZoom(CPnt3D &out, const bool *covered, const CPnt3D &pos, float zoom);

    ActionItem m_actions[30];   // 0x18
    int m_numActions;   // 0x1f8 (the destructor zeroes it)
    /* the control's rectangle on screen (SOCOM II's; the zooms centre on it; the names ours) */
    int m_left;   // 0x1fc
    int m_top;   // 0x200
    int m_right;   // 0x204
    int m_bottom;   // 0x208
    zdb::CNode *m_node;   // 0x20c: the map's node (the pad's zooms move it)
};

/* built with no actions and no node (SOCOM 1's CUIMapCtrl(), 0x386DA0 there: CUIMapCtrlSpec::
 * CreateDisplayObj calls it, then LoadSpec) */
CUIMapCtrl::CUIMapCtrl() : m_numActions(0), m_node(0)
{
}

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

/* a pan of the map control as its command keeps it (SOCOM 1's CUIMapCtrlAnimCmdHdr: SOCOM II's 32
 * bytes, the direction and the three rates after the 16-byte header; the names ours) */
class CUIMapCtrlAnimCmdHdr : public C2DAnimCmdHdr {
public:
    int m_dir;   // 0x10: UP, DOWN, LEFT, RIGHT
    int m_speed;   // 0x14
    int m_accel;   // 0x18
    int m_limit;   // 0x1c
};   // 0x20 bytes

/* the map's node as the zooms move it (znode.h's CNode: its matrix's translation row, the
 * position and scale accessors) */
namespace zdb {
class CNode {
public:
    float GetScale();
    void SetScale(float s);
    void SetPosition(float x, float y, float z);
    void SetPosition(const CPnt3D &p) { SetPosition(p.x, p.y, p.z); }
    CPnt3D GetPosition() const { return *(const CPnt3D *)m_matrix.m_matrix[3]; }

    CMatrix m_matrix;   // 0x0
};
}

/* the menu being shown (zui.cpp's UiGetCurMenu, 0x36A200, named by the integrator after the UI's round 18): a node, its vtable
 * pointer at 0x60 as CNode's -- slot +0x24 the child found by name (znode.h's order) */
class CMenuNode {
public:
    unsigned char _0[0x60];
    virtual ~CMenuNode();
    virtual void _Copy();
    virtual void UpdateGrid();
    virtual void ReserveChildren(int n);
    virtual void AddChild(zdb::CNode *child);
    virtual int DeleteChild(zdb::CNode *child);
    virtual void DeleteChildren();
    virtual zdb::CNode *FindChild(const char *name, bool deep);
};

CMenuNode *UiGetCurMenu();

/* the GS scissor rectangle (zvid) */
void zVid_Scissor(int x1, int y1, int x2, int y2);

/* slot 2: the bitmap drawn scissored to the filled part -- the bar's corner and the fill's corner
 * placed by the matrix -- and the menu's node of the bar's name moved to the fill's edge (SOCOM
 * 1's CHProgressBar::Draw: CPlainBmp::Draw inlined, the corners transformed out of line in SOCOM
 * II) */
void CHProgressBar::Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr)
{
    CPnt3D p[2];

    p[0].x = 0.0f;
    p[0].y = 0.0f;
    p[0].z = 0.0f;
    p[1].x = m_width * m_fill;
    p[1].y = m_iHeight;
    p[1].z = 0.0f;
    m.Transform(p, 2);
    mgr->PreGetSprPacket(0);
    zVid_Scissor((int)p[0].x, (int)p[0].y, (int)p[1].x, (int)p[1].y);
    CPlainBmp::Draw(m, mgr);
    mgr->PreGetSprPacket(0);
    zVid_Scissor(0, 0, 640, 448);
    CMenuNode *menu = UiGetCurMenu();
    zdb::CNode *node = menu->FindChild(m_str.c_str(), true);

    if (node)
        node->SetPosition(p[1].x, p[0].y, 0.0f);
}

/* a position zoomed about a point (ours: the zooms' step, the position copied and its x and y
 * pulled toward the point) */
inline CPnt3D ZoomAbout(const CPnt3D &pos, float x, float y, float zoom)
{
    CPnt3D p = pos;

    p.x = x - zoom * (x - pos.x);
    p.y = y - zoom * (y - pos.y);
    return p;
}

/* the node zoomed in about the control's centre, by half the time's step at the given rate, no
 * further than twice its unit scale: its position pulled out from the centre by the step, its
 * scale multiplied by it (SOCOM II's; the name ours). The clamp is a MIN ternary (`if (!(z < m))
 * z = m;` inverts the branch); the position's copies are the inline ZoomAbout's -- the value
 * initialised from GetPosition, the inline's copy of it block-copied, its x and y computed from the
 * value's fields, returned by a block copy and assigned back field by field (AnchorZoom's tail is
 * the same inline) -- byte-exact since the HUD's round 39 (open since round 30). Pinned: ten
 * statements, -inline auto would take it into TickZAnimCmd and HandlePad, which call it */
#pragma auto_inline off
void CUIMapCtrl::ZoomIn(float scale, zdb::CNode *node, float dT)
{
    float cx = 0.5f * (m_left + m_right);
    float cy = 0.5f * (m_top + m_bottom);
    float zoom = 1.0f + 0.5f * dT * scale;
    float maxzoom = 2.0f / node->GetScale();

    zoom = zoom < maxzoom ? zoom : maxzoom;

    CPnt3D pos = node->GetPosition();

    pos = ZoomAbout(pos, cx, cy, zoom);
    node->SetPosition(pos.x, pos.y, pos.z);
    node->SetScale(zoom * node->GetScale());
}
#pragma auto_inline reset

/* the point a zoom out holds still, by which of the corners (top left, top right, bottom left,
 * bottom right: bits 3 to 0) are still covered -- a corner or an edge's middle, none when no corner
 * or every corner is (SOCOM II's table, 0x3F8F50) */
static const unsigned char s_zoomAnchor[16] = {0, 1, 2, 5, 3, 6, 0, 1, 4, 0, 7, 2, 8, 3, 4, 0};

/* the position zoomed out about that point (SOCOM II's: ZoomOut's, when some corner is no longer
 * covered; the name ours) */
void CUIMapCtrl::AnchorZoom(CPnt3D &out, const bool *covered, const CPnt3D &pos, float zoom)
{
    float x, y;

    switch (s_zoomAnchor[covered[0] << 3 | covered[1] << 2 | covered[2] << 1 | covered[3]]) {
    case 1:
        x = m_left;
        y = m_top;
        break;
    case 2:
        x = m_right;
        y = m_top;
        break;
    case 3:
        x = m_left;
        y = m_bottom;
        break;
    case 4:
        x = m_right;
        y = m_bottom;
        break;
    case 5:
        x = 0.5f * (m_left + m_right);
        y = m_top;
        break;
    case 8:
        x = 0.5f * (m_left + m_right);
        y = m_bottom;
        break;
    case 6:
        x = m_left;
        y = 0.5f * (m_top + m_bottom);
        break;
    case 7:
        x = m_right;
        y = 0.5f * (m_top + m_bottom);
        break;
    default:
        return;
    }
    out = ZoomAbout(pos, x, y, zoom);
}

/* the frame the last pan step was taken in and how long the pan has run (SOCOM II's statics, OVL1
 * 0x3E0430 and bss 0x49EBF0; the names ours) */
static int s_panFrame = -1;
static float s_panTime;

/* a step of the pan: the animation's node moved in the command's direction by its speed and its
 * acceleration over the pan's time, no more than its limit, while the map still covers the
 * control's corners; when it would not, crept from where it is by the speed alone for as long as
 * it does (SOCOM 1's CUIMapCtrl::OnPan, its time SOCOM II's). `frame <= s_panFrame + 1` gives the
 * target's `slt at` (the reversed compare materialises into v0), `if (step > max)` its one-arm clamp
 * (a MIN ternary is branchy here) */
void CUIMapCtrl::OnPan(const CUIMapCtrlAnimCmdHdr &cmd, float dT) const
{
    ZAnim.m_CurAnim->Refresh(false);

    zdb::CNode *node = ZAnim.m_CurAnim->GetNode(cmd.node_ref_index, false);
    CPnt3D pos = node->GetPosition();
    int frame = zVid.frameNumber;
    float t;

    if (frame <= s_panFrame + 1)
        t = s_panTime + dT;
    else
        t = 0.0f;
    s_panFrame = frame;
    s_panTime = t;

    float step = cmd.m_speed * dT + cmd.m_accel * t;
    float max = cmd.m_limit * dT;

    if (step > max)
        step = max;
    switch (cmd.m_dir) {
    case 0:
        pos.y += step;
        break;
    case 1:
        pos.y -= step;
        break;
    case 2:
        pos.x += step;
        break;
    case 3:
        pos.x -= step;
        break;
    }
    if (AllCovered(pos, node->GetScale())) {
        node->SetPosition(pos.x, pos.y, pos.z);
        return;
    }

    CPnt3D p = node->GetPosition();

    for (;;) {
        CPnt3D q = p;

        switch (cmd.m_dir) {
        case 0:
            q.y += cmd.m_speed * dT;
            break;
        case 1:
            q.y -= cmd.m_speed * dT;
            break;
        case 2:
            q.x += cmd.m_speed * dT;
            break;
        case 3:
            q.x -= cmd.m_speed * dT;
            break;
        }
        if (!AllCovered(q, node->GetScale()))
            break;
        p = q;
    }
    node->SetPosition(p.x, p.y, p.z);
}

/* the node zoomed out about the control's centre, by a third of the time's step at the given rate,
 * while the map still covers the control's four corners; when it would not, zoomed about the
 * point the covered corners leave instead, and only if that covers them all (SOCOM II's; the name
 * ours) */
void CUIMapCtrl::ZoomOut(float scale, zdb::CNode *node, float dT)
{
    float cx = 0.5f * (m_left + m_right);
    float cy = 0.5f * (m_top + m_bottom);
    float zoom = 1.0f - 0.33333f * dT * scale;
    const CPnt3D &cur = *(const CPnt3D *)node->m_matrix.m_matrix[3];
    CPnt3D pos = ZoomAbout(cur, cx, cy, zoom);
    float newscale = zoom * node->GetScale();

    if (AllCovered(pos, newscale)) {
        node->SetPosition(pos.x, pos.y, pos.z);
        node->SetScale(newscale);
        return;
    }

    float l = m_left;
    float r = m_right;
    bool tl = IsCovered(l, m_top, pos, newscale);
    bool tr = IsCovered(r, m_top, pos, newscale);
    bool bl = IsCovered(l, m_bottom, pos, newscale);
    bool br = IsCovered(r, m_bottom, pos, newscale);
    bool covered[4];

    covered[0] = tl;
    covered[1] = tr;
    covered[2] = bl;
    covered[3] = br;
    AnchorZoom(pos, covered, cur, zoom);
    if (AllCovered(pos, newscale)) {
        node->SetPosition(pos.x, pos.y, pos.z);
        node->SetScale(newscale);
    }
}

/* the control's rectangle from the spec's corners, its actions copied (each marking the menu as
 * having actions), and each element made from its spec, placed at its offset and added as a child
 * (SOCOM 1's CUIMapCtrl::LoadSpec, 0x3866C0 there; the library unused) */
void CUIMapCtrl::LoadSpec(const CUIMapCtrlSpec &spec, C2DFont *font, zdb::CAssetLib *lib,
                          zdb::CTextureRelocMgr *mgr)
{
    IPNT2D ul = spec.m_upperLeft;

    m_left = ul.x;
    m_top = ul.y;

    IPNT2D lr = spec.m_lowerRight;

    m_right = lr.x;
    m_bottom = lr.y;
    for (int i = 0; i < (int)spec.m_animations.size(); i++) {
        g_hasAnims = 1;
        m_numActions++;
        m_actions[i] = spec.m_animations[i];
    }
    for (int i = 0; i < (int)spec.m_elements.size(); i++) {
        const CDesignDlgObj &e = spec.m_elements[i];
        C2D *obj = e.m_spec->CreateDisplayObj(font, mgr);

        obj->SetMapOffset(e.m_xpos, e.m_ypos);
        push_back(obj);
    }
}

/* each child that is a plain bitmap, its corner and size scaled about pos, tested against the
 * point (SOCOM 1's CUIMapCtrl::IsCovered, the walk's const pieces inlined in SOCOM II) */
bool CUIMapCtrl::IsCovered(float x, float y, const CPnt3D &pos, float scale) const
{
    for (const_iterator i = begin(); i != end(); ++i) {
        CPlainBmp *bmp = dynamic_cast<CPlainBmp *>(*i);

        if (bmp) {
            float left = pos.x + scale * bmp->m_x;
            float top = pos.y + scale * bmp->m_y;
            float right = left + scale * bmp->m_iWidth;
            float bottom = top + scale * bmp->m_iHeight;
            bool inx = false;

            if (left <= x && x <= right)
                inx = true;

            bool iny = false;

            if (top <= y && y <= bottom)
                iny = true;
            if (inx && iny)
                return true;
        }
    }
    return false;
}

/* PAN: a step of the pan; ZOOM_IN and ZOOM_OUT: the current animation refreshed and the zoom made
 * towards the command's node; each done at once (SOCOM 1's PAN; the zooms SOCOM II's) */
bool CUIMapCtrl::TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done)
{
    C2DAnimCmdHdr *cmd = (C2DAnimCmdHdr *)hdr;

    switch (cmd->m_type - C2D::GetZAnimCmdType(0)) {
    case 1:
        OnPan(*(CUIMapCtrlAnimCmdHdr *)cmd, *dT);
        return *done = true;
    case 2: {
        ZAnim.m_CurAnim->Refresh(false);
        zdb::CNode *node = ZAnim.m_CurAnim->GetNode(cmd->node_ref_index, false);

        ZoomIn(1.0f, node, *dT);
        return *done = true;
    }
    case 3: {
        ZAnim.m_CurAnim->Refresh(false);
        zdb::CNode *node = ZAnim.m_CurAnim->GetNode(cmd->node_ref_index, false);

        ZoomOut(1.0f, node, *dT);
        return *done = true;
    }
    default:
        return false;
    }
}

/* the menus' frame time as the map control last ticked it: HandlePad zooms by it, having no dT of
 * its own (SOCOM II's; 0x49EBE8, the name ours) */
static float s_mapCtrlDT;

/* the frame time kept, nothing else ticked (SOCOM II's: SOCOM 1's CUIMapCtrl has no Tick) */
void CUIMapCtrl::Tick(float dT)
{
    s_mapCtrlDT = dT;
}

/* PAN with its direction and rates, or ZOOM_IN, or ZOOM_OUT, as a command (SOCOM 1's); anything
 * else none. The direction is UP unless the name says otherwise; each rate 0 unless given */
C2DAnimCmdHdr *CUIMapCtrl::ParseZAnimCmd(_zrdr *rdr)
{
    C2DAnimCmdHdr *cmd;
    _zrdr *pan = zrdr_findtag(rdr, "PAN");

    if (pan) {
        CUIMapCtrlAnimCmdHdr *pc =
            (CUIMapCtrlAnimCmdHdr *)_calloc(1, sizeof(CUIMapCtrlAnimCmdHdr), 0, 0);

        pc->header.timeless = 1;
        pc->header.data_size = sizeof(CUIMapCtrlAnimCmdHdr);
        pc->m_type = GetZAnimCmdType("PAN");

        const char *dir = zrdr_findstring(pan, "DIRECTION");

        pc->m_dir = 0;
        if (!strcmp(dir, "UP"))
            pc->m_dir = 0;
        else if (!strcmp(dir, "DOWN"))
            pc->m_dir = 1;
        else if (!strcmp(dir, "LEFT"))
            pc->m_dir = 2;
        else if (!strcmp(dir, "RIGHT"))
            pc->m_dir = 3;
        pc->m_speed = 0;
        zrdr_findint(pan, "SPEED", &pc->m_speed, 1);
        pc->m_accel = 0;
        zrdr_findint(pan, "ACCEL", &pc->m_accel, 1);
        pc->m_limit = 0;
        zrdr_findint(pan, "LIMIT", &pc->m_limit, 1);
        cmd = pc;
    } else if (zrdr_findtag(rdr, "ZOOM_IN")) {
        cmd = (C2DAnimCmdHdr *)_calloc(1, sizeof(C2DAnimCmdHdr), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(C2DAnimCmdHdr);
        cmd->m_type = GetZAnimCmdType("ZOOM_IN");
    } else if (zrdr_findtag(rdr, "ZOOM_OUT")) {
        cmd = (C2DAnimCmdHdr *)_calloc(1, sizeof(C2DAnimCmdHdr), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(C2DAnimCmdHdr);
        cmd->m_type = GetZAnimCmdType("ZOOM_OUT");
    } else
        cmd = 0;
    return cmd;
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
    /* an item added with the box's font and scale: at the end, or (any other `where`) before the
     * highlighted row (SOCOM II's, 0x373F20, the name ours: UpdateUIVar and 0x372980 call it) */
    void AddItem(ListItem *item, int where);
    /* an item's text: its action's argument, else its own string, else its look's (SOCOM II's,
     * 0x374B50, the name ours: the lobby's 0x2F0EA0 calls it) */
    const char *GetItemText(int idx);
    /* every item erased and deleted, then the box's own font (SOCOM II's, 0x375C70, the name ours:
     * UpdateUIVar and 0x372980 call it) */
    void RemoveAllItems();
    /* slot 24 of its table 0x407670: the box's eleven commands after C2D's (0x373640) */
    int GetZAnimCmdType(const char *name);

    unsigned char _0[0xa0];
    C2DFont *m_font;   // 0xa0
    unsigned char _a4[0xbc - 0xa4];
    C2DFont *m_ownFont;   // 0xbc: a font of the box's own, deleted with its items
    std::list<ListItem *> m_items;   // 0xc0
    float m_scale;   // 0xcc: the items' scale
    unsigned char _d0[0xd4 - 0xd0];
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

/* each item's node erased before the item is deleted, the next iterator kept apart and ASSIGNED
 * the erase's result (its conversion's temporary then copied through a float register; initialised
 * from it, MW builds it in place: 16 bytes short, two registers swapped); the font deleted and
 * cleared if there is one */
void CUIListBox::RemoveAllItems()
{
    std::list<ListItem *>::iterator it;

    it = m_items.begin();
    while (it != m_items.end()) {
        ListItem *item = *it;
        std::list<ListItem *>::iterator next;

        next = m_items.erase(it);

        delete item;
        it = next;
    }
    if (m_ownFont) {
        delete m_ownFont;
        m_ownFont = 0;
    }
}

/* none for an index out of range; the walk is AddItem's (the iterator assigned from begin(), in
 * memory, stepped by a counted loop MW unrolls 8x). One return of a local preset to none: early
 * returns branch out of range through a block of their own */
const char *CUIListBox::GetItemText(int idx)
{
    const char *text = 0;

    if (idx >= 0 && (unsigned int)idx < m_items.size()) {
        std::list<ListItem *>::iterator it;

        it = m_items.begin();
        for (int i = 0; i < idx; i++)
            ++it;

        ListItem *item = *it;

        if (item->m_action.Argument)
            text = item->m_action.Argument;
        else if (item->m_text)
            text = item->m_text;
        else
            text = item->GetString();
    }
    return text;
}

/* the iterator taken from begin() is a local in memory (ASSIGNED from begin(), its copy goes through
 * a float register; initialised from it, it lives in a register: 0xC4 against 0xE4), stepped on to
 * the highlighted row or the end.
 * OPEN, 6 instructions (stack slots): the target puts push_back's end and returned iterators at
 * sp+60/64, below the insert's returned one at 68; here they are at 64/68 and the insert's at 60.
 * Tried: the if/else the other way round (0xEC), the iterator declared at the top, the insert's
 * result assigned or kept, insert(end(), item) for the push_back (6 each) */
void CUIListBox::AddItem(ListItem *item, int where)
{
    item->SetFont(m_font);
    item->SetScale(m_scale);
    if (where == -1)
        m_items.push_back(item);
    else {
        std::list<ListItem *>::iterator it;

        it = m_items.begin();
        for (int i = 0; i < m_highlighted && it != m_items.end(); i++)
            ++it;
        m_items.insert(it, item);
    }
}

/* the box's commands (SOCOM 1's CUIListBox::GetZAnimCmdType: its eleven names after C2D's, which
 * C2D's inline lookup counts and falls back to; RemoveAllItems takes 11 and GetContents 10, tested
 * in that order) */
/* C2D's own lookup, inline (ztwod.h's: the null name its count) */
static inline int C2DCmdType(const char *name)
{
    return GetUICommandType(0);
}

int CUIListBox::GetZAnimCmdType(const char *name)
{
    if (!name)
        return C2DCmdType(0) + 12;
    if (!*name)
        return 0;
    if (!strcasecmp("Finish", name))
        return C2DCmdType(0) + 1;
    if (!strcasecmp("Highlight_on", name))
        return C2DCmdType(0) + 2;
    if (!strcasecmp("Highlight_off", name))
        return C2DCmdType(0) + 3;
    if (!strcasecmp("Has_Items", name))
        return C2DCmdType(0) + 4;
    if (!strcasecmp("GetItemCount", name))
        return C2DCmdType(0) + 5;
    if (!strcasecmp("GetFocusIndex", name))
        return C2DCmdType(0) + 6;
    if (!strcasecmp("SetFocusIndex", name))
        return C2DCmdType(0) + 7;
    if (!strcasecmp("AddItem", name))
        return C2DCmdType(0) + 8;
    if (!strcasecmp("RemoveItem", name))
        return C2DCmdType(0) + 9;
    if (!strcasecmp("RemoveAllItems", name))
        return C2DCmdType(0) + 11;
    if (!strcasecmp("GetContents", name))
        return C2DCmdType(0) + 10;
    return C2DCmdType(name);
}

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

/* the string table's id for a name, 0 if it has none (ztwod's, 0x361B20) */
int GetStringId(const char *name);

/* CAPTION as a command: an array of several strings joined into one name, or a single string --
 * its string table id when it has one, else the name added to the animation's; VALVE's name
 * added likewise; anything else none (SOCOM 1's took one CAPTION string) */
C2DAnimCmdHdr *CRenderableString::ParseZAnimCmd(_zrdr *rdr)
{
    C2DAnimCmdHdr *cmd;
    const ZRdrNode *r = (const ZRdrNode *)zrdr_findtag(rdr, "CAPTION");
    const char *valve = zrdr_findstring(rdr, "VALVE");

    if (r) {
        cmd = (C2DAnimCmdHdr *)_calloc(1, sizeof(C2DAnimCmdHdr), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(C2DAnimCmdHdr);
        cmd->m_type = GetZAnimCmdType("CAPTION");
        if (r->type == 4 && RdrCount(r) > 1) {
            CSaferStr name;

            name = "";
            for (int i = 0; i < RdrCount(r); i++) {
                if (RdrElem(r, i)->type == 3)
                    name += RdrString(RdrElem(r, i));
            }
            cmd->m_arg = ZAnim.m_CurAnim->AddName(name.c_str());
            cmd->m_stringid = 0;
        } else {
            const char *str = RdrString(RdrElem(r, 0));
            int id = GetStringId(str);

            if (id)
                cmd->m_stringid = id;
            else {
                cmd->m_arg = ZAnim.m_CurAnim->AddName(str);
                cmd->m_stringid = 0;
            }
        }
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

/* the strip's check boxes set to the choice: each checked one but the chosen unchecked, its second
 * action run, then the chosen one checked, its first action run (SOCOM 1's CRadioStrip::PressActive,
 * 0x38A880 there: the same two walks; SOCOM II's pad handler calls ShowChoice first) */
void CRadioStrip::PressActive()
{
    int n = 0;
    iterator e = end();
    iterator i = begin();

    for (; i != e; ++i, ++n) {
        CCheckBox *box = (CCheckBox *)*i;

        if (box && box->m_checked && n != m_choice) {
            box->m_checked = false;
            box->m_action1.Run();
        }
    }
    n = 0;
    iterator e2 = end();
    for (i = begin(); i != e2; ++i, ++n) {
        CCheckBox *box = (CCheckBox *)*i;

        if (box && !box->m_checked && n == m_choice) {
            box->m_checked = true;
            box->m_action0.Run();
        }
    }
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
    unsigned char _11[0x21c - 0x11];
    float m_zoom;   // 0x21c: a stick's axis (the map control zooms by it)
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

/* the same, whether the animation was found (the map control's walk ors the answers: an inline
 * bool, its `return true` an assignment; the name ours) */
inline bool ActionItem::Start() const
{
    CZAnim *anim = ZAnim.GetAnim(Animation, 0);

    if (!anim)
        return false;
    if (Argument)
        anim->ActivateArg(1, 8, Argument, 0);
    else
        anim->Activate(0);
    return true;
}

/* the menus' stick (zui.cpp's: whether it is within a radius of the centre, and the side it is
 * pushed to, 1-4) and the menus' frame time (zui.cpp's bss, by address) */
bool IsStickNearCenter(float r);
unsigned char GetStickDir();

/* the pad event a stick push stands for, by the side it is pushed to (ours: a byte enum -- its
 * constant stores need no masking, its compare with the action's event byte zero-extends) */
enum STICK_EVENT { STICK_EV_2 = 2, STICK_EV_3 = 3, STICK_EV_4 = 4, STICK_EV_5 = 5, STICK_EV_NONE = 16 };

/* the actions run on their pad events -- a stick action (state 2) when the stick is pushed out of
 * the centre toward its button's side -- whether any ran; then the map zoomed in or out by the
 * pad's stick (SOCOM 1's CUIMapCtrl::HandlePad, slot 15) */
bool CUIMapCtrl::HandlePad(const CPad *pad, int port)
{
    bool handled = false;

    if (!CanHandleInput())
        return false;
    for (int i = 0; i < m_numActions; i++) {
        const ActionItem *a = &m_actions[i];

        if (!m_actions[i].Event)
            continue;
        if (a->State != pad->m_state[a->Event]) {
            if (a->State != 2)
                continue;
            if (IsStickNearCenter(0.6f))
                continue;
            STICK_EVENT dir = STICK_EV_NONE;

            switch (GetStickDir()) {
            case 3:
                dir = STICK_EV_4;
                break;
            case 1:
                dir = STICK_EV_3;
                break;
            case 2:
                dir = STICK_EV_2;
                break;
            case 4:
                dir = STICK_EV_5;
                break;
            }
            if (dir != a->Event)
                continue;
        }
        handled |= a->Start();
    }
    float z = pad->m_zoom;

    if (z > 0.0f)
        ZoomIn(z, m_node, s_mapCtrlDT);
    else
        ZoomOut(-z, m_node, s_mapCtrlDT);
    return handled;
}

/* the menus (zui's: the stack the pop-up is on, the selection sound) */
class CMenuStack {
public:
    void KillPopUp();
};

CMenuStack *GetMenuStack();
void PlaySelectionSound();

/* slot 15: the pad handed to the 2D object it points at, if any; else a press of button 9 sets
 * the chosen button's answer in the valve (and shows that button pressed), the cancel button 66,
 * each closing the pop-up unless it stays up; and either side button moves the choice to the other
 * button when that one is there -- taken either way (SOCOM 1's CPopUpDialog::HandlePad) */
bool CPopUpDialog::HandlePad(const CPad *pad, int port)
{
    if (m_32c)
        return m_32c->HandlePad(pad, port);
    if (pad->m_state[9] == 1) {
        if (m_330) {
            m_330->SetValue(m_choice);
            if (m_choice == 1)
                m_340.Slot27();
            else
                m_790.Slot27();
        }
        if (!m_keep) {
            m_text.SetString("", true);
            GetMenuStack()->KillPopUp();
        }
        PlaySelectionSound();
    }
    if (m_339 && pad->m_state[6] == 1) {
        if (m_330)
            m_330->SetValue(66);
        if (!m_keep) {
            m_text.SetString("", true);
            GetMenuStack()->KillPopUp();
        }
        PlaySelectionSound();
    }
    if (pad->m_state[3] == 1 || pad->m_state[2] == 1) {
        if (m_choice == 1) {
            if (m_790._fWidth > 0.0f) {
                m_choice = 2;
                m_340.SetNormal();
                m_790.SetActive();
                PlaySelectionSound();
            }
        } else if (m_340._fWidth > 0.0f) {
            m_choice = 1;
            m_340.SetActive();
            m_790.SetNormal();
            PlaySelectionSound();
        }
    }
    return true;
}

/* slot 15: while it takes input, left steps the choice back and right or cross on, round the
 * strip; the buttons shown, the check boxes set and the selection sound played -- taken (SOCOM 1's
 * CRadioStrip::HandlePad, 0x38AA70 there, with SOCOM II's ShowChoice before PressActive) */
bool CRadioStrip::HandlePad(const CPad *pad, int port)
{
    if (!CanHandleInput())
        return false;
    if (pad->m_state[3] == 1) {   // left
        if (m_choice == 0)
            m_choice = size() - 1;
        else
            m_choice--;
        ShowChoice();
        PressActive();
        PlaySelectionSound();
        return true;
    }
    if (pad->m_state[2] == 1) {   // right
        if (m_choice >= size() - 1)
            m_choice = 0;
        else
            m_choice++;
        ShowChoice();
        PressActive();
        PlaySelectionSound();
        return true;
    }
    if (pad->m_state[9] == 1) {   // cross
        if (m_choice >= size() - 1)
            m_choice = 0;
        else
            m_choice++;
        ShowChoice();
        PressActive();
        PlaySelectionSound();
        return true;
    }
    return false;
}

/* set up from its spec: the button's own (SOCOM 1's CHandlerButton::LoadSpec, 0x38EC90 there,
 * the same eight bytes) */
void CHandlerButton::LoadSpec(const CHandlerButtonSpec &spec)
{
    C2DButton::LoadSpec(spec);
}

/* set up from its spec, by address (SOCOM 1's CRadioStrip::LoadSpec, 0x38AD50 there, the same twelve
 * bytes): the flag set, then the button's own; the texture unused */
void CRadioStrip::LoadSpec(const CRadioStripSpec *spec, zdb::CTexHandle *tex)
{
    m_444 = true;
    C2DButton::LoadSpec(*spec);
}

/* nothing of its own to set up (SOCOM II's: SOCOM 1's CScrollText::LoadSpec read DISPLAYABLELINES
 * and called the string's; SOCOM II's CScrollSpec::CreateDisplayObj calls it last, empty) */
void CScrollText::LoadSpec(const CScrollSpec &spec)
{
}

/* set up from its spec and its check's texture (SOCOM 1's CCheckBox::LoadSpec, 0x38B380 there): the
 * button's own first, the check's place and size, the two actions copied, the check bitmap loaded
 * white and 100 opaque at the origin in its size, the spec's variable watched */
void CCheckBox::LoadSpec(const CCheckBoxSpec &spec, zdb::CTexHandle *tex)
{
    C2DButton::LoadSpec(spec);
    m_checkX = spec.m_checkOffset.x;
    m_checkY = spec.m_checkOffset.y;
    m_checkW = spec.m_checkSize.x;
    m_checkH = spec.m_checkSize.y;
    m_action0 = *spec.m_onCheck;
    m_action1 = *spec.m_onUncheck;
    m_check.Load(0.0f, 0.0f, m_checkW, m_checkH, tex);
    m_check.SetColor(255.0f, 255.0f, 255.0f);
    m_check.SetTrans(100.0f);
    CUIVariable *var = spec.GetUIVar();

    if (var)
        WatchUIVar(var);
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
