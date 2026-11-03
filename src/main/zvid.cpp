/* zvid.cpp, MAIN's copy (MAIN 0x181C20): MAIN links its own copy of the video unit
 * (src/gamez/zvid/zvid.cpp, OVL1 0x3B2C80-) as it does zsys_memory.cpp, and only zVid_Assert --
 * the allocators' check -- survives MAIN's dead-stripping. It sits right after zsys_memory.cpp's
 * copy and is called out of line by it: another unit (an inline in the same unit would be
 * auto-inlined). The words are OVL1's zVid_Assert's bar the link's addresses. The game's compiler
 * and flags (scdiff's default). */

#include "../gamez/zvid/zvid.h"

/* the machine's reset (OVL1 0x22ED10, not named yet) */
extern "C" void func_0022ED10();

/* a failed check stops the machine; SOCOM II's callers pass neither file nor line */
void zVid_Assert(bool ok, unsigned int color, char *file, int line)
{
    if (!ok)
        func_0022ED10();
}
