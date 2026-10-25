/* hud_teamnames.cpp -- the team's names (OVL1 ...-0x221BF0): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_teamnames.cpp), before hud_posebitmap.cpp as there: Clear, Init and the
 * constructor at its end (0x220FB0, 0x221070, 0x221B10). Only Clear so far; CTeamNames is 0xF30
 * bytes in SOCOM II (SOCOM 1's 0x770), its two string arrays moved to 0x28 and 0x288 and nothing
 * else placed. Defined in reverse of their image order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"

class CTeamNames : public C2D {
public:
    void Clear();

    unsigned char _18[0x28 - 0x18];
    C2DString m_teamname[4];   // 0x28 (SOCOM 1: CHudBrightText at 0x8c)
    C2DString m_order[4];   // 0x288 (SOCOM 1: 0x2ac)
    unsigned char _4e8[0xf30 - 0x4e8];
} __attribute__((aligned(16)));

/* two strings of the unit's own (OVL1 bss 0x40D7A0 and 0x40D850; no rows yet: SOCOM 1's Clear
 * empties two globals the same way) */
extern C2DString data_0040D7A0;
extern C2DString data_0040D850;

/* every name and order wiped, the two shared strings emptied, the children let go */
void CTeamNames::Clear()
{
    for (int index = 0; index < 4; index++) {
        m_teamname[index].ClearAndWipe();
        m_order[index].ClearAndWipe();
    }
    data_0040D7A0.SetString(0, true);
    data_0040D850.SetString(0, true);
    clear();
}
