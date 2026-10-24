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

extern char data_00408DC0[];                     /* g_relocDblBuf (the unit's) */
extern zdb::CAssetLib *data_00408DB8;            /* g_pSkbLib (the unit's) */
extern CZAnimZAR *data_00408DE8;                 /* animfile (the unit's) */
extern char data_0049E2A0[];                     /* theIOPManager */
extern unsigned char data_00488ED4;              /* thePipe's (SOCOM 1: +0x640) */
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

extern char data_00437690[];                     /* the network's message queue */
extern int data_0049E8B0;                        /* zdb::CTexManager::TexPhase */
extern int data_00408DD4;
extern int data_00408DD8;
extern int data_00408DDC;
}

extern unsigned char theNetwork[];
extern unsigned char zVid[];

void zUISchedulerClear();

const char *CMenuStateMenu::GetReaderFileName()
{
    return theGame.m_menuState.m_DefaultMenu;
}

/* a frame of the menus: the film, input, sound, the menu ticked, the network's messages shown,
 * the menu's backplane, the world behind it (if one is loaded) and the foreplane drawn between
 * texture loads, and the frame swapped -- not waited for while swaps are skipped.
 * OPEN (the size and every instruction's kind right): the saved registers -- the target keeps
 * this in s2, the queue in s1 and the message's text in s0 (this here takes s0), and keeps the
 * second and third of the three words cleared after each texture load in s1/s0 too (here each
 * store makes its own address); no declaration order or local moved them */
bool CMenuState::Tick(float dT)
{
    CQueuedMessage msg;

    func_00366940(m_menu);
    func_002D9E40(dT);
    func_00341A60(dT, 0);
    func_00341250(dT);
    func_003654C0(m_menu, dT);
    void *queue = data_00437690;

    func_002A4290(&msg);
    while (func_002A4270(queue)) {
        func_002A3E60(queue, &msg);
        Slot2C(msg.text, 0, 0, 0, 0, 1, 0, msg.type);
    }
    data_004887C0.m_uiNode = m_menu->m_pBackplaneNode;
    data_004887C0.m_renderingUi = 1;
    data_004887C0.RenderUiNode(m_menu->m_pBackplaneNode);
    data_004887C0.m_renderingUi = 0;
    func_00356BE0(data_0049E880.data_[0], true);
    data_00408DD4 = 0;
    data_00408DD8 = 0;
    data_00408DDC = 0;
    if (zdb::CWorld::m_world) {
        func_0033BF30(&data_004887C0, data_0049E880.data_[data_0049E8B0]);
        if (zdb::CWorld::m_world->m_camera)
            zdb::CWorld::m_world->m_camera->Update(zdb::ZCAM_TYPE_0);
        func_00339DE0(&data_004887C0, zdb::CWorld::m_world);
    }
    m_menu->DrawForeplane();
    func_00356BE0(data_0049E880.data_[0], true);
    data_00408DD4 = 0;
    data_00408DD8 = 0;
    zSysFifoEnd();
    data_00408DDC = 0;
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
    if (!theNetwork[9])
        func_0030E300(theNetwork + 8);
    zSysFifoEnd();
    zVid[0x7d] = 1;
    sceGsSyncPath(0, 0);
    func_00356080(&data_0049E880, false);
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
