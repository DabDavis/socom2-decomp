/* st_menu.cpp -- the front end's menus (SOCOM 1's Apps/FTS/st_menu.cpp; OVL1
 * 0x1F3B40-0x1F5C2F). Defined in reverse of their image order (-inline deferred). */

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
void func_0030E300(void *net);
void func_00356080(void *texmanager, bool all);  /* zdb::CTexManager::DestroyBuffers */
void func_003654A0(void *reloc);                 /* (SOCOM 1 cleared g_relocDblBuf's two words) */
void func_0038D4E0();                            /* UnloadTheWorld */
void func_001E8FE0();
void *func_00274930();                           /* GetMenuStack */
void func_0027EDF0(void *stack, bool all);       /* CMenuStack::FlushHistory */
void func_0039C590(void *iop, int handle);       /* CIOPManager::UnRegisterRebootCallback */
void func_00350CC0(int arg);
void *func_002AFBA0();
void func_0033ED10(void *arg);
void *func_00361BC0();
void func_003A3740(void *snd, const char *bank);
void func_00366A00(CMenuStateMenu *menu, int arg);   /* CGameMenu::RestartBackgroundMPEG */
void func_00366B10(CMenuStateMenu *menu);            /* CGameMenu::HaltBackgroundMPEG */

extern char data_0049E880[];                     /* zdb::CTexManager::m_texmanager */
extern char data_00408DC0[];                     /* g_relocDblBuf (the unit's) */
extern zdb::CAssetLib *data_00408DB8;            /* g_pSkbLib (the unit's) */
extern CZAnimZAR *data_00408DE8;                 /* animfile (the unit's) */
extern char data_0049E2A0[];                     /* theIOPManager */
extern unsigned char data_00488ED4;              /* thePipe's (SOCOM 1: +0x640) */
}

extern unsigned char theNetwork[];
extern unsigned char zVid[];

void zUISchedulerClear();

const char *CMenuStateMenu::GetReaderFileName()
{
    return theGame.m_menuState.m_DefaultMenu;
}

/* back on top: the background film restarted, the menu's pop event run */
void CMenuState::OnPop(int userdata)
{
    func_00366A00(m_menu, 0);
    RunEventAnim("ON_STATE_POP", 0);
    m_skip_swap = 2;
}

/* covered: the background film halted */
void CMenuState::OnPush(int userdata)
{
    func_00366B10(m_menu);
}

/* torn down: offline, the network's let go of; the texture buffers, the menu, the soft keyboard's
 * library, the world, the menu history, the UI's scheduler, the reboot callback, the menu's
 * anims, the UI's sound bank */
void CMenuState::UnInit()
{
    if (!theNetwork[9])
        func_0030E300(theNetwork + 8);
    zSysFifoEnd();
    zVid[0x7d] = 1;
    sceGsSyncPath(0, 0);
    func_00356080(data_0049E880, false);
    func_003654A0(data_00408DC0);
    delete m_menu;
    m_menu = 0;
    zdb::CAssetMgr::ReleaseLibRef(data_00408DB8);
    data_00408DB8 = 0;
    func_0038D4E0();
    func_001E8FE0();
    func_0027EDF0(func_00274930(), true);
    zUISchedulerClear();
    SelectionSound = 0;
    func_0039C590(data_0049E2A0, m_iopregh);
    data_00488ED4 = 0;
    func_00350CC0(0);
    if (data_00408DE8) {
        ZAnim.CloseZAR(data_00408DE8);
        data_00408DE8 = 0;
    }
    func_0033ED10(func_002AFBA0());
    func_003A3740(func_00361BC0(), "UIMn");
}
