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
#include <string>

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

/* SOCOM 1's CSaferStr (zutil.h: MSL's basic_string under a name of its own, a null pointer
 * assigned as the empty string; SOCOM 1's symbols name its operator=(const char *)). Its copy, copy
 * assignment and destructor are implicit: they run include/string's out-of-class members (0x22E560,
 * 0x22E4D0, 0x1E7EF0 -- the destructor with 0, a base subobject's) */
class CSaferStr : public std::string {
public:
    CSaferStr() {}
    /* the string made empty, then assigned (SOCOM 1's, as its operator=) */
    CSaferStr(const char *sz) { *this = sz; }
    /* nothing returned: the out-of-line copy (0x22DCC0) keeps no `this` for a result */
    void operator=(const char *sz)
    {
        if (sz)
            assign(sz);
        else
            erase();
    }
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

class CDesignDlgObj;

/* a dialog object's specification (SOCOM 1's; the vtable at 0, its three answers about children
 * inline -- their copies are an earlier unit's, 0x368540-0x368567; a child is a design object) */
class CDlgObjSpec {
public:
    CDlgObjSpec(_zrdr *rdr);
    CDlgObjSpec(CDlgObjSpec &spec);
    virtual ~CDlgObjSpec();                                                                // 0x08
    virtual CDlgObjSpec *Copy() = 0;                                                       // 0x0C
    virtual C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const = 0;   // 0x10
    virtual const char *GetType() = 0;                                                     // 0x14
    virtual CDesignDlgObj *GetChild(int i) const { return 0; }                            // 0x18
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

/* a pad button (SOCOM 1's DWARF, zinput.h's) */
enum PAD_BUTTON {
    PAD_START, PAD_SELECT, PAD_RIGHT, PAD_LEFT, PAD_UP, PAD_DOWN, PAD_TRIANGLE, PAD_CIRCLE,
    PAD_SQUARE, PAD_CROSS, PAD_TRIGGER_R1, PAD_TRIGGER_R2, PAD_TRIGGER_L1, PAD_TRIGGER_L2,
    PAD_RIGHT_STICK, PAD_LEFT_STICK, PAD_MAX_BUTTONS
};

/* a button's state (SOCOM 1's DWARF, zinput.h's) */
enum KEY_STATE { KEY_UP, KEY_FALLING, KEY_DOWN, KEY_RISING };

/* what an action answers: a pad button or an event (SOCOM 1's DWARF) */
enum ACTION_TYPE { ACTION_TYPE_BUTTON = 0, ACTION_TYPE_EVENT = 1 };

/* what a button does on an event (SOCOM 1's DWARF: 0x10 bytes, the three enums a byte each).
 * The constructor is inline: the specs' constructors inline it, and the units that hand it to
 * __construct_array (CGameMenu's, C2DButton's, the list box's) emit its copy, zui.cpp's linked
 * (0x369F70). The assignment is zui.cpp's (SOCOM 1's lines 404-407 there), the destructor
 * zui_cleanup.cpp's (lines 23-24 there: its first function) */
class ActionItem {
public:
    ActionItem() : Type(ACTION_TYPE_BUTTON), Animation(0), Argument(0), State(KEY_FALLING), PadButton(PAD_MAX_BUTTONS) {}
    /* a copy (SOCOM 1's, inline: the strings cleared and the state set, then the assignment --
     * vector<ActionItem>'s range init 0x3836E0 builds each element so) */
    ActionItem(const ActionItem &a) : Animation(0), Argument(0), State(KEY_FALLING) { *this = a; }
    ~ActionItem();
    void operator=(const ActionItem &a);

    ACTION_TYPE Type;          // 0x0
    char *Animation;           // 0x4
    char *Argument;            // 0x8
    KEY_STATE State;           // 0xc
    union {
        PAD_BUTTON PadButton;  // 0xd
        unsigned char Event;   // 0xd: EVENT
    };
};   // 0x10 bytes

/* an action read from its rdr block (zui.cpp's, 0x365D50) */
void ParseButtonAnim(_zrdr *rdr, ActionItem *item);

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
    /* (the copy is implicit, CButtonSpec(CButtonSpec &) as SOCOM 1's name has it -- CDlgObjSpec's
     * takes a plain reference: inlined one level where a spec's copy needs it, its members' copies
     * called -- CDlgObjSpec's 0x388F20, ButtonState's 0x37E210, CSaferStr's 0x37E1E0, the actions'
     * vector's 0x37E190 -- and its own copy out of line in zui_cleanup.cpp, 0x387630, everything
     * inlined there) */
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
    bool m_108;                          // 0x108 (copied as a byte)
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
    /* (inline: HasChildren's copy follows the destructor, its first caller, 0x3881E0; the other two
     * come with the table, 0x389780 and 0x3897B0) */
    CDesignDlgObj *GetChild(int i) const { return m_child; }
    bool HasChildren() const { return m_child != 0; }
    int NumChildren() const { return HasChildren(); }

    CDesignDlgObj *m_child;              // 0x118: the SUBORDINATE block's object
};   // 0x11c bytes

/* a strip of buttons, one of them chosen (SOCOM 1's CRadioStripSpec) */
class CRadioStripSpec : public CButtonSpec {
public:
    CRadioStripSpec(_zrdr *rdr);
    ~CRadioStripSpec();
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "RADIOSTRIP"; }
    /* (inline: their copies come with the table, 0x389140-0x389173) */
    CDesignDlgObj *GetChild(int i) const { return (CDesignDlgObj *)m_items.data_[i]; }
    bool HasChildren() const { return m_items.size() != 0; }
    int NumChildren() const { return m_items.size(); }

    std::vector<CDesignDlgObj *> m_items;  // 0x118: the strip's objects
};   // 0x124 bytes

/* a check box (SOCOM 1's CCheckBoxSpec; SOCOM II's 0x13C bytes): the actions run on checking and
 * unchecking, the check's offset, size and texture */
class CCheckBoxSpec : public CButtonSpec {
public:
    CCheckBoxSpec(_zrdr *rdr);
    CCheckBoxSpec(CCheckBoxSpec &spec);
    /* the actions deleted (inline: its copy comes with the table, 0x389190, ~ActionItem inlined in
     * zui_cleanup.cpp, which defines it) */
    ~CCheckBoxSpec()
    {
        if (m_onCheck)
            delete m_onCheck;
        if (m_onUncheck)
            delete m_onUncheck;
    }
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "CHECKBOX"; }

    ActionItem *m_onCheck;               // 0x118: ONCHECK
    ActionItem *m_onUncheck;             // 0x11c: ONUNCHECK
    IPNT2D m_checkOffset;                // 0x120: CHECK_X, CHECK_Y or CHECK_OFFSET
    IPNT2D m_checkSize;                  // 0x128: CHECKSIZE_X, CHECKSIZE_Y or CHECK_SIZE
    CSaferStr m_checkTexture;            // 0x130: CHECK_TEXTURE
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

/* one object of a dialog's design: its spec, where it sits and under which parent (SOCOM 1's
 * CDesignDlgObj; the assignment copies the spec through its Copy) */
class CDesignDlgObj {
public:
    /* (SOCOM II's; the handler button's constructor news one so) */
    CDesignDlgObj() : m_spec(0), m_xpos(0), m_ypos(0), m_opacity(1.0f) {}
    /* a copy: built empty, then assigned (vector<CDesignDlgObj>'s range init 0x389B00 builds each
     * element so) */
    CDesignDlgObj(const CDesignDlgObj &o) : m_spec(0), m_xpos(0), m_ypos(0), m_opacity(1.0f) { *this = o; }
    /* (ordinary functions of zui_spec.cpp's, 0x37D9B0 and 0x37D910: zui.cpp's tail, the specs'
     * destructors and the vectors' copies call them out of line) */
    ~CDesignDlgObj();
    void operator=(const CDesignDlgObj &o);
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

    CDlgObjSpec *m_spec;          // 0x0: SPEC
    int m_xpos;                   // 0x4: XPOS
    int m_ypos;                   // 0x8: YPOS
    float m_opacity;              // 0xC: OPACITY (1)
    CSaferStr m_name;             // 0x10: NAME ("unnamed")
    CSaferStr m_childOf;          // 0x1C: CHILDOF
};   // 0x28 bytes

/* the map control (SOCOM 1's CUIMapCtrlSpec; SOCOM II's 0x30 bytes) */
class CUIMapCtrlSpec : public CDlgObjSpec {
public:
    CUIMapCtrlSpec(_zrdr *rdr);
    CDlgObjSpec *Copy();
    C2D *CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const;
    const char *GetType() { return "MAPCONTROL"; }

    IPNT2D m_upperLeft;                   // 0x8: UPPERLEFT
    IPNT2D m_lowerRight;                  // 0x10: LOWERRIGHT
    std::vector<CDesignDlgObj> m_elements;  // 0x18: ELEMENTS (each its XOFFSET, YOFFSET, TYPE and SPEC)
    std::vector<ActionItem> m_animations;   // 0x24: ANIMATIONS
};   // 0x30 bytes

#endif
