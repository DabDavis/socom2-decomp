/* hud_teamnames.cpp -- the team's names (OVL1 ...-0x221BF0): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_teamnames.cpp), before hud_posebitmap.cpp as there: Clear, Init and the
 * constructor at its end (0x220FB0, 0x221070, 0x221B10), then the inline CHudBrightText
 * constructor's copy (0x221BF0). Written: Clear and the constructor (with that copy); Init and the
 * two functions after it (0x2216D0, 0x221970) not yet. CTeamNames is 0xF30 bytes in SOCOM II
 * (SOCOM 1's 0x770). Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "../../gamez/ztwod/ztwod.h"
#include "hud_weaponsel.h"
#include "hud_mpinfobox.h"

/* the team's names: SOCOM 1's DWARF moved by SOCOM II's sizes, with a health bar and a bitmap for
 * each of four teammates (laid out from the constructor) */
class CTeamNames : public C2D {
public:
    CTeamNames();
    void Clear();

    unsigned char m_18[0x10];   // 0x18: zeroed by the constructor
    CHudBrightText m_teamname[4];   // 0x28 (SOCOM 1: 0x8c)
    C2DString m_order[4];   // 0x288 (SOCOM 1: 0x2ac)
    C2DBitmap m_bitmap1;   // 0x4f0
    C2DBitmap m_bitmap2;   // 0x570
    CHealthBar m_healthbar[4];   // 0x5f0
    C2DBitmap m_bitmaps[4];   // 0xd30
} __attribute__((aligned(16)));   // 0xf30 bytes

/* the arrays built, the first words zeroed (the unit's last function; the inline CHudBrightText
 * constructor's copy follows it, its address handed to the array constructor) */
CTeamNames::CTeamNames()
{
    memset(m_18, 0, sizeof(m_18));
}

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
