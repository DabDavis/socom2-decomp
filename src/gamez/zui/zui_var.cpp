/* zui_var.cpp -- the UI variables (OVL1 0x385610-0x386E1F): CUIVarManager (the variables, saved
 * as a CSaveModule), UIVarMixin's attach and detach, CUIVariable's assignment, formatting and its
 * watchers. SOCOM 1's demo holds the same run (UnSerialize ... ~CUIVarManager, DetachUIVar,
 * AttachUIVar, IsAttached, _Refresh, Format, operator=, UpdateDependents, Set, DetachAll, Destroy,
 * then the thunks) between zui_spec.cpp's tail and zui_cleanup.cpp; no assert names the file, so
 * zui_var.cpp is ours (the press beta's zui.cpp may be it).
 *
 * OPEN, not written yet: the manager's Serialize, Add, Find, CollectGarbage, Reset and the other
 * variable methods; DetachUIVar, AttachUIVar and ~CUIVarManager stand as generated drafts. */

#include "zui.h"

/* the variable emptied: its watchers detached, its value freed unless packed, made an integer
 * (SOCOM 1's Destroy; SOCOM II calls DetachAll for its first loop) */
void CUIVariable::Destroy()
{
    DetachAll();
    if (!packed)
        DeleteChildren();
    m_14 = 0;
    type = 1;
    isclone = 0;
    packed = 0;
    m_longevity = 0;
}

/* every watcher detached: a successful detach changes the vector, so the walk restarts (called
 * out of line by Destroy: pinned) */
#pragma auto_inline off
void CUIVariable::DetachAll()
{
    std::vector<UIVarMixin *>::iterator i = m_attached_items.begin();

    while (!m_attached_items.empty() && i != m_attached_items.end()) {
        if ((*i)->DetachUIVar(this))
            i = m_attached_items.begin();
        else
            ++i;
    }
    m_attached_items.clear();
}
#pragma auto_inline reset

void CUIVariable::UpdateDependents()
{
    std::vector<UIVarMixin *>::iterator i;

    for (i = m_attached_items.begin(); i != m_attached_items.end(); ++i)
        (*i)->UpdateUIVar(this);
}
