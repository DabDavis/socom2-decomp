/* zui_spec.h -- the dialog objects' specifications: SOCOM 1's CDlgObjSpec family, defined in
 * zui_spec.cpp and zui_cleanup.cpp in both builds (the press beta's asserts name zui_cleanup.cpp;
 * the demo's unnamed namespace names zui_spec.cpp). A spec is what a dialog's rdr says about one
 * control: parsed once (the _zrdr constructors), copied for each dialog that uses it (Copy) and
 * made into the C2D object that draws it (CreateDisplayObj). Any spec may declare a UI variable
 * (CUIVariableSpec, the rdr's UIVAR block).
 *
 * The demo has no DWARF for these units: the classes' names and their virtuals' are the demo's
 * symbols, the layouts are read off SOCOM II's code (the member names ours, after the rdr tags
 * each is parsed from). */
#ifndef ZUI_SPEC_H
#define ZUI_SPEC_H

#include <iterator>

#include "zui.h"
#include "../zutil/util_stable.h"
#include "../zmath/zmath.h"

class C2D;
class C2DFont;
namespace zdb {
class CTextureRelocMgr;
}

/* SOCOM 1's DWARF */
struct IPNT2D {
    int x, y;
};

_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);
bool zrdr_findIPNT2D(const _zrdr *rdr, const char *tag, IPNT2D *out);
char *zrdr_findstring(const _zrdr *rdr, const char *tag);
bool zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n);
bool zrdr_findint(const _zrdr *rdr, const char *tag, int *out, int n);
bool zrdr_findPNT3D(const _zrdr *rdr, const char *tag, PNT3D *out);
extern "C" int strcasecmp(const char *a, const char *b);
extern "C" int strcmp(const char *a, const char *b);

/* <string>'s out-of-line members, by address: include/string has neither the copy constructor
 * (0x22E560: the short form copied whole, the long form reserved and replaced in) nor the copy
 * assignment (0x22E4D0) yet. The others are include/string's placeholders and the map's
 * destructor (0x1E7EF0) */
extern "C" {
void *func_0022E560(void *str, const void *src);
void *func_0022E4D0(void *str, const void *src);
void func_001E7EF0(void *str, int flags);
void *func_001EEB40(void *str, const char *s);
void *func_001EE780(void *str, unsigned int pos, unsigned int n1, const char *first, const char *last,
                    std::forward_iterator_tag);
}

/* SOCOM 1's CSaferStr (zutil.h: MSL's basic_string under a name of its own, a null pointer
 * assigned as the empty string). A stand-in of SOCOM II's 12-byte string until include/string has
 * the copies: its pieces call the string's out-of-line members, the destructor handing the
 * string's 0 as the real one's implicit destructor does for its base */
class CSaferStr {
public:
    CSaferStr() { init(); }
    /* the string made empty, then assigned (SOCOM 1's, as its operator=) */
    CSaferStr(const char *sz)
    {
        init();
        *this = sz;
    }
    CSaferStr(const CSaferStr &s) { func_0022E560(this, &s); }
    ~CSaferStr() { func_001E7EF0(this, 0); }
    CSaferStr &operator=(const CSaferStr &s)
    {
        func_0022E4D0(this, &s);
        return *this;
    }
    void operator=(const char *sz)
    {
        if (sz)
            assign(sz);
        else
            erase();
    }
    void assign(const char *s) { func_001EEB40(this, s); }
    void erase()
    {
        std::forward_iterator_tag category;

        func_001EE780(this, 0, (unsigned int)-1, 0, 0, category);
    }
    bool is_long() const { return w_[0] & 1; }
    const char *c_str() const { return is_long() ? (const char *)w_[2] : (const char *)this + 1; }

private:
    void init()
    {
        unsigned int (&a)[3] = w_;

        for (unsigned int i = 0; i < 3; ++i)
            a[i] = 0;
    }

    unsigned int w_[3];
};   // 0xC bytes

/* a UI variable a spec declares: its name, its value with a string table of its own, and how
 * long it lives (SOCOM 1's CUIVariableSpec; 0x2C bytes in SOCOM II, whose CSTable is 0x20) */
class CUIVariableSpec {
public:
    CUIVariableSpec(_zrdr *rdr);
    /* a copy (SOCOM 1's out of line, 0x39EE90; SOCOM II inlines it and keeps no copy) */
    CUIVariableSpec(CUIVariableSpec *v) : m_name(0), m_value(0), m_table(0, 1024)
    {
        m_name = _strdup(v->m_name, 0, 0);
        if (v->m_value)
            m_value = new _zrdr(v->m_value, &m_table);
        m_longevity = v->m_longevity;
    }
    ~CUIVariableSpec();

    char *m_name;                  // 0x0
    _zrdr *m_value;                // 0x4
    CSTable m_table;               // 0x8: the value's strings
    UIVAR_LONGEVITY m_longevity;   // 0x28
};   // 0x2C bytes

/* a dialog object's specification (SOCOM 1's; the vtable at 0, its three answers about children
 * inline -- their copies are an earlier unit's, 0x368540-0x368567) */
class CDlgObjSpec {
public:
    CDlgObjSpec(_zrdr *rdr);
    CDlgObjSpec(CDlgObjSpec &spec);
    virtual ~CDlgObjSpec();                                                                // 0x08
    virtual CDlgObjSpec *Copy() = 0;                                                       // 0x0C
    virtual C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const = 0;   // 0x10
    virtual const char *GetType() = 0;                                                     // 0x14
    virtual CDlgObjSpec *GetChild(int i) const { return 0; }                              // 0x18
    virtual bool HasChildren() const { return false; }                                     // 0x1C
    virtual int NumChildren() const { return 0; }                                          // 0x20
    /* (ours: the copy reads the source's variable spec through it -- written out, the copy's
     * pointer and the source's trade saved registers) */
    CUIVariableSpec *GetVarSpec() { return m_var; }
    /* the variable the spec declares, made in the manager, or 0 (ours: the CreateDisplayObjs test
     * m_var again inside it) */
    CUIVariable *GetUIVar() const
    {
        return m_var ? theUIVarManager.Add(m_var->m_name, m_var->m_value, m_var->m_longevity) : 0;
    }

    CUIVariableSpec *m_var;   // 0x4: the UIVAR block's, or 0
};   // 0x8 bytes

/* a line of text (SOCOM 1's CTextSpec: the caption, its scale, justification and colour, and a
 * font the rdr may name) */
class CTextSpec : public CDlgObjSpec {
public:
    CTextSpec(_zrdr *rdr);
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "TEXT"; }

    CSaferStr m_caption;           // 0x8: CAPTION
    float m_scale;                 // 0x14: SCALE (1)
    bool m_hasFont;                // 0x18: a FONT named
    bool m_hcentered;              // 0x19: HCENTERED
    bool m_rightJustified;         // 0x1A: RIGHT_JUSTIFIED
    CSaferStr m_font;              // 0x1C: FONT
    CPnt3D m_color;                // 0x28: COLOR (128, 128, 128)
};   // 0x34 bytes

/* a clock: a text spec drawn as a C2DClock (SOCOM 1's) */
class CClockSpec : public CTextSpec {
public:
    CClockSpec(_zrdr *rdr);
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "CLOCK"; }
};   // 0x34 bytes

/* a counter: a text spec counting from StartTime to EndTime or for a Duration, its value in the
 * UI variable ValVar names (SOCOM 1's) */
class CCounterSpec : public CTextSpec {
public:
    CCounterSpec(_zrdr *rdr);
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "COUNTER"; }

    float m_startTime;       // 0x34: StartTime
    bool m_counting;         // 0x38: an EndTime or a Duration given
    float m_endTime;         // 0x3C: EndTime, or Duration
    CSaferStr m_valVar;      // 0x40: ValVar
};   // 0x4C bytes

/* a text spec wrapped to a width, scrolled by arrows (SOCOM 1's CWrapSpec) */
class CWrapSpec : public CTextSpec {
public:
    CWrapSpec(_zrdr *rdr);
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "WRAPPEDTEXT"; }

    float m_width;                // 0x34: WIDTH
    int m_spacing;                // 0x38: SPACING (10)
    float m_height;               // 0x3C: HEIGHT (-1)
    bool m_displayOneLine;        // 0x40: DISPLAY_ONE_LINE
    CSaferStr m_upTexture;        // 0x44: UP_ARROW's TEXTURE
    CSaferStr m_downTexture;      // 0x50: DOWN_ARROW's TEXTURE
    IPNT2D m_upOffset;            // 0x5C: UP_ARROW's OFFSET
    IPNT2D m_upSize;              // 0x64: UP_ARROW's SIZE
    IPNT2D m_downOffset;          // 0x6C: DOWN_ARROW's OFFSET
    IPNT2D m_downSize;            // 0x74: DOWN_ARROW's SIZE
    bool m_autoScroll;            // 0x7C: AUTOSCROLL
};   // 0x80 bytes

/* wrapped text that scrolls by itself (SOCOM 1's CScrollSpec) */
class CScrollSpec : public CWrapSpec {
public:
    /* its DISPLAYABLELINES (inline: GetControlSpec's, calling the wrapped text's out of line) */
    CScrollSpec(_zrdr *rdr) : CWrapSpec(rdr), m_80(0) { zrdr_findint(rdr, "DISPLAYABLELINES", &m_80, 1); }
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "SCROLLINGTEXT"; }

    int m_80;                     // 0x80
};   // 0x84 bytes

/* a password: a text spec drawn as stars (SOCOM II's) */
class CSecureTextSpec : public CTextSpec {
public:
    CSecureTextSpec(_zrdr *rdr) : CTextSpec(rdr) {}
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "SECURETEXT"; }
};   // 0x34 bytes

/* a teletype: text typed out a character at a time (SOCOM 1's CTtySpec) */
class CTtySpec : public CTextSpec {
public:
    /* the teletype's MAXHEIGHT (-1: unbounded), PULSERATE, SPACING, INTERVAL, MAXLENGTH and its
     * cursor (SOCOM 1's; inlined one level in GetObjectSpec, no copy of it) */
    CTtySpec(_zrdr *rdr)
        : CTextSpec(rdr), m_interval(0.0f), m_maxLength(0.0f), m_spacing(0.0f), m_pulseRate(0.0f), m_maxHeight(-1)
    {
        zrdr_findint(rdr, "MAXHEIGHT", &m_maxHeight, 1);
        zrdr_findreal(rdr, "PULSERATE", &m_pulseRate, 1);
        zrdr_findreal(rdr, "SPACING", &m_spacing, 1);
        zrdr_findreal(rdr, "INTERVAL", &m_interval, 1);
        zrdr_findreal(rdr, "MAXLENGTH", &m_maxLength, 1);
        if (zrdr_findtag(rdr, "NOCURSOR"))
            m_noCursor = true;
        else
            m_noCursor = false;
        if (zrdr_findtag(rdr, "FADECURSOR"))
            m_fadeCursor = true;
        else
            m_fadeCursor = false;
    }
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "TELETYPE"; }

    float m_interval;             // 0x34: INTERVAL
    float m_maxLength;            // 0x38: MAXLENGTH
    float m_spacing;              // 0x3C: SPACING
    float m_pulseRate;            // 0x40: PULSERATE
    int m_maxHeight;              // 0x44: MAXHEIGHT (-1)
    bool m_noCursor;              // 0x48: NOCURSOR
    bool m_fadeCursor;            // 0x49: FADECURSOR
};   // 0x4C bytes

/* a bitmap (SOCOM 1's CImageSpec) */
class CImageSpec : public CDlgObjSpec {
public:
    CImageSpec(_zrdr *rdr);
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "IMAGE"; }

    CSaferStr m_filename;         // 0x8: FILENAME
    IPNT2D m_size;                // 0x14: SIZE, or XSIZE and YSIZE
    bool m_pulse;                 // 0x1C: PULSE
    float m_pulseRate;            // 0x20: PULSE's value
};   // 0x24 bytes

/* a horizontal progress bar: a bitmap whose FILENAME may name a UI variable holding the
 * texture's name, and the node its leading edge follows (SOCOM 1's) */
class CHProgressBarSpec : public CImageSpec {
public:
    CHProgressBarSpec(_zrdr *rdr);
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "PROGRESSBAR"; }

    bool m_filenameIsUivar;       // 0x24: FILENAME_IS_UIVAR
    CSaferStr m_leadingEdgeNode;  // 0x28: LEADING_EDGE_NODE
};   // 0x34 bytes

class C2DButton;
class CDesignDlgObj;

/* what a button does on an event (SOCOM 1's DWARF: 0x10 bytes; its constructor is zui.cpp's,
 * 0x369F70, out of line) */
class ActionItem {
public:
    ActionItem();

    int Type;                  // 0x0: ACTION_TYPE
    char *Animation;           // 0x4
    char *Argument;            // 0x8
    unsigned char State;       // 0xc: KEY_STATE
    unsigned char PadButton;   // 0xd: PAD_BUTTON, or EVENT
};   // 0x10 bytes

/* one state's look, as a button spec reads it (SOCOM 1's ButtonState, 0x2C bytes there; SOCOM II's
 * 0x34 with the 12-byte string): IMAGE, the TEXT_GLOW's RANGE and TIME, PULSE, TEXT_COLOR, SCALE */
struct ButtonState {
    CSaferStr _TexName;           // 0x0: IMAGE
    zdb::CTexHandle *_Texture;    // 0xc
    float _iGlowStart;            // 0x10: TEXT_GLOW's RANGE
    float _iGlowEnd;              // 0x14
    bool _bGlowy;                 // 0x18: a TEXT_GLOW given
    float _fPulseTime;            // 0x1c: PULSE
    float _fGlowTime;             // 0x20: TEXT_GLOW's TIME
    CPnt3D _pntTextColor;         // 0x24: TEXT_COLOR
    float _fScale;                // 0x30: SCALE
};   // 0x34 bytes

/* a button (SOCOM 1's CButtonSpec): its size and offset, its four states, caption, font and the
 * ANIMATIONS it runs (layout read off SOCOM II's constructor and destructor) */
class CButtonSpec : public CDlgObjSpec {
public:
    CButtonSpec(_zrdr *rdr);
    CButtonSpec(CButtonSpec &spec);
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "BUTTON"; }
    /* the states' textures prepared and the button set up from them (SOCOM 1's) */
    void InitializeButton(C2DButton *button, zdb::CTextureRelocMgr *mgr) const;

    IPNT2D m_size;                       // 0x8: XSIZE, YSIZE or SIZE
    IPNT2D m_offset;                     // 0x10: XOFFSET, YOFFSET or OFFSET
    float m_scale;                       // 0x18: SCALE
    ButtonState m_pressed;               // 0x1c: PRESSED
    ButtonState m_active;                // 0x50: ACTIVE
    ButtonState m_normal;                // 0x84: NORMAL
    ButtonState m_disabled;              // 0xb8: DISABLED
    CSaferStr m_caption;                 // 0xec: CAPTION
    bool m_hcentered;                    // 0xf8: HCENTERED
    std::vector<ActionItem> m_commands;  // 0xfc: ANIMATIONS
    int m_108;                           // 0x108
    CSaferStr m_font;                    // 0x10c: FONT
};   // 0x118 bytes

/* a button with a spec of its own to show (SOCOM 1's CHandlerButtonSpec) */
class CHandlerButtonSpec : public CButtonSpec {
public:
    CHandlerButtonSpec(_zrdr *rdr);
    ~CHandlerButtonSpec();
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "HANDLERBUTTON"; }
    CDlgObjSpec *GetChild(int i) const;
    bool HasChildren() const;
    int NumChildren() const;

    CDesignDlgObj *m_child;              // 0x118: the SUBORDINATE block's object
};   // 0x11c bytes

/* a strip of buttons, one of them chosen (SOCOM 1's CRadioStripSpec) */
class CRadioStripSpec : public CButtonSpec {
public:
    CRadioStripSpec(_zrdr *rdr);
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "RADIOSTRIP"; }
    CDlgObjSpec *GetChild(int i) const;
    bool HasChildren() const;
    int NumChildren() const;

    std::vector<CDlgObjSpec *> m_items;  // 0x118
};   // 0x124 bytes

/* a check box (SOCOM 1's CCheckBoxSpec; SOCOM II's 0x13C bytes, its check's texture at 0x130) */
class CCheckBoxSpec : public CButtonSpec {
public:
    CCheckBoxSpec(_zrdr *rdr);
    CCheckBoxSpec(CCheckBoxSpec &spec);
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "CHECKBOX"; }

    unsigned char _118[0x130 - 0x118];
    CSaferStr m_checkTexture;            // 0x130
};   // 0x13c bytes

/* a slider: a button with a knob (SOCOM 1's CSliderBarSpec; its constructor inline, in
 * GetControlSpec): NUM_INTERVALS (10), KNOB_TEXTURE, KNOB_SIZE and CENTER_VALUE (0.5) */
class CSliderBarSpec : public CButtonSpec {
public:
    CSliderBarSpec(_zrdr *rdr) : CButtonSpec(rdr), m_centerValue(0.5f), m_numIntervals(10)
    {
        zrdr_findint(rdr, "NUM_INTERVALS", &m_numIntervals, 1);
        m_knobTexture = zrdr_findstring(rdr, "KNOB_TEXTURE");
        m_knobSize.x = m_knobSize.y = 0;
        zrdr_findIPNT2D(rdr, "KNOB_SIZE", &m_knobSize);
        zrdr_findreal(rdr, "CENTER_VALUE", &m_centerValue, 1);
    }
    /* (declared first: the key function, so a unit defining it emits the table and the implicit
     * destructor) */
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    CDlgObjSpec *Copy();
    const char *GetType() { return "SLIDERBAR"; }

    CSaferStr m_knobTexture;             // 0x118: KNOB_TEXTURE
    IPNT2D m_knobSize;                   // 0x124: KNOB_SIZE
    float m_centerValue;                 // 0x12c: CENTER_VALUE
    int m_numIntervals;                  // 0x130: NUM_INTERVALS
};   // 0x134 bytes

/* a list box: an image with columns and items (SOCOM 1's CUIListBoxSpec; SOCOM II's 0x120 bytes) */
class CUIListBoxSpec : public CImageSpec {
public:
    CUIListBoxSpec(_zrdr *rdr);
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "LISTBOX"; }

    unsigned char _24[0x120 - 0x24];
};   // 0x120 bytes

/* the map control (SOCOM 1's CUIMapCtrlSpec; SOCOM II's 0x30 bytes) */
class CUIMapCtrlSpec : public CDlgObjSpec {
public:
    CUIMapCtrlSpec(_zrdr *rdr);
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "MAPCONTROL"; }

    unsigned char _8[0x30 - 0x8];
};   // 0x30 bytes

/* one object of a dialog's design: its spec, where it sits and under which parent (SOCOM 1's
 * CDesignDlgObj; the assignment copies the spec through its Copy) */
class CDesignDlgObj {
public:
    /* (SOCOM II's; the handler button's constructor news one so) */
    CDesignDlgObj() : m_spec(0), m_xpos(0), m_ypos(0), m_opacity(1.0f) {}
    ~CDesignDlgObj() { delete m_spec; }
    /* (ours: the handler button's constructor sets its object so, each pointer loaded once) */
    void SetSpec(CDlgObjSpec *spec)
    {
        delete m_spec;
        m_spec = spec;
    }
    void SetPos(int x, int y)
    {
        m_xpos = x;
        m_ypos = y;
    }
    void SetName(const char *name)
    {
        if (!name || !*name)
            name = "unnamed";
        m_name = name;
    }
    void operator=(const CDesignDlgObj &o)
    {
        delete m_spec;
        m_spec = o.m_spec ? o.m_spec->Copy() : 0;
        m_xpos = o.m_xpos;
        m_ypos = o.m_ypos;
        m_opacity = o.m_opacity;
        m_name = o.m_name;
        m_childOf = o.m_childOf;
    }

    CDlgObjSpec *m_spec;          // 0x0: SPEC
    int m_xpos;                   // 0x4: XPOS
    int m_ypos;                   // 0x8: YPOS
    float m_opacity;              // 0xC: OPACITY (1)
    CSaferStr m_name;             // 0x10: NAME ("unnamed")
    CSaferStr m_childOf;          // 0x1C: CHILDOF
};   // 0x28 bytes

#endif
