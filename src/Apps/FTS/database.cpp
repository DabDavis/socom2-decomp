/* database.cpp -- the game's database: its archive and the world's load from it (SOCOM 1's
 * Apps/FTS/database.cpp, between main.cpp and st_core.cpp; OVL1 0x1E8FE0-0x1EB03F, after ezctl.c's
 * dmaIOP). Its initialiser __sinit_database.cpp 0x3FD710 builds <iostream>'s pair and then
 * `archive`, a zar::CZAR with no name and no I/O, registering ~CZAR -- SOCOM 1's built its
 * MultiplayerMsgStrings table as well; the archive is read only by this unit's functions
 * (0x1E8FE0, 0x1E91C0, 0x1EABD0, 0x1EAC80, which main calls: the command line's taken up and let
 * go of). Only the static so far: the functions (0x1E8FE0-0x1EA24F, 0x1EABD0-0x1EB03F; 0x1E91C0
 * looks like SOCOM 1's testLoad, 0x1090 bytes) and the template tail (0x1EA250-0x1EABCF, the
 * pointer lists' and CNodeVector's pieces every later unit links to) stay with the drafts.
 * Defined in reverse of their image order (-inline deferred). */

#include <iostream>

#include "../../gamez/zar/zar.h"

/* the database's archive (SOCOM 1's file static) */
static zar::CZAR archive(0, 0);
