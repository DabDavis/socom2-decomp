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
#include <algorithm>

#include "../zutil/util_stable.h"
#include "zui_uivar.h"

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
    /* the two read through a const item (ours: CreateSoftKeyboard's grid items -- through a const
     * pointer the type tested once serves both, through a non-const one each reloads it) */
    int Integer() const
    {
        if (type == 1)
            return integer;
        else if (type == 2)
            return (int)real;
        else
            return 0;
    }
    const char *String() const { return type == 3 ? string : 0; }
};   // 0x8 bytes

/* how long a variable lives (SOCOM 1's DWARF, names and values; a dormant variable is a free
 * slot of the manager's) */
enum UIVAR_LONGEVITY {
    UIVL_DORMANT = 0,
    UIVL_TEMPORARY = 1,
    UIVL_PERMANENT = 2,
    UIVL_PERSISTENT = 3
};

/* a UI variable: a ZRDR value with a name and the mixins watching it (SOCOM 1's DWARF: 0x1C
 * bytes, : _zrdr, the watchers a vector at 0xC). SOCOM II's is 0x30 bytes (the manager's Find
 * walks 48-byte elements) and itself a watcher: UIVarMixin is its second base at 0x8 (its table
 * 0x4050B0 stored there, m_uivar 0xC cleared, by the copy constructor), and as its first base has
 * no vtable the class has a vtable pointer of its own at 0x10 -- 0x4062B0, {RTTI 0x3F1D58, 0}, its
 * slots after the mixin's table 0x4062B8 (the two thunks 0x2B2180/0x2B2170, then ~CUIVariable
 * 0x2B1530 and UpdateUIVar 0x2B1A10: fts_mission.cpp's copies, the vector's erase destroying an
 * element through slot 0x18). Its name at 0x14, the watchers at 0x18, the longevity a byte enum at
 * 0x24 (copied lb/sb), a callback at 0x28 with its argument at 0x2C */
class CUIVariable : public _zrdr, public UIVarMixin {
public:
    /* a free slot: the name and the callback cleared, room for 16 watchers (SOCOM II's;
     * fts_mission.cpp's resize inlines it, calling _zrdr's and UIVarMixin's constructors) */
    CUIVariable() : m_name(0), m_longevity(UIVL_DORMANT), m_callback(0), m_callbackData(0)
    {
        m_attached_items.reserve(16);
    }
    /* a copy: the name made again in the manager's current table and the longevity carried --
     * not the value, nor the callback (SOCOM II's, 0x386BF0: the vector insert's copies call it) */
    CUIVariable(const CUIVariable &v);
    /* (SOCOM 1's: Destroy; SOCOM II's virtual, its copy fts_mission.cpp's 0x2B1530) */
    virtual ~CUIVariable() { Destroy(); }
    /* a watched variable changed: the callback told, with its argument (SOCOM II's, 0x2B1A10) */
    virtual void UpdateUIVar(CUIVariable *var)
    {
        if (m_callback)
            m_callback(var, m_callbackData);
    }
    void UpdateDependents();
    void DetachAll();
    void Destroy();
    CUIVariable &operator=(const char *str);
    void Attach(UIVarMixin *mixin);
    /* whether the mixin watches the variable (SOCOM 1's IsAttached, out of line there; SOCOM II's
     * inline -- Attach inlines it one level, calling the watcher vector's begin(), end(), find and
     * != out of line) */
    bool IsAttached(UIVarMixin *mixin)
    {
        bool found = false;

        if (std::find(m_attached_items.begin(), m_attached_items.end(), mixin) != m_attached_items.end())
            found = true;
        return found;
    }
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

    /* 0x8: UIVarMixin (SOCOM 1's m_varname sat here); 0x10: the vtable pointer */
    char *m_name;   // 0x14: SOCOM II's name, the string table's copy (Destroy clears it; the manager's Find compares it)
    std::vector<UIVarMixin *> m_attached_items;   // 0x18 (SOCOM 1's at 0xC)
    UIVAR_LONGEVITY m_longevity;   // 0x24
    void (*m_callback)(CUIVariable *var, void *arg);   // 0x28: SOCOM II's (the names ours)
    void *m_callbackData;   // 0x2c
};   // 0x30 bytes

class CRdrFile;
namespace zar {
class CZAR;
}

class CSaveManager;

/* a part of a save (the zsav lane's zsave.h has the whole class; SOCOM 1's DWARF: 0x18 bytes,
 * the vtable at 0 -- zsave.h's layout; the states lane's round 22: 0x1C put CUIVarManager's own
 * vtable pointer, which its initialiser and destructor store at 0x24, at 0x28) */
class CSaveModule {
public:
    CSaveModule(const char *name, CSaveManager *manager);   /* 0x33D300 */
    virtual ~CSaveModule();
    virtual void Serialize(int flags) = 0;
    virtual void UnSerialize(const char *name, CRdrFile *file, int flags);
    virtual void UnSerialize(const char *name, zar::CZAR *zar, int flags);
    virtual bool TestFlags(int flags) const;
    bool SaveRdr(const char *name, CRdrFile *file);

    unsigned char _4[0x18 - 0x4];
};   // 0x18 bytes

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
    /* the virtuals ahead of the members: MW gives the class a vtable pointer of its own (its first
     * base has none) where the first is declared, 0x24 after the 0x18-byte CSaveModule at 0xC --
     * 0x407CF0, {RTTI, 0}, then the save module's slots at 0x407CF8 (the initialiser 0x400160 and
     * ~CUIVarManager 0x385CE0 store both) */
    ~CUIVarManager();
    void Serialize(int flags);
    void UnSerialize(const char *name, CRdrFile *file, int flags);

    /* the variables' slots, the tables and the save module (SOCOM 1's __sinit_fts_mission.cpp
     * builds theUIVarManager inline: fts_mission.cpp defines it, with the module's manager) */
    CUIVarManager();

    int m_currtab;                       // 0x28: the table strings are made in
    CUIVarSTable m_stab[2];              // 0x2C
    char m_packbuf[2][0x4000];           // 0x6C: the arrays packed, a buffer per table

    CUIVariable *Add(const char *name, _zrdr *value, UIVAR_LONGEVITY longevity);
    CUIVariable *Find(const char *name);
    CUIVariable *FindDormantVar(const char *name, _zrdr *value, UIVAR_LONGEVITY longevity);
    void CollectGarbage();
    void Reset();
};
extern CUIVarManager theUIVarManager;

#endif
