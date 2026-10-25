/* hud_actionbitmap.cpp -- the HUD's action icon (OVL1 0x21D9D0-0x220FAF, between the weapon
 * selector and the team names as SOCOM 1's): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_actionbitmap.cpp). SOCOM 1 kept the radio bitmaps here too; SOCOM II renamed
 * them (CRadioIcon_Static and CRadioIcon_Dynamic in its RTTI) and their Ticks sit with the
 * network info box's (hud_mpinfobox.cpp, 0x239250 and 0x2393F0), so the static icon's Tick moved
 * there. Written: UnInit (0x21DE50), which wipes the unit's static string (0x40D564) and turns
 * off its 22 action entries' bitmaps (0x40C200, 0xE0 apart), which the unit's initialiser
 * (0x3FE550) builds. Defined in reverse of their image order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"

/* an action's entry (0xE0 bytes; only the bitmap at 0x10 placed: the name ours) */
struct ACTION_ENTRY {
    unsigned char _0[0x10];
    C2DBitmap m_bitmap;   // 0x10
    unsigned char _90[0xe0 - 0x90];
};

/* the unit's statics (bss; the initialiser builds them; no rows yet): the action entries and the
 * string the action's prompt is written in */
extern ACTION_ENTRY data_0040C200[22];
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
        data_0040C200[i].m_bitmap.SetTrans(0.0f);
        data_0040C200[i].m_bitmap.Off();
    }
}
