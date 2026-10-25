/* zui_cleanup.cpp -- the specs' copies, destructors and type names (OVL1 0x386E20-0x389C5F): the
 * press beta's asserts name the unit (0x3879E0, 0x389160). SOCOM 1 kept here what each spec does
 * once parsed -- CUIVariableSpec, CDlgObjSpec's copy and destructor, the Copy and the destructor
 * of the specs zui_spec.cpp does not define, their GetTypes -- in the same order. Defined in
 * reverse of their image order (-inline deferred).
 *
 * CDlgObjSpec's destructor and copy are defined here, so this unit inlines them (and the
 * variable spec's) where zui_spec.cpp calls them; that makes every spec's implicit copy and
 * destructor taller here: CTextSpec's copy is inlined one level (its members' copies are calls). */

#include "zui_spec.h"

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

CDlgObjSpec *CTtySpec::Copy()
{
    return new CTtySpec(*this);
}

CDlgObjSpec *CSecureTextSpec::Copy()
{
    return new CSecureTextSpec(*this);
}

CDlgObjSpec *CWrapSpec::Copy()
{
    return new CWrapSpec(*this);
}

CDlgObjSpec *CScrollSpec::Copy()
{
    return new CScrollSpec(*this);
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
