/* valve_main.cpp -- a valve's name and its ZAnim comparison command (OVL1 0x352960-0x35450F).
 * The file name is SOCOM 1's (its asserts name valve_main.cpp from CValve::FreeName on).
 * Defined in reverse of their image order (-inline deferred). */

#include <stdlib.h>
#include <string.h>

#include "valve.h"
#include "../zsav/zsave.h"

extern "C" long func_001914B8(const char *s);   /* newlib's atol (MAIN) */
char *zrdr_findstring(const _zrdr *rdr, const char *tag);
bool zrdr_findint(const _zrdr *rdr, const char *tag, int *out, int n);

/* the rdr lane's CRdrEditor (zrdr.h's, which redefines zanim.h's _zrdr): what the valves' save
 * calls of it, its size (0x4C bytes) */
class CRdrEditor {
public:
    CRdrEditor();
    ~CRdrEditor();
    bool push(const char *tag);
    bool pop();
    bool addint(const char *tag, int i);
    bool addSTRING(const char *tag, const char *s);

    char m_data[0x4C];
};

/* the persistent valves' part of a save (SOCOM 1's class; CSaveModule's slots) */
class CValveSaveManager : public CSaveModule {
public:
    void Serialize(int flags);
    void UnSerialize(const char *name, CRdrFile *file, int flags);
    static void SerializeValves(CRdrEditor *ed);
};

/* SOCOM 1's: every persistent valve written as a NAME and a VAL; SOCOM II's own part of
 * Serialize, which SOCOM 1 inlined (name ours). Pinned: Serialize calls it.
 * OPEN, 1 instruction long (0xE8 of 0xE4): RemoveTask(name)'s include/list shape -- across the
 * editor's calls the wrapper iterator's reference-returning * keeps the element's address (node +
 * 8) in a saved register of its own, where the target re-reads the node from the iterator in
 * memory (a by-value * would: include/list's note on GetWeapon/GetAmmo). A const_iterator walk
 * (by-value *) through a const reference does not inline the const begin() whole (0x120) */
#pragma auto_inline off
void CValveSaveManager::SerializeValves(CRdrEditor *ed)
{
    ed->push(0);
    std::list<CValve *>::iterator i, e = CValve::m_list.end();

    for (i = CValve::m_list.begin(); i != e; ++i) {
        if ((*i)->m_type == VALVE_PERSIST) {
            ed->push(0);
            ed->addSTRING("NAME", (*i)->m_name);
            ed->addint("VAL", (*i)->m_value);
            ed->pop();
        }
    }
    ed->pop();
}
#pragma auto_inline reset

/* the persistent valves saved under "PersistentValves" (SOCOM 1's) */
void CValveSaveManager::Serialize(int flags)
{
    CRdrEditor ed;

    SerializeValves(&ed);
    SaveRdr("PersistentValves", (CRdrFile *)&ed);
}

/* the saved valves made again, persistent, with their values (SOCOM 1's) */
void CValveSaveManager::UnSerialize(const char *name, CRdrFile *file, int flags)
{
    _zrdr *list = (_zrdr *)file;

    for (unsigned int i = 0; i < list->Count(); i++) {
        _zrdr *item = list->Item(i);
        int value;
        char *vname = zrdr_findstring(item, "NAME");

        zrdr_findint(item, "VAL", &value, 1);
        CValve::Create(vname, value, VALVE_PERSIST);
    }
}

/* a ZAnim argument instead of a valve: "INPUT_VALVE<n>" names argument n, anything else a valve,
 * created if new (SOCOM 1's; its debug walk of the argument table after the create is gone) */
CValve *VALVE_CMD_DATA::ParseValveName(const char *name, int &index)
{
    if (name && !strncmp(name, "INPUT_VALVE", 11)) {
        index = 0;
        if ((int)strlen(name) > 11)
            index = func_001914B8(name + 11);
        return 0;
    }
    return CValve::Create(name, (VALVE_TYPE)2);
}

/* SOCOM 1's: the name freed, the callbacks deleted, the list emptied.
 * OPEN, 4 instructions short (0x134 of 0x144): DeleteCallbacks' list-header shape -- the target
 * keeps &end_.next_ (this+16) in a saved register of its own from the walk on and reads both
 * clears' begin through it (clear()'s and the implicit ~CValveCBList's), where include/list reads
 * it off the end node's register; the walk, the deletes and the four this-tests match */
CValve::~CValve()
{
    FreeName();
    CValveCBList::iterator e = m_callbacks.end();

    for (CValveCBList::iterator i = m_callbacks.begin(); i != e; ++i)
        delete *i;
    m_callbacks.clear();
}

/* SOCOM 1's: the bit-fields set in the initialiser list (each read, masked and written back as a
 * word, as the target's are), the name copied */
CValve::CValve(const char *name, unsigned int value, VALVE_TYPE type)
    : m_name(0), m_value(value), m_type(type), m_name_pooled(0), m_pooled(0), m_socom2_bit26(0),
      m_unused(0)
{
    SetName(name);
}

/* the name taken as it is, owned by the caller's pool */
void CValve::AssignName(char *name)
{
    FreeName();
    m_name = name;
    m_name_pooled = 1;
}

void CValve::SetName(const char *name)
{
    FreeName();
    m_name = name ? _strdup(name, 0, 0) : 0;
    m_name_pooled = 0;
}

void CValve::FreeName()
{
    if (m_name_pooled)
        m_name = 0;
    else {
        if (m_name)
            _free(m_name, 0, 0);
        m_name = 0;
    }
    m_name_pooled = 0;
}
