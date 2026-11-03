/* st_menu.cpp -- the front end's menus (SOCOM 1's Apps/FTS/st_menu.cpp; OVL1
 * 0x1F3B40-0x1F5C2F). Defined in reverse of their image order (-inline deferred). */

#include "fts_state.h"

/* unnamed yet (SOCOM 1's names in the comments) */
extern "C" {
void func_0030E300(void *net);
void func_003654A0(void *reloc);                 /* (SOCOM 1 cleared g_relocDblBuf's two words) */
void func_001E8FE0();

}

/* the lobby's save manager (zsav_lobby.h has the class; fts_mission.cpp's accessor): only its close */
class CLobbySaveManager {
public:
    void Close();   /* 0x33ED10 */
};
CLobbySaveManager *GetLobbySaveManager();   /* 0x2AFBA0 */

/* a message from the network's queue: its type, then its text */
struct CQueuedMessage {
    MSGTYPE type;
    char text[0xaf];
};

extern "C" {
void func_00341250(float dT);
void func_003654C0(CGameMenu *menu, float dT);   /* CGameMenu::Tick */
void func_002A4290(CQueuedMessage *msg);
bool func_002A4270(void *queue);
void func_002A3E60(void *queue, CQueuedMessage *msg);
void func_0033BF30(CPipe *pipe, void *texbuffer);   /* CPipe::FIFO_Signal */
void func_00339DE0(CPipe *pipe, zdb::CWorld *world);   /* CPipe::RenderWorld */
void func_003AFF30(bool a, bool b);              /* zVid_Swap */

}

/* the localised string tables, the menu history, the IOP's reboot callbacks, the UI and the GS
 * FIFO's sync (their units'; only what is called) */
class CStringTables {
public:
    void Unload(const char *name);   /* 0x3A3740 */
};
CStringTables *GetStringTables();   /* 0x361BC0 */

class CMenuStack {
public:
    void FlushHistory(bool all);   /* 0x27EDF0 */
};
CMenuStack *GetMenuStack();   /* 0x274930 */

class CIOPManager {
public:
    void UnRegisterRebootCallback(int handle);   /* 0x39C590 */
};
extern CIOPManager theIOPManager;

void ZuiUnInit();   /* 0x38D4E0 */
void zSysFifoSetSync(bool on);   /* 0x350CC0 */

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

/* the two ways a message reaches the player (0x1F5020 while the game mode byte is 11, 0x1F4DF0
 * otherwise: unwritten) */
extern "C" {
void func_001F4DF0(const char *text, const char *who, int a, int b, int c, unsigned char d,
                   unsigned char e, MSGTYPE type);
void func_001F5020(const char *text, const char *who, int a, int b, int c, unsigned char d,
                   unsigned char e, MSGTYPE type);
}
extern unsigned char data_0044F3D0;

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
/* a message for the player (CGameState's slot 0x2C): a system message of the multiplayer kind is
 * the menu's MP_SYSTEM_MESSAGE event, any other goes to one of two displays by the mode byte (a
 * switch: the target tests the one case and branches to the default) */
void CMenuState::DisplayMessage(const char *text, const char *who, int a, int b, int c,
                                unsigned char d, unsigned char e, MSGTYPE type)
{
    if (type == 1 && d == 2) {
        RunEventAnim("MP_SYSTEM_MESSAGE", text);
        return;
    }
    switch (data_0044F3D0) {
    case 11:
        func_001F5020(text, who, a, b, c, d, e, type);
        break;
    default:
        func_001F4DF0(text, who, a, b, c, d, e, type);
        break;
    }
}

bool CMenuState::Tick(float dT)
{
    CQueuedMessage msg;

    m_menu->TickMPEG();
    CInput::Tick(dT);
    CSnd::PostTick(dT, 0);
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
    zdb::CTexManager::m_texmanager.m_buffer.data_[0]->Load(true);
    g_relocDblBuf.ClearSaved();
    if (zdb::CWorld::m_world) {
        func_0033BF30(&thePipe, zdb::CTexManager::m_texmanager.m_buffer.data_[zdb::CTexManager::TexPhase[0]]);
        if (zdb::CWorld::m_world->m_camera)
            zdb::CWorld::m_world->m_camera->Update(zdb::ZCAM_TYPE_0);
        func_00339DE0(&thePipe, zdb::CWorld::m_world);
    }
    m_menu->DrawForeplane();
    zdb::CTexManager::m_texmanager.m_buffer.data_[0]->Load(true);
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
    m_menu->RestartBackgroundMPEG(0);
    RunEventAnim("ON_STATE_POP", 0);
    m_skip_swap = 2;
}

/* covered: the background film halted */
void CMenuState::OnPush(int userdata)
{
    m_menu->HaltBackgroundMPEG();
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
    zdb::CTexManager::DestroyBuffers(zdb::CTexManager::m_texmanager, false);
    func_003654A0(&g_relocDblBuf);
    delete m_menu;
    m_menu = 0;
    zdb::CAssetMgr::ReleaseLibRef(g_pSkbLib);
    g_pSkbLib = 0;
    ZuiUnInit();
    func_001E8FE0();
    GetMenuStack()->FlushHistory(true);
    zUISchedulerClear();
    SelectionSound = 0;
    theIOPManager.UnRegisterRebootCallback(m_iopregh);
    thePipe.m_714 = 0;
    zSysFifoSetSync(false);
    if (animfile) {
        ZAnim.CloseZAR(animfile);
        animfile = 0;
    }
    GetLobbySaveManager()->Close();
    GetStringTables()->Unload("UIMn");
}
