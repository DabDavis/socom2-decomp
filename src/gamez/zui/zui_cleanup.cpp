/* zui_cleanup.cpp -- the specs' copies, destructors and type names (OVL1 0x386E20-0x389C5F): the
 * press beta's asserts name the unit (0x3879E0, 0x389160). SOCOM 1 kept here what each spec does
 * once parsed -- CUIVariableSpec, CDlgObjSpec's copy and destructor, the Copy and the destructor
 * of the specs zui_spec.cpp does not define, their GetTypes -- in the same order. Defined in
 * reverse of their image order (-inline deferred).
 *
 * CDlgObjSpec's destructor and copy are defined here, so this unit inlines them (and the
 * variable spec's) where zui_spec.cpp calls them; that makes every spec's implicit copy and
 * destructor taller here: CTextSpec's copy is inlined one level (its members' copies are calls).
 *
 * OPEN, not written yet: the button family's Copies and destructors (CButtonSpec's copy out of
 * line 0x387630, the radio strip's, check box's, slider's, handler button's, list box's and map
 * control's; their destructors call 0x36E650 and 0x367670 -- the 0x34-byte button-state string's
 * and vector<ActionItem>'s destructors, rows outside the lane), the CreateDisplayObjs (the C2D
 * classes' constructors, as zui_spec.cpp's) and 0x389B00 (a vector<CDesignDlgObj> instance). */

#include "zui_2d.h"

CDlgObjSpec *CImageSpec::Copy()
{
    return new CImageSpec(*this);
}

/* a copy of the spec, its variable spec copied too (SOCOM 1's; the variable spec's copy inlined
 * in SOCOM II) */
CDlgObjSpec::CDlgObjSpec(CDlgObjSpec &spec)
{
    m_var = 0;
    if (spec.GetVarSpec())
        m_var = new CUIVariableSpec(spec.GetVarSpec());
}

CDlgObjSpec::~CDlgObjSpec()
{
    if (m_var)
        delete m_var;
}

CDlgObjSpec *CTextSpec::Copy()
{
    return new CTextSpec(*this);
}

/* a renderable string of the spec's text (SOCOM 1's) */
C2D *CTextSpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    CRenderableString *str = new CRenderableString;

    str->SetFont(font);
    str->LoadSpec(*this);
    return str;
}

CDlgObjSpec *CTtySpec::Copy()
{
    return new CTtySpec(*this);
}

/* a teletype of the spec's text (SOCOM 1's) */
C2D *CTtySpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    CTeleType *tty = new CTeleType;

    tty->SetFont(font);
    func_00378B00(tty, this);
    return tty;
}

CDlgObjSpec *CSecureTextSpec::Copy()
{
    return new CSecureTextSpec(*this);
}

/* the spec's text drawn as stars (SOCOM II's) */
C2D *CSecureTextSpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    CSecureText *str = new CSecureText;

    str->SetFont(font);
    str->LoadSpec(*this);
    return str;
}

CDlgObjSpec *CWrapSpec::Copy()
{
    return new CWrapSpec(*this);
}

/* wrapped text of the spec's (SOCOM 1's) *
 * OPEN until a row outside the lane is named: byte-exact with 0x20CD50 (hud_newhudmap.cpp's tail)
 * as __ct__9CPlainBmpFv -- CPlainBmp's inline constructor out of line, which CWrappedText's body
 * calls for its two arrows (measured with the row renamed) */
C2D *CWrapSpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    CWrappedText *text = new CWrappedText;

    text->SetFont(font);
    func_0037A430(text, this, mgr);
    return text;
}

CDlgObjSpec *CScrollSpec::Copy()
{
    return new CScrollSpec(*this);
}

/* scrolling text of the spec's (SOCOM 1's) */
C2D *CScrollSpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    CScrollText *text = new CScrollText;

    text->SetFont(font);
    func_00370340(text, this);
    return text;
}

CDlgObjSpec *CHProgressBarSpec::Copy()
{
    return new CHProgressBarSpec(*this);
}

/* the rdr's UIVAR block: the NAME, the VALUE (a one-item array is its item) cloned into the
 * spec's own string table, the LONGEVITY (temporary unless the rdr says) -- SOCOM 1's */
CUIVariableSpec::CUIVariableSpec(_zrdr *rdr)
    : m_name(_strdup(zrdr_findstring(rdr, "NAME"), 0, 0)), m_value(0), m_table(0, 1024)
{
    _zrdr *value = zrdr_findtag(rdr, "VALUE");

    if (value && value->Type() == 4 && value->Count() == 1)
        value = value->Item(0);
    if (value)
        m_value = new _zrdr(value, &m_table);
    m_longevity = UIVL_TEMPORARY;
    const char *longevity = zrdr_findstring(rdr, "LONGEVITY");
    if (longevity) {
        if (!strcasecmp(longevity, "PERSISTENT"))
            m_longevity = UIVL_PERSISTENT;
        else if (!strcasecmp(longevity, "PERMANENT"))
            m_longevity = UIVL_PERMANENT;
        else if (!strcasecmp(longevity, "TEMPORARY"))
            m_longevity = UIVL_TEMPORARY;
    }
}

/* the variable's name and value freed (SOCOM 1's; its string table is a member) */
CUIVariableSpec::~CUIVariableSpec()
{
    if (m_name)
        _free(m_name, 0, 0);
    m_name = 0;
    if (m_value) {
        m_value->DeleteChildren();
        delete m_value;
    }
}

/* a radio strip (SOCOM 1's) *
 * OPEN until a row outside the lane is named: byte-exact with 0x377800 (zui_2d.cpp's) as
 * __ct__9C2DButtonFv -- C2DButton's constructor, out of line (measured with the row renamed) */
C2D *CRadioStripSpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    CRadioStrip *strip = new CRadioStrip;

    strip->_strCaption.SetFont(font);
    func_003710C0(strip, this, 0);
    InitializeButton(strip, mgr);
    return strip;
}

/* a handler button (SOCOM 1's) *
 * OPEN until a row outside the lane is named: byte-exact with 0x377800 (zui_2d.cpp's) as
 * __ct__9C2DButtonFv -- C2DButton's constructor, out of line (measured with the row renamed) */
C2D *CHandlerButtonSpec::CreateDisplayObj(C2DFont *font, zdb::CTextureRelocMgr *mgr) const
{
    CHandlerButton *button = new CHandlerButton;

    button->_strCaption.SetFont(font);
    func_00376690(button, this);
    InitializeButton(button, mgr);
    return button;
}
