/* sc: overlay=MAIN */
/* zvid_init.cpp, MAIN's copy (MAIN 0x181C20, initialiser 0x1D4B80): MAIN links its own copy of the
 * video set-up unit -- OVL1's zvid_init.cpp (src/gamez/znode/zvid_init.cpp, closed by zVid_Assert
 * 0x3B2C80: the weapons lane's round 18) -- as it does zsys_memory.cpp, and only zVid_Assert, the allocators' check,
 * survives MAIN's dead-stripping. It sits right after zsys_memory.cpp's copy and is called out of line
 * by it: another unit (an inline in the same unit would be auto-inlined). The words are OVL1's
 * zVid_Assert's bar the link's addresses. The game's compiler and flags (scdiff's default).
 *
 * THE UNIT (round 6; round 1 had called it zvid.cpp, after src/gamez/zvid/zvid.cpp, which defines
 * OVL1's zVid_Assert): its static initialiser is 0x1D4B80, <iostream>'s two objects alone -- word for
 * word OVL1's __sinit_zvid_init.cpp (0x404490) and SOCOM 1's (0x46E130), whose zvid_init.cpp holds
 * zVid_Assert too (0x307A40, inside 0x307790-0x308F3F). The initialisers' link order puts the unit
 * between the pool unit (0x1D4B00) and iostream.cpp (0x1D4BF0): of MAIN's units there only two are the
 * game's, this copy and zsys_memory.cpp's, and OVL1's zsys_memory.cpp builds no initialiser (none
 * between __sinit_zsys_sched.cpp 0x403420 and __sinit_zsys_init.cpp 0x403460; it includes no
 * <iostream>). Its two objects (0x1D57B8, 0x1D57D0, each after its destructor-chain record) are
 * MAIN's .bss right after the pool unit's six pools. The file name is what makes MW name the
 * initialiser __sinit_zvid_init.cpp (the 0x1D4B80 row is marked `ovl-copy:` of OVL1's). */

#include <iostream>

#include "../gamez/zvid/zvid.h"

/* the machine's reset (OVL1 0x22ED10, st_reboot.cpp's) */
void RebootAfterError();

/* a failed check stops the machine; SOCOM II's callers pass neither file nor line */
void zVid_Assert(bool ok, unsigned int color, char *file, int line)
{
    if (!ok)
        RebootAfterError();
}
