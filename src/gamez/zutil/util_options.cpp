/* util_options.cpp -- the start-up options (OVL1 0x3A2510-0x3A30FF): a map of option names to
 * handlers, filled by main (0x1EABD0 registers "menu_state" and hands over argc, argv and
 * "ftsxp.rdr"), then run against the command line and an rdr file of options when the IOP is
 * rebooted (CIOPManager::RebootIOP calls ParseOptions twice). A word on the command line (its
 * leading dashes dropped) names an option, "help", or an rdr file whose first list and RELEASE list
 * hold more; an option's value follows it, typed by the option's low byte (a zrdr type, 0 for none),
 * and the option is taken only where its flags allow (0x100, 0x200). SOCOM 1 has none of it: every
 * name here is ours. Defined in reverse of their image order (-inline deferred); the map's template
 * tail follows (find_or_insert, find, the tree's constructor, insert_node_at: std::map keyed by
 * std::string, the MSL headers' round 5). */

#include <string>
#include <map>
#include <stdlib.h>
#include <string.h>
#include "../zui/zui.h"

class CRdrFile;

/* the rdr reader's (zrdr.h's declarations; zui.h's _zrdr, whose default constructor the command
 * line's value needs) */
enum {
    ZRDR_INTEGER = 1,
    ZRDR_REAL = 2,
    ZRDR_STRING = 3,
    ZRDR_ARRAY = 4
};
_zrdr *zrdr_read(const char *name, const char *path, int flags);
int zrdr_free(CRdrFile *file);
_zrdr *zrdr_flat_findtag_startidx(const _zrdr *rdr, const char *tag, int start);
bool zrdr_isitemtype(const _zrdr *rdr, int index, int type);

/* an option's handler: the option's name and its value (none for an untyped option) */
typedef void (*OptionHandler)(const std::string &name, _zrdr *value);

/* what an option is: its handler and its flags, the low byte the value's zrdr type (ours) */
struct OPTION_INFO {
    OPTION_INFO() : m_handler(0), m_flags(0) {}

    OptionHandler m_handler;   // 0x0
    union {
        unsigned int m_flags;   // 0x4: 0x100, 0x200 where the option is taken
        unsigned char m_type;   // 0x4: the value's zrdr type, 0 for none
    };
};   // 0x8 bytes

/* a name made for a lookup: assigned, never null-tested (ours; destroyed as a base, with 0) */
class COptionName : public std::string {
public:
    COptionName(const char *s) { assign(s); }
};

/* the map's allocator built with no code (MSL's allocator() is inline; include/memory keeps it out
 * of class for the deque's out-of-line 0x2CEE30, so this instance is specialised inline here: the
 * target's map() passes a bare stack slot) */
template <>
inline std::allocator<std::pair<const std::string, OPTION_INFO> >::allocator() throw()
{
}

/* the options (ours: main news 16 bytes and calls the constructor, 0x3A2BE0) */
class COptionMap : public std::map<std::string, OPTION_INFO> {
public:
    COptionMap();
};

/* a value's type through an accessor (ours: the test then shares its load with String()) */
static inline unsigned int TypeOf(const _zrdr *z)
{
    return z->type;
}

/* a string value's text, or 0 (ours, as zrdr.h's const String()) */
static inline char *StringOf(const _zrdr *z)
{
    return z->type == ZRDR_STRING ? z->string : 0;
}

static int s_argc;   // 0x4A3FF8
static char **s_argv;   // 0x4A4000
static const char *s_rdrName;   // 0x4A4008
static COptionMap *s_options;   // 0x4A4010

int ParseOptionList(COptionMap *map, _zrdr *list);
int ParseCommandLine(COptionMap *map, int argc, char **argv, unsigned int mask);

/* the command line, the rdr file and the map handed over (main's) */
void SetOptionSource(COptionMap *map, int argc, char **argv, const char *rdrName)
{
    s_options = map;
    s_argc = argc;
    s_argv = argv;
    s_rdrName = rdrName;
}

/* an rdr file of options: its first list, then its RELEASE list (ours; inlined into both callers) */
static inline int ParseOptionFile(COptionMap *map, const char *name)
{
    _zrdr *rdr = zrdr_read(name, 0, 1);
    int n = 0;

    if (rdr) {
        n = ParseOptionList(map, rdr->Item(0));
        n += ParseOptionList(map, zrdr_flat_findtag_startidx(rdr, "RELEASE", 1));
        zrdr_free((CRdrFile *)rdr);
    }
    return n;
}

/* the options run, once: with 0x100 the command line alone (and kept for the next call); else the
 * command line, or the rdr file when it took nothing, and the source let go */
void ParseOptions(unsigned int flags)
{
    if (!s_options)
        return;
    if (flags & 0x100) {
        ParseCommandLine(s_options, s_argc, s_argv, flags);
        return;
    }
    if (!ParseCommandLine(s_options, s_argc, s_argv, flags))
        ParseOptionFile(s_options, s_rdrName);
    s_options = 0;
    s_argc = 0;
    s_argv = 0;
    s_rdrName = 0;
}

COptionMap::COptionMap()
{
}

/* "help": the options walked (whatever printed them compiled out; called out of line: pinned) */
#pragma auto_inline off
void ListOptions(COptionMap *map)
{
    for (COptionMap::iterator i = map->begin(); i != map->end(); ++i)
        ;
}
#pragma auto_inline reset

/* an option registered: accepted with 0x200 unless 0x100 says otherwise */
void AddOption(COptionMap *map, const char *name, OptionHandler handler, unsigned int flags)
{
    if (!(flags & 0x100))
        flags |= 0x200;
    COptionName key(name);
    OPTION_INFO &info = (*map)[key];

    info.m_handler = handler;
    info.m_flags = flags;
}

/* an rdr list of options: each name, then its value when the option has one (a list holding a
 * value of the option's type, or any list for an array option); the options taken counted */
int ParseOptionList(COptionMap *map, _zrdr *list)
{
    int count = 0;

    if (list) {
        int n = list->Count();

        for (int i = 0; i < n; i++) {
            _zrdr *item = list->Item(i);

            if (TypeOf(item) != ZRDR_STRING)
                continue;
            const char *name = StringOf(item);
            COptionMap::iterator it = map->find(COptionName(name));

            if (it == map->end())
                continue;
            OPTION_INFO &info = it->second;
            int type = info.m_type;

            if (!type) {
                info.m_handler(COptionName(name), 0);
                count++;
                continue;
            }
            _zrdr *value = list->Item(++i);

            switch (type) {
            case ZRDR_INTEGER:
            case ZRDR_STRING:
            case ZRDR_REAL:
                if (zrdr_isitemtype(value, 0, type)) {
                    info.m_handler(it->first, value->Item(0));
                    count++;
                }
                break;
            case ZRDR_ARRAY:
                info.m_handler(it->first, value);
                count++;
                break;
            }
        }
    }
    return count;
}

/* the command line: each word, its dashes dropped, an option (with the next word its value, read
 * as the option's type), "help", or an rdr file of options; the words taken counted */
int ParseCommandLine(COptionMap *map, int argc, char **argv, unsigned int mask)
{
    _zrdr value;
    int count = 0;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        while (*arg == '-')
            arg++;
        COptionMap::iterator it = map->find(COptionName(arg));

        if (it == map->end()) {
            if (!strcmp(arg, "help"))
                ListOptions(map);
            else if (ParseOptionFile(map, arg))
                count++;
            continue;
        }
        OPTION_INFO &info = it->second;

        if (!(info.m_flags & mask))
            continue;
        int type = info.m_type;

        if (!type) {
            info.m_handler(it->first, 0);
            count++;
            continue;
        }
        const char *word;
        bool ok = false;

        word = argv[++i];

        switch (type) {
        case ZRDR_STRING:
            value.string = (char *)word;
            value.type = ZRDR_STRING;
            ok = true;
            break;
        case ZRDR_INTEGER:
            value.integer = atol(word);
            value.type = ZRDR_INTEGER;
            ok = true;
            break;
        case ZRDR_REAL:
            value.real = atof(word);
            value.type = ZRDR_REAL;
            ok = true;
            break;
        case ZRDR_ARRAY:
            value.type = 0;
            break;
        }
        if (ok) {
            info.m_handler(it->first, &value);
            count++;
        }
    }
    return count;
}
