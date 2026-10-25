/* zui_spec.cpp -- the dialog objects' specifications parsed (OVL1 0x37D170-0x38560F): each spec's
 * _zrdr constructor, CreateDisplayObj and some Copies, the dialog design's loaders and the
 * template instances closing the unit. SOCOM 1's demo names the unit (its unnamed namespace,
 * @unnamed@zui_spec_cpp@) and holds the same run in the same order, from CClockSpec's destructor
 * to its vector instances. Defined in reverse of their image order (-inline deferred). */

#include "zui_spec.h"

/* a real the rdr may give, else the default (ours: the counter's StartTime initialises its member
 * so, the read before the string member's construction) */
static inline float FindReal(_zrdr *rdr, const char *tag, float def)
{
    float f = def;

    zrdr_findreal(rdr, tag, &f, 1);
    return f;
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

/* the wrapped text's WIDTH, line SPACING (10) and HEIGHT (-1: unbounded), its arrows' TEXTURE,
 * SIZE and OFFSET, AUTOSCROLL and DISPLAY_ONE_LINE (SOCOM 1's) *
 * OPEN until three rows outside the lane are named: byte-exact with 0x22DCC0 as
 * __as__9CSaferStrFPCc, 0x2A8F30 as __ct__6CPnt3DFfff and 0x20F1F0 as __as__6CPnt3DFRC6CPnt3D
 * (CTextSpec's constructor inlined one level calls them; measured with the three renamed) */
CWrapSpec::CWrapSpec(_zrdr *rdr) : CTextSpec(rdr), m_width(0.0f), m_spacing(10), m_height(-1.0f)
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
 * construction follows the StartTime read and the ValVar lookup, as the target orders them *
 * OPEN until three rows outside the lane are named: byte-exact with 0x22DCC0 as
 * __as__9CSaferStrFPCc, 0x2A8F30 as __ct__6CPnt3DFfff and 0x20F1F0 as __as__6CPnt3DFRC6CPnt3D
 * (CTextSpec's constructor inlined one level calls them; measured with the three renamed) */
CCounterSpec::CCounterSpec(_zrdr *rdr)
    : CTextSpec(rdr), m_startTime(FindReal(rdr, "StartTime", 0.0f)), m_counting(false), m_endTime(0.0f),
      m_valVar(zrdr_findstring(rdr, "ValVar"))
{
    if (!zrdr_findreal(rdr, "EndTime", &m_endTime, 1) && zrdr_findreal(rdr, "Duration", &m_endTime, 1))
        m_counting = true;
}

CDlgObjSpec *CCounterSpec::Copy()
{
    return new CCounterSpec(*this);
}

/* the bar's FILENAME_IS_UIVAR flag and the node its LEADING_EDGE follows (SOCOM 1's) */
CHProgressBarSpec::CHProgressBarSpec(_zrdr *rdr) : CImageSpec(rdr)
{
    m_filenameIsUivar = zrdr_findtag(rdr, "FILENAME_IS_UIVAR") != 0;
    m_leadingEdgeNode = zrdr_findstring(rdr, "LEADING_EDGE_NODE");
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

/* a copy (SOCOM 1's; CDlgObjSpec's copy is zui_cleanup.cpp's, a call here, the rest inlined) */
CDlgObjSpec *CClockSpec::Copy()
{
    return new CClockSpec(*this);
}
