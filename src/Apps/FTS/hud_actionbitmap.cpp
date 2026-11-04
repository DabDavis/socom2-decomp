/* hud_actionbitmap.cpp -- the HUD's action icon (OVL1 0x21D9D0-0x220FAF, between the weapon
 * selector and the team names as SOCOM 1's): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_actionbitmap.cpp). SOCOM 1 kept the radio bitmaps here too; SOCOM II renamed
 * them (CRadioIcon_Static and CRadioIcon_Dynamic in its RTTI) and their Ticks sit with the
 * network info box's (hud_mpinfobox.cpp, 0x239250 and 0x2393F0), so the static icon's Tick moved
 * there. Written: UnInit (0x21DE50), which wipes the unit's static string (0x40D564) and turns
 * off its 22 action entries' bitmaps (0x40C200, 0xE0 apart), which the unit's initialiser
 * (0x3FE550) builds. Defined in reverse of their image order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"

#include <string.h>

/* an action's name before it has one (OVL1 .data 0x3DC680, a pointer variable: the entries'
 * constructor and 0x2209C0 load it; the name ours) */
static const char *c_szUnknownAction = "UNKNOWN";

/* an action's icon (SOCOM 1's CActionTxtr, its DWARF: 0xE0 bytes -- the texture, the action, the
 * bitmap at 0x10, the name at 0x90; SOCOM II's action a byte and its flag at 0xD1). Its inline
 * constructor's and implicit destructor's copies are this unit's tail (0x220CA0, 0x220C30), the
 * array's __construct_array and __arraydtor 0x220C10 calling them; the destructor byte-exact.
 * OPEN, the constructor (0x48 against 0x54): the target reaches the name's load through a folded
 * test -- `b` over a dead `b; move a1,zero` arm, a ternary `COND ? c_szUnknownAction : 0` whose
 * condition MW knew true -- where the plain copy loads the pointer straight. Tried: `true ?`,
 * `1 ?`, the pointer itself, `this` (a real beqz, 5 off), m_name, &m_2DBitmap, &c_szUnknownAction */
class CActionTxtr {
public:
    CActionTxtr()
    {
        m_texhandle = 0;
        m_action_id = 0;
        strcpy(m_name, c_szUnknownAction);
        m_active = false;
    }

    zdb::CTexHandle *m_texhandle;   // 0x0
    unsigned char m_action_id;   // 0x4
    C2DBitmap m_2DBitmap;   // 0x10
    char m_name[64];   // 0x90
    unsigned char m_d0;   // 0xd0
    bool m_active;   // 0xd1
};

/* the action icons (OVL1 bss 0x40C200, 22 of them: the unit's initialiser 0x3FE550 builds them;
 * SOCOM 1's machine held them by pointer -- the name ours) */
CActionTxtr g_arrActionTxtrs[22];

/* the string the action's prompt is written in (the texture machine's at 0x40D550 + 0x14, by
 * address until the machine is written) */
extern C2DString data_0040D564;

class CZActionBitmap : public C2DBitmap {
public:
    void UnInit();
};

/* the prompt let go of, every entry's bitmap clear and off. The string through a reference: on
 * the static itself MW calls C2DString's functions directly, where the target goes through the
 * vtable; the array's elements it dispatches as they stand (a pointer local per element costs a
 * saved register the target does not have) */
void CZActionBitmap::UnInit()
{
    C2DString &text = data_0040D564;

    text.ClearAndWipe();
    for (unsigned int i = 0; i < 22; i++) {
        g_arrActionTxtrs[i].m_2DBitmap.SetTrans(0.0f);
        g_arrActionTxtrs[i].m_2DBitmap.Off();
    }
}
