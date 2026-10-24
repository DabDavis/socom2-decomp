/* st_load.cpp -- the mission's load screen (SOCOM 1's Apps/FTS/st_load.cpp, without the E3
 * build's mission config; OVL1 0x1F5D00-0x1F6A8F). Defined in reverse of their image order
 * (-inline deferred). */

#include "fts_state.h"

namespace zdb {
class CAssetLib;
class CAssetMgr {
public:
    static void ReleaseLibRef(CAssetLib *lib);
};
}

/* unnamed yet (SOCOM 1's names in the comments) */
extern "C" {
int InitThread(void);
void *func_00361BC0();
void func_003A3740(void *snd, const char *bank);
void func_00321D30();                            /* zdb::EmptyAssetRecycleBin */
void func_00385C30(void *vars);                  /* CUIVarManager::Reset */

extern zdb::CAssetLib *data_00408E28;            /* the load screen's asset library */
}

extern char theUIVarManager[];

const char *CLoadMenu::GetReaderFileName()
{
    return "dlgLoad.rdr";
}

/* the load thread's kernel set up */
CLoadState::CLoadState() : CGameState("LoadState")
{
    m_menu = 0;
    InitThread();
}

CLoadState::~CLoadState()
{
    if (m_menu)
        delete m_menu;
    m_menu = 0;
}

/* torn down: the menu, the UI's sound bank, the screen's library, the recycled assets, the UI's
 * variables -- and an aborted load's mission unloaded (if it is not still loading), the load
 * thread's stack freed */
void CLoadState::UnInit()
{
    zSysFifoEnd();
    sceGsSyncPath(0, 0);
    if (m_menu)
        delete m_menu;
    m_menu = 0;
    func_003A3740(func_00361BC0(), "UILd");
    zdb::CAssetMgr::ReleaseLibRef(data_00408E28);
    data_00408E28 = 0;
    func_00321D30();
    func_00385C30(theUIVarManager);
    if (m_abort && theGame.m_coreState.IsLoaded())
        theGame.m_coreState.Unload();
    theGame.m_coreState.FreeLoadThread();
}
