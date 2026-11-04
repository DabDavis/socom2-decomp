/* zui_var.cpp -- the UI variables (OVL1 0x385610-0x386E1F): CUIVarManager (the variables, saved
 * as a CSaveModule), UIVarMixin's attach and detach, CUIVariable's assignment, formatting and its
 * watchers. SOCOM 1's demo holds the same run (UnSerialize ... ~CUIVarManager, DetachUIVar,
 * AttachUIVar, IsAttached, _Refresh, Format, operator=, UpdateDependents, Set, DetachAll, Destroy,
 * then the thunks) between zui_spec.cpp's tail and zui_cleanup.cpp; no assert names the file, so
 * zui_var.cpp is ours (the press beta's zui.cpp may be it).
 *
 * Round 19: the manager's Serialize and the variable's Format (0x3860F0, SOCOM 1's) byte-exact.
 * Round 20: RefreshUIVar 0x385E50 and _Refresh 0x385EA0 byte-exact (the latter once the integrator
 * named its two rdr callees). OPEN, not written yet: 0x3866F0; DetachUIVar, ~CUIVarManager, the revival 0x386B80, the watcher vector's iterator pieces
 * (0x386910-0x3869EF, 0x386AB0) and the thunks stand as generated drafts. Round 13: CUIVariable's
 * copy constructor 0x386BF0 byte-exact (zui.h's CUIVariable : _zrdr, UIVarMixin); Attach written,
 * open (below). */

#include <stdio.h>
#include <string.h>
#include "zui.h"

/* an rdr value packed into a buffer, its size returned (zrdr_pack, 0x334DB0); by address: two values the
 * same (0x335E20) and the variable revived as a new one (0x386B80) -- the drafts stand for them */
unsigned int zrdr_pack(_zrdr *rdr, void *buf, unsigned int size);   /* (zrdr_pack.cpp's) */
extern "C" int func_00335E20(_zrdr *a, _zrdr *b);

/* a value's strings made again in the table: a string's text re-created, an array's items in turn
 * (0x385EA0; SOCOM 1's _Refresh, at the same place in the run). The walk inlines itself twice and
 * calls itself below that, the accessors cut a level more in each copy -- zui.h's real accessor
 * heights (the UI lane's round 20): String() const over an inline IsString(), Count() over an inline
 * IsArray(), Item() over Count(). The outer body inlines them whole, the first copy calls IsString,
 * IsArray and Item out of line, the second String, Count and Item; the recursion stops at the
 * inline_max_size cut (flat accessors give the target's two copies with the wrong cuts, the tall ones
 * three copies with an `unsigned` counter -- an `int` counter, i*8 recomputed each pass, gives two).
 * The cast picks zui.h's const String() (the one over IsString, as zrdr.h has it): the non-const one
 * stays flat for LoadFromRdr, whose type test and String() share one load in the target.
 * Byte-exact since the integrator named the out-of-line IsString 0x32C650 and String 0x335700 (the
 * rdr units' rows) IsString__5_zrdrCFv and String__5_zrdrCFv, as zrdr.h declares them */
void _Refresh(_zrdr *rdr, CSTable *table)
{
    switch (rdr->type) {
    case 1:
    case 2:
        break;
    case 3:
        rdr->string = table->CreateString(((const _zrdr *)rdr)->String());
        break;
    case 4:
        for (int i = 0; i < rdr->Count(); i++)
            _Refresh(rdr->Item(i), table);
        break;
    }
}

/* a variable's name re-created in the table, then its value's strings (0x385E50; ours: SOCOM 1's
 * CollectGarbage inlined the two, SOCOM II's calls this out of line -- MW's refusal to inline the
 * recursive _Refresh propagates to it, the engine core's round 5) */
void RefreshUIVar(CUIVariable *var, CSTable *table)
{
    var->m_name = table->CreateString(var->m_name);
    _Refresh(var, table);
}
extern "C" void func_00386B80(CUIVariable *var, const char *name, _zrdr *value, UIVAR_LONGEVITY longevity);

/* (zanim.h has CValve whole, zrdr.h CRdrEditor: : CRdrFile, 0x4C bytes) */
enum VALVE_TYPE {};
class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);

    char *m_name;                 // 0x0
    unsigned int m_value : 16;    // 0x4
};
class CRdrEditor {
public:
    CRdrEditor();
    ~CRdrEditor();
    bool push(const char *tag);
    bool pop();
    bool addrdrnode(_zrdr *rdr);

    unsigned char _0[0x4C];
};

/* the collection in progress: a full table met while collecting gives up (ours) */
static int s_collecting;

/* the table is full: the garbage collected into the other table, and the string made there --
 * once; a full table met while collecting gives no string (SOCOM 1's CUIVarSTable) */
char *CUIVarSTable::OnTableFull(const char *str)
{
    char *s = 0;

    if (s_collecting++ == 0) {
        theUIVarManager.CollectGarbage();
        s = theUIVarManager.m_stab[theUIVarManager.m_currtab].CreateString(str);
    }
    s_collecting--;
    return s;
}

/* a copy of a variable: its name made again in the manager's current table and its longevity
 * carried; the value, the watchers and the callback are not (SOCOM II's: the vector insert's
 * copies, fts_mission.cpp's 0x2B1BD0, call it) */
CUIVariable::CUIVariable(const CUIVariable &v) : m_longevity(UIVL_DORMANT)
{
    m_attached_items.reserve(16);
    m_name = theUIVarManager.m_stab[theUIVarManager.m_currtab].CreateString(v.m_name);
    m_longevity = v.m_longevity;
}

/* the variable emptied: its watchers detached, its value freed unless packed, made an integer
 * (SOCOM 1's Destroy; SOCOM II calls DetachAll for its first loop). Reset calls it out of line:
 * pinned */
#pragma auto_inline off
void CUIVariable::Destroy()
{
    DetachAll();
    if (!packed)
        DeleteChildren();
    m_name = 0;
    type = 1;
    isclone = 0;
    packed = 0;
    m_longevity = UIVL_DORMANT;
}
#pragma auto_inline reset

/* the mixin made a watcher of the variable unless it is one, and told so (SOCOM II's: AttachUIVar
 * calls it once m_uivar is set).
 * OPEN, 0xE4 against 0xC0: the target inlines IsAttached ONE level -- its found flag (s0, set with
 * daddiu) and its by-value parameter's copy (76(sp), find's value) inline, the watcher vector's
 * begin(), end() twice, std::find and != called out of line (0x3869C0, 0x3869D0, 0x386AB0,
 * 0x386980: the drafts' rows) -- so its tree is taller than five there (begin() three high here);
 * here IsAttached inlines whole. The original's begin()/end() a level or two deeper (README "Level
 * 6, or 8?") would do it; AttachUIVar is pinned (Attach calls it out of line) */
void CUIVariable::Attach(UIVarMixin *mixin)
{
    if (!IsAttached(mixin))
        m_attached_items.push_back(mixin);
    mixin->AttachUIVar(this);
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

void CUIVariable::Set(_zrdr *value)
{
    if (func_00335E20(this, value))
        return;
    if (!packed)
        DeleteChildren();
    _zrdr *copy = new _zrdr(value, &theUIVarManager.m_stab[theUIVarManager.m_currtab]);
    *(_zrdr *)this = *copy;
    if (Type() == 4)
        packed = false;
    delete copy;
    /* the watchers told, the loop written out as SOCOM 1's Set has it (its demo calls data() in
     * place, never UpdateDependents) */
    std::vector<UIVarMixin *>::iterator i;

    for (i = m_attached_items.begin(); i != m_attached_items.end(); ++i)
        (*i)->UpdateUIVar(this);
}

/* a string value, made in the manager's current table; the same string again changes nothing
 * (SOCOM 1's; its watcher loop written out, as Set's) */
CUIVariable &CUIVariable::operator=(const char *str)
{
    if (!str)
        return *this;
    if (type == 3 && !strcmp(string, str))
        return *this;
    if (!packed)
        DeleteChildren();
    type = 3;
    string = theUIVarManager.m_stab[theUIVarManager.m_currtab].CreateString(str);
    std::vector<UIVarMixin *>::iterator i;

    for (i = m_attached_items.begin(); i != m_attached_items.end(); ++i)
        (*i)->UpdateUIVar(this);
    return *this;
}

void CUIVariable::UpdateDependents()
{
    std::vector<UIVarMixin *>::iterator i;

    for (i = m_attached_items.begin(); i != m_attached_items.end(); ++i)
        (*i)->UpdateUIVar(this);
}

/* a value written as text into a buffer of that size -- a number when 16 bytes are left, a string
 * when it fits with two to spare (an empty one for none), an array's items one after another when 6
 * are left; the buffer returned (SOCOM 1's Format: SOCOM II's array is its items in place, and no
 * separator follows each). `size > 15` branches on `slti at` (`size >= 16` computes v0 first); the
 * counter an int, so `i * 8` is recomputed each pass (unsigned, it is strength-reduced into a
 * fifth saved register) */
char *CUIVariable::Format(_zrdr *value, char *buf, int size)
{
    switch (value->type) {
    case 1:
        if (size > 15)
            sprintf(buf, "%d", value->integer);
        break;
    case 2:
        if (size > 15)
            sprintf(buf, "%f", value->real);
        break;
    case 3:
        if (value->string) {
            if (strlen(value->string) + 2 < size)
                sprintf(buf, "%s", value->string);
        } else
            sprintf(buf, "");
        break;
    case 4:
        if (size > 5) {
            for (int i = 0; i < value->Count(); i++) {
                int len = strlen(buf);

                Format(value->Item(i), buf + len, size - len);
            }
        }
        break;
    }
    return buf;
}

/* the mixin made a watcher of the variable, its old one detached first (SOCOM 1's AttachUIVar
 * pushed itself; SOCOM II's leaves that to CUIVariable::Attach, which calls it back out of line:
 * pinned) */
#pragma auto_inline off
void UIVarMixin::AttachUIVar(CUIVariable *var)
{
    if (m_uivar != var) {
        if (m_uivar)
            DetachUIVar(m_uivar);
        m_uivar = var;
        if (m_uivar)
            m_uivar->Attach(this);
    }
}
#pragma auto_inline reset

/* the temporary variables destroyed, the garbage collected (SOCOM 1's) */
void CUIVarManager::Reset()
{
    iterator i;

    for (i = begin(); i != end(); ++i) {
        if ((*i).m_longevity == UIVL_TEMPORARY)
            (*i).Destroy();
    }
    CollectGarbage();
}

/* the live variables' strings made again in the other table, the old table cleared, and their
 * arrays packed into the other table's buffer (SOCOM 1's; zrdr_pack, SOCOM 1's name, is 0x334DB0).
 *
 * OPEN, 3 instructions short (0x1C0 of 0x1CC), registers: the target saves six registers, keeping
 * `this` in s0, the old table in s1, the walk in s2, the packed size in s3, the room left in s4 and
 * the buffer in s5, and computes the packed flag's byte address (s2 + 1) into s1 in the delay slot
 * of the flag's test, reading and writing the flag through it after DeleteChildren; written so
 * (or with the locals declared at the top in that order), MW keeps `this` in s4, saves five and
 * addresses the flag at 1(s2) */
void CUIVarManager::CollectGarbage()
{
    int old = m_currtab;
    iterator i;

    m_currtab = !old;
    for (i = begin(); i != end(); ++i)
        RefreshUIVar(&*i, &m_stab[m_currtab]);
    m_stab[old].Clear();
    char *buf = m_packbuf[m_currtab];
    int left = 0x4000;
    for (i = begin(); i != end(); ++i) {
        if ((*i).type == 4) {
            int n = zrdr_pack(&*i, buf, left);

            if (!(*i).packed)
                (*i).DeleteChildren();
            left -= n;
            (*i).type = ((_zrdr *)buf)->type;
            (*i).length = ((_zrdr *)buf)->length;
            (*i).array = ((_zrdr *)buf)->array;
            buf += n;
            (*i).packed = true;
        }
    }
}

/* a free slot made the variable (SOCOM 1's; SOCOM II revives it out of line). Add calls it out of
 * line: pinned */
#pragma auto_inline off
CUIVariable *CUIVarManager::FindDormantVar(const char *name, _zrdr *value, UIVAR_LONGEVITY longevity)
{
    iterator i, e = end();

    for (i = begin(); i != e; ++i) {
        if ((*i).m_longevity == UIVL_DORMANT) {
            func_00386B80(&*i, name, value, longevity);
            return &*i;
        }
    }
    return 0;
}
#pragma auto_inline reset

/* the live variable of that name, or 0 (SOCOM 1's; SOCOM II compares without case) */
CUIVariable *CUIVarManager::Find(const char *name)
{
    CUIVariable *found = 0;
    iterator i;

    for (i = begin(); i != end(); ++i) {
        if ((*i).m_longevity && !strcasecmp(name, (*i).m_name)) {
            found = &*i;
            break;
        }
    }
    return found;
}

/* the variable of that name, made from a free slot if there is none; a value replaces the old
 * one unless the variable is permanent or persistent; the longevity only grows (SOCOM 1's;
 * UnSerialize calls it out of line: pinned) */
#pragma auto_inline off
CUIVariable *CUIVarManager::Add(const char *name, _zrdr *value, UIVAR_LONGEVITY longevity)
{
    CUIVariable *var = Find(name);

    if (!var)
        var = FindDormantVar(name, value, longevity);
    else if (value && var->m_longevity != UIVL_PERSISTENT && var->m_longevity != UIVL_PERMANENT)
        var->Set(value);
    if (longevity > var->m_longevity)
        var->m_longevity = longevity;
    return var;
}
#pragma auto_inline reset

/* each persistent variable saved, as an rdr of its own value -- the player's password and the
 * flag to store it only when the SecurePassword valve is on (SOCOM 1's, the password SOCOM II's).
 * The flag as `!(bool)value` gives the target's sltu/xori/andi; the name is a reference bound
 * between the longevity test and its own null test (`if (!name) continue;`), which puts its
 * address (s3) in the null test's delay slot -- the `a && b` test with the reference inside the
 * block left it two instructions late, the reference ahead of both tests is 13 off (round 19) */
void CUIVarManager::Serialize(int flags)
{
    bool nopassword = !(bool)CValve::Create("SecurePassword", (VALVE_TYPE)1)->m_value;
    iterator i;

    for (i = begin(); i != end(); ++i) {
        if ((*i).m_longevity == UIVL_PERSISTENT) {
            char *&name = (*i).m_name;
            if (!name) continue;

            if ((strcasecmp("PLAYERPASSWORD", name) && strcasecmp("STORE_PASSWORD", name)) || !nopassword) {
                CRdrEditor rdr;

                rdr.push(0);
                rdr.addrdrnode(&*i);
                rdr.pop();
                SaveRdr(name, (CRdrFile *)&rdr);
            }
        }
    }
}

/* a saved variable read back: persistent, its value the file's first item (SOCOM II's) */
void CUIVarManager::UnSerialize(const char *name, CRdrFile *file, int flags)
{
    _zrdr *value = ((_zrdr *)file)->Item(0);

    Add(name, value, UIVL_PERSISTENT)->Set(value);
}
