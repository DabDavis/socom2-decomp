/* zui_spec.cpp -- the dialog objects' specifications parsed (OVL1 0x37D170-0x38560F): each spec's
 * _zrdr constructor, CreateDisplayObj and some Copies, the dialog design's loaders and the
 * template instances closing the unit. SOCOM 1's demo names the unit (its unnamed namespace,
 * @unnamed@zui_spec_cpp@) and holds the same run in the same order, from CClockSpec's destructor
 * to its vector instances. Defined in reverse of their image order (-inline deferred). */

#include "zui_spec.h"

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
