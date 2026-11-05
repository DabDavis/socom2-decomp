/* options.cpp -- the command line taken up and let go of (SOCOM II's; OVL1 0x1EABD0-0x1EB03F, the
 * unit between database.cpp and st_core.cpp; the name ours). main calls func_001EABD0 with argc and
 * argv first: two flags cleared and one set, the option map newed (util_options.cpp's COptionMap,
 * 16 bytes), "menu_state" registered with its handler and a string value, then the map, argc, argv
 * and "ftsxp.rdr" handed to the options' parser (util_options.cpp's SetOptionSource, which
 * CIOPManager::RebootIOP's ParseOptions runs); func_001EAC80, at the end, deletes the map. Defined
 * in reverse of their image order (-inline deferred); the map's teardown closes the unit (the tree's
 * destroy self-inlined twice, ~pair, the node allocator's deallocate, node_alloc(), the value
 * allocator's destroy and alloc(): 0x1EAD10-0x1EB03F, include/tree's shapes as they stand).
 *
 * This unit's view of the map is not util_options.cpp's (the MSL headers round 26, measured over
 * the nine rows): there the key is a plain std::string (its find<basic_string>, insert_node_at's
 * construct inlined whole), but here the pair's key is destroyed as a class derived from it -- a
 * second this-test, and ~basic_string called with 0, a base's destructor, where std::map<string,
 * int>'s ~pair passes -1 (0x2CE4F0) -- and the deleted map has four destructor levels above the
 * root's test where a class over std::map has three (an intermediate class: COptionList, ours).
 * Both are the shapes the image holds; what the original declared to give them is not known. */

#include "fts_state.h"
#include "../../gamez/zui/zui.h"

#include <map>
#include <string.h>

/* what an option is (util_options.cpp's): its handler and its flags, the low byte the value's
 * zrdr type */
typedef void (*OptionHandler)(const std::string &name, _zrdr *value);

struct OPTION_INFO {
    OPTION_INFO() : m_handler(0), m_flags(0) {}

    OptionHandler m_handler;   // 0x0
    union {
        unsigned int m_flags;   // 0x4
        unsigned char m_type;   // 0x4
    };
};   // 0x8 bytes

/* the options by name (ours: the level between the option map and std::map) */
class COptionList : public std::map<CSaferStr, OPTION_INFO> {
};

/* the options (util_options.cpp's: its constructor 0x3A2BE0 out of line) */
class COptionMap : public COptionList {
public:
    COptionMap();
};

void AddOption(COptionMap *map, const char *name, OptionHandler handler, unsigned int flags);
void SetOptionSource(COptionMap *map, int argc, char **argv, const char *rdrName);

extern "C" {
extern char data_00408568[0x40];   /* the menu state to start in */
extern char data_0049E2CE;
extern char data_0049E2CF;
extern char data_0045A1C8;
extern COptionMap *data_00408B80;   /* the options */
}

/* "menu_state": the menu state named on the command line (or in ftsxp.rdr) kept */
static void MenuStateOption(const std::string &, _zrdr *value)
{
    strncpy(data_00408568, value->type == 3 ? value->string : 0, 0x40);
}

/* the command line let go of (main's last call) */
extern "C" void func_001EAC80()
{
    delete data_00408B80;
}

/* the command line taken up (main's first call) */
extern "C" void func_001EABD0(int argc, char **argv)
{
    data_0049E2CE = 0;
    data_0049E2CF = 0;
    data_0045A1C8 = 1;
    data_00408B80 = new COptionMap;
    AddOption(data_00408B80, "menu_state", MenuStateOption, 3);
    SetOptionSource(data_00408B80, argc, argv, "ftsxp.rdr");
}
