/* util_loc.cpp -- the localised string tables (OVL1 0x3A3100-0x3A59BF): LOC.rdr's tables per language
 * (RUN\LOCALE\<language>, DATA\COMMON\ZRDR\LOCRDR\<language>\<table>LOC.rdr), the language choice
 * (LanguageChoice, from the system's when it is unset) and the lookups by id and by name. SOCOM 1
 * has none of it: the file's name and every name here are ours. Defined in reverse of their image
 * order (-inline deferred); the template tail follows the unit's own code: the entries' sort
 * (std::sort, include/algorithm: 0x3A4560, its __sort132 0x3A5500 and __selection_sort 0x3A57E0),
 * the entries' vector (insert 0x3A4D10, reserve 0x3A5220, the deleter's clear 0x3A5400 and
 * destructor 0x3A5450).
 *
 * The manager of the tables (CStringTables, hud_main.cpp's name for it) is the font unit's global
 * at 0x49E9D0, reached through 0x361BC0; it is a vector of the tables with the entry found last. */

#include <vector>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../zui/zui.h"
#include "../zsnd/zsnd_subtitles.h"

class CRdrFile : public _zrdr {};
CRdrFile *zrdr_read(const char *name, const char *path, int flags);
void zrdr_free(CRdrFile *file);
bool zrdr_findint(const _zrdr *rdr, const char *tag, int *out, int n);
bool zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n);
char *zrdr_findstring(const _zrdr *rdr, const char *tag);

namespace zar {
class CZAR {
public:
    void ReOpen(int mode, unsigned int flags);
};
}

/* the archives (zrdr.h's CRdrArchive, only what the tables use) */
class CRdrArchive : public zar::CZAR {
public:
    static CRdrArchive *AddArchive(const char *name, const char *path);
    static bool RemoveArchive(const char *name, const char *path);
};

/* the valves, as zanim.h declares them (only what the language uses) */
enum VALVE_TYPE {};
enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};

class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);
    void MakeCallbacks(VALVE_STATE state);

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
};

/* CValve::SetValue, inlined in both builds (as zanim_menu.h's SetValveValue) */
inline void SetValveValue(CValve *valve, int value)
{
    VALVE_STATE state = VALVE_SAME;

    if (valve->m_value != value)
        state = value == 0 ? VALVE_OFF : VALVE_ON;
    valve->m_value = value;
    if (state != VALVE_SAME)
        valve->MakeCallbacks(state);
}

/* the font unit's: the manager of the tables (0x49E9D0), and a count of the strings' changes */
class CStringTables *GetStringTables();
void IncLanguageVersion();
extern "C" {
int sceScfGetLanguage();
}
/* the archives' open mode (zrdr_archive.cpp's?) */
extern int data_003F7020;

/* the languages, as the table of their names lists them (the names ours): zsnd_subtitles.h's LANGUAGE,
 * the subtitle table's (the two copies were the same enumerators; one now, the unit unchanged) */

/* the languages' directory names, and whether the tables come from a ZAR per language */
static const char *s_languageNames[LANG_COUNT] = {
    "STATES", "UK", "FRANCE", "GERMANY", "SPAIN", "ITALY", "JAPAN", "KOREA", "TEST"
};
static bool s_useZar = true;

/* the sound system's language: CSnd's subtitle table (zsnd_csnd.cpp's global 0x48DC30), its
 * Load(LANGUAGE) 0x34D190 the language's VAG index (the world's round 15 named it) */
extern CSubtitleTable data_0048DC30;

/* the language chosen (LANG_UNSET until the system's is taken) */
static LANGUAGE s_language;

/* a localised string: its id, its name (MSG_ID) and its text (LOCALIZED) -- 12 bytes; the
 * destructor clears the id (the deleter's destroy loop and every temporary's end store a zero
 * halfword) */
struct LOCENTRY {
    LOCENTRY(unsigned short id, char *msgid, char *text) : m_id(id), m_msgid(msgid), m_text(text) {}
    ~LOCENTRY() { m_id = 0; }

    unsigned short m_id;   // 0x0
    char *m_msgid;   // 0x4: MSG_ID
    char *m_text;   // 0x8: LOCALIZED
};   // 0xc bytes

/* the entries' order, by id (std::sort's comparator, empty, by value) */
struct LocEntryLess {
    LocEntryLess() {}
    bool operator()(const LOCENTRY &a, const LOCENTRY &b) const { return a.m_id < b.m_id; }
};

/* a name is a message id: it starts with an underscore */
static inline bool IsMsgId(const char *name)
{
    return name && *name == '_';
}

/* a name's bucket: its first and middle characters summed. The sign test an if/else assigning h (UI
 * round 25): the ternary leaves the join unpadded with the second modulo's shift in the first arm's
 * `b` delay slot, the target pads the join and leaves the slot empty (CLocTable::Find(name)) */
static inline int HashName(const char *name)
{
    int h = name[0] + name[strlen(name) / 2];

    if (h < 0)
        h = -h % 17;
    else
        h = h % 17;
    return h;
}

/* a table's entries (a plain vector: Load inlines the table's constructor whole, which a vector
 * subclass -- Unload's three this-tests -- makes six levels high; see Unload) */
typedef std::vector<LOCENTRY> CLocEntries;

/* one table of localised strings: the largest id, the strings, the entries sorted by id, the
 * table's name, the entry found last and the entries hashed by name */
class CLocTable {
public:
    CLocTable(_zrdr *rdr) : m_strings(128000, rdr->Item(0)->Count() * 2 + 1)
    {
        m_entries.reserve(1);
        m_maxId = -1;
        m_last = 0;
    }
    LOCENTRY *Find(int id);
    LOCENTRY *Find(const char *name);
    void Read(const char *name, _zrdr *rdr);
    /* an entry added while there is room (the strings made in the table) */
    void Add(int id, const char *msgid, const char *text)
    {
        if (m_entries.size() < m_entries.capacity()) {
            int max = m_maxId;

            if (max < id)
                max = id;
            m_maxId = max;
            m_entries.push_back(LOCENTRY(id, m_strings.CreateString(msgid), m_strings.CreateString(text)));
        }
    }

    int m_maxId;   // 0x0
    CSTable m_strings;   // 0x4
    CLocEntries m_entries;   // 0x24
    char m_name[0x40];   // 0x30
    LOCENTRY *m_last;   // 0x70: the entry found last
    std::vector<LOCENTRY *> m_hash[17];   // 0x74: the entries by name
};   // 0x140 bytes

/* the tables (the font unit's global): a vector of them and the entry found last */
class CStringTables : public std::vector<CLocTable *> {
public:
    LOCENTRY *Find(const char *name, const char *table);
    LOCENTRY *Find(int id, const char *table);
    void Reload(LANGUAGE lang);
    void Unload(const char *name);
    void Load(const char *name, LANGUAGE lang);
    unsigned short GetId(const char *name, const char *table);
    const char *GetString(int id, const char *table);
    /* a table by name, or 0 (the walk's end through Last(): see there) */
    CLocTable *FindTable(const char *name)
    {
        if (name) {
            for (iterator i = begin(); i != Last(); ++i) {
                if (!strcmp(name, (*i)->m_name))
                    return *i;
            }
        }
        return 0;
    }
    /* the end built straight from the storage (the online lobby's round 20 Last()): through
     * include/vector's end() -- four inline levels high since the MSL headers' round 24 -- FindTable's
     * `i != end()` is six deep where Find(int, const char *) expands it, so at -inline level=5 MW keeps
     * end()'s body and calls the raw end and __iter; through Last() the walk inlines whole at 5, as the
     * target's (data + size * 4 at each test), and the unit's object is the depth-6 stand-in's
     * (`#pragma inline_depth(6)`, which it no longer carries) */
    iterator Last() { return iterator((elem_t *)this->data_ + this->size_); }

    LOCENTRY *m_last;   // 0xc: the entry found last
};

void SetLanguage(LANGUAGE lang);
LANGUAGE LanguageFromName(const char *name);

/* the language chosen, from the system's when it is unset */
LANGUAGE GetLanguage()
{
    if (s_language == LANG_UNSET)
        SetLanguage(LanguageFromName("SYSTEM"));
    return s_language;
}

/* the language changed: the tables reloaded, the strings' change counted, the sound told, and
 * LanguageChoice set (GetLanguage calls it out of line: pinned) */
#pragma auto_inline off
void SetLanguage(LANGUAGE lang)
{
    if (lang != s_language) {
        s_language = lang;
        GetStringTables()->Reload(lang);
        IncLanguageVersion();
        data_0048DC30.Load(lang);
    }
    SetValveValue(CValve::Create("LanguageChoice", (VALVE_TYPE)2), lang);
}
#pragma auto_inline reset

/* the entry with this id: the one found last, the first when the id is not past it, else the
 * entries walked (the walk's find kept).
 * OPEN, 2 instructions long (0xC0 of 0xB8): the target loads the data pointer into v0 for the first
 * entry's test and the `return first` path, and walks a copy of it (`move a2,v0` in the size test's
 * delay slot); here the pointer lands in a2 and the first-entry return moves it to v0. The block
 * order (the first-entry test branching straight to the shared return, i = 0 in its delay slot)
 * needed the walk nested in the test with i declared first; an unsigned walk index read through an
 * operator[] keeps the per-pass element address; `id == x` puts the parameter first in each bne.
 * Tried: a pointer local for the first entry (before or after i), the test inverted, `id <=`. */
LOCENTRY *CLocTable::Find(int id)
{
    if (id <= 0 || id > m_maxId)
        return 0;
    if (m_last && id == m_last->m_id)
        return m_last;
    int i = 0;

    if (m_entries[0].m_id < id) {
        for (; i < m_entries.size(); i++) {
            LOCENTRY *e = &m_entries[i];

            if (id == e->m_id) {
                m_last = e;
                return e;
            }
        }
        return 0;
    }
    return &m_entries[0];
}

/* the entry with this name: the one found last, else its bucket walked. The empty test `*name == 0`
 * (UI round 25): the target tests the '_' on the byte that test loaded (`xori v0,v1,0x5f`) -- `!*name`
 * makes IsMsgId re-extend it (dsll32/dsra32), as the target's CStringTables::Find(name) does (the
 * UI's round 21 rule for a byte: `!x` then `x == k` re-extends, `x == 0` reuses it as loaded). The
 * bucket walk is `m_hash[h].end()` re-read each pass (the size's and the data's addresses in two
 * saved registers) and the hash's modulo in each arm of the sign test (two divs), both as the
 * target's */
LOCENTRY *CLocTable::Find(const char *name)
{
    if (!name || *name == 0)
        return 0;
    if (!IsMsgId(name))
        return 0;
    if (m_last && !strcmp(name, m_last->m_msgid))
        return m_last;
    int h = HashName(name);

    for (std::vector<LOCENTRY *>::iterator i = m_hash[h].begin(); i != m_hash[h].end(); ++i) {
        LOCENTRY *e = *i;

        if (!strcmp(name, e->m_msgid)) {
            m_last = e;
            return e;
        }
    }
    return 0;
}

/* a language by its name, the system's for none or SYSTEM, else the United States */
LANGUAGE LanguageFromName(const char *name)
{
    if (!name || !strcasecmp(name, "SYSTEM")) {
        switch (sceScfGetLanguage()) {
        case 0:
            return LANG_UK;
        case 1:
            return LANG_UK;
        case 2:
            return LANG_FRANCE;
        case 3:
            return LANG_SPAIN;
        case 4:
            return LANG_GERMANY;
        case 5:
            return LANG_ITALY;
        case 6:
            return LANG_UK;
        case 7:
            return LANG_UK;
        default:
            return LANG_UK;
        }
    }
    for (int i = 0; i < LANG_COUNT; i++) {
        if (!strcasecmp(name, s_languageNames[i]))
            return (LANGUAGE)i;
    }
    return LANG_UK;
}

/* the table read from its rdr: the null entry, then each entry (ID, MSG_ID, LOCALIZED, SCALE,
 * OFFSET), sorted by id and hashed by name, the table named, and the strings moved into a buffer
 * of their own */
/* OPEN, 1 instruction (the target's size since UI round 25, every other word the target's but the
 * three alignment nops that follow the shift): the null entry's `max < 0` folds to bgez here where
 * the target keeps `slt at,v0,zero; beqz at` -- the second Add's shape with the id's register as
 * $zero, as if the 0 were a variable when the compare was lowered (the max through a local, a
 * reference- or value-returning helper, a `const int &` parameter: no change or worse; round 25: the
 * id an `unsigned short`, a zero local, the loop's address-taken id declared first -- its store then
 * kept --, or n, i or j as the null entry's id with a second definition later: no change). The
 * relocation test reads the table's bytes used (m_bytes, +0x18: round 25), not its size. Item(j)
 * with an int j, the MSG_ID found twice and the OFFSET pair's byte-wise `= {0}` are the target's */
void CLocTable::Read(const char *name, _zrdr *rdr)
{
    Add(0, "", "");
    if (rdr) {
        _zrdr *list = rdr->Item(0);
        int n = list->Count();

        m_entries.reserve(n + 1);
        for (int i = 0; i < 17; i++)
            m_hash[i].reserve(n + 1);
        m_strings.reserve(n * 2 + 1);
        for (int j = 0; j < n; j++) {
            _zrdr *item = list->Item(j);
            int id = 0;

            zrdr_findint(item, "ID", &id, 1);
            const char *msgid = 0;

            if (zrdr_findstring(item, "MSG_ID"))
                msgid = zrdr_findstring(item, "MSG_ID");
            const char *text = zrdr_findstring(item, "LOCALIZED");
            float scale = 1.0f;

            zrdr_findreal(item, "SCALE", &scale, 1);
            int offset[2] = {0};

            zrdr_findint(item, "OFFSET", offset, 2);
            Add(id, msgid, text);
        }
    }
    std::sort(m_entries.begin(), m_entries.end(), LocEntryLess());
    for (std::vector<LOCENTRY>::iterator e = m_entries.begin(); e != m_entries.end(); ++e) {
        LOCENTRY *p = &*e;

        m_hash[HashName(p->m_msgid)].push_back(p);
    }
    sprintf(m_name, "%s", name);
    if (m_strings.m_bytes < 0x5c00) {
        unsigned int size = m_strings.m_bytes < 0x800 ? 0x800 : 0x5c00;
        int delta = m_strings.Relocate(_malloc(size, 0, 0), size, true);

        for (std::vector<LOCENTRY>::iterator e = m_entries.begin(); e != m_entries.end(); ++e) {
            e->m_msgid += delta;
            e->m_text += delta;
        }
    }
}

/* a table's file read: <name>LOC.rdr from <name>LOC.zar under RUN\LOCALE\<language>, or under the
 * LOCRDR path */
static inline CRdrFile *ReadLocFile(const char *name, LANGUAGE lang)
{
    char path[256];
    char zar[256];
    char dir[256];
    CRdrFile *file;

    if (s_useZar) {
        sprintf(path, "%sLOC.rdr", name);
        sprintf(zar, "%sLOC.zar", name);
        sprintf(dir, "RUN\\LOCALE\\%s", s_languageNames[lang]);
        CRdrArchive::AddArchive(zar, dir)->ReOpen(data_003F7020, 1);
        file = zrdr_read(path, 0, 4);
    } else {
        sprintf(path, "DATA\\COMMON\\ZRDR\\LOCRDR\\%s\\%sLOC.rdr", s_languageNames[lang], name);
        file = zrdr_read(path, 0, 0);
    }
    return file;
}

/* a table's ZAR let go of again */
static inline void CloseLocZar(const char *name, LANGUAGE lang)
{
    if (s_useZar) {
        char zar[256];
        char dir[256];

        sprintf(zar, "%sLOC.zar", name);
        sprintf(dir, "RUN\\LOCALE\\%s", s_languageNames[lang]);
        CRdrArchive::RemoveArchive(zar, dir);
    }
}

/* a table loaded (unless it is already): read from its language's ZAR or file, and added */
void CStringTables::Load(const char *name, LANGUAGE lang)
{
    if (name) {
        for (iterator i = begin(); i != end(); ++i) {
            if (!strcmp(name, (*i)->m_name))
                return;
        }
        CRdrFile *file = ReadLocFile(name, lang);

        if (file) {
            CLocTable *t = new CLocTable(file);

            t->Read(name, file);
            push_back(t);
        }
        CloseLocZar(name, lang);
        if (file)
            zrdr_free(file);
    }
}

/* a table unloaded by name.
 * OPEN, 2 instructions short: the target's inlined ~CLocTable tests the entries' address three times
 * before the deleter's destructor (a vector subclass's implicit destructor -- matched so), but a
 * subclass makes CLocTable() six levels high and Load (0x2C0 bytes, byte-exact with a plain vector)
 * then calls the members' constructors out of line; include/vector's class-vector constructor (4
 * levels) leaves no room for both at -inline level=5. The walk -- `next` its own variable, stepped
 * before the compare and set from erase -- is the target's */
void CStringTables::Unload(const char *name)
{
    if (name) {
        iterator next = begin(), i = next;

        while (i != end()) {
            ++next;
            if (!strcmp((*i)->m_name, name)) {
                CLocTable *t = *i;

                next = erase(i);
                delete t;
            }
            i = next;
        }
        m_last = 0;
    }
}

/* the id of the string with this name, or 0 (the font unit's 0x361B20 asks the manager, no table) */
unsigned short CStringTables::GetId(const char *name, const char *table)
{
    unsigned short id = 0;
    LOCENTRY *e = Find(name, table);

    if (e)
        id = e->m_id;
    return id;
}

/* the text of the string with this id, or "(UNLOCALIZED)" (the font unit's 0x361B40 asks the
 * manager, no table) */
const char *CStringTables::GetString(int id, const char *table)
{
    const char *s = "(UNLOCALIZED)";
    LOCENTRY *e = Find(id, table);

    if (e)
        s = e->m_text;
    return s;
}

/* every table read again in a language */
void CStringTables::Reload(LANGUAGE lang)
{
    for (iterator i = begin(); i != end(); ++i) {
        CLocTable *t = *i;

        t->m_entries.clear();
        t->m_strings.Clear();
        for (int j = 0; j < 17; j++)
            t->m_hash[j].clear();
        t->m_last = 0;
        CRdrFile *file = ReadLocFile((*i)->m_name, lang);

        if (file)
            (*i)->Read((*i)->m_name, file);
        CloseLocZar((*i)->m_name, lang);
        if (file)
            zrdr_free(file);
    }
    m_last = 0;
}

/* the entry with this id: the one found last, else in the named table or in each */
LOCENTRY *CStringTables::Find(int id, const char *table)
{
    LOCENTRY *e = 0;

    if (m_last && id == m_last->m_id)
        return m_last;
    if (table) {
        CLocTable *t = FindTable(table);

        if (t) {
            LOCENTRY *f = t->Find(id);

            if (f)
                m_last = f;
            return f;
        }
        return 0;
    }
    for (iterator i = begin(); i != end(); ++i) {
        if ((e = (*i)->Find(id)) != 0) {
            m_last = e;
            return e;
        }
    }
    return e;
}

/* the entry with this name: the one found last, else in the named table or in each. The last walk
 * through a named pointer declared ahead of e, compared through the iterators (UI round 25: a
 * NUMBERING stand-in): MW colours this function's over-K variables highest-index first (the game
 * states' round 34), and the walk's scalarised iterator -- a temporary, numbered after every named
 * local -- took s0 before e could, so e needed a register of its own (one saved register more, 3
 * instructions long). A named pointer declared first is numbered below e: e is coloured first and
 * shares s0 with FindTable's iterator, the parameter table s1 with the walk, as the target's (read off
 * MW's colouring with the igdump tools). Compared raw (`p != end`) MW branches straight on the
 * pointers, 6 instructions short; Find(id, table), where e crosses no call, keeps the plain walk */
LOCENTRY *CStringTables::Find(const char *name, const char *table)
{
    CLocTable **p;
    LOCENTRY *e = 0;

    if (!name || !*name)
        return 0;
    if (!IsMsgId(name))
        return 0;
    if (m_last && !strcmp(name, m_last->m_msgid))
        return m_last;
    if (table) {
        CLocTable *t = FindTable(table);

        if (t) {
            LOCENTRY *f = t->Find(name);

            if (f)
                m_last = f;
            return f;
        }
        return 0;
    }
    for (p = begin().m_p; iterator(p) != end(); ++p) {
        if ((e = (*p)->Find(name)) != 0) {
            m_last = e;
            return e;
        }
    }
    return e;
}
