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

#include "../zutil/util_stable.h"

class CSTable;

/* a ZRDR value (SOCOM 1's DWARF): a typed word -- 1 an integer, 2 a real, 3 a string, 4 an array.
 * SOCOM II's has the rdr lane's nofree flag after packed (the clone constructor clears five
 * fields, zrdr.h's layout) */
struct _zrdr {
    unsigned int type : 8;   // 0x0
    unsigned int isclone : 1;   // 0x0
    unsigned int packed : 1;   // 0x0
    unsigned int nofree : 1;   // 0x0: SOCOM II's (zrdr.h's name)
    unsigned int unused : 5;   // 0x0
    unsigned int length : 16;   // 0x0
    union {
        float real;
        int integer;
        char *string;
        _zrdr *array;
    };   // 0x4

    /* a copy of src, its strings made in the table (SOCOM 1's out of line, 0x23E140 there;
     * inlined in each of SOCOM II's callers). The fields are cleared in the mem-initializer
     * list: each is then read, masked and written back as a whole word, `(0 & mask) << shift`
     * (CDlgObjSpec's copy); assigned in the body, MW narrows the stores to bytes and halfwords */
    _zrdr(_zrdr *src, CSTable *strings)
        : type(0), isclone(false), packed(false), nofree(false), unused(0), length(0)
    {
        Clone(src, strings);
    }
    /* an empty value, cleared the same way (util_options.cpp's local the command line's words are
     * read into: the target's word-wide read-mask-write of each field) */
    _zrdr() : type(0), isclone(false), packed(false), nofree(false), unused(0), length(0) {}
    ~_zrdr();
    void DeleteChildren();
    void Clone(_zrdr *src, CSTable *strings);
    bool IsArray() const;

    /* (ours) */
    bool IsReal() { return type == 2; }
    /* the value's type, an array's count and its items (ours, as zrdr.h's) */
    unsigned int Type() const { return type; }
    unsigned int Count() const { return type == 4 ? length : 0; }
    _zrdr *Item(unsigned int i) const { return i < Count() ? &array[i] : 0; }
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
    /* an integer: a real truncated, anything else 0 (ours: the counter's spec reads its start so) */
    int Integer()
    {
        if (type == 1)
            return integer;
        else if (type == 2)
            return (int)real;
        else
            return 0;
    }
    /* a string's text, or 0 (ours: the image specs read a texture name so, a ternary) */
    const char *String() { return type == 3 ? string : 0; }
};   // 0x8 bytes

/* how long a variable lives (SOCOM 1's DWARF, names and values; a dormant variable is a free
 * slot of the manager's) */
enum UIVAR_LONGEVITY {
    UIVL_DORMANT = 0,
    UIVL_TEMPORARY = 1,
    UIVL_PERMANENT = 2,
    UIVL_PERSISTENT = 3
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
    /* the value replaced by a copy of value, its strings in the manager's current table, and the
     * watchers told (SOCOM 1's) */
    void Set(_zrdr *value);
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

class CRdrFile;
namespace zar {
class CZAR;
}

/* a part of a save (the zsav lane's zsave.h has the whole class; SOCOM 1's DWARF: 0x1C bytes,
 * the vtable at 0) */
class CSaveModule {
public:
    virtual ~CSaveModule();
    virtual void Serialize(int flags) = 0;
    virtual void UnSerialize(const char *name, CRdrFile *file, int flags);
    virtual void UnSerialize(const char *name, zar::CZAR *zar, int flags);
    virtual bool TestFlags(int flags) const;
    bool SaveRdr(const char *name, CRdrFile *file);

    unsigned char _4[0x1C - 0x4];
};   // 0x1C bytes

/* a string table of the manager's: when it is full, the variables' strings are collected into
 * the other table and the string made there (SOCOM 1's, no DWARF members of its own) */
class CUIVarSTable : public CSTable {
public:
    CUIVarSTable();
    char *OnTableFull(const char *str);
};   // 0x20 bytes

/* the variables (SOCOM 1's DWARF: CUIVarManager : vector, CSaveModule, 0x8064 bytes; SOCOM II's
 * tables are 0x20 bytes, so its pack buffers start at 0x6C) */
class CUIVarManager : public std::vector<CUIVariable>, public CSaveModule {
public:
    /* the members ahead of the virtuals: declared after them, MW gives the class a vtable pointer
     * of its own at 0x28 (its first base has none) and the tables move to 0x30 */
    int m_currtab;                       // 0x28: the table strings are made in
    CUIVarSTable m_stab[2];              // 0x2C
    char m_packbuf[2][0x4000];           // 0x6C: the arrays packed, a buffer per table

    ~CUIVarManager();
    void Serialize(int flags);
    void UnSerialize(const char *name, CRdrFile *file, int flags);
    CUIVariable *Add(const char *name, _zrdr *value, UIVAR_LONGEVITY longevity);
    CUIVariable *Find(const char *name);
    CUIVariable *FindDormantVar(const char *name, _zrdr *value, UIVAR_LONGEVITY longevity);
    void CollectGarbage();
    void Reset();
};
extern CUIVarManager theUIVarManager;

#endif
