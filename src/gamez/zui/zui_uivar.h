/* zui_uivar.h -- UIVarMixin, what watches a UI variable, in a header of its own (ours; SOCOM 1's
 * DWARF files the class with zui.h's): zui.h includes it ahead of CUIVariable, which SOCOM II
 * derives from it (UIVarMixin at 0x8, CUIVariable's own vtable pointer at 0x10 -- 0x2B1530's
 * RTTI), and zui_2d.h for its display objects' second base. */
#ifndef ZUI_UIVAR_H
#define ZUI_UIVAR_H

class CUIVariable;

/* what watches a variable: told by UpdateUIVar when it changes (SOCOM 1's DWARF: the vtable
 * pointer at 0, the variable at 4) */
class UIVarMixin {
public:
    UIVarMixin() { m_uivar = 0; }
    virtual ~UIVarMixin() { DetachUIVar(0); }
    virtual void UpdateUIVar(CUIVariable *var) = 0;
    bool DetachUIVar(CUIVariable *var);
    void AttachUIVar(CUIVariable *var);

    CUIVariable *m_uivar;   // 0x4
};   // 0x8 bytes

#endif
