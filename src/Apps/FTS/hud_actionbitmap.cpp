/* hud_actionbitmap.cpp -- the HUD's action icon (OVL1 0x21D9D0-0x220FAF, between the weapon
 * selector and the team names as SOCOM 1's): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_actionbitmap.cpp). SOCOM 1 kept the radio bitmaps here too; SOCOM II renamed
 * them (CRadioIcon_Static and CRadioIcon_Dynamic in its RTTI) and their Ticks sit with the
 * network info box's (hud_mpinfobox.cpp, 0x239250 and 0x2393F0), so the static icon's Tick moved
 * there. Nothing of CZActionBitmap's own is written yet: UnInit (0x21DE50) wipes the unit's
 * static string (0x40D564) and turns off its 22 action textures (0x40C200, 0xE0 apart), which
 * the unit's initialiser (0x3FE550) builds. Defined in reverse of their image order (-inline
 * deferred). */

#include "../../gamez/ztwod/ztwod.h"
