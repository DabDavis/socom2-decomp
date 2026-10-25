/* st_menu.cpp -- the front end's menus (SOCOM 1's Apps/FTS/st_menu.cpp; OVL1
 * 0x1F3B40-0x1F5C2F). Defined in reverse of their image order (-inline deferred). */

#include "fts_state.h"

/* unnamed yet (SOCOM 1's names in the comments) */
extern "C" {
void func_0030E300(void *net);
void func_00356080(TexBufferVector *texmanager, bool all);  /* zdb::CTexManager::DestroyBuffers */
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

}

/* a message from the network's queue: its type, then its text */
struct CQueuedMessage {
    char type;
    char text[0xaf];
};

extern "C" {
void func_00366940(CMenuStateMenu *menu);        /* CGameMenu::TickMPEG */
void func_002D9E40(float dT);                    /* CInput::Tick */
void func_00341A60(float dT, void *data);        /* CSndInstance::PostTick */
void func_00341250(float dT);
void func_003654C0(CGameMenu *menu, float dT);   /* CGameMenu::Tick */
void func_002A4290(CQueuedMessage *msg);
bool func_002A4270(void *queue);
void func_002A3E60(void *queue, CQueuedMessage *msg);
void func_00356BE0(void *texbuffer, bool now);   /* zdb::CGSTexBuffer::Load */
void func_0033BF30(CPipe *pipe, void *texbuffer);   /* CPipe::FIFO_Signal */
void func_00339DE0(CPipe *pipe, zdb::CWorld *world);   /* CPipe::RenderWorld */
void func_003AFF30(bool a, bool b);              /* zVid_Swap */

}

extern char theIOPManager[];

namespace {
/* the menus' double-buffered texture relocation (SOCOM 1's CDblBufTextureRelocator, 0x1C bytes:
 * its saved packet, size and palette; SOCOM II's are three words at 0x14, cleared after each
 * texture load -- the name of the clear ours) */
struct CRelocDblBuf {
    unsigned char _0[0x14];
    int m_saved[3];   // 0x14

    void ClearSaved()
    {
        for (int i = 0; i < 3; i++)
            m_saved[i] = 0;
    }
};

zdb::CAssetLib *g_pSkbLib;
CRelocDblBuf g_relocDblBuf;
CZAnimZAR *animfile;
}

void zUISchedulerClear();

const char *CMenuStateMenu::GetReaderFileName()
{
    return theGame.m_menuState.m_DefaultMenu;
}

/* a frame of the menus: the film, input, sound, the menu ticked, the network's messages shown,
 * the menu's backplane, the world behind it (if one is loaded) and the foreplane drawn between
 * texture loads -- the relocator's saved packet cleared after each -- and the frame swapped, not
 * waited for while swaps are skipped. (SOCOM 1 cleared the relocator through a virtual; SOCOM II's
 * clear is an inline loop over three words, which is what keeps the second and third words'
 * addresses in s1/s0 for the second clear and, being the last values made, puts this in s2) */
bool CMenuState::Tick(float dT)
{
    CQueuedMessage msg;

    func_00366940(m_menu);
    func_002D9E40(dT);
    func_00341A60(dT, 0);
    func_00341250(dT);
    func_003654C0(m_menu, dT);
    void *queue = theMission.m_netQueue;

    func_002A4290(&msg);
    while (func_002A4270(queue)) {
        func_002A3E60(queue, &msg);
        DisplayMessage(msg.text, 0, 0, 0, 0, 1, 0, msg.type);
    }
    thePipe.m_uiNode = m_menu->m_pBackplaneNode;
    thePipe.m_renderingUi = 1;
    thePipe.RenderUiNode(m_menu->m_pBackplaneNode);
    thePipe.m_renderingUi = 0;
    func_00356BE0(zdb::CTexManager::m_texmanager.data_[0], true);
    g_relocDblBuf.ClearSaved();
    if (zdb::CWorld::m_world) {
        func_0033BF30(&thePipe, zdb::CTexManager::m_texmanager.data_[zdb::CTexManager::TexPhase[0]]);
        if (zdb::CWorld::m_world->m_camera)
            zdb::CWorld::m_world->m_camera->Update(zdb::ZCAM_TYPE_0);
        func_00339DE0(&thePipe, zdb::CWorld::m_world);
    }
    m_menu->DrawForeplane();
    func_00356BE0(zdb::CTexManager::m_texmanager.data_[0], true);
    g_relocDblBuf.ClearSaved();
    zSysFifoEnd();
    if (m_skip_swap > 0) {
        m_skip_swap--;
        func_003AFF30(false, true);
    } else {
        func_003AFF30(true, true);
    }
    return false;
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
    if (!theNetwork.m_9)
        func_0030E300(&theNetwork.m_8);
    zSysFifoEnd();
    zVid.m_7d = 1;
    sceGsSyncPath(0, 0);
    func_00356080(&zdb::CTexManager::m_texmanager, false);
    func_003654A0(&g_relocDblBuf);
    delete m_menu;
    m_menu = 0;
    zdb::CAssetMgr::ReleaseLibRef(g_pSkbLib);
    g_pSkbLib = 0;
    func_0038D4E0();
    func_001E8FE0();
    func_0027EDF0(func_00274930(), true);
    zUISchedulerClear();
    SelectionSound = 0;
    func_0039C590(theIOPManager, m_iopregh);
    thePipe.m_714 = 0;
    func_00350CC0(0);
    if (animfile) {
        ZAnim.CloseZAR(animfile);
        animfile = 0;
    }
    func_0033ED10(func_002AFBA0());
    func_003A3740(func_00361BC0(), "UIMn");
}
