/* zui_spec.cpp -- the dialog objects' specifications parsed (OVL1 0x37D170-0x38560F): each spec's
 * _zrdr constructor, CreateDisplayObj and some Copies, the dialog design's loaders and the
 * template instances closing the unit. SOCOM 1's demo names the unit (its unnamed namespace,
 * @unnamed@zui_spec_cpp@) and holds the same run in the same order, from CClockSpec's destructor
 * to its vector instances. Defined in reverse of their image order (-inline deferred).
 *
 * The CreateDisplayObjs inline the display objects' constructors (zui_2d.h); InitializeButton is
 * pinned (they call it). CDesignDlgObj's destructor and assignment are ordinary functions here
 * (0x37D9B0, 0x37D910: zui.cpp's tail and the tail copies call them), the assignment pinned.
 *
 * Round 16 wrote the dialog design's loaders (SOCOM 1's CGameDlgDesign::LoadFromRdr 0x381550 and
 * LoadModels 0x3812B0, byte-exact; LoadDlgObjects 0x383360 open, below), whose requests bring the
 * CSaferStr and CDlgDesignModel instances into the tail: resize<CSaferStr> 0x383800, the model
 * vector's insert 0x384ED0 with its __fill_n 0x385370 and __copy_backward 0x385410, its __copy
 * 0x384D50, CSaferStr's assignment 0x384E00 and the model deleter's clear 0x36AE50 byte-exact.
 * resize<CDesignDlgObj> 0x383960 and resize<CDlgDesignModel> 0x383AA0 are byte-exact since the MSL
 * headers' round 17: the temporary's SECOND string's inlined destructor keeps the member's address in
 * a0 and reads the text pointer through it (`lw a0,8(a0)`) once include/string's destructor reads the
 * text as a `charT *` through the words (its comment).
 * OPEN, the tail's string pieces: the CSaferStr erase 0x383BC0 (15) and __copy 0x383D00 (the
 * copy here calls is_long() out of line) and the model vector's erase 0x384BD0 (20); the model's
 * copy constructor 0x3854C0 is byte-exact since the MSL headers' round 18 (include/string's copy
 * constructor reads the long size through a reference accessor: the second string's address of it
 * kept in s3 across reserve).
 * OPEN, not written yet: the button family's constructors (CButtonSpec's 0x380620, 0xC8C bytes,
 * CRadioStripSpec's 0x37DA50, CUIListBoxSpec's 0x37EAB0), the list box's CreateDisplayObj 0x37E8B0
 * (CUIListBox's constructor, zui_2d.cpp's class), and the rest of the template tail
 * 0x3836E0-0x3855FF; the ActionItem and CDesignDlgObj erases (0x383E00 and 0x384410) need MSL's destroy loop
 * in the class vector's erase (probed: erase<ActionItem> byte-exact with `for (p = first + n; p < e;
 * ++p) p->~T();` after the copy -- a request), the CDesignDlgObj ones also MSL's out-of-line
 * std::copy (0x3845B0) and its kin (0x384AA0, 0x384B00, 0x384B70).
 * Round 20: those four byte-exact once CDesignDlgObj's assignment is no longer pinned (MW's own
 * refusal to inline it propagates to the helpers and the copy constructor, as in the target).
 * OPEN: erase<CDesignDlgObj> 0x384410 (the target's size, 19 instructions: the end and the
 * count/destroy cursor in s0/s1 swapped -- the MSL headers' round 18 measured that a string
 * element's erase wants `iterator e = end()`, which loses 28 other erases); insert<CDesignDlgObj>
 * 0x384620 (0x540 against 0x478: the copy constructor inlined at the six construct sites where the
 * target calls it after the out-of-line placement new -- include/vector's construct cut, the
 * integrator's accepted loss of round 20). */

#include "zui_2d.h"

/* a real the rdr may give, else the default (ours: the counter's StartTime initialises its member
 * so, the read before the string member's construction) */
static inline float FindReal(_zrdr *rdr, const char *tag, float def)
{
    float f = def;

    zrdr_findreal(rdr, tag, &f, 1);
    return f;
}

/* the spec an object's TYPE names, from its rdr (SOCOM 1's; SOCOM II added the secure text) */
CDlgObjSpec *GetObjectSpec(_zrdr *rdr, const char *type)
{
    if (!type)
        return 0;
    CDlgObjSpec *spec = 0;
    if (!strcmp("TEXT", type))
        spec = new CTextSpec(rdr);
    else if (!strcmp("IMAGE", type))
        spec = new CImageSpec(rdr);
    else if (!strcmp("TELETYPE", type))
        spec = new CTtySpec(rdr);
    else if (!strcmp("WRAPPEDTEXT", type))
        spec = new CWrapSpec(rdr);
    else if (!strcmp("PROGRESSBAR", type))
        spec = new CHProgressBarSpec(rdr);
    else if (!strcmp("COUNTER", type))
        spec = new CCounterSpec(rdr);
    else if (!strcmp("CLOCK", type))
        spec = new CClockSpec(rdr);
    else if (!strcmp("SECURETEXT", type))
        spec = new CSecureTextSpec(rdr);
    return spec;
}

/* the bitmap's FILENAME, its SIZE (or XSIZE and YSIZE) and a PULSE rate (SOCOM 1's; called out
 * of line by the progress bar's constructor, GetObjectSpec and the list box's: pinned) */
#pragma auto_inline off
CImageSpec::CImageSpec(_zrdr *rdr) : CDlgObjSpec(rdr), m_pulse(false)
{
    m_size.x = 0;
    m_size.y = 0;
    m_filename = zrdr_findstring(rdr, "FILENAME");
    zrdr_findIPNT2D(rdr, "SIZE", &m_size);
    zrdr_findint(rdr, "XSIZE", &m_size.x, 1);
    zrdr_findint(rdr, "YSIZE", &m_size.y, 1);
    if (zrdr_findtag(rdr, "PULSE")) {
        zrdr_findreal(rdr, "PULSE", &m_pulseRate, 1);
        m_pulse = true;
    }
}
#pragma auto_inline reset

/* a plain bitmap of the spec's texture: the one its UI variable names, else FILENAME (SOCOM 1's) */
C2D *CImageSpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    const char *name;

    if (m_var)
        name = GetUIVar()->String();
    else
        name = m_filename.c_str();
    zdb::CTexHandle *tex = PrepTexture(name, mgr);
    CPlainBmp *bmp = new CPlainBmp;
    bmp->LoadSpec(*this, tex);
    return bmp;
}

/* the control spec an object's TYPE names, from its rdr, else the plain object's (SOCOM 1's; SOCOM II
 * dropped the cycle button) */
CDlgObjSpec *GetControlSpec(_zrdr *rdr, const char *type)
{
    if (!type)
        return 0;
    if (!strcmp("BUTTON", type))
        return new CButtonSpec(rdr);
    if (!strcmp("SCROLLINGTEXT", type))
        return new CScrollSpec(rdr);
    if (!strcmp("HANDLERBUTTON", type))
        return new CHandlerButtonSpec(rdr);
    if (!strcmp("LISTBOX", type))
        return new CUIListBoxSpec(rdr);
    if (!strcmp("SLIDERBAR", type))
        return new CSliderBarSpec(rdr);
    if (!strcmp("CHECKBOX", type))
        return new CCheckBoxSpec(rdr);
    if (!strcmp("RADIOSTRIP", type))
        return new CRadioStripSpec(rdr);
    if (!strcmp("MAPCONTROL", type))
        return new CUIMapCtrlSpec(rdr);
    return GetObjectSpec(rdr, type);
}

/* the button and the object its SUBORDINATE block describes: its NAME ("unnamed" if none), TYPE,
 * XPOS, YPOS and SPEC (SOCOM 1's) *
 * OPEN, 8 instructions (two delay slots): the target leaves the spec test's slot empty and puts
 * `li a0,40` in new's, and repeats the name string's address in both arms of the inlined
 * CSaferStr assignment; here MW fills the test's slot with the li, hoists the address into the
 * name test's slot and pads before "unnamed". The design object's three inline setters are what
 * give the target's loads of m_child (written out, 34 instructions differ); an early return for
 * the spec test is 8 bytes longer */
CHandlerButtonSpec::CHandlerButtonSpec(_zrdr *rdr) : CButtonSpec(rdr), m_child(0)
{
    _zrdr *sub = zrdr_findtag(rdr, "SUBORDINATE");

    if (!sub)
        return;
    const char *name = zrdr_findstring(sub, "NAME");
    const char *type = zrdr_findstring(sub, "TYPE");
    int x, y;

    zrdr_findint(sub, "XPOS", &x, 1);
    zrdr_findint(sub, "YPOS", &y, 1);
    _zrdr *specRdr = zrdr_findtag(sub, "SPEC");
    if (!specRdr)
        return;
    CDlgObjSpec *spec = GetControlSpec(specRdr, type);
    if (spec) {
        m_child = new CDesignDlgObj;
        m_child->SetSpec(spec);
        m_child->SetPos(x, y);
        m_child->SetName(name);
    }
}

/* the states' textures prepared, the button set up from them and its caption, and shown in its
 * normal state (SOCOM 1's). The four names are read first, both builds keeping them in saved
 * registers across the lookups: written into the calls, MW reads each after the call before it.
 * The CreateDisplayObjs call it out of line: pinned */
#pragma auto_inline off
void CButtonSpec::InitializeButton(C2DButton *button, zdb::CTextureRelocMgr *mgr) const
{
    const char *n = m_normal._TexName.c_str();
    const char *a = m_active._TexName.c_str();
    const char *p = m_pressed._TexName.c_str();
    const char *d = m_disabled._TexName.c_str();
    zdb::CTexHandle *normal = PrepTexture(n, mgr);
    zdb::CTexHandle *active = PrepTexture(a, mgr);
    zdb::CTexHandle *pressed = PrepTexture(p, mgr);
    zdb::CTexHandle *disabled = PrepTexture(d, mgr);

    InitButton(button, normal, active, pressed, disabled, m_caption.c_str());
    button->SetNormal();
}
#pragma auto_inline reset

/* a button (SOCOM 1's; 0x377800 is C2DButton's constructor) */
C2D *CButtonSpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    C2DButton *button = new C2DButton;

    button->_strCaption.SetFont(font);
    button->LoadSpec(*this);
    InitializeButton(button, mgr);
    return button;
}

/* the wrapped text's WIDTH, line SPACING (10) and HEIGHT (-1: unbounded), its arrows' TEXTURE,
 * SIZE and OFFSET, AUTOSCROLL and DISPLAY_ONE_LINE (SOCOM 1's). Declared inline: GetObjectSpec
 * inlines it one level, and GetControlSpec's inlined CScrollSpec calls this copy */
inline CWrapSpec::CWrapSpec(_zrdr *rdr) : CTextSpec(rdr), m_width(0.0f), m_spacing(10), m_height(-1.0f)
{
    zrdr_findreal(rdr, "WIDTH", &m_width, 1);
    zrdr_findint(rdr, "SPACING", &m_spacing, 1);
    zrdr_findreal(rdr, "HEIGHT", &m_height, 1);
    _zrdr *arrow = zrdr_findtag(rdr, "DOWN_ARROW");
    if (arrow) {
        m_downTexture = zrdr_findstring(arrow, "TEXTURE");
        zrdr_findIPNT2D(arrow, "SIZE", &m_downSize);
        zrdr_findIPNT2D(arrow, "OFFSET", &m_downOffset);
    }
    arrow = zrdr_findtag(rdr, "UP_ARROW");
    if (arrow) {
        m_upTexture = zrdr_findstring(arrow, "TEXTURE");
        zrdr_findIPNT2D(arrow, "SIZE", &m_upSize);
        zrdr_findIPNT2D(arrow, "OFFSET", &m_upOffset);
    }
    if (zrdr_findtag(rdr, "AUTOSCROLL"))
        m_autoScroll = true;
    else
        m_autoScroll = false;
    if (zrdr_findtag(rdr, "DISPLAY_ONE_LINE"))
        m_displayOneLine = true;
    else
        m_displayOneLine = false;
}

/* the line's CAPTION, SCALE, justification (HCENTERED or RIGHT_JUSTIFIED, the later winning),
 * COLOR (grey) and FONT (SOCOM 1's). Declared inline: the counter's and the wrapped text's
 * constructors inline it one level (its own callees out of line) */
inline CTextSpec::CTextSpec(_zrdr *rdr) : CDlgObjSpec(rdr), m_scale(1.0f), m_hcentered(false), m_rightJustified(false)
{
    m_caption = zrdr_findstring(rdr, "CAPTION");
    zrdr_findreal(rdr, "SCALE", &m_scale, 1);
    const char *font = zrdr_findstring(rdr, "FONT");
    if (zrdr_findtag(rdr, "HCENTERED")) {
        m_hcentered = true;
        m_rightJustified = false;
    }
    if (zrdr_findtag(rdr, "RIGHT_JUSTIFIED")) {
        m_hcentered = false;
        m_rightJustified = true;
    }
    CPnt3D grey(128.0f, 128.0f, 128.0f);
    if (zrdr_findtag(rdr, "COLOR"))
        zrdr_findPNT3D(rdr, "COLOR", &m_color);
    else
        m_color = grey;
    if (font) {
        m_hasFont = true;
        m_font = font;
    } else
        m_hasFont = false;
}

/* the counter's StartTime, then an EndTime or a Duration, and the UI variable its value goes to,
 * ValVar (SOCOM 1's). The members are initialised in the mem-initializer list: the string's
 * construction follows the StartTime read and the ValVar lookup, as the target orders them */
CCounterSpec::CCounterSpec(_zrdr *rdr)
    : CTextSpec(rdr), m_startTime(FindReal(rdr, "StartTime", 0.0f)), m_counting(false), m_endTime(0.0f),
      m_valVar(zrdr_findstring(rdr, "ValVar"))
{
    if (!zrdr_findreal(rdr, "EndTime", &m_endTime, 1) && zrdr_findreal(rdr, "Duration", &m_endTime, 1))
        m_counting = true;
}

/* a counter from the spec's times, starting at its ValVar variable's value (SOCOM 1's) */
C2D *CCounterSpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    CUIVariable *var = theUIVarManager.Find(m_valVar.c_str());
    int value = var ? var->Integer() : 0;
    C2DCounter *counter = new C2DCounter(m_startTime, m_counting, m_endTime, value);

    counter->SetFont(font);
    counter->LoadSpec(*this);
    return counter;
}

namespace {
void LoadDlgObjects(std::vector<CDesignDlgObj> *objs, _zrdr *rdr, const char *tag,
                    CDlgObjSpec *(*getSpec)(_zrdr *, const char *));
}

/* the design read from a dialog's rdr (SOCOM 1's; SOCOM II added the UIVAR libraries, USES_MEMCARD,
 * the MPEG backgrounds and the SCRIPT_LIBRARY list): everything cleared, the LIBRARY and LIBRARIES
 * -- `UIVAR <name>` naming a variable whose value is the library --, the font, the background, the
 * ANIMATIONS, CONTROLS and OBJECTS, then the MODELS (0x381550) */
void CGameDlgDesign::LoadFromRdr(_zrdr *rdr)
{
    m_libraries.clear();
    m_font = 0;
    m_background = 0;
    m_backgroundType = 0;
    m_animations.clear();
    m_controls.clear();
    m_objects.clear();
    m_models.clear();
    m_scriptLibraries.clear();

    const char *lib = zrdr_findstring(rdr, "LIBRARY");
    if (lib) {
        m_libraries.resize(m_libraries.size() + 1);
        m_libraries.back() = lib;
    }
    _zrdr *libs = zrdr_findtag(rdr, "LIBRARIES");
    for (int i = 0; i < (libs ? (int)libs->Count() : 0); i++) {
        _zrdr *item = libs->Item(i);
        const char *name = 0;

        if (item->type == 3)
            name = item->String();
        if (!strcasecmp(name, "UIVAR") && i < (libs ? (int)libs->Count() : 0) - 1) {
            _zrdr *var = libs->Item(++i);

            if (var && var->type == 3) {
                CUIVariable *v = theUIVarManager.Add(var->String(), 0, UIVL_TEMPORARY);

                if (v && v->type == 3)
                    name = v->String();
            }
        }
        if (name) {
            m_libraries.resize(m_libraries.size() + 1);
            m_libraries.back() = name;
        }
    }
    m_usesMemcard = zrdr_findtag(rdr, "USES_MEMCARD") != 0;
    m_font = zrdr_findstring(rdr, "FONT");
    if (zrdr_findtag(rdr, "NOFONTLIB"))
        m_fontLib = false;
    else
        m_fontLib = true;
    _zrdr *bg = zrdr_findtag(rdr, "BACKGROUND");
    if (bg) {
        const char *type = zrdr_findstring(bg, "TYPE");

        if (type) {
            if (!strcasecmp(type, "IMAGE"))
                m_backgroundType = 0;
            else if (!strcasecmp(type, "MPEG"))
                m_backgroundType = 1;
            else if (!strcasecmp(type, "MPEG_LOOPING"))
                m_backgroundType = 2;
        }
        m_background = zrdr_findstring(bg, "FILENAME");
    }
    _zrdr *anims = zrdr_findtag(rdr, "ANIMATIONS");
    for (int j = 0; j < (anims ? (int)anims->Count() : 0); j++) {
        m_animations.resize(m_animations.size() + 1);
        ParseButtonAnim(anims->Item(j), &m_animations.back());
    }
    LoadDlgObjects(&m_controls, rdr, "CONTROLS", GetControlSpec);
    LoadDlgObjects(&m_objects, rdr, "OBJECTS", GetObjectSpec);
    _zrdr *scripts = zrdr_findtag(rdr, "SCRIPT_LIBRARY");
    if (scripts) {
        /* an int counter against the unsigned count (sltu): an unsigned one is strength-reduced to a
         * byte offset stepped by 8, the target shifts the index each pass */
        for (int k = 0; k < scripts->Count(); k++) {
            _zrdr *item = scripts->Item(k);

            if (item->type == 3)
                m_scriptLibraries.push_back(CSaferStr(item->String()));
        }
    }
    LoadModels(rdr);
}

/* the design's MODELS: each one's NAME and MODEL, where the camera sits (AZIMUTH, ELEV, DISTANCE --
 * 20 unless given) and its rotation, and whether it takes its model from a UI variable (0x3812B0;
 * SOCOM 1's -- both builds read ROTX, ROTY and ROTZ into the same member) */
void CGameDlgDesign::LoadModels(_zrdr *rdr)
{
    _zrdr *models = zrdr_findtag(rdr, "MODELS");

    for (int i = 0; i < (models ? (int)models->Count() : 0); i++) {
        m_models.resize(m_models.size() + 1);
        CDlgDesignModel &m = m_models.back();
        _zrdr *item = models->Item(i);

        m.m_name = zrdr_findstring(item, "NAME");
        m.m_model = zrdr_findstring(item, "MODEL");
        m.m_azimuth = FindReal(item, "AZIMUTH", 0.0f);
        m.m_elev = FindReal(item, "ELEV", 0.0f);
        m.m_distance = FindReal(item, "DISTANCE", 20.0f);
        m.m_rotX = FindReal(item, "ROTX", 0.0f);
        m.m_rotX = FindReal(item, "ROTY", 0.0f);
        m.m_rotX = FindReal(item, "ROTZ", 0.0f);
        if (zrdr_findtag(item, "USEMODELUIVAR"))
            m.m_useModelUIVar = true;
    }
}

CDlgObjSpec *CCounterSpec::Copy()
{
    return new CCounterSpec(*this);
}

/* a progress bar of the spec's texture, or of the one the variable FILENAME names (SOCOM 1's) */
C2D *CHProgressBarSpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    CHProgressBar *bar = new CHProgressBar;
    const char *name = m_filename.c_str();

    if (m_filenameIsUivar) {
        CUIVariable *var = theUIVarManager.Find(name);

        if (var)
            name = var->String();
    }
    bar->LoadSpec(*this, PrepTexture(name, mgr));
    return bar;
}

class ListItem;
struct ColumnData;

/* the list box (zui_2d.cpp's CUIListBox, SOCOM 1's; SOCOM II's 0x610 bytes) as its spec builds it --
 * the layout zui_2d.cpp declares, its members' names, with what that unit stands in for spelled out
 * where the constructor stores (the two bitmaps at 0x2E0 and 0x380, the words at 0xD0 and 0x180);
 * its virtuals only declared: the table 0x407670 (and the mixin's, 0x70 on) is zui_2d.cpp's. The
 * constructor is inline: CreateDisplayObj inlines it one level, its members' constructors called --
 * the item list's 0x37EA80 and the column vector's 0x37EA60 (their copies follow their first
 * caller), CPnt3D's 0x1FB3C0, CPlainBmp's 0x20CD50, the actions' through __construct_array -- and
 * it ends taking input (C2D's slot 17) */
class CUIListBox : public CPlainBmp {
public:
    CUIListBox()
        : m_offset(0), m_scrollY(0.0f), m_scroll(0), m_ownFont(0), m_scale(0.0f), m_d0(0),
          m_highlightAlpha(80), m_noHighlight(false), m_followEnd(false), m_ea(false), m_texEC(0),
          m_texF0(0), m_texF4(0), m_texF8(0), m_texFC(0), m_seqStart(0.0f), m_seqLength(0.0f),
          m_pulseMin0(0.0f), m_pulseMax0(0.0f), m_pulseTime0(0.0f), m_pulseMin(0.0f),
          m_pulseMax(0.0f), m_pulseTime(0.0f), m_184(60.0f), m_tex188(0), m_tex18c(0), m_190(0),
          m_highlighted(0), m_numActions(0)
    {
        SetCanHandleInput(true);
    }
    virtual ~CUIListBox();
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr);
    virtual int GetZAnimCmdType(const char *name);
    virtual void SetActive();
    virtual void SetNormal();
    virtual void UpdateUIVar(CUIVariable *var);
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *hdr, float *dT, bool *done);
    /* set up from its spec (0x375E20, zui_2d.cpp's) */
    void LoadSpec(const CUIListBoxSpec &spec, zdb::CTexHandle *tex, zdb::CTextureRelocMgr *mgr);

    C2DFont *m_font;   // 0xa0
    int m_a4;   // 0xa4
    int m_a8;   // 0xa8
    int m_lineHeight;   // 0xac
    int m_offset;   // 0xb0
    float m_scrollY;   // 0xb4
    int m_scroll;   // 0xb8
    C2DFont *m_ownFont;   // 0xbc
    std::list<ListItem *> m_items;   // 0xc0
    float m_scale;   // 0xcc
    int m_d0;   // 0xd0
    std::vector<ColumnData *> m_columns;   // 0xd4
    int m_highlightAlpha;   // 0xe0
    zdb::CTextureRelocMgr *m_mgr;   // 0xe4
    bool m_noHighlight;   // 0xe8
    bool m_followEnd;   // 0xe9
    bool m_ea;   // 0xea
    zdb::CTexHandle *m_texEC;   // 0xec
    zdb::CTexHandle *m_texF0;   // 0xf0
    zdb::CTexHandle *m_texF4;   // 0xf4
    zdb::CTexHandle *m_texF8;   // 0xf8
    zdb::CTexHandle *m_texFC;   // 0xfc
    int m_rect100[4];   // 0x100 (zui_2d.cpp's IRECTs)
    int m_rect110[4];   // 0x110
    int m_rect120[4];   // 0x120
    CPnt3D m_color;   // 0x130
    CPnt3D m_color13c;   // 0x13c
    CPnt3D m_color148;   // 0x148
    CPnt3D m_highlightColor;   // 0x154
    float m_seqStart;   // 0x160
    float m_seqLength;   // 0x164
    float m_pulseMin0;   // 0x168
    float m_pulseMax0;   // 0x16c
    float m_pulseTime0;   // 0x170
    float m_pulseMin;   // 0x174
    float m_pulseMax;   // 0x178
    float m_pulseTime;   // 0x17c
    int m_180;   // 0x180
    float m_184;   // 0x184
    zdb::CTexHandle *m_tex188;   // 0x188
    zdb::CTexHandle *m_tex18c;   // 0x18c
    int m_190;   // 0x190
    int m_highlighted;   // 0x194
    int m_198[2];   // 0x198
    CPlainBmp m_highlight;   // 0x1a0
    CPlainBmp m_bar;   // 0x240
    CPlainBmp m_bmp2e0;   // 0x2e0
    CPlainBmp m_bmp380;   // 0x380
    ActionItem m_actions[30];   // 0x420
    int m_numActions;   // 0x600
};   // 0x610 bytes

/* a list box of the spec's texture, in the dialog's font (SOCOM 1's) */
C2D *CUIListBoxSpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    CUIListBox *box = new CUIListBox;
    zdb::CTexHandle *tex = PrepTexture(m_filename.c_str(), mgr);

    box->m_font = font;
    box->LoadSpec(*this, tex, mgr);
    return box;
}

/* a slider of the spec's knob (SOCOM 1's): C2DButton's constructor out of line (0x377800,
 * zui_2d.cpp's; byte-exact since the HUD's round 31 named it) */
C2D *CSliderBarSpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    CSliderBar *slider = new CSliderBar;
    zdb::CTexHandle *knob = PrepTexture(m_knobTexture.c_str(), mgr);

    slider->_strCaption.SetFont(font);
    slider->LoadSpec(*this, knob);
    InitializeButton(slider, mgr);
    return slider;
}

/* the bar's FILENAME_IS_UIVAR flag and the node its LEADING_EDGE follows (SOCOM 1's; GetObjectSpec
 * calls it out of line: pinned) */
#pragma auto_inline off
CHProgressBarSpec::CHProgressBarSpec(_zrdr *rdr) : CImageSpec(rdr)
{
    m_filenameIsUivar = zrdr_findtag(rdr, "FILENAME_IS_UIVAR") != 0;
    m_leadingEdgeNode = zrdr_findstring(rdr, "LEADING_EDGE_NODE");
}
#pragma auto_inline reset

namespace {

/* a design's CONTROLS or OBJECTS (the tag): each item's NAME, TYPE, XPOS, YPOS and OPACITY, its SPEC
 * made by the given parser (GetControlSpec or GetObjectSpec), the object added only when one is made
 * -- unnamed ones called "unnamed" -- with its CHILDOF (0x383360; SOCOM 1's @unnamed@zui_spec_cpp@
 * LoadDlgObjects, which LoadFromRdr calls twice).
 * OPEN, 0x29C against 0x2B8 (written round 16): the calls, stack slots and stores are the target's;
 * the target lays out the loop head's inlined Count() and Item() ternaries -- and the bottom test's
 * -- as then-arm, `b join`, else-arm, the else's `move ...,zero` copied into the branch's delay slot
 * and the branch retargeted past it, leaving the else block orphaned (seven words in all), where MW
 * here drops the else block and lets the then-arm fall into the join (LoadModels and LoadFromRdr,
 * the same inlines, come out the target's). getSpec's `move a1,type` goes into the jalr's delay
 * slot there, into the null test's here. Tried: the item assigned after its declaration, the spec
 * through a ternary, an `&&`, `(*getSpec)(...)`, the spec declared first; declared `inline` the
 * function is absorbed into LoadFromRdr (0x954). Round 23: LoadModels' target lays out the same
 * Count() diamond (its delay slot filled from above, so the else block stays live) and its Item()
 * as a triangle -- there the else's instruction went into the slot and the dead block and its
 * `b` were removed, as here; the target's LoadDlgObjects alone keeps the dead blocks. Measured:
 * `#pragma optimization_level 3` around it gives 0x2A4, optimize_for_size 0x28C, levels 1 and 2
 * far off. (fseek's orphaned else is a different shape: a plain if/else, its beqz not retargeted.) */
void LoadDlgObjects(std::vector<CDesignDlgObj> *objs, _zrdr *rdr, const char *tag,
                    CDlgObjSpec *(*getSpec)(_zrdr *, const char *))
{
    _zrdr *list = zrdr_findtag(rdr, tag);

    for (int i = 0; i < (list ? (int)list->Count() : 0); i++) {
        _zrdr *item = list->Item(i);
        const char *name = zrdr_findstring(item, "NAME");
        const char *type = zrdr_findstring(item, "TYPE");
        int x = 0;
        int y = 0;
        float opacity = 1.0f;

        zrdr_findint(item, "XPOS", &x, 1);
        zrdr_findint(item, "YPOS", &y, 1);
        zrdr_findreal(item, "OPACITY", &opacity, 1);
        _zrdr *specRdr = zrdr_findtag(item, "SPEC");
        if (specRdr) {
            CDlgObjSpec *spec = getSpec(specRdr, type);
            if (spec) {
                objs->resize(objs->size() + 1);
                CDesignDlgObj &o = objs->back();
                o.SetSpec(spec);
                o.SetPos(x, y);
                o.m_opacity = opacity;
                o.SetName(name);
                o.m_childOf = zrdr_findstring(item, "CHILDOF");
            }
        }
    }
}

}

/* the spec's UI variable, if its rdr declares one (SOCOM 1's; inlined in each spec's constructor,
 * the copy at 0x3832E0) */
CDlgObjSpec::CDlgObjSpec(_zrdr *rdr)
{
    m_var = 0;
    if (rdr) {
        _zrdr *var = zrdr_findtag(rdr, "UIVAR");

        if (var)
            m_var = new CUIVariableSpec(var);
    }
}

/* (SOCOM II's; out of line, GetObjectSpec calls it) */
CClockSpec::CClockSpec(_zrdr *rdr) : CTextSpec(rdr)
{
}

/* a copy (SOCOM 1's; CDlgObjSpec's copy is zui_cleanup.cpp's, a call here, the rest inlined) */
CDlgObjSpec *CClockSpec::Copy()
{
    return new CClockSpec(*this);
}

/* a clock of the spec's text (SOCOM 1's) */
C2D *CClockSpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    C2DClock *clock = new C2DClock;

    clock->SetFont(font);
    clock->LoadSpec(*this);
    return clock;
}

/* the check's offset, size and texture and the actions run on checking and unchecking (SOCOM 1's;
 * CButtonSpec's constructor out of line, ActionItem's inlined) */
CCheckBoxSpec::CCheckBoxSpec(_zrdr *rdr) : CButtonSpec(rdr)
{
    zrdr_findint(rdr, "CHECK_X", &m_checkOffset.x, 1);
    zrdr_findint(rdr, "CHECK_Y", &m_checkOffset.y, 1);
    zrdr_findIPNT2D(rdr, "CHECK_OFFSET", &m_checkOffset);
    zrdr_findint(rdr, "CHECKSIZE_X", &m_checkSize.x, 1);
    zrdr_findint(rdr, "CHECKSIZE_Y", &m_checkSize.y, 1);
    zrdr_findIPNT2D(rdr, "CHECK_SIZE", &m_checkSize);
    m_checkTexture = zrdr_findstring(rdr, "CHECK_TEXTURE");
    m_onCheck = new ActionItem;
    m_onUncheck = new ActionItem;
    _zrdr *anim = zrdr_findtag(rdr, "ONCHECK");
    if (anim)
        ParseButtonAnim(anim, m_onCheck);
    anim = zrdr_findtag(rdr, "ONUNCHECK");
    if (anim)
        ParseButtonAnim(anim, m_onUncheck);
}

/* a copy (SOCOM 1's, out of line: Copy calls it): the button's implicit copy inlined one level,
 * the check's fields assigned, its actions copied */
CCheckBoxSpec::CCheckBoxSpec(CCheckBoxSpec &spec) : CButtonSpec(spec)
{
    m_checkOffset = spec.m_checkOffset;
    m_checkSize = spec.m_checkSize;
    m_checkTexture = spec.m_checkTexture;
    m_onCheck = new ActionItem(*spec.m_onCheck);
    m_onUncheck = new ActionItem(*spec.m_onUncheck);
}

/* a check box of the spec's check (SOCOM 1's): C2DButton's constructor out of line (0x377800,
 * zui_2d.cpp's; byte-exact since the HUD's round 31 named it) */
C2D *CCheckBoxSpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    CCheckBox *box = new CCheckBox;
    zdb::CTexHandle *check = PrepTexture(m_checkTexture.c_str(), mgr);

    box->_strCaption.SetFont(font);
    box->LoadSpec(*this, check);
    InitializeButton(box, mgr);
    return box;
}

/* the design object's spec deleted, its strings destroyed (SOCOM II's, an ordinary function: zui.cpp's
 * tail and the radio strip's and handler button's destructors call it) */
CDesignDlgObj::~CDesignDlgObj()
{
    delete m_spec;
}

/* a copy of another's: the old spec deleted and the other's copied through its Copy slot, the
 * position, opacity, name and parent assigned (SOCOM 1's CDesignDlgObj assignment; out of line).
 * NOT pinned (the UI lane's round 20): MW refuses to auto-inline it on its own, and the refusal
 * propagates (the engine core's round 5) -- std::copy's, copy_backward's and fill_n's helpers over
 * CDesignDlgObj (0x3845B0, 0x384B00, 0x384AA0) are then refused in turn and called out of line
 * from the tail's erase and insert, each calling this out of line, as in the target; pinned with
 * `#pragma auto_inline off`, the helpers inlined whole and their three rows were never emitted */
void CDesignDlgObj::operator=(const CDesignDlgObj &o)
{
    delete m_spec;
    m_spec = o.m_spec ? o.m_spec->Copy() : 0;
    m_xpos = o.m_xpos;
    m_ypos = o.m_ypos;
    m_opacity = o.m_opacity;
    m_name = o.m_name;
    m_childOf = o.m_childOf;
}

/* the map's corners, its ELEMENTS (each an object spec at an offset, added to the design objects)
 * and its ANIMATIONS (SOCOM 1's; CDlgObjSpec's constructor inlined) *
 * OPEN, 14 instructions, one register pair: in the ELEMENTS loop the target gives the hoisted
 * `elements == 0` bool s0 and the element pointer s1, here the other way round (the ANIMATIONS
 * loop matches). No change from the element declared ahead of the loop, after the offsets, the
 * spec declared ahead of its test, or an inline count helper (12 bytes longer) */
CUIMapCtrlSpec::CUIMapCtrlSpec(_zrdr *rdr) : CDlgObjSpec(rdr)
{
    zrdr_findIPNT2D(rdr, "UPPERLEFT", &m_upperLeft);
    zrdr_findIPNT2D(rdr, "LOWERRIGHT", &m_lowerRight);
    _zrdr *elements = zrdr_findtag(rdr, "ELEMENTS");
    for (int i = 0; i < (elements ? (int)elements->Count() : 0); i++) {
        _zrdr *e = elements->Item(i);
        int x, y;

        zrdr_findint(e, "XOFFSET", &x, 1);
        zrdr_findint(e, "YOFFSET", &y, 1);
        const char *type = zrdr_findstring(e, "TYPE");
        _zrdr *specRdr = zrdr_findtag(e, "SPEC");
        if (specRdr) {
            CDlgObjSpec *spec = GetObjectSpec(specRdr, type);
            if (spec) {
                m_elements.resize(m_elements.size() + 1);
                CDesignDlgObj &o = m_elements.back();
                o.SetSpec(spec);
                o.SetPos(x, y);
            }
        }
    }
    _zrdr *anims = zrdr_findtag(rdr, "ANIMATIONS");
    for (int j = 0; j < (anims ? (int)anims->Count() : 0); j++) {
        m_animations.resize(m_animations.size() + 1);
        ParseButtonAnim(anims->Item(j), &m_animations.back());
    }
}
