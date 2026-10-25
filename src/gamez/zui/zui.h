/* zui.h -- the UI's variables (CUIVariable, the manager CUIVarManager, the mixin a display object
 * or a tuner watches one through) and what the widget units share. SOCOM 1's DWARF names a
 * gamez\zui\zui.h; this one is ours, declarations as far as the units here need them. Layouts
 * are SOCOM 1's where its DWARF has the class, moved to SOCOM II's offsets as the code reads them.
 *
 * _zrdr is declared here as zanim.h declares it (SOCOM 1's DWARF) with the members these units
 * call: a unit includes this header or zanim.h, never both. */
#ifndef ZUI_H
#define ZUI_H

#include <string.h>
#include <vector>

/* a ZRDR value (SOCOM 1's DWARF): a typed word -- 1 an integer, 2 a real, 3 a string, 4 an array */
struct _zrdr {
    unsigned int type : 8;   // 0x0
    unsigned int isclone : 1;   // 0x0
    unsigned int packed : 1;   // 0x0
    unsigned int unused : 6;   // 0x0
    unsigned int length : 16;   // 0x0
    union {
        float real;
        int integer;
        char *string;
        _zrdr *array;
    };   // 0x4

    ~_zrdr();
    void DeleteChildren();
    bool IsArray() const;

    /* (ours) */
    bool IsReal() { return type == 2; }
    /* a number: a real as it is, an integer converted, anything else 0 (ours, as zanim.h's) */
    float Real()
    {
        if (type == 2)
            return real;
        else if (type == 1)
            return (float)integer;
        else
            return 0.0f;
    }
};   // 0x8 bytes

/* how long a variable lives (SOCOM 1's enum; the values' names ours, as zanim_menu.h's) */
enum UIVAR_LONGEVITY {
    UIVAR_TEMPORARY = 0,
    UIVAR_SESSION = 1,
    UIVAR_PERMANENT = 2
};

class UIVarMixin;

/* a UI variable: a ZRDR value with a name and the mixins watching it (SOCOM 1's DWARF: 0x1C
 * bytes, the watchers a vector at 0xC; SOCOM II's watchers are at 0x18 and its longevity a byte
 * at 0x24, its name at 0x14 and its size 0x30 -- the manager's Find walks 48-byte elements; what
 * it added at 0x8-0x13 (two table pointers its copy constructor stores) is not read here) */
class CUIVariable : public _zrdr {
public:
    void UpdateDependents();
    void DetachAll();
    void Destroy();
    CUIVariable &operator=(const char *str);
    void Attach(UIVarMixin *mixin);
    /* (ours: the tuners set their defaults so, inlined) */
    CUIVariable &operator=(float f)
    {
        if (type != 2 || real != f) {
            DeleteChildren();
            type = 2;
            real = f;
            UpdateDependents();
        }
        return *this;
    }

    void *m_8;   // 0x8: SOCOM II's (a table pointer the copy constructor stores; SOCOM 1's m_varname sat here)
    unsigned char _c[0x14 - 0xc];
    char *m_name;   // 0x14: SOCOM II's name, the string table's copy (Destroy clears it; the manager's Find compares it)
    std::vector<UIVarMixin *> m_attached_items;   // 0x18 (SOCOM 1's at 0xC)
    unsigned char m_longevity;   // 0x24: UIVAR_LONGEVITY
    unsigned char _25[0x30 - 0x25];
};

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

/* the variables (SOCOM 1's DWARF: CUIVarManager : vector, CSaveModule, 0x8064 bytes) */
class CUIVarManager : public std::vector<CUIVariable> {
public:
    CUIVariable *Add(const char *name, _zrdr *value, UIVAR_LONGEVITY longevity);
    CUIVariable *Find(const char *name);

    unsigned char _c[0x8064 - 0xc];
};
extern CUIVarManager theUIVarManager;

#endif
