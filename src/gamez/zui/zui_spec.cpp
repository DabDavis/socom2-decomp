/* zui_spec.cpp -- the dialog objects' specifications parsed (OVL1 0x37D170-0x38560F): each spec's
 * _zrdr constructor, CreateDisplayObj and some Copies, the dialog design's loaders and the
 * template instances closing the unit. SOCOM 1's demo names the unit (its unnamed namespace,
 * @unnamed@zui_spec_cpp@) and holds the same run in the same order, from CClockSpec's destructor
 * to its vector instances. Defined in reverse of their image order (-inline deferred).
 *
 * OPEN, not written yet: every CreateDisplayObj but the map control's and the text's drafts (they
 * inline the C2D classes' constructors, zui_2d.cpp's -- CRenderableString's 0x37A770 and its
 * LoadSpec 0x3792F0, CPlainBmp's, CHProgressBar's -- whose rows the C2D lane has to name first),
 * the button family's constructors (CButtonSpec's 0x380620, CRadioStripSpec's 0x37DA50,
 * CCheckBoxSpec's two, CHandlerButtonSpec's, CUIListBoxSpec's 0x37EAB0, CUIMapCtrlSpec's 0x37D600;
 * ~CButtonSpec 0x37DD30 needs 0x36AC10 named, the ActionItem vector deleter's destructor),
 * CButtonSpec::InitializeButton 0x380500, GetControlSpec 0x382890, LoadDlgObjects 0x383360, the
 * dialog design's loaders 0x3812B0/0x381550, CDesignDlgObj's assignment and destructor copies
 * (0x37D910, 0x37D9B0: emitted with the vector instances) and the template instances
 * 0x3836E0-0x3855FF (include/vector's class-vector insert, erase and resize). */

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
    func_00379080(bmp, this, tex);
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

/* a button (SOCOM 1's) -- held out of the build until 0x377800 is named: the generated draft
 * (src/auto/ovl1/00380470.c) matches through the placeholder, and this one, measured byte-exact
 * with the row renamed, would supersede it with a wrong reference until then */
#if 0
/* a button (SOCOM 1's) */
C2D *CButtonSpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    C2DButton *button = new C2DButton;

    button->_strCaption.SetFont(font);
    func_00376BD0(button, this);
    InitializeButton(button, mgr);
    return button;
}
#endif

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

/* a slider of the spec's knob (SOCOM 1's) *
 * OPEN until a row outside the lane is named: byte-exact with 0x377800 (zui_2d.cpp's) as
 * __ct__9C2DButtonFv -- C2DButton's constructor, out of line (measured with the row renamed) */
C2D *CSliderBarSpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    CSliderBar *slider = new CSliderBar;
    zdb::CTexHandle *knob = PrepTexture(m_knobTexture.c_str(), mgr);

    slider->_strCaption.SetFont(font);
    func_003724F0(slider, this, knob);
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

/* a check box of the spec's check (SOCOM 1's) *
 * OPEN until a row outside the lane is named: byte-exact with 0x377800 (zui_2d.cpp's) as
 * __ct__9C2DButtonFv -- C2DButton's constructor, out of line (measured with the row renamed) */
C2D *CCheckBoxSpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    CCheckBox *box = new CCheckBox;
    zdb::CTexHandle *check = PrepTexture(m_checkTexture.c_str(), mgr);

    box->_strCaption.SetFont(font);
    func_00371550(box, this, check);
    InitializeButton(box, mgr);
    return box;
}
