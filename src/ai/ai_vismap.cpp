/* ai_vismap.cpp (the name SOCOM 1's): the AI's visibility maps -- in SOCOM II only the unit's
 * static initialiser is left, the empty 0x6687D0. SOCOM 1's __sinit_ai_vismap.cpp (the demo's
 * 0x46A420) follows __sinit_ai_maps.cpp (0x46A370) in the initialiser table, as SOCOM II's empty
 * one follows ai_maps.cpp's 0x668710 and precedes seal_ai.cpp's 0x6687E0. SOCOM 1's built two
 * CAiVisMapList statics (pointer lists, through the list deleter's constructor) and registered
 * their destructor (0x1A2B30); SOCOM II's map holds no CAiVisMap code, and its initialiser neither
 * constructs nor registers anything (the AI core round 25). */

#include "ai.h"

/* a static whose construction compiles to nothing and which has no destructor to register: what
 * the empty initialiser needs (a stand-in -- the type of SOCOM II's static, if the unit kept one
 * of SOCOM 1's, is not known; the name is SOCOM 1's class) */
class CAiVisMapList {
public:
    CAiVisMapList() {}
};

static CAiVisMapList s_visMaps;
