/* zvid.cpp -- the video system (OVL1 0x3B2C80-...). SOCOM 1 links its zVid_ functions together
 * (zVid_ZTestOn .. zVid_Swap) in a unit without debug information; the file's name is ours. Defined
 * in reverse of their image order (-inline deferred). */

#include "zvid.h"

/* the machine's reset, out of the CD and the SIF (0x22ED10, not named yet) */
extern "C" void func_0022ED10();

/* a failed check stops the machine. SOCOM 1's prints the file and line in the colour given first;
 * SOCOM II's callers pass neither, and it only resets */
void zVid_Assert(bool ok, unsigned int color, char *file, int line)
{
    if (!ok)
        func_0022ED10();
}
