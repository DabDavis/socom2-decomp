/* st_mpexit.cpp -- the end of an online game: the stats screen (SOCOM 1's
 * Apps/FTS/st_mpexit.cpp; OVL1 0x223290-0x2265F7). Defined in reverse of their image order
 * (-inline deferred). */

#include "fts_state.h"

/* unnamed yet */
extern "C" {
void func_001EEB40(CStateString *s, const char *text);   /* std::string::assign */
}

/* the final screen once the game is over, else the round's */
const char *CMPExitMenu::GetReaderFileName()
{
    return m_gameover ? "dlgMultiplayerFinal.rdr" : "dlgMultiplayerRound.rdr";
}

/* the splash screen's asset library named; no menu yet */
CMPExitState::CMPExitState() : CGameState("MPExitState")
{
    func_001EEB40(&m_libname, "common/assetlib/splash");
    m_menu = 0;
    m_28 = 0;
}
