/* saferstr.h -- CSaferStr, the engine's string: SOCOM 1's zutil.h class (its DWARF: `class CSaferStr :
 * public basic_string`, nothing of its own; its one out-of-line member, operator=(const char *) at
 * 0x37F4E0, returns nothing), in SOCOM II over MSL 3.0's 12-byte basic_string (include/string). ONE
 * declaration for the shared headers that need it -- zentity.h (CEntity's ten SOP strings at 0xEC,
 * built and destroyed through __construct_array / __destroy_arr with this class's inline constructor
 * and implicit destructor, out of line in main.cpp: 0x1E7B60, 0x1E7A50) and zsav_lobby.h (the save
 * modules' names) -- so that a unit can include both (fts_mission.cpp through ai.h: the MSL headers
 * round 30; until then zentity.h carried an opaque 12-byte stand-in and the two could not meet).
 * The header's name is ours.
 *
 * Units that declare a CSaferStr of their own (fts_state.h, zui_spec.h, zonline.h, zanim_menu.h and
 * several .cpp files: each the members its functions need, some with SOCOM II's out-of-line copies'
 * extra shapes -- CSaferStr(const char *) 0x2C6060, the copy assignment 0x384E00) include neither
 * shared header; folding them in is a unit-by-unit measurement, not this header's.
 *
 * The constructor is inline and empty (the base's three words cleared); the destructor implicit (the
 * base's called with 0, a base subobject's); a null text assigned empties it (SOCOM 1's zutil.h
 * inline, lines 546-551: assign when non-null, else erase()) */
#ifndef SAFERSTR_H
#define SAFERSTR_H

#include <string>

class CSaferStr : public std::string {
public:
    CSaferStr() {}
    /* (the kit, round 62: PlayMessage 0x5E7960's two locals, each operator=(const char *) a level deeper: its dead erase()'s tag keeps a frame slot below the body's) */
    CSaferStr(const char *sz) { *this = sz; }
    void operator=(const char *sz)
    {
        if (sz)
            assign(sz);
        else
            erase();
    }
};

#endif
