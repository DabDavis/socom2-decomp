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

class CUIVariable;

/* a ZRDR value as the UI variable's base (zrdr.h's _zrdr: its type, flags, count and value; the
 * flags' packed bit as the bit-field it is there). Its destructor is declared (0x32F300): a list
 * box's column holds one by value and the box's destructor tears it down (round 56) */
struct _zrdr {
    ~_zrdr();
    void DeleteChildren();

    unsigned char m_type;   // 0x0
    unsigned char m_isclone : 1;   // 0x1
    unsigned char m_packed : 1;
    unsigned char m_flags : 6;
    unsigned short m_length;   // 0x2: an array's count (zrdr.h's 16-bit length)
    union {
        const char *m_str;   // 0x4
        float m_real;
        int m_int;
        CUIVariable *m_array;   // an array's items, 8 bytes each
    };
};

/* a UI variable as the plain bitmap reads it: its type (3 a string) and the string */
class CUIVariable : public _zrdr {
public:
    void UpdateDependents();
    /* the value replaced by a copy of an rdr's (zui_var.cpp's, 0x386460) */
    void Set(_zrdr *value);
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

    /* an array's count and its items (zrdr.h's _zrdr::Count and Item: 4 an array) */
    bool IsArray() const { return m_type == 4; }
    unsigned int Count() const { return IsArray() ? m_length : 0; }
    CUIVariable *Item(unsigned int i) const { return i < Count() ? &m_array[i] : 0; }
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
     * secondary tables' offsets stand). Pure: the mixin's table 0x4050B0 holds 0 in its slot */
    virtual void UpdateUIVar(CUIVariable *var) = 0;

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
    CUIVariable *Find(const char *name);
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
    /* the variable ignored (0x37C1A0, empty: the slot after the tables, +0x80, and the mixin's
     * through the thunk 0x37D100) */
    virtual void UpdateUIVar(CUIVariable *var);
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
 * first: defined here since round 42 (0x379D90), so this unit takes its table and the thunk 0x37D010 */
class CWrappedText : public CRenderableString {
public:
#if Z_PORT
    /* the port: zui_2d.h's inline constructor out of line, as the image's copy 0x27F250 is
     * (zanim_menu.h's view calls it; the host emits an implicit one only as a base's) */
    CWrappedText();
#endif
    virtual ~CWrappedText();
    virtual void SetString(const char *s, bool translate);
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr);
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    virtual int GetZAnimCmdType(const char *name);
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    /* the lines laid out (ours: an accessor, the count read before the spacing's call) */
    int GetNumLines() { return m_numLines; }

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

    /* a spec's look taken over (inline: C2DButton::LoadSpec copies its four so, as SOCOM 1's -- the
     * fields, then the image's name through its text, the scale last). The name and the scale
     * through a reference of their own: the target builds the look's address for the string's
     * c_str and keeps it across the assignment for the scale (SOCOM 1's data() call did the same);
     * read through `s` MW folds both into the spec's offsets, and the reference also gives the
     * target's saved registers (this s1, the spec s0) */
    const char *TexName() const { return m_texName.c_str(); }
    void operator=(const ButtonState &s)
    {
        m_tex = s.m_tex;
        m_fadeMin = s.m_fadeMin;
        m_fadeMax = s.m_fadeMax;
        m_pulse = s.m_pulse;
        m_period = s.m_period;
        m_fadeTime = s.m_fadeTime;
        m_color = s.m_color;

        const ButtonState &n = s;

        m_texName = n.m_texName.c_str();
        m_scale = n.m_scale;
    }

    /* a look being built taken over (inline: the pop-up's command, through the button's setters --
     * apart from LoadSpec's, as in SOCOM 1, whose two copies call different string members): the
     * same fields, but the image's name through an accessor. The string's c_str is the argument of
     * an inline statement, whose arguments wait a pass, so the accessor and the setter put it past
     * the last pass: the target's six out-of-line calls of OVL1's copy 0x36E860; read through a
     * parameter the scale folds into the look's frame offset */
    void Set(const ButtonState &s)
    {
        m_tex = s.m_tex;
        m_fadeMin = s.m_fadeMin;
        m_fadeMax = s.m_fadeMax;
        m_pulse = s.m_pulse;
        m_period = s.m_period;
        m_fadeTime = s.m_fadeTime;
        m_color = s.m_color;
        m_texName = s.TexName();
        m_scale = s.m_scale;
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
    /* an event a menu runs its actions on (zui_spec.h's enum: only the slider's, 40) */
    enum EVENT { SLIDER_MOVED = 40 };
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

/* a button's spec (zui_spec.h's CButtonSpec, 0x118 bytes, its names: its PRESSED look the first of
 * the four, as SOCOM 1's C2DButton's _PressedState, the one C2DButton's m_active below takes) and a
 * check box's (CCheckBoxSpec: the two actions, the check's place and size, its texture's name) */
class CButtonSpec : public CDlgObjSpec {
public:
    IPNT2D m_size;   // 0x8: XSIZE, YSIZE or SIZE
    IPNT2D m_offset;   // 0x10: XOFFSET, YOFFSET or OFFSET
    float m_scale;   // 0x18: SCALE
    ButtonState m_pressed;   // 0x1c: PRESSED
    ButtonState m_active;   // 0x50: ACTIVE
    ButtonState m_normal;   // 0x84: NORMAL
    ButtonState m_disabled;   // 0xb8: DISABLED
    CSaferStr m_caption;   // 0xec: CAPTION
    bool m_hcentered;   // 0xf8: HCENTERED
    std::vector<ActionItem> m_commands;   // 0xfc: ANIMATIONS
    bool m_hasFont;   // 0x108: a FONT given
    CSaferStr m_font;   // 0x10c: FONT
};   // 0x118 bytes

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
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr);
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    virtual void SetString(const char *s, bool translate);
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    void LoadSpec(const CTtySpec &spec);
    /* the string's scale with its font's (inline: Draw's, ours) */
    float GetScale() { return m_scale * (m_font ? m_font->GetScaleFactor() : 1.0f); }

    CSaferStr m_text;   // 0xc0
    float m_cc;   // 0xcc: the cursor's pulse, an angle (or, fading, the fade)
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
    float m_1a0;   // 0x1a0: the pause's time left (the text's #P)
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
    /* pure, as the table 0x407820 holds them (0 in slots 0x50, 0x54 and 0x70-0x78): C2D's
     * SetActive and SetNormal made pure again, and the three new */
    virtual void SetActive() = 0;
    virtual void SetNormal() = 0;
    virtual void SetBackgroundAlpha(float a) = 0;
    virtual void Slot27() = 0;
    virtual void DrawBackground(const CMatrix &m, zdb::CTextureRelocMgr *mgr) = 0;
    void SetCaptionStyle(const CPnt3D &color, bool pulse, float time, float min, float max);
    /* the text's place in the button (inline: the pop-up's command, y loaded first; the name ours) */
    void SetTextOffset(int x, int y)
    {
        _iTextX = x;
        _iTextY = y;
    }

    char *_szCaption;   // 0x18
    C2DString _strCaption;   // 0x1c
    int _iTextX;   // 0xb4
    int _iTextY;   // 0xb8
    bool _bCaptionCentered;   // 0xbc
};   // 0xc0 bytes

/* the animations (zanim.h's CZAnimMain and CZAnim: only what an action calls; ZAnim OVL1 bss
 * 0x414BB0) */
class CZAnimArgList {
public:
    /* the index'th string argument (zanim.h's) */
    char *GetCharP(int index);
};

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

    unsigned char _0[0x18];
    CZAnimArgList *m_arg_list;   // 0x18
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
    /* made with a value if new (zanim.h's: the list box's count and focus valves) */
    static CValve *Create(const char *name, unsigned int value, VALVE_TYPE type);
    static CValve *GetByName(const char *name);
    /* the value (inline, const: the list box's RemoveItem reads it again at every step of its
     * walk's remainder loop, as the target does; read straight MW hoists it) */
    unsigned int GetValue() const { return m_value; }
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
bool zrdr_findPNT3D(const _zrdr *rdr, const char *tag, PNT3D *out);

extern CZAnimMain ZAnim;

/* a button (SOCOM 1's DWARF: : C3StateButton; SOCOM II's 0x440 bytes with a UIVarMixin at 0xC0,
 * its background a plain bitmap at 0x1A0) */
class C2DButton : public C3StateButton, public UIVarMixin {
public:
    C2DButton();
    virtual ~C2DButton();
    /* slot 26: the background's alpha, a fraction of 128 (0x37C2C0) */
    virtual void SetBackgroundAlpha(float a) { m_background.SetTrans(128.0f * a); }
    /* slot 27 (0x370C20, its draft's; the five button tables hold it) */
    virtual void Slot27();
    virtual void DrawBackground(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void SetCanHandleInput(bool val);
    virtual bool HandlePad(const CPad *pad, int port);
    void ShowState(ButtonState *state);
    /* a look taken over (inline: the pop-up's command; the names ours) */
    void SetPressedState(const ButtonState &s) { m_pressed.Set(s); }
    void SetNormalState(const ButtonState &s) { m_normal.Set(s); }
    void SetActiveState(const ButtonState &s) { m_active.Set(s); }
    virtual void Tick(float dT);
    virtual void SetActive();
    virtual void SetNormal();
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr);
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    virtual int GetZAnimCmdType(const char *name);
    virtual void UpdateUIVar(CUIVariable *var);
    void LoadSpec(const CButtonSpec &spec);   // 0x376BD0

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
    virtual bool HandlePad(const CPad *pad, int port);
    void LoadSpec(const CSliderBarSpec &spec, zdb::CTexHandle *tex);
    /* the value's real written to the UI variable (ours: HandlePad's four arms) */
    inline void ApplyValue();

    float m_value;   // 0x440
    zdb::CTexHandle *m_knobTex;   // 0x444: the knob's texture
    int m_knobW;   // 0x448: the knob's size
    int m_knobH;   // 0x44c
    float m_center;   // 0x450: the variable's value at the slider's middle
    unsigned char _454[0x460 - 0x454];
    CPlainBmp m_knob;   // 0x460
    int m_intervals;   // 0x500
    float m_repeat;   // 0x504: how long the pad's direction has been held
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
 * C2DButton's own table (this unit's since round 42, with ~C2DButton) takes the destructor's thunk
 * and CCheckBox overrides the slot */
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
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr);
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    virtual int GetZAnimCmdType(const char *name);
    bool Load(zdb::CTextureRelocMgr *mgr);

    /* a plain bitmap (Draw inlines CPlainBmp's), its text wrapped text at 0xB0 placed at an offset
     * (Load sets its scale, width and spacing and calls its Update, slot +0xA0 -- CWrappedText's
     * 0x250 bytes end at the font); the parts its Tick ticks (SOCOM II's two members, through
     * their tables; SOCOM 1 ticked the pointer alone), each with an offset after it, drawn when
     * 0x330 is set; the 2D object drawn instead of it all when 0x32C points at one */
    zdb::CTextureRelocMgr *m_mgr;   // 0xa0: the relocator Load was given
    unsigned char _a4[0xb0 - 0xa4];
    CWrappedText m_text;   // 0xb0 (HandlePad empties it through slot 26)
    C2DFont *m_font;   // 0x300: the font (UiSys::LoadFromDesign stores it before Load)
    IPNT2D m_textOffset;   // 0x304
    int m_width;   // 0x30c: its command's WIDTH
    int m_height;   // 0x310: its command's HEIGHT
    unsigned char _314[0x32c - 0x314];
    C2D *m_32c;   // 0x32c
    CValve *m_330;   // 0x330: the valve the answer is set in (the two buttons drawn only with one)
    int m_choice;   // 0x334: the button chosen, 1 or 2
    unsigned char _338;
    bool m_339;   // 0x339: the cancel button (6) answers 66
    unsigned char _33a[0x340 - 0x33a];
    C2DButton m_340;   // 0x340: the first button
    IPNT2D m_340Offset;   // 0x780
    IPNT2D m_340Text;   // 0x788: its text's offset
    C2DButton m_790;   // 0x790: the second
    IPNT2D m_790Offset;   // 0xbd0
    IPNT2D m_790Text;   // 0xbd8: its text's offset
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
 * Byte-exact since round 62 (open since round 58, 22 instructions in the two parts' block): ONE
 * `pos` serves the parts and the text. IRO_ExpressionPropagation (mweppc IroPropagate.c) moves an
 * expression into its use only when the scalarised variable has one use and is used in no other
 * flow node, so with a `pos` of its own in each block pos.z's copy of p.z sank into the third
 * cell's store and p.z's address was first reused after the cells' -- IRO_CommonSubs numbers its
 * temporaries at each expression's first reuse, in linear order; shared across the blocks the copy
 * stays where it is written, p.z is loaded with the other loads and &p.z's temporary follows &p.y's */
void CPopUpDialog::Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr)
{
    CPnt3D p = CPnt3D::zero;

    m.Transform(&p, 1);
    if (m_32c) {
        m_32c->Draw(m, mgr);
        return;
    }
    CPlainBmp::Draw(m, mgr);
    CPnt3D pos;
    if (m_330) {
        CMatrix mat;

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
    pos.x = p.x + m_textOffset.x;
    pos.y = p.y + m_textOffset.y;
    pos.z = p.z;
    CMatrix mat;

    mat(3, 0) = pos.x;
    mat(3, 1) = pos.y;
    mat(3, 2) = pos.z;
    m_text.Draw(mat, mgr);
}

/* the relocator kept and the font given to the text and the two buttons' captions; the text at
 * scale 1, wrapped to 100 with a spacing of 20, and brought up to date (SOCOM 1's
 * CPopUpDialog::Load, 0x389770 there: the same body, its spacing an int; UiSys::LoadFromDesign
 * calls it with the menu's relocator right after storing the font) */
bool CPopUpDialog::Load(zdb::CTextureRelocMgr *mgr)
{
    m_mgr = mgr;
    m_text.SetFont(m_font);
    m_340._strCaption.SetFont(m_font);
    m_790._strCaption.SetFont(m_font);
    m_text.m_scale = 1.0f;
    m_text.m_width = 100.0f;
    m_text.m_height = 20.0f;
    m_text.Update();
    return false;
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

/* the teletype's cursor and window (SOCOM II's, between the speeds and the noise in the data,
 * 0x3E0390-0x3E03A3; the names ours): the cursor's offset below the line and its width (scaled as
 * the text is), and the window's bottom against its height -- CTeleType::Draw's alone */
namespace {
int g_iTtyCursorDY = -5;
int g_iTtyCursorWidth = 8;
int g_iTtyClipDY = -11;
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

void zVid_Scissor(int x1, int y1, int x2, int y2);

/* a string's lines' height (ours: an inline over the string, its count in a local of its own: read
 * before the spacing call and kept, where the caller's own read of the count after the call is a
 * load of its own -- the product written in the caller, or the count read straight here, MW shares
 * one load between the two) */
static inline float TextHeight(C2DString *s)
{
    int n = s->m_numLines;

    return n * s->GetSpacing();
}

/* slot 2: the text at the corner the matrix places, its first lines skipped until the rest fit
 * the window's height, clipped to the window while it scrolls; then the cursor after the last
 * character unless the text starts with a space (SOCOM 1's CTeleType::Draw; SOCOM II clips and
 * scales the cursor by the font). The scale an inline member, so its two expansions share one load
 * of the font with the height (through a free inline taking the string, each reloads it); the
 * skipped count declared ahead of the text pointer (the other way round the two saved registers
 * swap); the cursor's rectangle built in Load's own argument list -- as named locals MW folds the
 * right edge into the left's conversion and sinks the left past both spacing calls (0x390) */
void CTeleType::Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr)
{
    if (!IsOn())
        return;
    CPnt3D p = CPnt3D::zero;

    m.Transform(&p, 1);
    CMatrix mat;
    float height = (m_font->m_displaybottom - m_font->m_displaytop) * GetScale();
    float width = g_iTtyCursorWidth * GetScale();

    C2DString::Tick(0.0f);
    SetPos((int)p.x, (int)p.y);
    UpdatePos();
    int total = (int)TextHeight(this);
    int lines = m_numLines - m_1a8;

    if (m_shown > 0 && m_shown < total) {
        int skip = 0;
        const char *s = m_string;

        while (lines * GetSpacing() > m_shown) {
            do
                skip++;
            while (*s++ != '\n');
            lines--;
        }
        m_firstCharPos = skip;
    } else
        m_firstCharPos = 0;
    if (!strlen(m_string))
        return;
    if (m_1a8)
        zVid_Scissor(0, 0, (int)(p.x + m_maxLength), (int)(g_iTtyClipDY + (p.y + m_shown)));
    C2DString::Draw(mgr, 0, 0);
    if (m_showCursor && m_string[0] != ' ') {
        int x = m_shortLength;

        m_cursor.Load(x + 2, g_iTtyCursorDY + (1.0f + (p.y + (lines + m_1a8) * GetSpacing())), width + (x + 2),
                      g_iTtyCursorDY + (p.y + (lines + m_1a8) * GetSpacing() - height), 0);
        m_cursor.Draw(mat, mgr);
    }
    zVid_Scissor(0, 0, 640, 448);
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

/* the variable ignored (0x37C1A0: the slot after the tables, +0x80 of 0x407950, and the mixin's
 * slot through the thunk 0x37D100) */
void CPlainBmp::UpdateUIVar(CUIVariable *var)
{
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

/* set when a menu object takes actions (zui.cpp's global, SOCOM 1's g_hasAnims at gp-25344 there;
 * OVL1 bss 0x49EAA0) */
extern unsigned char g_hasAnims;

/* the mission's fonts' rdr (the font loaders read it) */
_zrdr *FetchMissionFontsRdr();

/* set up from its spec (SOCOM 1's C2DButton::LoadSpec, 0x38F090 there): the spec's actions taken
 * (the menu told it has some), the size, the caption's scale and place, a font of its own when the
 * spec names one, the four looks -- normal, then the spec's ACTIVE into 0xFC (SetActive's), its
 * PRESSED into 0xC8, then disabled, SOCOM 1's order -- the caption's centring, the spec's variable
 * watched and the button brought up to date with it. The size and the offset each through a pair
 * of locals declared together (both loaded, then converted or stored, as the target does: one
 * statement each interleaves them, an IPNT2D local goes through the stack); the centring through
 * a bool local (chained, the bool's conversion is redone between the stores) */
void C2DButton::LoadSpec(const CButtonSpec &spec)
{
    for (int i = 0; i < (int)spec.m_commands.size(); i++) {
        g_hasAnims = 1;
        m_numActions++;
        m_actions[i] = spec.m_commands[i];
    }
    float w = spec.m_size.x, h = spec.m_size.y;

    _fWidth = w;
    _fHeight = h;
    _strCaption.m_scale = spec.m_scale;

    int x = spec.m_offset.x, y = spec.m_offset.y;

    _iTextX = x;
    _iTextY = y;
    if (spec.m_hasFont) {
        const char *name = spec.m_font.c_str();

        m_alternateFont = new C2DFont;
        m_alternateFont->Load(FetchMissionFontsRdr(), name);
        _strCaption.SetFont(m_alternateFont);
    }
    m_normal = spec.m_normal;
    m_pressed = spec.m_active;
    m_active = spec.m_pressed;
    m_disabled = spec.m_disabled;
    bool centered = spec.m_hcentered;

    _strCaption.m_isCentered = centered;
    _bCaptionCentered = centered;

    CUIVariable *var = spec.GetUIVar();

    if (var)
        WatchUIVar(var);
    if (UIVarMixin::m_uivar)
        UpdateUIVar(UIVarMixin::m_uivar);
}

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

/* CONTINUE, SCROLL_UP, SCROLL_DOWN and FINISH as commands of their own, no argument -- all four
 * tags looked up first; anything else the renderable string's (slot 22 of CTeleType's vtable
 * 0x4078A0, after TickZAnimCmd as SOCOM 1's ParseZAnimCmd 0x3904E0, which parsed the same four) */
C2DAnimCmdHdr *CTeleType::ParseZAnimCmd(_zrdr *rdr)
{
    C2DAnimCmdHdr *cmd;
    _zrdr *cont = zrdr_findtag(rdr, "CONTINUE");
    _zrdr *up = zrdr_findtag(rdr, "SCROLL_UP");
    _zrdr *down = zrdr_findtag(rdr, "SCROLL_DOWN");
    _zrdr *finish = zrdr_findtag(rdr, "FINISH");

    if (cont) {
        cmd = (C2DAnimCmdHdr *)_calloc(1, sizeof(C2DAnimCmdHdr), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(C2DAnimCmdHdr);
        cmd->m_type = GetZAnimCmdType("CONTINUE");
    } else if (up) {
        cmd = (C2DAnimCmdHdr *)_calloc(1, sizeof(C2DAnimCmdHdr), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(C2DAnimCmdHdr);
        cmd->m_type = GetZAnimCmdType("SCROLL_UP");
    } else if (down) {
        cmd = (C2DAnimCmdHdr *)_calloc(1, sizeof(C2DAnimCmdHdr), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(C2DAnimCmdHdr);
        cmd->m_type = GetZAnimCmdType("SCROLL_DOWN");
    } else if (finish) {
        cmd = (C2DAnimCmdHdr *)_calloc(1, sizeof(C2DAnimCmdHdr), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(C2DAnimCmdHdr);
        cmd->m_type = GetZAnimCmdType("FINISH");
    } else
        cmd = CRenderableString::ParseZAnimCmd(rdr);
    return cmd;
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

/* the text laid out in lines (SOCOM 1's CWrappedText::SetString, 0x391630 there, with SOCOM II's
 * flag and colour codes): the old lines deleted, then a new line after a new line in the running
 * colour and bold state -- leading spaces skipped, characters added until the width is passed with a
 * space to go back to, or a newline; "#C" and three bytes change the colour and end the line, "#B"
 * toggles the bold -- backed up to the last space, and kept if anything was added */
void CWrappedText::SetString(const char *s, bool translate)
{
    bool bold = false;

    if (!s || !*s)
        return;

    const char *str = translate ? TranslateString(s) : s;
    int len = strlen(str);
    int pos = 0;
    iterator i;
    iterator e = end();

    for (i = begin(); i != e;) {
        C2D *p = *i;
        iterator next;

        next = erase(i);
        delete p;
        i = next;
    }

    CPnt3D color = *(const CPnt3D *)m_color;

    while (pos < len) {
        CRenderableString *line = new CRenderableString;
        int lastspace = 0;
        bool any = false;

        line->m_scale = m_scale * (m_font ? m_font->GetScaleFactor() : 1.0f);
        line->SetFont(m_font);
        line->SetString("", false);
        line->m_height = GetSpacing();
        line->SetColor(color.x, color.y, color.z);
        line->UpdatePos();
        line->m_socom2_8c = bold;
        while ((str[pos] == ' ' || str[pos] == '\n') && pos < len)
            pos++;
        while (pos < len && (line->GetLength() < m_width || !lastspace)) {
            if (str[pos] == ' ')
                lastspace = pos;
            if (str[pos] == '#' && str[pos + 1] == 'C' && pos + 4 < len) {
                color.x = str[pos + 2];
                color.y = str[pos + 3];
                color.z = str[pos + 4];
                pos += 5;
                break;
            }
            if (str[pos] == '#' && str[pos + 1] == 'B')
                bold = !bold;
            line->AddCharacter(str[pos]);
            any = true;
            line->UpdatePos();
            pos++;
            if (str[pos] == '\n')
                break;
        }
        if (!(line->GetLength() < m_width) && lastspace > 0 && lastspace < pos) {
            do {
                if (str[pos] == '#' && str[pos + 1] == 'B')
                    bold = !bold;
                line->DeleteCharacter();
                pos--;
            } while (lastspace < pos);
        }
        if (any)
            push_back(line);
        else
            delete line;
    }
    m_numLines = -1;
    if (m_autoScroll)
        m_offset = 0;
}

#if Z_PORT
/* the port: the inline constructor (the image's copy 0x27F250): the lines the window shows -1, the
 * scroll and the arrows' textures cleared, the two arrows plain bitmaps, not one line only */
CWrappedText::CWrappedText()
    : m_shown(-1), m_c8(0), m_offset(0.0f), m_autoScroll(false), m_downTex(0), m_upTex(0),
      m_displayOneLine(false)
{
}
#endif

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

class CGameMenu {
public:
    bool RunEventAnim(ActionItem::EVENT event, const char *arg);
};

class CMenuNode;
CMenuNode *UiGetCurMenu();

/* slot 12: typed out (SOCOM 1's CTeleType::Tick, SOCOM II's codes and word wrap): at the speed's
 * factor; while a #P pause runs only the cursor pulses; past the text's end the rest is added and the
 * cursor pulses or fades; else the next word measured -- it goes down a line when it would not fit --,
 * the clock advanced, a line break or a #N, #P code taken, or the next (UTF-8) character added; the
 * menu told (event 21) when the text ends */
void CTeleType::Tick(float dT)
{
    if (!IsOn())
        return;
    dT *= g_fTtySpeedFactor;
    if (m_1a0 > 0.0f) {
        m_1a0 -= dT;
        m_cc += dT * m_pulseRate;
        if (m_cc > 6.2831855f)
            m_cc -= 6.2831855f;
        m_cursor.SetTrans(96.0f + 32.0f * sinf(m_cc));
        C2DString::Tick(dT);
        return;
    }
    UpdatePos();
    if (!(m_typed < m_len)) {
        if (m_text[m_typed]) {
            while (m_text[m_typed])
                AddCharacter(m_text[m_typed++]);
            if (m_typed > 0 && g_bTtyNoise)
                PlayCountSound();
        }
        m_cc += dT * m_pulseRate;
        if (m_fadeCursor) {
            if (m_cc > 1.0f)
                m_cc = 1.0f;
            m_cursor.SetTrans(128.0f * (1.0f - m_cc));
        } else {
            if (m_cc > 6.2831855f)
                m_cc -= 6.2831855f;
            m_cursor.SetTrans(96.0f + 32.0f * sinf(m_cc));
        }
        C2DString::Tick(dT);
        return;
    }

    float last = m_e8;
    bool wrap = false;

    if (dT > 0.0f || (m_typed > 0 && m_text[m_typed - 1] == ' ')) {
        C2DString word;

        word.SetFont(m_font);
        word.m_scale = m_scale * (m_font ? m_font->GetScaleFactor() : 1.0f);
        word.C2DString::SetString("", true);

        int i = m_typed;

        while (i < m_len && m_text[i] != ' ' && m_text[i] != '\n')
            word.AddCharacter(m_text[i++]);
        word.UpdatePos();
        if (word.m_length + m_shortLength > m_maxLength) {
            wrap = true;
            m_typed--;
        }
    }
    m_cursor.SetTrans(128.0f);
    m_e8 += dT * m_interval * (m_scale * (m_font ? m_font->GetScaleFactor() : 1.0f));
    if ((m_e8 > m_maxLength && m_text[m_typed] == ' ' && m_shortLength > m_maxLength) || wrap) {
        NewLine();
        m_f0 += m_shortLength;
        m_e8 -= m_shortLength;
        m_typed++;
        if (m_e8 < 0.0f)
            m_e8 = 0.0f;
        else
            Tick(0.0f);
    } else if (m_text[m_typed] == '\n') {
        NewLine();
        m_f0 = m_length;
        m_e8 = 0.01f;
        m_typed++;
    } else if (m_text[m_typed] == '#') {
        m_typed++;
        if (m_text[m_typed] == 'N') {
            NewLine();
            m_f0 = m_length;
            m_e8 = 0.01f;
            m_typed++;
        } else if (m_text[m_typed] == 'P') {
            m_typed++;

            char buf[256] = {0};

            while (m_text[m_typed] != ' ' && m_typed < m_len) {
                sprintf(buf, "%s%c", buf, m_text[m_typed]);
                m_typed++;
            }
            m_typed++;
            m_1a0 = (float)atof(buf);
        }
    } else if (m_e8 + m_f0 > m_length) {
        if (m_typed < m_len) {
            if (dT > 0.0f && m_typed > 0 && g_bTtyNoise)
                PlayCountSound();

            const char *p = &m_text[m_typed];
            int n = UTF8CharLen(p);

            for (int i = 0; i < n; i++) {
                AddCharacter(m_text[m_typed]);
                m_typed++;
            }
            UpdatePos();
            if (m_typed == m_len)
                ((CGameMenu *)UiGetCurMenu())->RunEventAnim((ActionItem::EVENT)21, 0);
            Tick(0.0f);
        } else if (last + m_f0 <= m_length)
            ((CGameMenu *)UiGetCurMenu())->RunEventAnim((ActionItem::EVENT)21, 0);
    }
    C2DString::Tick(dT);
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

/* the map's node as the zooms move it (znode.h's CNode: its matrix's translation row, the
 * position and scale accessors) and the pop-up's command places by its name */
namespace zdb {
class CNode {
public:
    float GetScale();
    void SetScale(float s);
    void SetPosition(float x, float y, float z);
    void SetPosition(const CPnt3D &p) { SetPosition(p.x, p.y, p.z); }
    CPnt3D GetPosition() const { return *(const CPnt3D *)m_matrix.m_matrix[3]; }

    CMatrix m_matrix;   // 0x0
    unsigned char _40[0x90 - sizeof(CMatrix)];
    char *m_name;   // 0x90
};
}

/* a pop-up's button as its command keeps it (SOCOM 1's; 0x4C bytes each, the names ours) */
struct CPopUpButtonCmd {
    unsigned char m_tex[3];   // 0x0: the name indices of the pressed, the normal and the active look's images
    int m_x;   // 0x4: POS
    int m_y;   // 0x8
    int m_width;   // 0xc: WIDTH
    int m_height;   // 0x10: HEIGHT
    unsigned char m_captionName;   // 0x14
    unsigned short m_captionId;   // 0x16
    int m_textX;   // 0x18: TEXTOFFSET
    int m_textY;   // 0x1c
    int m_color[3][3];   // 0x20: the looks' TEXT_COLOR
    unsigned char m_centered;   // 0x44: HCENTERED
    float m_scale;   // 0x48: SCALE
};   // 0x4c bytes

/* the pop-up's command (ParseZAnimCmd's: POPUP's, 0xEC bytes; the names ours) */
class CPopUpAnimCmd : public C2DAnimCmdHdr {
public:
    unsigned char m_captionName;   // 0x10
    unsigned short m_captionId;   // 0x12
    unsigned char m_background;   // 0x14
    int m_x;   // 0x18: POSITION
    int m_y;   // 0x1c
    int m_height;   // 0x20: HEIGHT
    int m_width;   // 0x24: WIDTH
    int m_yoffset;   // 0x28: YOFFSET
    int m_xoffset;   // 0x2c: XOFFSET
    float m_scale;   // 0x30: SCALE
    int m_color[3];   // 0x34: COLOR
    unsigned char _40[0x50 - 0x40];
    unsigned char m_cancellable;   // 0x50: CANCELLABLE
    unsigned char m_protected;   // 0x51: PROTECTED
    unsigned char m_yesno;   // 0x52: YESNO
    unsigned char m_valve;   // 0x53: VALVE
    CPopUpButtonCmd m_yes;   // 0x54: YES
    CPopUpButtonCmd m_no;   // 0xa0: NO
};   // 0xec bytes

const char *GetString(int id);
int GetStringId(const char *name);
bool zrdr_findPNT2D(const _zrdr *rdr, const char *tag, PNT2D *out);
bool zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n);

/* POPUP's command read from its block: its place, text, background, sizes, colour and flags, and
 * with YESNO the valve and the two buttons, each with its place, text and three looks (SOCOM 1's
 * CPopUpDialog::ParseZAnimCmd, 0x387C00 there, grown by the string ids). YES's lack of a caption
 * clears NO's name, and either button's absence the dialog's own text offset's y, as the target does */
C2DAnimCmdHdr *CPopUpDialog::ParseZAnimCmd(_zrdr *rdr)
{
    CPopUpAnimCmd *cmd = (CPopUpAnimCmd *)_calloc(1, sizeof(CPopUpAnimCmd), 0, 0);

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(CPopUpAnimCmd);

    PNT2D pos;

    pos.y = 0.0f;
    pos.x = 0.0f;
    zrdr_findPNT2D(rdr, "POSITION", &pos);
    cmd->m_x = pos.x;
    cmd->m_y = pos.y;

    const char *caption = zrdr_findstring(rdr, "CAPTION");

    if (caption) {
        unsigned short name, sid;
        int id = GetStringId(caption);

        if (id)
            sid = id;
        else {
            name = ZAnim.m_CurAnim->AddName(caption);
            sid = 0;
        }
        cmd->m_captionName = name;
        cmd->m_captionId = sid;
    }

    const char *background = zrdr_findstring(rdr, "BACKGROUND");

    if (background)
        cmd->m_background = ZAnim.m_CurAnim->AddName(background);
    zrdr_findint(rdr, "HEIGHT", &cmd->m_height, 1);
    zrdr_findint(rdr, "WIDTH", &cmd->m_width, 1);
    zrdr_findint(rdr, "XOFFSET", &cmd->m_xoffset, 1);
    zrdr_findint(rdr, "YOFFSET", &cmd->m_yoffset, 1);
    zrdr_findreal(rdr, "SCALE", &cmd->m_scale, 1);
    cmd->m_no.m_scale = cmd->m_yes.m_scale = cmd->m_scale;

    PNT3D color;

    color.x = 128.0f;
    color.y = 128.0f;
    color.z = 128.0f;
    zrdr_findPNT3D(rdr, "COLOR", &color);
    cmd->m_color[0] = color.x;
    cmd->m_color[1] = color.y;
    cmd->m_color[2] = color.z;
    cmd->_40[0] = 0;
    if (zrdr_findtag(rdr, "CANCELLABLE"))
        cmd->m_cancellable = 1;
    else
        cmd->m_cancellable = 0;
    if (zrdr_findtag(rdr, "PROTECTED"))
        cmd->m_protected = 1;
    else
        cmd->m_protected = 0;
    if (!cmd->_40[0]) {
        _zrdr *yesno = zrdr_findtag(rdr, "YESNO");

        if (yesno) {
            cmd->m_yesno = 1;

            const char *valve = zrdr_findstring(yesno, "VALVE");

            if (valve) {
                cmd->m_valve = ZAnim.m_CurAnim->AddName(valve);
                CValve::Create(valve, 0, VALVE_TEMP);
            }

            _zrdr *yes = zrdr_findtag(yesno, "YES");

            if (yes) {
                pos.y = 0.0f;
                pos.x = 0.0f;
                zrdr_findPNT2D(yes, "POS", &pos);
                cmd->m_yes.m_x = pos.x;
                cmd->m_yes.m_y = pos.y;
                pos.y = 0.0f;
                pos.x = 0.0f;
                zrdr_findPNT2D(yes, "TEXTOFFSET", &pos);
                cmd->m_yes.m_textX = pos.x;
                cmd->m_yes.m_textY = pos.y;
                zrdr_findint(yes, "WIDTH", &cmd->m_yes.m_width, 1);
                zrdr_findint(yes, "HEIGHT", &cmd->m_yes.m_height, 1);
                zrdr_findreal(yes, "SCALE", &cmd->m_yes.m_scale, 1);
                const char *yesCaption = zrdr_findstring(yes, "CAPTION");

                if (yesCaption) {
                    unsigned short name, sid;
                    int id = GetStringId(yesCaption);

                    if (id)
                        sid = id;
                    else {
                        name = ZAnim.m_CurAnim->AddName(yesCaption);
                        sid = 0;
                    }
                    cmd->m_yes.m_captionName = name;
                    cmd->m_yes.m_captionId = sid;
                } else {
                    cmd->m_yes.m_captionName = 255;
                    cmd->m_no.m_captionName = 0;
                }
                _zrdr *active = zrdr_findtag(yes, "ACTIVE");

                if (active) {
                    ButtonState state;

                    state.m_texName = zrdr_findstring(active, "IMAGE");
                    zrdr_findPNT3D(active, "TEXT_COLOR", (PNT3D *)&state.m_color);
                    zrdr_findreal(active, "PULSE", &state.m_period, 1);
                    zrdr_findreal(active, "SCALE", &state.m_scale, 1);
                    _zrdr *glow = zrdr_findtag(active, "TEXT_GLOW");

                    if (glow) {
                        float range[2];

                        state.m_pulse = true;
                        zrdr_findreal(glow, "RANGE", range, 2);
                        state.m_fadeMin = range[0];
                        state.m_fadeMax = range[1];
                        zrdr_findreal(glow, "TIME", &state.m_fadeTime, 1);
                    } else
                        state.m_pulse = false;
                    cmd->m_yes.m_color[0][0] = state.m_color.x;
                    cmd->m_yes.m_color[0][1] = state.m_color.y;
                    cmd->m_yes.m_color[0][2] = state.m_color.z;
                    if (*state.m_texName.c_str())
                        cmd->m_yes.m_tex[0] = ZAnim.m_CurAnim->AddName(state.m_texName.c_str());
                    else
                        cmd->m_yes.m_tex[0] = 255;
                } else {
                    cmd->m_yes.m_color[0][0] = cmd->m_yes.m_color[0][1] = cmd->m_yes.m_color[0][2] = cmd->m_yes.m_tex[0] = 255;
                }
                _zrdr *normal = zrdr_findtag(yes, "NORMAL");

                if (normal) {
                    ButtonState state;

                    state.m_texName = zrdr_findstring(normal, "IMAGE");
                    zrdr_findPNT3D(normal, "TEXT_COLOR", (PNT3D *)&state.m_color);
                    zrdr_findreal(normal, "PULSE", &state.m_period, 1);
                    zrdr_findreal(normal, "SCALE", &state.m_scale, 1);
                    _zrdr *glow = zrdr_findtag(normal, "TEXT_GLOW");

                    if (glow) {
                        float range[2];

                        state.m_pulse = true;
                        zrdr_findreal(glow, "RANGE", range, 2);
                        state.m_fadeMin = range[0];
                        state.m_fadeMax = range[1];
                        zrdr_findreal(glow, "TIME", &state.m_fadeTime, 1);
                    } else
                        state.m_pulse = false;
                    cmd->m_yes.m_color[1][0] = state.m_color.x;
                    cmd->m_yes.m_color[1][1] = state.m_color.y;
                    cmd->m_yes.m_color[1][2] = state.m_color.z;
                    if (*state.m_texName.c_str())
                        cmd->m_yes.m_tex[1] = ZAnim.m_CurAnim->AddName(state.m_texName.c_str());
                    else
                        cmd->m_yes.m_tex[1] = 255;
                } else {
                    cmd->m_yes.m_color[1][0] = cmd->m_yes.m_color[1][1] = cmd->m_yes.m_color[1][2] = cmd->m_yes.m_tex[1] = 255;
                }
                _zrdr *pressed = zrdr_findtag(yes, "PRESSED");

                if (pressed) {
                    ButtonState state;

                    state.m_texName = zrdr_findstring(pressed, "IMAGE");
                    zrdr_findPNT3D(pressed, "TEXT_COLOR", (PNT3D *)&state.m_color);
                    zrdr_findreal(pressed, "PULSE", &state.m_period, 1);
                    zrdr_findreal(pressed, "SCALE", &state.m_scale, 1);
                    _zrdr *glow = zrdr_findtag(pressed, "TEXT_GLOW");

                    if (glow) {
                        float range[2];

                        state.m_pulse = true;
                        zrdr_findreal(glow, "RANGE", range, 2);
                        state.m_fadeMin = range[0];
                        state.m_fadeMax = range[1];
                        zrdr_findreal(glow, "TIME", &state.m_fadeTime, 1);
                    } else
                        state.m_pulse = false;
                    cmd->m_yes.m_color[2][0] = state.m_color.x;
                    cmd->m_yes.m_color[2][1] = state.m_color.y;
                    cmd->m_yes.m_color[2][2] = state.m_color.z;
                    if (*state.m_texName.c_str())
                        cmd->m_yes.m_tex[2] = ZAnim.m_CurAnim->AddName(state.m_texName.c_str());
                    else
                        cmd->m_yes.m_tex[2] = 255;
                } else {
                    cmd->m_yes.m_color[2][0] = cmd->m_yes.m_color[2][1] = cmd->m_yes.m_color[2][2] = cmd->m_yes.m_tex[2] = 255;
                }
                if (zrdr_findtag(yes, "HCENTERED"))
                    cmd->m_yes.m_centered = 1;
                else
                    cmd->m_yes.m_centered = 0;
            } else {
                cmd->m_yes.m_captionName = cmd->m_yes.m_tex[0] = cmd->m_yes.m_tex[1] = cmd->m_yes.m_tex[2] = 255;
                cmd->m_yes.m_x = cmd->m_yes.m_y = cmd->m_yes.m_width = cmd->m_yes.m_height = 0;
                cmd->m_yes.m_textX = m_340Text.y = 0;
                cmd->m_yes.m_centered = 0;
            }

            _zrdr *no = zrdr_findtag(yesno, "NO");

            if (no) {
                pos.y = 0.0f;
                pos.x = 0.0f;
                zrdr_findPNT2D(no, "POS", &pos);
                cmd->m_no.m_x = pos.x;
                cmd->m_no.m_y = pos.y;
                pos.y = 0.0f;
                pos.x = 0.0f;
                zrdr_findPNT2D(no, "TEXTOFFSET", &pos);
                cmd->m_no.m_textX = pos.x;
                cmd->m_no.m_textY = pos.y;
                zrdr_findint(no, "WIDTH", &cmd->m_no.m_width, 1);
                zrdr_findint(no, "HEIGHT", &cmd->m_no.m_height, 1);
                zrdr_findreal(no, "SCALE", &cmd->m_no.m_scale, 1);
                const char *noCaption = zrdr_findstring(no, "CAPTION");

                if (noCaption) {
                    unsigned short name, sid;
                    int id = GetStringId(noCaption);

                    if (id)
                        sid = id;
                    else {
                        name = ZAnim.m_CurAnim->AddName(noCaption);
                        sid = 0;
                    }
                    cmd->m_no.m_captionName = name;
                    cmd->m_no.m_captionId = sid;
                } else {
                    cmd->m_no.m_captionName = 255;
                    cmd->m_no.m_captionId = 0;
                }
                _zrdr *active = zrdr_findtag(no, "ACTIVE");

                if (active) {
                    ButtonState state;

                    state.m_texName = zrdr_findstring(active, "IMAGE");
                    zrdr_findPNT3D(active, "TEXT_COLOR", (PNT3D *)&state.m_color);
                    zrdr_findreal(active, "PULSE", &state.m_period, 1);
                    zrdr_findreal(active, "SCALE", &state.m_scale, 1);
                    _zrdr *glow = zrdr_findtag(active, "TEXT_GLOW");

                    if (glow) {
                        float range[2];

                        state.m_pulse = true;
                        zrdr_findreal(glow, "RANGE", range, 2);
                        state.m_fadeMin = range[0];
                        state.m_fadeMax = range[1];
                        zrdr_findreal(glow, "TIME", &state.m_fadeTime, 1);
                    } else
                        state.m_pulse = false;
                    cmd->m_no.m_color[0][0] = state.m_color.x;
                    cmd->m_no.m_color[0][1] = state.m_color.y;
                    cmd->m_no.m_color[0][2] = state.m_color.z;
                    if (*state.m_texName.c_str())
                        cmd->m_no.m_tex[0] = ZAnim.m_CurAnim->AddName(state.m_texName.c_str());
                    else
                        cmd->m_no.m_tex[0] = 255;
                } else {
                    cmd->m_no.m_color[0][0] = cmd->m_no.m_color[0][1] = cmd->m_no.m_color[0][2] = cmd->m_no.m_tex[0] = 255;
                }
                active = zrdr_findtag(no, "NORMAL");

                if (active) {
                    ButtonState state;

                    state.m_texName = zrdr_findstring(active, "IMAGE");
                    zrdr_findPNT3D(active, "TEXT_COLOR", (PNT3D *)&state.m_color);
                    zrdr_findreal(active, "PULSE", &state.m_period, 1);
                    zrdr_findreal(active, "SCALE", &state.m_scale, 1);
                    _zrdr *glow = zrdr_findtag(active, "TEXT_GLOW");

                    if (glow) {
                        float range[2];

                        state.m_pulse = true;
                        zrdr_findreal(glow, "RANGE", range, 2);
                        state.m_fadeMin = range[0];
                        state.m_fadeMax = range[1];
                        zrdr_findreal(glow, "TIME", &state.m_fadeTime, 1);
                    } else
                        state.m_pulse = false;
                    cmd->m_no.m_color[1][0] = state.m_color.x;
                    cmd->m_no.m_color[1][1] = state.m_color.y;
                    cmd->m_no.m_color[1][2] = state.m_color.z;
                    if (*state.m_texName.c_str())
                        cmd->m_no.m_tex[1] = ZAnim.m_CurAnim->AddName(state.m_texName.c_str());
                    else
                        cmd->m_no.m_tex[1] = 255;
                } else {
                    cmd->m_no.m_color[1][0] = cmd->m_no.m_color[1][1] = cmd->m_no.m_color[1][2] = cmd->m_no.m_tex[1] = 255;
                }
                active = zrdr_findtag(no, "PRESSED");

                if (active) {
                    ButtonState state;

                    state.m_texName = zrdr_findstring(active, "IMAGE");
                    zrdr_findPNT3D(active, "TEXT_COLOR", (PNT3D *)&state.m_color);
                    zrdr_findreal(active, "PULSE", &state.m_period, 1);
                    zrdr_findreal(active, "SCALE", &state.m_scale, 1);
                    _zrdr *glow = zrdr_findtag(active, "TEXT_GLOW");

                    if (glow) {
                        float range[2];

                        state.m_pulse = true;
                        zrdr_findreal(glow, "RANGE", range, 2);
                        state.m_fadeMin = range[0];
                        state.m_fadeMax = range[1];
                        zrdr_findreal(glow, "TIME", &state.m_fadeTime, 1);
                    } else
                        state.m_pulse = false;
                    cmd->m_no.m_color[2][0] = state.m_color.x;
                    cmd->m_no.m_color[2][1] = state.m_color.y;
                    cmd->m_no.m_color[2][2] = state.m_color.z;
                    if (*state.m_texName.c_str())
                        cmd->m_no.m_tex[2] = ZAnim.m_CurAnim->AddName(state.m_texName.c_str());
                    else
                        cmd->m_no.m_tex[2] = 255;
                } else {
                    cmd->m_no.m_color[2][0] = cmd->m_no.m_color[2][1] = cmd->m_no.m_color[2][2] = cmd->m_no.m_tex[2] = 255;
                }
                if (zrdr_findtag(no, "HCENTERED"))
                    cmd->m_no.m_centered = 1;
                else
                    cmd->m_no.m_centered = 0;
            } else {
                cmd->m_no.m_captionName = cmd->m_no.m_tex[0] = cmd->m_no.m_tex[1] = cmd->m_no.m_tex[2] = 255;
                cmd->m_no.m_x = cmd->m_no.m_y = cmd->m_no.m_width = cmd->m_no.m_height = 0;
                cmd->m_no.m_textX = m_790Text.y = 0;
                cmd->m_no.m_centered = 0;
            }
        } else
            cmd->m_yesno = 0;
    } else
        cmd->m_yesno = 0;
    return cmd;
}

/* POPUP: the pop-up taken from its command -- text, background and place; with YESNO, the valve the
 * answer goes to and the two buttons, each with three looks (the pressed one pulsing); any 2D object
 * it held dropped (SOCOM 1's CPopUpDialog::TickZAnimCmd, 0x387020 there, grown by the string ids). */
bool CPopUpDialog::TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done)
{
    CPopUpAnimCmd *cmd = (CPopUpAnimCmd *)hdr;
    CZAnim *anim = ZAnim.m_CurAnim;

    if (cmd) {
        const char *background = anim->GetName(cmd->m_background);
        const char *text = cmd->m_captionId ? GetString(cmd->m_captionId) : anim->GetName(cmd->m_captionName);

        if (!strcmp(text, "INPUT_STRING"))
            text = anim->m_arg_list->GetCharP(0);

        zdb::CNode *node = ZAnim.m_CurAnim->GetNode(cmd->node_ref_index, false);

        if (node && !strcmp("POPUP", node->m_name))
            node->SetPosition(cmd->m_x, cmd->m_y, 0.0f);
        m_width = cmd->m_width;
        m_height = cmd->m_height;
        m_textOffset.x = cmd->m_xoffset;
        m_textOffset.y = cmd->m_yoffset;
        m_339 = cmd->m_cancellable != 0;
        m_keep = cmd->m_protected != 0;
        if (m_330)
            m_330 = 0;
        if (cmd->m_yesno) {
            const char *caption;
            float w, h;

            m_330 = CValve::Create(anim->GetName(cmd->m_valve), VALVE_TEMP);
            m_choice = 1;

            caption = (cmd->m_yes.m_captionId ? GetString(cmd->m_yes.m_captionId)
                                                                : ZAnim.m_CurAnim->GetName(cmd->m_yes.m_captionName));
            m_340._szCaption = (char *)caption;
            if (caption) {
                m_340._strCaption.SetString(caption, true);
                m_340._strCaption.UpdatePos();
            }
            w = cmd->m_yes.m_width;
            h = cmd->m_yes.m_height;
            m_340._fWidth = w;
            m_340._fHeight = h;
            m_340._strCaption.m_scale = cmd->m_yes.m_scale;
            m_340Offset.x = cmd->m_yes.m_x;
            m_340Offset.y = cmd->m_yes.m_y;
            m_340Text.x = cmd->m_yes.m_textX;
            m_340Text.y = cmd->m_yes.m_textY;
            m_340.SetTextOffset(m_340Text.x, m_340Text.y);

            ButtonState state;

            state.m_texName = ZAnim.m_CurAnim->GetName(cmd->m_yes.m_tex[0]);
            state.m_tex = PrepTexture(state.m_texName.c_str(), m_mgr);
            state.m_color.x = cmd->m_yes.m_color[0][0];
            state.m_color.y = cmd->m_yes.m_color[0][1];
            state.m_color.z = cmd->m_yes.m_color[0][2];
            state.m_scale = cmd->m_yes.m_scale;
            state.m_pulse = true;
            state.m_fadeMin = 0.5f;
            state.m_fadeMax = 1.0f;
            state.m_fadeTime = 0.5f;
            m_340.SetPressedState(state);

            state.m_texName = ZAnim.m_CurAnim->GetName(cmd->m_yes.m_tex[1]);
            state.m_tex = PrepTexture(state.m_texName.c_str(), m_mgr);
            state.m_color.x = cmd->m_yes.m_color[1][0];
            state.m_color.y = cmd->m_yes.m_color[1][1];
            state.m_color.z = cmd->m_yes.m_color[1][2];
            state.m_scale = cmd->m_yes.m_scale;
            state.m_pulse = false;
            m_340.SetNormalState(state);

            state.m_texName = ZAnim.m_CurAnim->GetName(cmd->m_yes.m_tex[2]);
            state.m_tex = PrepTexture(state.m_texName.c_str(), m_mgr);
            state.m_color.x = cmd->m_yes.m_color[2][0];
            state.m_color.y = cmd->m_yes.m_color[2][1];
            state.m_color.z = cmd->m_yes.m_color[2][2];
            state.m_scale = cmd->m_yes.m_scale;
            state.m_pulse = false;
            m_340.SetActiveState(state);

            bool centered = cmd->m_yes.m_centered == 1;

            m_340._strCaption.m_isCentered = centered;
            m_340._bCaptionCentered = centered;

            caption = (cmd->m_no.m_captionId ? GetString(cmd->m_no.m_captionId)
                                                               : ZAnim.m_CurAnim->GetName(cmd->m_no.m_captionName));
            m_790._szCaption = (char *)caption;
            if (caption) {
                m_790._strCaption.SetString(caption, true);
                m_790._strCaption.UpdatePos();
            }
            w = cmd->m_no.m_width;
            h = cmd->m_no.m_height;
            m_790._fWidth = w;
            m_790._fHeight = h;
            m_790._strCaption.m_scale = cmd->m_no.m_scale;
            m_790Offset.x = cmd->m_no.m_x;
            m_790Offset.y = cmd->m_no.m_y;
            m_790Text.x = cmd->m_no.m_textX;
            m_790Text.y = cmd->m_no.m_textY;
            m_790.SetTextOffset(m_790Text.x, m_790Text.y);

            state.m_texName = ZAnim.m_CurAnim->GetName(cmd->m_no.m_tex[0]);
            state.m_tex = PrepTexture(state.m_texName.c_str(), m_mgr);
            state.m_color.x = cmd->m_no.m_color[0][0];
            state.m_color.y = cmd->m_no.m_color[0][1];
            state.m_color.z = cmd->m_no.m_color[0][2];
            state.m_scale = cmd->m_no.m_scale;
            state.m_pulse = true;
            state.m_fadeMin = 0.5f;
            state.m_fadeMax = 1.0f;
            state.m_fadeTime = 0.5f;
            m_790.SetPressedState(state);

            state.m_texName = ZAnim.m_CurAnim->GetName(cmd->m_no.m_tex[1]);
            state.m_tex = PrepTexture(state.m_texName.c_str(), m_mgr);
            state.m_color.x = cmd->m_no.m_color[1][0];
            state.m_color.y = cmd->m_no.m_color[1][1];
            state.m_color.z = cmd->m_no.m_color[1][2];
            state.m_scale = cmd->m_no.m_scale;
            state.m_pulse = false;
            m_790.SetNormalState(state);

            state.m_texName = ZAnim.m_CurAnim->GetName(cmd->m_no.m_tex[2]);
            state.m_tex = PrepTexture(state.m_texName.c_str(), m_mgr);
            state.m_color.x = cmd->m_no.m_color[2][0];
            state.m_color.y = cmd->m_no.m_color[2][1];
            state.m_color.z = cmd->m_no.m_color[2][2];
            state.m_scale = cmd->m_no.m_scale;
            state.m_pulse = false;
            m_790.SetActiveState(state);

            centered = cmd->m_no.m_centered == 1;
            m_790._strCaption.m_isCentered = centered;
            m_790._bCaptionCentered = centered;

            m_340.SetActive();
            m_790.SetNormal();
        }
        if (m_32c) {
            delete m_32c;
            m_32c = 0;
        }
        m_text.m_scale = cmd->m_scale;
        m_text.SetWidth(m_width - m_textOffset.x * 2);
        m_text.SetString(text, false);
        m_text.Update();
        m_text.SetColor(cmd->m_color[0], cmd->m_color[1], cmd->m_color[2]);
        zdb::CTexHandle *tex = PrepTexture(background, m_mgr);

        C2DBitmap::Load(0.0f, 0.0f, m_width, m_height, tex);
    }
    return true;
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
    CRdrEditor();
    ~CRdrEditor();
    bool push(const char *name);
    bool pop();
    bool addintnode(int i);
    bool addrealnode(float f);
    bool addstringnode(const char *s);

    unsigned char _0[0x4c];   // zrdr.h's: an rdr file (the value first) and the open lists' stack
};

/* strtod (MSL's; include/stdlib.h has no declaration of it) */
extern "C" double strtod(const char *s, char **end);

class CUIListBox;

/* the engine's string table (util_stable.h's CSTable, 0x20 bytes: only its destructor, 0x39FB30) */
class CSTable {
public:
    ~CSTable();

    unsigned char _0[0x20];
};

/* a list box's column (SOCOM 1's ColumnData: only what the items read -- its kind, 0 an image,
 * and the image's size). Round 56: its texture names are a ZRDR value with a string table of its
 * own after it -- the box's destructor (0x375710) inlines the teardown: the names' children freed
 * unless packed, then the table and the value destroyed as members (-1), then the texture list */
/* a column's textures (ours: a class over the pointer vector, ~CUIListBox's four tests) */
class CColumnTexList : public std::vector<zdb::CTexHandle *> {
};

class ColumnData {
public:
    ~ColumnData()
    {
        if (!m_texNames.m_packed)
            m_texNames.DeleteChildren();
    }
    const CUIVariable &Names() const { return (const CUIVariable &)m_texNames; }

    unsigned char m_type;   // 0x0
    int m_x;   // 0x4: the column's offset
    int m_width;   // 0x8
    int m_height;   // 0xc
    int m_y;   // 0x10
    unsigned char m_justify;   // 0x14: the string's 0x6A
    float m_scale;   // 0x18: the string look's scale when not 0
    CColumnTexList m_textures;   // 0x1c: an image column's, prepared by the box
    _zrdr m_texNames;   // 0x28: their names, a ZRDR array (read as zui_2d.cpp's variable reads one)
    CSTable m_strings;   // 0x30: the names' strings
};   // 0x50

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
    /* (SOCOM II's: the array look's reader constructor news one and gives it its look) */
    ListItem() : m_text(0), m_24(true), m_impl(0) {}
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
    /* the item's look a picture of the size given (ours: an inline, its arguments evaluated and
     * `this` taken ahead of the allocation -- the array look's reader constructor's s2) */
    void MakeImage(int w, int h, zdb::CTexHandle *tex);
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

/* four ints the list box's spec gives it (rectangles, by their use; the name ours) */
struct IRECT {
    int l, t, r, b;
};

/* a list box's spec (zui_spec.h's CUIListBoxSpec, 0x120 bytes: an image's, then what the box's
 * LoadSpec reads -- the names ours, by the box's fields they set; its three colours copied out
 * through inline getters returning them by value, as the target's stack temporaries show) */
class CUIListBoxSpec : public CImageSpec {
public:
    CPnt3D GetColor() const { return m_color; }
    CPnt3D GetColor13C() const { return m_color13c; }
    CPnt3D GetColor148() const { return m_color148; }

    int m_lineHeight;   // 0x24
    float m_scale;   // 0x28
    int m_2c;   // 0x2c
    int m_30;   // 0x30
    bool m_hasFont;   // 0x34
    bool m_followEnd;   // 0x35
    bool m_noHighlight;   // 0x36
    bool m_37;   // 0x37
    CSaferStr m_font;   // 0x38
    CSaferStr m_tex44;   // 0x44
    CSaferStr m_tex50;   // 0x50
    CSaferStr m_tex5c;   // 0x5c
    CSaferStr m_tex68;   // 0x68
    CSaferStr m_tex74;   // 0x74
    CSaferStr m_tex80;   // 0x80
    IRECT m_rect8c;   // 0x8c
    IRECT m_rect9c;   // 0x9c
    IRECT m_rectac;   // 0xac
    std::vector<ColumnData *> m_columns;   // 0xbc
    CPnt3D m_color;   // 0xc8
    CPnt3D m_color13c;   // 0xd4
    CPnt3D m_color148;   // 0xe0
    std::vector<ListItem *> m_items;   // 0xec
    std::vector<ActionItem> m_commands;   // 0xf8
    float m_seqStart;   // 0x104
    float m_seqLength;   // 0x108
    float m_pulseMin;   // 0x10c
    float m_pulseMax;   // 0x110
    float m_pulseTime;   // 0x114
    float m_118;   // 0x118
    int m_11c;   // 0x11c
};   // 0x120 bytes

/* a list box command (SOCOM II's 20 bytes): the header's type and argument -- a valve's name added
 * to the animation's -- and a UI variable's name index at 0x10 */
struct CListBoxCmd : public C2DAnimCmdHdr {
    unsigned char m_var;   // 0x10
};   // 0x14 bytes

/* the list box as its items read it (SOCOM 1's CUIListBox: the highlighted row, its opacity and
 * its pulse -- what DrawPrep gives the row it draws; the rest stood in). A plain bitmap first, as
 * its vtable 0x407670 holds CPlainBmp's Draw and SetMapOffset and C2DBitmap's flag setters */
class CUIListBox : public CPlainBmp {
public:
    /* slot 0 (0x375710: its items, columns and font freed) */
    virtual ~CUIListBox();
    /* slot 2: the box, its rows and arrows (0x373850) */
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    /* slot 22: the box's commands parsed (SOCOM 1's; SOCOM II's eleven) */
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr);
    /* slot 24 of its table 0x407670: the box's eleven commands after C2D's (0x373640) */
    virtual int GetZAnimCmdType(const char *name);
    /* slot 18: the box made active (0x374CB0) */
    virtual void SetActive();
    /* slot 19: the rows shown normal (0x376420, its m2c draft matches) */
    virtual void SetNormal();
    /* set up from its spec (0x375E20: CUIListBoxSpec::CreateDisplayObj's call) */
    void LoadSpec(const CUIListBoxSpec &spec, zdb::CTexHandle *tex, zdb::CTextureRelocMgr *mgr);

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
    /* the row above or below highlighted, the box's actions on event 43 run (SOCOM II's, 0x374010
     * and 0x3742F0, the names ours: HandlePad's up and down return them) */
    bool HighlightPrev();
    bool HighlightNext();
    /* every item erased and deleted, then the box's own font (SOCOM II's, 0x375C70, the name ours:
     * UpdateUIVar and 0x372980 call it) */
    void RemoveAllItems();
    /* the secondary table's slot (0x4076E0: after the thunk 0x37D060): the variable's list shown */
    virtual void UpdateUIVar(CUIVariable *var);
    /* slot 23: the box's commands run */
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    /* slot 12: the pulse, the scroll, the press and the rows (0x375040) */
    virtual void Tick(float dT);
    /* slot 15: the pad (0x3745E0) */
    virtual bool HandlePad(const CPad *pad, int port);
    /* a slot of its own, +0x84 after UpdateUIVar's (0x374A50; SOCOM 1's SetPressed, its own slot
     * there too: HandlePad calls it through the table) */
    virtual void SetPressed();

    /* the highlighted row set and the window scrolled to it (inline: UpdateUIVar and the
     * focus command do it alike, the row read back for the product) */
    void Highlight(int idx)
    {
        m_highlighted = idx;
        m_scroll = 0;
        m_offset = m_highlighted * m_lineHeight;
    }

    C2DFont *m_font;   // 0xa0
    int m_a4;   // 0xa4 (the spec's 0x2C)
    int m_a8;   // 0xa8 (the spec's 0x30)
    int m_lineHeight;   // 0xac: a row's height (the spec's)
    int m_offset;   // 0xb0: the highlighted row's offset in the list
    float m_scrollY;   // 0xb4: the window's offset, negative
    int m_scroll;   // 0xb8
    C2DFont *m_ownFont;   // 0xbc: a font of the box's own, deleted with its items
    std::list<ListItem *> m_items;   // 0xc0
    float m_scale;   // 0xcc: the items' scale
    float m_pressTime;   // 0xd0: 0.2 once pressed (SetPressed's, SOCOM 1's; the name ours)
    std::vector<ColumnData *> m_columns;   // 0xd4
    int m_highlightAlpha;   // 0xe0
    zdb::CTextureRelocMgr *m_mgr;   // 0xe4: the items' textures' relocator
    bool m_noHighlight;   // 0xe8: HIGHLIGHT_OFF's (the spec's byte at 0x36)
    bool m_followEnd;   // 0xe9: a new list highlights its last row (the spec's byte at 0x35)
    bool m_ea;   // 0xea (the spec's 0x37)
    zdb::CTexHandle *m_texEC;   // 0xec: the spec's textures, by their names at 0x50, 0x5C, 0x44
    zdb::CTexHandle *m_texF0;   // 0xf0
    zdb::CTexHandle *m_texF4;   // 0xf4
    zdb::CTexHandle *m_texF8;   // 0xf8: the highlight's texture (0xF4's, SetActive's)
    zdb::CTexHandle *m_texFC;   // 0xfc: and 0x68
    IRECT m_rect100;   // 0x100 (the spec's 0x8C)
    IRECT m_rect110;   // 0x110 (the spec's 0xAC)
    IRECT m_rect120;   // 0x120 (the spec's 0x9C)
    CPnt3D m_color;   // 0x130: the rows' colour
    CPnt3D m_color13c;   // 0x13c: the highlight's (SetActive copies it to 0x154)
    CPnt3D m_color148;   // 0x148
    CPnt3D m_highlightColor;   // 0x154
    float m_seqStart;   // 0x160: the first row's sequence start
    float m_seqLength;   // 0x164: each row's sequence length
    float m_pulseMin0;   // 0x168: the pulse's limits and time as the spec gives them (SetActive's)
    float m_pulseMax0;   // 0x16c
    float m_pulseTime0;   // 0x170
    float m_pulseMin;   // 0x174
    float m_pulseMax;   // 0x178
    float m_pulseTime;   // 0x17c
    float m_repeat;   // 0x180: the held direction's repeat time (HandlePad zeroes it, steps past 0.4; the name ours)
    float m_184;   // 0x184 (the spec's 0x118)
    zdb::CTexHandle *m_tex188;   // 0x188: the spec's textures by their names at 0x80 and 0x74
    zdb::CTexHandle *m_tex18c;   // 0x18c
    int m_190;   // 0x190 (the spec's 0x11C)
    int m_highlighted;   // 0x194
    unsigned char _198[0x1a0 - 0x198];
    CPlainBmp m_highlight;   // 0x1a0: the highlight's bitmap
    CPlainBmp m_bar;   // 0x240
    CPlainBmp m_2e0;   // 0x2e0 (the destructor tears down four plain bitmaps)
    CPlainBmp m_380;   // 0x380
    ActionItem m_actions[30];   // 0x420: the box's actions (the spec's ANIMATIONS)
    int m_numActions;   // 0x600
};

/* made active (SOCOM 1's CUIListBox::SetActive, 0x393D60 there; SOCOM II's body): the highlight's
 * colour, the bar on and loaded, the highlight's bitmap textured, white and at 100, the pulse's
 * limits and time taken from the spec's; with rows, the highlighted one found (no further than
 * the last), the window scrolled to it, its own action run, then the box's actions on event 2 --
 * each given its own argument, else the row's text when the row passes it on. One counter for the
 * search and the actions' walk (each its own swaps the walk's two saved registers); the row's
 * animation tested through a const view (the target reads it twice); the scroll's difference a
 * local (in the product MW puts the height first) */
void CUIListBox::SetActive()
{
    m_highlightColor = m_color13c;
    m_bar.On();
    m_bar.Load(0.0f, 0.0f, m_rect100.r, m_rect100.b, m_texFC);

    zdb::CTexHandle *t = m_texF4;

    m_texF8 = t;
    m_highlight.Load(0.0f, 0.0f, 10.0f, 10.0f, m_texF8);
    if (!t)
        m_highlight.SetHasTexture(false);
    else
        m_highlight.SetHasTexture(true);
    m_highlight.SetColor(255.0f, 255.0f, 255.0f);
    m_highlight.SetTrans(m_highlightAlpha);
    m_highlight.SetTrans(100.0f);
    m_highlightAlpha = 100;
    m_pulseTime = m_pulseTime0;
    m_pulseMin = m_pulseMin0;
    m_pulseMax = m_pulseMax0;
    if (m_items.size()) {
        std::list<ListItem *>::iterator it;
        int i;

        it = m_items.begin();
        for (i = 0; i < m_highlighted && i < m_items.size() - 1; i++)
            ++it;
        m_highlighted = i;
        if (m_highlighted < m_scroll)
            m_scroll = m_highlighted;
        int d = m_highlighted - m_scroll;

        m_offset = d * m_lineHeight;
        m_scrollY = -(m_scroll * m_lineHeight);

        ListItem *item = *it;

        if (((const ListItem *)item)->m_action2.Animation) {
            CZAnim *anim = ZAnim.GetAnim(item->m_action2.Animation, 0);

            if (anim) {
                if (item->m_action2.Argument)
                    anim->ActivateArg(1, 8, item->m_action2.Argument, 0);
                else
                    anim->Activate(0);
            }
        }
        for (i = 0; i < m_numActions; i++)
            if (m_actions[i].Type == 1 && m_actions[i].Event == 2) {
                CZAnim *anim = ZAnim.GetAnim(m_actions[i].Animation, 0);

                if (m_actions[i].Argument)
                    anim->ActivateArg(1, 8, m_actions[i].Argument, 0);
                else if (item->m_text && item->m_24)
                    anim->ActivateArg(1, 8, item->m_text, 0);
                else if (item->m_24)
                    anim->ActivateArg(1, 8, item->GetString(), 0);
            }
    }
}

/* slot 12 (SOCOM 1's CUIListBox::Tick, 0x38D5E0 there): the bitmap's pulse; the highlighted row
 * kept two rows clear of the bottom and above the top by scrolling, the box's actions on event 41
 * run when it scrolled; the window slid toward the scroll at twice the distance a second (no
 * slower than the spec's 0x118), snapped once within a step, the actions on event 42 run when it
 * arrives; the press time run down (at its end the highlight back to active when still the
 * pressed colour); the repeat time counted and every row ticked. SOCOM II's additions: the two
 * events, the minimum speed, the press's end. Byte-exact (round 52) with the bottom margin a sum
 * (`2 * h` is sll, the target's addu) and the rows' end a local ahead of the walk (in the test, the
 * walk's two saved registers swap; ahead of the repeat's add, alike) */
void CUIListBox::Tick(float dT)
{
    CPlainBmp::Tick(dT);

    int scroll = m_scroll;

    while (m_offset > m_iHeight - (m_lineHeight + m_lineHeight)) {
        m_offset -= m_lineHeight;
        m_scroll++;
    }
    while (m_offset < 0) {
        m_offset += m_lineHeight;
        m_scroll--;
    }
    if (scroll != m_scroll)
        for (int i = 0; i < m_numActions; i++)
            if (m_actions[i].Type == 1 && m_actions[i].Event == 41) {
                CZAnim *anim = ZAnim.GetAnim(m_actions[i].Animation, 0);

                if (anim) {
                    if (m_actions[i].Argument)
                        anim->ActivateArg(1, 8, m_actions[i].Argument, 0);
                    else
                        anim->Activate(0);
                }
            }

    float target = -m_scroll * m_lineHeight;
    float diff = target - m_scrollY;
    float speed = 2.0f * (float)fabs(diff);

    if (speed < m_184)
        speed = m_184;

    float step = speed * dT;

    if (m_pressTime > 0.0f) {
        if (m_pressTime > dT)
            m_pressTime -= dT;
        else {
            m_pressTime = 0.0f;
            if (m_highlightColor == m_color148)
                SetActive();
        }
    }
    if (step <= (float)fabs(diff)) {
        if (diff < 0.0f)
            step = -step;
        m_scrollY += step;
    } else
        m_scrollY = target;
    if (diff != 0.0f && m_scrollY == target)
        for (int i = 0; i < m_numActions; i++)
            if (m_actions[i].Type == 1 && m_actions[i].Event == 42) {
                CZAnim *anim = ZAnim.GetAnim(m_actions[i].Animation, 0);

                if (anim) {
                    if (m_actions[i].Argument)
                        anim->ActivateArg(1, 8, m_actions[i].Argument, 0);
                    else
                        anim->Activate(0);
                }
            }
    std::list<ListItem *>::iterator e = m_items.end();

    m_repeat += dT;
    for (std::list<ListItem *>::iterator it = m_items.begin(); it != e; ++it)
        (*it)->Tick(dT);
}

/* a list variable shown (nothing else): every item removed, one made from each of its values with
 * the box's columns, font and scale -- a string's own text set -- and added at the end; the last
 * row highlighted when the box follows its end; the rows' sequences laid one after another from the
 * box's start; then the actions on event 46 run (SOCOM II's: SOCOM 1's built its rows otherwise).
 * The counter an int (unsigned, MW strength-reduces Item's i*8 into a sixth saved register); the
 * sequence walk's end a local declared before its iterator (end() in the test swaps the two's saved
 * registers) */
void CUIListBox::UpdateUIVar(CUIVariable *var)
{
    if (var->IsArray()) {
        RemoveAllItems();
        for (int i = 0; i < var->Count(); i++) {
            CUIVariable *e = var->Item(i);
            ListItem *item = new ListItem(e, m_columns, m_font, m_scale);

            if (e && e->IsString())
                item->SetString(e->GetString());
            AddItem(item, -1);
        }
        if (m_followEnd)
            Highlight(m_items.size() - 1);

        float t = m_seqStart;
        std::list<ListItem *>::iterator e = m_items.end();

        for (std::list<ListItem *>::iterator it = m_items.begin(); it != e; ++it)
            t = (*it)->SetSequenceTime(t, m_seqLength);
        for (int i = 0; i < m_numActions; i++)
            if (m_actions[i].Type == 1 && m_actions[i].Event == 46) {
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

/* set up from its spec (SOCOM 1's CUIListBox::LoadSpec, 0x38E2E0 there; SOCOM II's columns and
 * rows): the plain bitmap's own setup, the spec's sizes, scale, flags and timings, a font of its
 * own when the spec names one; each column taken over, an image column's textures prepared from
 * its names; each row prepared for the columns and taken over; the variable watched again; the
 * actions; the textures, rectangles and colours; shown normal. One counter for the three walks
 * (each loop's own swaps it with the push_back end node's and the action pointer's saved registers);
 * the column and the row each read into a local of their own beside the one push_back takes by
 * reference (through that one MW reloads it from its frame slot for the test and the call) */
void CUIListBox::LoadSpec(const CUIListBoxSpec &spec, zdb::CTexHandle *tex, zdb::CTextureRelocMgr *mgr)
{
    int i;

    CPlainBmp::LoadSpec(spec, tex);
    m_tex188 = 0;
    m_tex18c = 0;
    m_mgr = mgr;
    m_a4 = spec.m_2c;
    m_a8 = spec.m_30;
    m_lineHeight = spec.m_lineHeight;
    m_scale = spec.m_scale;
    m_noHighlight = spec.m_noHighlight;
    m_followEnd = spec.m_followEnd;
    m_ea = spec.m_37;
    m_seqStart = spec.m_seqStart;
    m_seqLength = spec.m_seqLength;
    m_pulseMax0 = spec.m_pulseMax;
    m_pulseMin0 = spec.m_pulseMin;
    m_pulseTime0 = spec.m_pulseTime;
    m_184 = spec.m_118;
    m_190 = spec.m_11c;
    if (spec.m_hasFont) {
        const char *name = spec.m_font.c_str();

        m_ownFont = new C2DFont;
        m_ownFont->Load(FetchMissionFontsRdr(), name);
        m_font = m_ownFont;
    }
    for (i = 0; i < (int)spec.m_columns.size(); i++) {
        ColumnData *c = spec.m_columns[i];
        ColumnData *col = c;

        if (c->m_type == 0)
            for (int j = 0; j < col->Names().Count(); j++) {
                zdb::CTexHandle *t = PrepTexture(col->Names().Item(j)->GetString(), mgr);

                col->m_textures.push_back(t);
            }
        m_columns.push_back(col);
    }
    for (i = 0; i < (int)spec.m_items.size(); i++) {
        ListItem *p = spec.m_items[i];
        ListItem *item = p;

        p->PrepListItem(m_columns, m_font, m_scale, m_mgr);
        m_items.push_back(item);
    }

    CUIVariable *var = spec.GetUIVar();

    if (var)
        WatchUIVar(var);
    for (i = 0; i < (int)spec.m_commands.size(); i++) {
        g_hasAnims = 1;
        m_numActions++;
        m_actions[i] = spec.m_commands[i];
    }
    m_texEC = PrepTexture(spec.m_tex50.c_str(), mgr);
    m_texF0 = PrepTexture(spec.m_tex5c.c_str(), mgr);
    m_texF4 = PrepTexture(spec.m_tex44.c_str(), mgr);
    m_texFC = PrepTexture(spec.m_tex68.c_str(), mgr);
    m_rect100 = spec.m_rect8c;
    m_color = spec.GetColor();
    m_color13c = spec.GetColor13C();
    m_color148 = spec.GetColor148();
    SetNormal();
    m_tex18c = PrepTexture(spec.m_tex74.c_str(), mgr);
    m_tex188 = PrepTexture(spec.m_tex80.c_str(), mgr);
    m_rect110 = spec.m_rectac;
    m_rect120 = spec.m_rect9c;
}

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

/* the box torn down (slot 0; SOCOM 1's has no DWARF here): every item's node erased before the item
 * is deleted, as RemoveAllItems does, then every column's (the pointer vector's erase, its result
 * kept in a register over the delete), then the box's own font under its own test (two null tests,
 * as the target's); then the members: the actions, four plain bitmaps (0x1A0-0x41F), the columns'
 * and items' lists, the plain bitmap's base.
 * Byte-exact since round 65 (open since round 56): the target tests a column's texture list FOUR times before freeing it
 * -- a class over the pointer vector (zdb::CTexList's shape, round 27's rule), one level more than a plain vector's three --
 * and with ColumnData's written destructor that chain is six high. A MODELLING DEVICE closes it at level 5 with no pragma
 * (the weapons' round 40: a member's destructor called from the function's own text inlines whole): `delete col` written
 * out, ColumnData's destructor and its members' in their order; a "Level 6, or 8?" witness, the original likely a plain
 * `delete col`. The items' end a local iterator: scalarised ahead of the column's erase result, as the replay asked */
CUIListBox::~CUIListBox()
{
    std::list<ListItem *>::iterator it;
    std::list<ListItem *>::iterator e = m_items.end();

    it = m_items.begin();
    while (it != e) {
        ListItem *item = *it;
        std::list<ListItem *>::iterator next;

        next = m_items.erase(it);

        delete item;
        it = next;
    }
    std::vector<ColumnData *>::iterator c = m_columns.begin();

    while (c != m_columns.end()) {
        ColumnData *col = *c;
        std::vector<ColumnData *>::iterator next = m_columns.erase(c);

        if (col) {
            if (!col->m_texNames.m_packed)
                col->m_texNames.DeleteChildren();
            col->m_strings.~CSTable();
            col->m_texNames.~_zrdr();
            col->m_textures.~CColumnTexList();
            operator delete(col);
        }
        c = next;
    }
    if (m_ownFont)
        delete m_ownFont;
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

/* the pad as the buttons read it: each button's key state (zinput.h's CPad::m_state from
 * 0x1; 2 a press, 1 a release) */
class CPad {
public:
    unsigned char m_0;   // 0x0
    unsigned char m_state[16];   // 0x1
    unsigned char _11[0x21c - 0x11];
    float m_zoom;   // 0x21c: a stick's axis (the map control zooms by it)
};

void PlaySelectionSound();
/* zui.cpp's: the stick pushed up or down since it was last centred (UiGetDirStickMoved 3 or 4) */
bool DidStickMovePrev();
bool DidStickMoveNext();

/* the window's clip against the lines' base (a variable: read with lwc1 and converted; .data
 * 0x3E0370, -4; the name ours) */
static int s_wrappedClipDY = -4;

/* drawn (SOCOM 1's CWrappedText::Draw, 0x391110 there): the window scissored, the lines scrolled --
 * from the bottom when the text outgrows an auto-scrolling window --, each line placed and drawn if it
 * falls on the screen, advancing across for one line or down by its own count, the total kept, and
 * the two arrows drawn with no matrix: the up one while there is scroll to go back, the down one while
 * there is more below */
void CWrappedText::Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr)
{
    CPnt3D zero = CPnt3D::zero;
    CPnt3D p = zero;
    CPnt3D q = zero;

    m.Transform(&p, 1);
    m.Transform(&q, 1);

    int lines = 0;

    if (m_shown > 0) {
        float &py = p.y;
        int x1 = (int)p.x;
        int y1 = (int)((float)s_wrappedClipDY + (py - GetSpacing()));
        int x2 = (int)(p.x + m_width);

        zVid_Scissor(x1, y1, x2, (int)((float)s_wrappedClipDY + (py + (float)m_shown - GetSpacing())));
        if (m_autoScroll && (float)GetNumLines() * GetSpacing() > (float)m_shown) {
            py += m_offset;
            py -= (float)GetNumLines() * GetSpacing() - (float)m_shown;
        } else {
            py -= m_offset;
        }
    }
    C2Dlist::iterator e = end();
    C2Dlist::iterator it = begin();

    while (it != e) {
        C2DString *line = (C2DString *)*it;

        line->SetPos((int)p.x, (int)(p.y + (float)lines * GetSpacing() - 3.0f));

        int y = (int)(line->m_posy + line->m_socom2_80);

        ++it;
        if (y > 0 && (unsigned int)y < zVid.renderHeight)
            line->Draw(mgr, 0, (int)p.z);
        if (m_displayOneLine)
            p.x += line->m_length;
        else if (line->m_numLines > 0)
            lines += line->m_numLines;
        else
            lines++;
    }
    m_numLines = lines;
    zVid_Scissor(0, 0, 640, 448);

    CMatrix mat;

    if (m_upTex) {
        float y = q.y + (float)m_upOffset.y;
        float x = q.x + (float)m_upOffset.x;

        m_upArrow.Load(x, y, (float)m_upSize.x + x, (float)m_upSize.y + y, m_upTex);
        if ((m_offset <= 0.0f && m_numLines != 0) || m_numLines == 0)
            m_upArrow.Off();
        else
            m_upArrow.On();
        m_upArrow.Draw(mat, mgr);
    }
    if (m_downTex) {
        float y = q.y + (float)m_downOffset.y;
        float x = q.x + (float)m_downOffset.x;

        m_downArrow.Load(x, y, (float)m_downSize.x + x, (float)m_downSize.y + y, m_downTex);
        if (m_offset < (float)GetNumLines() * GetSpacing() - (float)m_shown)
            m_downArrow.On();
        else
            m_downArrow.Off();
        m_downArrow.Draw(mat, mgr);
    }
}

/* the slider's menu (zui.cpp's CGameMenu: the event it runs, SLIDER_MOVED; PlaySelectionSound is
 * zui.cpp's too) */
/* the slider's value written to its variable: the left half of the bar scaled below the centre
 * value, the right half above it (ours: the inverse of UpdateUIVar below) */
inline void CSliderBar::ApplyValue()
{
    CUIVariable *var = UIVarMixin::m_uivar;
    float v;

    if (m_value < 0.5f)
        v = 2.0f * m_value * m_center;
    else
        v = m_center + 2.0f * (m_value - 0.5f) * (1.0f - m_center);
    if (var)
        var->SetReal(v);
}

/* the pad on the slider (slot 15 of its vtable; the 2003 slider has no counterpart in SOCOM 1's): left
 * and right step its value by the interval, a percent each, clamped; a fresh press restarts the
 * repeat's clock and runs the menu's event; held, it steps once the clock has passed 0.4; each step
 * writes the variable and plays the selection sound */
bool CSliderBar::HandlePad(const CPad *pad, int port)
{
    if (!CanHandleInput())
        return false;
    if (pad->m_state[3] == 1) {
        m_value -= 0.01f * (float)m_intervals;
        if (m_value < 0.0f)
            m_value = 0.0f;
        ApplyValue();
        PlaySelectionSound();
        m_repeat = 0.0f;
        ((CGameMenu *)UiGetCurMenu())->RunEventAnim(ActionItem::SLIDER_MOVED, 0);
        return true;
    }
    if (pad->m_state[3] == 2) {
        if (m_repeat > 0.4f) {
            m_value -= 0.01f * (float)m_intervals;
            if (m_value < 0.0f)
                m_value = 0.0f;
            ApplyValue();
            PlaySelectionSound();
        }
        return true;
    }
    if (pad->m_state[2] == 1) {
        m_value += 0.01f * (float)m_intervals;
        if (m_value > 1.0f)
            m_value = 1.0f;
        ApplyValue();
        PlaySelectionSound();
        m_repeat = 0.0f;
        ((CGameMenu *)UiGetCurMenu())->RunEventAnim(ActionItem::SLIDER_MOVED, 0);
        return true;
    }
    if (pad->m_state[2] == 2) {
        if (m_repeat > 0.4f) {
            m_value += 0.01f * (float)m_intervals;
            if (m_value > 1.0f)
                m_value = 1.0f;
            ApplyValue();
            PlaySelectionSound();
        }
        return true;
    }
    return false;
}

/* pressed (SOCOM 1's CUIListBox::SetPressed, 0x38D4E0 there): the highlight's colour the pressed
 * one, the highlight's bitmap loaded from the pressed texture (textured when there is one --
 * SOCOM II sets it through the table, SOCOM 1 the bit), white, its opacity the box's then 100,
 * and the press time 0.2 (SetActive's head with the pressed colour and texture) */
void CUIListBox::SetPressed()
{
    m_highlightColor = m_color148;

    zdb::CTexHandle *t = m_texF0;

    m_texF8 = t;
    m_highlight.Load(0.0f, 0.0f, 10.0f, 10.0f, m_texF8);
    if (!t)
        m_highlight.SetHasTexture(false);
    else
        m_highlight.SetHasTexture(true);
    m_highlight.SetColor(255.0f, 255.0f, 255.0f);
    m_highlight.SetTrans(m_highlightAlpha);
    m_highlight.SetTrans(100.0f);
    m_highlightAlpha = 100;
    m_pressTime = 0.2f;
}

/* slot 15: the pad (SOCOM 1's CUIListBox::HandlePad, 0x38D010 there): nothing without input or
 * rows; the highlighted row brought back inside the list (and the window to it), found by the
 * GetItemText walk; the row's own action on its button's press runs (its argument, else its text)
 * and presses the box; else the first of the box's actions whose event is in its state runs (its
 * argument, else the row's text or string, as SetActive's), pressing the box when the row passes
 * its text on; else up / down (or the stick, or held past the repeat time) highlight the row
 * above or below; else a sideways press or a stick move is taken when the box wraps. The counter
 * zeroed at the top and the walk not re-initialising it (as SOCOM 1's: `for (i = 0; ...)` is
 * unrolled 8x, 0x504 bytes); the event tested `!= 0` (a plain byte test re-masks it with andi).
 * The action test compares the state first (`bne v0,v1`) with the pad's state for the event
 * loaded before the state is: the pad's side a value assigned in two arms (round 53 -- the arms
 * the original's are not known, here alike). MW compares in source order and numbers each load
 * where it is generated, left first -- written state first, the state took v1 and the delay slot
 * emptied (18 off); pad first, the registers were the target's but the compare `bne v1,v0` (1
 * off). A value from a two-armed assignment is never forward-substituted into its use, so it is
 * generated ahead of the compare and the state after it; a single assignment (a named local, an
 * inline returning the load) is substituted back (18), and an inline for the state on the left
 * is swapped to the right by MW (1) */
bool CUIListBox::HandlePad(const CPad *pad, int port)
{
    int i = 0;

    if (!CanHandleInput())
        return false;
    if (!m_items.size())
        return false;
    if ((unsigned int)m_highlighted >= m_items.size()) {
        m_highlighted = m_items.size() - 1;
        if (m_highlighted < m_scroll)
            m_scroll = m_highlighted;

        int d = m_highlighted - m_scroll;

        m_offset = d * m_lineHeight;
    }

    std::list<ListItem *>::iterator it;

    it = m_items.begin();
    for (; i < m_highlighted; i++)
        ++it;

    ListItem *item = *it;

    if (item->m_action.Event < 16 && pad->m_state[item->m_action.Event] == 1 && item->m_24) {
        m_repeat = 0.0f;

        CZAnim *anim = ZAnim.GetAnim(item->m_action.Animation, 0);

        if (!anim)
            return false;
        if (item->m_action.Argument)
            anim->ActivateArg(1, 8, item->m_action.Argument, 0);
        else if (item->m_text)
            anim->ActivateArg(1, 8, item->m_text, 0);
        else
            anim->Activate(0);
        SetPressed();
        return true;
    }
    for (int j = 0; j < m_numActions; j++)
        if (m_actions[j].Event != 0
            && m_actions[j].State == (m_actions[j].Event ? pad->m_state[m_actions[j].Event]
                                                         : pad->m_state[m_actions[j].Event])) {
            CZAnim *anim = ZAnim.GetAnim(m_actions[j].Animation, 0);

            m_repeat = 0.0f;
            if (!anim)
                return false;
            if (m_actions[j].Argument)
                anim->ActivateArg(1, 8, m_actions[j].Argument, 0);
            else if (item->m_text && item->m_24)
                anim->ActivateArg(1, 8, item->m_text, 0);
            else if (item->m_24)
                anim->ActivateArg(1, 8, item->GetString(), 0);
            if (item->m_24)
                SetPressed();
            return true;
        }
    if (pad->m_state[4] == 1 || DidStickMovePrev()) {
        m_repeat = 0.0f;
        return HighlightPrev();
    }
    if (pad->m_state[4] == 2 && m_repeat > 0.4f)
        return HighlightPrev();
    if (pad->m_state[5] == 1 || DidStickMoveNext()) {
        m_repeat = 0.0f;
        return HighlightNext();
    }
    if (pad->m_state[5] == 2 && m_repeat > 0.4f)
        return HighlightNext();
    if (m_ea && (pad->m_state[3] == 1 || pad->m_state[2] == 1 || DidStickMoveNext() || DidStickMovePrev()))
        return true;
    return false;
}

/* the next row highlighted (SOCOM II's, 0x3742F0: HandlePad's down, the name ours): at the last
 * row, the box's wrap flag returned; else the row below taken -- the offset a row further, the
 * row found by the GetItemText walk -- its own action run, the selection sound, and the box's
 * actions on event 43, as SetActive's on event 2 (the row's index printed and unused) */
bool CUIListBox::HighlightNext()
{
    if (m_highlighted >= (int)m_items.size() - 1)
        return m_ea;
    m_highlighted++;
    m_offset += m_lineHeight;

    std::list<ListItem *>::iterator it;

    it = m_items.begin();
    for (int i = 0; i < m_highlighted; i++)
        ++it;

    ListItem *item = *it;

    if (((const ListItem *)item)->m_action2.Animation) {
        CZAnim *anim = ZAnim.GetAnim(item->m_action2.Animation, 0);

        if (anim) {
            if (item->m_action2.Argument)
                anim->ActivateArg(1, 8, item->m_action2.Argument, 0);
            else
                anim->Activate(0);
        }
    }
    PlaySelectionSound();

    char buf[12];

    sprintf(buf, "%d", m_highlighted);
    for (int i = 0; i < m_numActions; i++)
        if (m_actions[i].Type == 1 && m_actions[i].Event == 43) {
            CZAnim *anim = ZAnim.GetAnim(m_actions[i].Animation, 0);

            if (m_actions[i].Argument)
                anim->ActivateArg(1, 8, m_actions[i].Argument, 0);
            else if (item->m_text && item->m_24)
                anim->ActivateArg(1, 8, item->m_text, 0);
            else if (item->m_24)
                anim->ActivateArg(1, 8, item->GetString(), 0);
        }
    return true;
}

/* the previous row highlighted (SOCOM II's, 0x374010: HandlePad's up, the name ours): at the
 * first row, the box's wrap flag returned; else as HighlightNext, a row back */
bool CUIListBox::HighlightPrev()
{
    if (m_highlighted <= 0)
        return m_ea;
    m_highlighted--;
    m_offset -= m_lineHeight;

    std::list<ListItem *>::iterator it;

    it = m_items.begin();
    for (int i = 0; i < m_highlighted; i++)
        ++it;

    ListItem *item = *it;

    if (((const ListItem *)item)->m_action2.Animation) {
        CZAnim *anim = ZAnim.GetAnim(item->m_action2.Animation, 0);

        if (anim) {
            if (item->m_action2.Argument)
                anim->ActivateArg(1, 8, item->m_action2.Argument, 0);
            else
                anim->Activate(0);
        }
    }
    PlaySelectionSound();

    char buf[12];

    sprintf(buf, "%d", m_highlighted);
    for (int i = 0; i < m_numActions; i++)
        if (m_actions[i].Type == 1 && m_actions[i].Event == 43) {
            CZAnim *anim = ZAnim.GetAnim(m_actions[i].Animation, 0);

            if (m_actions[i].Argument)
                anim->ActivateArg(1, 8, m_actions[i].Argument, 0);
            else if (item->m_text && item->m_24)
                anim->ActivateArg(1, 8, item->m_text, 0);
            else if (item->m_24)
                anim->ActivateArg(1, 8, item->GetString(), 0);
        }
    return true;
}

/* the iterator taken from begin() is a local in memory (ASSIGNED from begin(), its copy goes through
 * a float register; initialised from it, it lives in a register: 0xC4 against 0xE4), stepped on to
 * the highlighted row or the end.
 * BYTE-EXACT SINCE THE MSL HEADERS' ROUND 39, through include/list's __push_back_deep and __insert_deep (the deeper pair by other
 * names: as push_back and insert themselves it is +1 / -15, two of them this unit's LoadSpecs) and `int i = 0` declared after the iterator.
 * Until then 6 instructions (stack slots): the target puts push_back's end and returned iterators at
 * sp+60/64, below the insert's returned one at 68; here they were at 64/68 and the insert's at 60.
 * Tried: the if/else the other way round (0xEC), the iterator declared at the top, the insert's
 * result assigned or kept, insert(end(), item) for the push_back (6 each); round 56: braces round
 * the push_back, the iterator at function scope (6), the push_back through a one-level-deeper
 * inline wrapper, by reference or by value (7, 0xF4: its temporaries then sink below the position
 * copy) -- the target's order reads like SOCOM 1's two-level push_back (include/list's "vc").
 * The MSL headers round 36 (build/p36/additem.py, untracked): with list<T *>::insert returning the
 * BASE iterator directly (`typename base::iterator insert(...) { return base::insert(...); }`) the
 * insert's return slot is the caller's, 68 -- the slots right -- and with `int i = 0;` declared after
 * the iterator (or the walk a while) AddItem is BYTE-EXACT; but the header costs 0x510B90 (the
 * wrapper's out-of-line insert, which wraps the base's result through a temporary), CGraph::
 * BuildRoute 0x2D14A0 and CSched_Manager::AddTask 0x34E6B0 (their insert slots below the other
 * temporaries): +1 / -3, not applied (include/list's note). Under today's header that spelling is
 * 4 off */
void CUIListBox::AddItem(ListItem *item, int where)
{
    item->SetFont(m_font);
    item->SetScale(m_scale);
    if (where == -1)
        m_items.__push_back_deep(item);
    else {
        std::list<ListItem *>::iterator it;
        int i = 0;

        it = m_items.begin();
        for (; i < m_highlighted && it != m_items.end(); i++)
            ++it;
        m_items.__insert_deep(it, item);
    }
}

/* the scissor's top against the first row's line (a variable: read with lw; .data 0x3E0378, -3; the
 * name ours) */
static int s_listClipDY = -3;

/* drawn (SOCOM 1's CUIListBox::Draw, SOCOM II's rows of several lines): with rows, the highlight's
 * and the bar's rectangles found -- the rows before the highlighted one each push it down by their
 * extra lines -- and the bar drawn; the box itself; then, scissored, the highlight and each row on
 * the screen, the highlighted row's opacity kept for the two scroll arrows, each drawn while there is
 * scroll to go that way. The walk's item and height are MODELLING DEVICES, one-member structs (the
 * kit's round 60: scalarised, so numbered after the iterator -- the replay wants the iterator between
 * rowy and them, and no plain local can follow a scalarised member) */
void CUIListBox::Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr)
{
    CPnt3D p = CPnt3D::zero;

    m.Transform(&p, 1);

    int hy = (int)(m_offset + (m_190 + (p.y + m_a8)) - 0.75f * m_lineHeight);
    int hx = (int)p.x;
    int x = (int)(p.x + m_a4);
    int y = (int)(p.y + m_a8);
    int barx = (int)(p.x + m_rect100.l);
    int bary = (int)(p.y + m_rect100.t);
    float trans;
    CMatrix unit;

    unit.m_matrix[3][0] = 0.0f;
    unit.m_matrix[3][1] = 0.0f;
    unit.m_matrix[3][2] = p.z;
    if (m_items.size()) {
        int height = 0;
        int i = 0;
        std::list<ListItem *>::iterator it = m_items.begin();
        std::list<ListItem *>::iterator e = m_items.end();

        for (; it != e; ++it) {
            if (i == m_highlighted) {
                height = m_lineHeight * (*it)->GetNumLines();
                break;
            }
            hy += m_lineHeight * ((*it)->GetNumLines() - 1);
            i++;
        }
        m_highlight.Load(hx, hy, hx + m_iWidth, hy + height, m_texF8);
        m_bar.Load(barx, bary, barx + m_rect100.r, bary + m_rect100.b, m_texFC);
        m_bar.Draw(unit, mgr);
    }
    CPlainBmp::Draw(m, mgr);

    int row = 0;

    zVid_Scissor(0, s_listClipDY + (y - m_lineHeight), zVid.renderWidth, (int)(p.y + m_iHeight));
    if (!m_noHighlight)
        m_highlight.Draw(unit, mgr);

    int offset = 0;
    std::list<ListItem *>::iterator it = m_items.begin();
    std::list<ListItem *>::iterator e = m_items.end();

    for (; it != e; ++it) {
        int rowy = (int)(offset + (y + m_scrollY));
        struct { ListItem *p; } row_item;
        row_item.p = *it;

        if (row == m_highlighted)
            trans = row_item.p->GetTrans();

        struct { int v; } row_lh;
        row_lh.v = m_lineHeight;
        int bottom = (int)(10.0f + (row_lh.v + (p.y + m_iHeight)));

        offset += m_lineHeight * row_item.p->GetNumLines();
        if (rowy >= y - row_lh.v + 1 && rowy <= bottom) {
            row_item.p->DrawPrep(*this, x, rowy, row);
            row_item.p->VisualGratification(mgr, p.z);
        }
        row++;
    }
    zVid_Scissor(0, 0, 640, 448);
    if (m_tex18c) {
        float y0 = p.y + m_rect120.t;
        float x0 = p.x + m_rect120.l;

        m_2e0.Load(x0, y0, m_rect120.r + x0, m_rect120.b + y0, m_tex18c);
        if (m_scrollY < 0.0f && m_items.size())
            m_2e0.SetTrans(trans);
        else
            m_2e0.SetTrans(0.0f);
        m_2e0.Draw(unit, mgr);
    }
    if (m_tex188) {
        float y0 = p.y + m_rect110.t;
        float x0 = p.x + m_rect110.l;

        m_380.Load(x0, y0, m_rect110.r + x0, m_rect110.b + y0, m_tex188);
        if (m_iHeight - m_lineHeight >= (int)(m_scrollY + m_lineHeight * (m_items.size() - 1)) ||
            !m_items.size())
            m_380.SetTrans(0.0f);
        else
            m_380.SetTrans(trans);
        m_380.Draw(unit, mgr);
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

/* the box's commands (SOCOM II's eleven; SOCOM 1's four): FINISH finishes every row and the
 * command; the highlight switched; HAS_ITEMS answers through *done; the count and the focus row
 * given to a valve made for it, or the focus taken from one; AddItem a row made from the n-th
 * value of a UI variable, n a valve's; RemoveItem the n-th row taken out and deleted; GetContents
 * the rows written into a UI variable; RemoveAllItems. Each is done at once; anything else none */
bool CUIListBox::TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done)
{
    CListBoxCmd *cmd = (CListBoxCmd *)hdr;

    switch (cmd->m_type - C2DCmdType(0)) {
    case 1:
        {
            std::list<ListItem *>::iterator e = m_items.end();

            for (std::list<ListItem *>::iterator it = m_items.begin(); it != e; ++it)
                (*it)->Finish();
        }
        *done = true;
        return true;
    case 2:
        m_noHighlight = false;
        return true;
    case 3:
        m_noHighlight = true;
        return true;
    case 4:
        *done = m_items.size() != 0;
        return true;
    case 5:
        {
            CValve *valve = CValve::Create(ZAnim.m_CurAnim->GetName(cmd->m_arg), 0, VALVE_TEMP);

            valve->SetValue(m_items.size());
        }
        return true;
    case 6:
        {
            CValve *valve = CValve::Create(ZAnim.m_CurAnim->GetName(cmd->m_arg), 0, VALVE_TEMP);

            if (valve)
                valve->SetValue(m_highlighted);
        }
        return true;
    case 7:
        {
            CValve *valve = CValve::GetByName(ZAnim.m_CurAnim->GetName(cmd->m_arg));

            if (valve)
                Highlight(valve->m_value);
        }
        return true;
    case 8:
        {
            CZAnim *anim = ZAnim.m_CurAnim;
            const char *valveName = anim->GetName(cmd->m_arg);
            const char *varName = anim->GetName(cmd->m_var);
            CValve *valve = CValve::GetByName(valveName);
            CUIVariable *var = theUIVarManager.Find(varName);

            if (valve && var) {
                CUIVariable *e = var->Item(valve->m_value);
                ListItem *item = new ListItem(e, m_columns, m_font, m_scale);

                if (e && e->IsString())
                    item->SetString(e->GetString());
                AddItem(item, -1);
                if (m_followEnd)
                    Highlight(m_items.size() - 1);
            }
        }
        return true;
    case 9:
        {
            CValve *valve = CValve::GetByName(ZAnim.m_CurAnim->GetName(cmd->m_arg));

            if (valve) {
                std::list<ListItem *>::iterator it;

                it = m_items.begin();
                for (unsigned int i = 0; i < valve->GetValue(); i++)
                    ++it;

                ListItem *item = *it;

                m_items.remove(item);
                delete item;
            }
        }
        return true;
    case 11:
        RemoveAllItems();
        return true;
    case 10:
        {
            CUIVariable *var = theUIVarManager.Find(ZAnim.m_CurAnim->GetName(cmd->m_var));

            if (var) {
                CRdrEditor ed;

                ed.push(0);
                std::list<ListItem *>::iterator e = m_items.end();

                for (std::list<ListItem *>::iterator it = m_items.begin(); it != e; ++it)
                    (*it)->AddToRdr(&ed);
                ed.pop();
                var->Set((_zrdr *)&ed);
            }
        }
        return true;
    default:
        return false;
    }
}

/* the eleven commands, each a 20-byte record typed by slot 24: the four queries and RemoveItem take
 * a valve's name, AddItem a valve's and a UI variable's, GetContents a UI variable's; anything else
 * none -- C2D's parser is not asked (SOCOM 1's took fewer, in the same order) */
C2DAnimCmdHdr *CUIListBox::ParseZAnimCmd(_zrdr *rdr)
{
    CListBoxCmd *cmd;

    if (zrdr_findtag(rdr, "FINISH")) {
        cmd = (CListBoxCmd *)_calloc(1, sizeof(CListBoxCmd), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(CListBoxCmd);
        cmd->m_type = GetZAnimCmdType("FINISH");
        cmd->m_arg = 0;
        cmd->m_var = 0;
    } else if (zrdr_findtag(rdr, "HIGHLIGHT_ON")) {
        cmd = (CListBoxCmd *)_calloc(1, sizeof(CListBoxCmd), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(CListBoxCmd);
        cmd->m_type = GetZAnimCmdType("HIGHLIGHT_ON");
        cmd->m_arg = 0;
        cmd->m_var = 0;
    } else if (zrdr_findtag(rdr, "HIGHLIGHT_OFF")) {
        cmd = (CListBoxCmd *)_calloc(1, sizeof(CListBoxCmd), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(CListBoxCmd);
        cmd->m_type = GetZAnimCmdType("HIGHLIGHT_OFF");
        cmd->m_arg = 0;
        cmd->m_var = 0;
    } else if (zrdr_findtag(rdr, "HAS_ITEMS")) {
        cmd = (CListBoxCmd *)_calloc(1, sizeof(CListBoxCmd), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(CListBoxCmd);
        cmd->m_type = GetZAnimCmdType("HAS_ITEMS");
        cmd->m_arg = 0;
        cmd->m_var = 0;
    } else if (zrdr_findtag(rdr, "GetItemCount")) {
        const char *valve = zrdr_findstring(rdr, "VALVE");

        cmd = (CListBoxCmd *)_calloc(1, sizeof(CListBoxCmd), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(CListBoxCmd);
        cmd->m_type = GetZAnimCmdType("GetItemCount");
        cmd->m_arg = ZAnim.m_CurAnim->AddName(valve);
        cmd->m_var = 0;
    } else if (zrdr_findtag(rdr, "GetFocusIndex")) {
        const char *valve = zrdr_findstring(rdr, "VALVE");

        cmd = (CListBoxCmd *)_calloc(1, sizeof(CListBoxCmd), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(CListBoxCmd);
        cmd->m_type = GetZAnimCmdType("GetFocusIndex");
        cmd->m_arg = ZAnim.m_CurAnim->AddName(valve);
        cmd->m_var = 0;
    } else if (zrdr_findtag(rdr, "SetFocusIndex")) {
        const char *valve = zrdr_findstring(rdr, "VALVE");

        cmd = (CListBoxCmd *)_calloc(1, sizeof(CListBoxCmd), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(CListBoxCmd);
        cmd->m_type = GetZAnimCmdType("SetFocusIndex");
        cmd->m_arg = ZAnim.m_CurAnim->AddName(valve);
        cmd->m_var = 0;
    } else if (zrdr_findtag(rdr, "AddItem")) {
        const char *valve = zrdr_findstring(rdr, "VALVE");
        const char *var = zrdr_findstring(rdr, "UIVAR");

        cmd = (CListBoxCmd *)_calloc(1, sizeof(CListBoxCmd), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(CListBoxCmd);
        cmd->m_type = GetZAnimCmdType("AddItem");
        cmd->m_arg = ZAnim.m_CurAnim->AddName(valve);
        cmd->m_var = ZAnim.m_CurAnim->AddName(var);
    } else if (zrdr_findtag(rdr, "RemoveItem")) {
        const char *valve = zrdr_findstring(rdr, "VALVE");

        cmd = (CListBoxCmd *)_calloc(1, sizeof(CListBoxCmd), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(CListBoxCmd);
        cmd->m_type = GetZAnimCmdType("RemoveItem");
        cmd->m_arg = ZAnim.m_CurAnim->AddName(valve);
        cmd->m_var = 0;
    } else if (zrdr_findtag(rdr, "GetContents")) {
        const char *var = zrdr_findstring(rdr, "UIVAR");

        cmd = (CListBoxCmd *)_calloc(1, sizeof(CListBoxCmd), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(CListBoxCmd);
        cmd->m_type = GetZAnimCmdType("GetContents");
        cmd->m_arg = 0;
        cmd->m_var = ZAnim.m_CurAnim->AddName(var);
    } else if (zrdr_findtag(rdr, "RemoveAllItems")) {
        cmd = (CListBoxCmd *)_calloc(1, sizeof(CListBoxCmd), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(CListBoxCmd);
        cmd->m_type = GetZAnimCmdType("RemoveAllItems");
        cmd->m_arg = 0;
        cmd->m_var = 0;
    } else
        cmd = 0;
    return cmd;
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

#if Z_PORT
}   /* the port: a global class's member is defined outside the anonymous namespace (MW allows it inside) */
#endif
inline void ListItem::MakeImage(int w, int h, zdb::CTexHandle *tex)
{
    m_impl = new CListItemImageTypeImpl(w, h, tex);
}
#if Z_PORT
namespace {
#endif

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
static inline unsigned int RdrCount(_zrdr *r) { return r->m_type == 4 ? r->m_length : 0; }
static inline const char *RdrString(_zrdr *r) { return r->m_type == 3 ? r->m_str : 0; }
/* a number's value as an int (zrdr's int reader, inline: 1 an int, 2 a float -- ours) */
static inline int RdrInt(_zrdr *r)
{
    if (r->m_type == 1)
        return r->m_int;
    if (r->m_type == 2)
        return (int)r->m_real;
    return 0;
}
static inline _zrdr *RdrElem(_zrdr *r, unsigned int i)
{
    return i < RdrCount(r) ? &r->m_array[i] : 0;
}

/* the string table's id for a name, 0 if it has none (ztwod's, 0x361B20) */
int GetStringId(const char *name);

/* CAPTION as a command: an array of several strings joined into one name, or a single string --
 * its string table id when it has one, else the name added to the animation's; VALVE's name
 * added likewise; anything else none (SOCOM 1's took one CAPTION string) */
C2DAnimCmdHdr *CRenderableString::ParseZAnimCmd(_zrdr *rdr)
{
    C2DAnimCmdHdr *cmd;
    _zrdr *r = zrdr_findtag(rdr, "CAPTION");
    const char *valve = zrdr_findstring(rdr, "VALVE");

    if (r) {
        cmd = (C2DAnimCmdHdr *)_calloc(1, sizeof(C2DAnimCmdHdr), 0, 0);
        cmd->header.timeless = 1;
        cmd->header.data_size = sizeof(C2DAnimCmdHdr);
        cmd->m_type = GetZAnimCmdType("CAPTION");
        if (r->m_type == 4 && RdrCount(r) > 1) {
            CSaferStr name;

            name = "";
            for (int i = 0; i < RdrCount(r); i++) {
                if (RdrElem(r, i)->m_type == 3)
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
 * every item is). Byte-exact since round 65 (10 off since round 25, all in the prologue): the
 * parameter read directly, no local copy. The scheduler issues two a cycle, so one instruction more
 * ahead of the first table address -- the parameter's copy at entry -- is what turns "both addresses
 * up front" into the target's serialised stores here, and the reverse in the sibling below; its
 * helpers take `_zrdr *`, not const: an inline's const conversion makes `(const _zrdr *)r + 2` a
 * subable expression (no longer IRO_IsSubableExpression's `variable + constant`), hoisted out of
 * the walk */
CListItemArrayTypeImpl::CListItemArrayTypeImpl(_zrdr *r, const std::vector<ColumnData *> &cols)
{
    int n = RdrCount(r) < cols.size() ? RdrCount(r) : cols.size();

    m_ready = true;
    for (int i = 0; i < n; i++) {
        _zrdr *e = RdrElem(r, i);

        if (e && e->m_type == 3) {
            ListItem *item = new ListItem(RdrString(e));

            if (!item->m_24)
                m_ready = false;
            item->SetString(RdrString(e));
            m_items.push_back(item);
        }
    }
}

/* an item per entry of the reader's array in a font and scale, as many as there are columns (SOCOM
 * II's, the plain constructor's walk grown; ready while every item is): a picture column's look a
 * bitmap of the column's size, its texture picked by the entry's number; the others' look by the
 * column's type, then the font, scale and size, and the entry's text -- from an array's VALUE, COLOR
 * and DISABLED, or formatted from anything else */
CListItemArrayTypeImpl::CListItemArrayTypeImpl(_zrdr *r, const std::vector<ColumnData *> &cols,
                                               C2DFont *font, float scale)
{
    int n = RdrCount(r) < cols.size() ? RdrCount(r) : cols.size();

    m_ready = true;
    for (int i = 0; i < n; i++) {
        _zrdr *e = RdrElem(r, i);
        ListItem *item = new ListItem;

        if (cols[i]->m_type == 0) {
            item->MakeImage(cols[i]->m_width, cols[i]->m_height,
                            cols[i]->m_textures[RdrInt(e) % cols[i]->m_textures.size()]);
        } else {
            if (cols[i]->m_type == 2)
                item->MakeTimedText();
            else if (cols[i]->m_type == 3)
                item->MakeWrappedText();
            item->SetFont(font);
            item->SetScale(scale);
            item->SetHeight(cols[i]->m_height);
            item->SetWidth(cols[i]->m_width);
            if (e->m_type == 4) {
                const char *value = zrdr_findstring(e, "VALUE");

                if (value)
                    item->SetString(value);
                if (zrdr_findtag(e, "COLOR")) {
                    PNT3D c;

                    zrdr_findPNT3D(e, "COLOR", &c);
                    item->SetColor((int)c.x, (int)c.y, (int)c.z);
                }
                if (zrdr_findtag(e, "DISABLED")) {
                    item->m_24 = false;
                    m_ready = false;
                }
            } else {
                char buf[800];

                item->SetString(CUIVariable::Format(e, buf, 800));
            }
        }
        m_items.push_back(item);
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
    _zrdr *r = rdr;

    if (r && RdrCount(r))
        m_impl = new CListItemArrayTypeImpl(rdr, cols);
}

/* an item from a reader in a font and scale: a string as a text look, an array as an array look,
 * the item not ready while its columns are not (SOCOM II's) */
ListItem::ListItem(_zrdr *rdr, const std::vector<ColumnData *> &cols, C2DFont *font, float scale)
    : m_text(0), m_24(true), m_impl(0)
{
    _zrdr *r = rdr;

    if (r->m_type == 3)
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

/* the side the stick was pushed to since it was last centred (zui.cpp's UiGetDirStickMoved, a
 * byte: 1 right, 2 left, 3 up, 4 down, zui.cpp's STICK_DIR) */
unsigned char UiGetDirStickMoved();

/* slot 15: with children, the pad handed to the child at the port's place; else a release of
 * button 9 runs slot 27, and the first action whose button is in its state -- or, an action on
 * the release of a direction button, the stick pushed that way -- runs and takes the pad (SOCOM 1's
 * C2DButton::HandlePad, slot 15; SOCOM II asks ZAnim alone for the animation). The count before the
 * size test (its zero in the test's delay slot), the walk's end and begin in locals in that order,
 * and the press tested first: every saved register and both spills (this, the count and the item
 * pointer at sp+236/208/160, two stick answers at 192/176) are the target's (tools/perm.py over the
 * six action tests and the four combinations, 1,000 orders).
 * Byte-exact since round 65: the action's state, event and the pad's state for it held in byte-enum locals, declared pad's
 * state, event, state (open since round 53 at 8-10): enums are not propagated (the game states' round 53, the kit's round 60),
 * so each keeps its declared number where every plain or const local was copy-propagated away; the replay had asked for the
 * pad's load below the event's (v83 -> 78.5). The enum is a modelling device, the original's fields may be such enums */
bool C2DButton::HandlePad(const CPad *pad, int port)
{
    if (!CanHandleInput())
        return false;
    int n = 0;

    if (size()) {
        iterator e = end();
        iterator i = begin();

        for (; i != e; ++i, ++n)
            if (n == port && (*i)->HandlePad(pad, 0))
                return true;
        return false;
    }
    if (pad->m_state[9] == 1)
        Slot27();
    for (int i = 0; i < m_numActions; i++) {
        const ActionItem *a = &m_actions[i];

        if (a->Type != 1) {
            enum EActionByte { kB0, kB1, kB2, kB3, kB4, kB5, kB6, kB7 };
            EActionByte padState;
            EActionByte event;
            EActionByte state;

            event = (EActionByte)a->Event;
            state = (EActionByte)a->State;
            padState = (EActionByte)pad->m_state[event];

            bool pressed = state == padState;
            bool falling = state == 1;
            bool isUp = event == 4;
            bool isDown = event == 5;
            bool isLeft = event == 3;
            bool isRight = event == 2;
            bool stickUp = UiGetDirStickMoved() == 3;
            bool stickDown = UiGetDirStickMoved() == 4;
            bool stickLeft = UiGetDirStickMoved() == 1;
            bool stickRight = UiGetDirStickMoved() == 2;
            bool up = falling && isUp && stickUp;
            bool down = falling && isDown && stickDown;
            bool left = falling && isLeft && stickLeft;
            bool right = falling && isRight && stickRight;

            if (pressed || up || down || left || right) {
                a->Run();
                return true;
            }
        }
    }
    return false;
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
 * children, the first to take it ending the walk -- taken either way (SOCOM 1's, slot 15). The walk
 * is C2D::GetAssetLibs's (the HUD's round 46): the end node declared first and taken after the
 * first node, so it is built in this's register (s3) as `this` dies and the node keeps s2 (with
 * either iterator initialised first the two swap: 11 off; end in the loop test 13, an assigned
 * iterator 0x110, a while walk 11, the base deleter's iterator 11, the walk in a helper 0xF4) */
bool CHandlerButton::HandlePad(const CPad *pad, int port)
{
    if (!CanHandleInput())
        return false;
    if (size() && pad->m_state[9] == 2) {
        Slot27();
        node_base *e;
        iterator i = begin();
        for (e = &end_node(); i != iterator(e); ++i)
            if ((*i)->HandlePad(pad, port))
                return true;
        return true;
    }
    return false;
}

/* the counter's destructor (implicit: the string's; its table 0x4070D0 goes with it, reached
 * through the thunk 0x37CFF0) */
C2DCounter::~C2DCounter()
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

/* the radio strip's destructor (implicit: the button's) */
CRadioStrip::~CRadioStrip()
{
}

/* the scrolling text's destructor (implicit: the wrapped text's) */
CScrollText::~CScrollText()
{
}

/* the clock's destructor (implicit: the string's; its tables 0x407190 and the mixin's 0x407220) */
C2DClock::~C2DClock()
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

/* the secure text's destructor (implicit: the string's; its table 0x406FA0 goes with it, and the
 * table's inline copies after it, 0x36AFA0-0x36AFEF) */
CSecureText::~CSecureText()
{
}
