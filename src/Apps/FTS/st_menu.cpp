/* st_menu.cpp -- the front end's menus (SOCOM 1's Apps/FTS/st_menu.cpp; OVL1
 * 0x1F3B40-0x1F5C2F). Defined in reverse of their image order (-inline deferred). */

#include "fts_state.h"

#include <iostream>

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

/* a message from the player's queue: its type, then its text (fts_mission.cpp's CQueuedMessage, its
 * constructor out of line there) */
class CQueuedMessage {
public:
    CQueuedMessage();   /* 0x2A4290 */

    MSGTYPE type;
    char text[0xaf];
};

/* the player's message queue (fts_mission.cpp's CMessageQueue, theMission's at 0x11B0: only what the
 * menu calls) */
class CMessageQueue {
public:
    bool HasMessages();   /* 0x2A4270 */
    void Dequeue(CQueuedMessage *msg);   /* 0x2A3E60 */
};

extern "C" {
void func_00341250(float dT);
void func_003654C0(CGameMenu *menu, float dT);   /* CGameMenu::Tick */
void func_003AFF30(bool a, bool b);              /* zVid_Swap */

}

/* the localised string tables, the menu history, the IOP's reboot callbacks, the UI and the GS
 * FIFO's sync (their units'; only what is called) */
enum LANGUAGE { LANGUAGE_0 };
LANGUAGE GetLanguage();   /* 0x3A4460 */

class CStringTables {
public:
    void Unload(const char *name);   /* 0x3A3740 */
    void Load(const char *name, LANGUAGE lang);   /* 0x3A3900 */
};
CStringTables *GetStringTables();   /* 0x361BC0 */

class CMenuStack {
public:
    void FlushHistory(bool all);   /* 0x27EDF0 */
};
CMenuStack *GetMenuStack();   /* 0x274930 */

class CIOPManager {
public:
    enum RebootPhase { REBOOT_PHASE_0 };
    enum RebootMode { REBOOT_MODE_0 };
    int RegisterRebootCallback(bool (*cb)(RebootPhase phase, RebootMode mode, void *data), void *data);
    void UnRegisterRebootCallback(int handle);   /* 0x39C590 */
};
extern CIOPManager theIOPManager;

void ZuiUnInit();   /* 0x38D4E0 */
void zSysFifoSetSync(bool on);   /* 0x350CC0 */

/* the menus' double-buffered texture relocator (zui.cpp's CDblBufTextureRelocator, its vtable
 * 0x406F60 there: SOCOM 1's 0x1C bytes with three GS buffers at 0x8 and each buffer's last texture
 * at 0x14, cleared after each texture load -- the name of the clear ours). Only its inline
 * constructor, which builds the unit's static in __sinit_st_menu.cpp 0x3FD940 (the buffer index
 * and the palette cleared, then the six words by zui.cpp's 0x3654A0), and the slots: declared,
 * not defined, so the unit emits no table */
class CDblBufTextureRelocator {
public:
    CDblBufTextureRelocator()
    {
        _iGsBufToFill = 0;
        _pSavedPalette = 0;
        func_003654A0(this);
    }

    virtual void AddToBatch(zdb::CTexHandle *tex) const;
    virtual void EndBatchReloc() const;
    virtual void PreGetSprPacket(zdb::CTexHandle *tex);
    virtual void StartBatchReloc() const;
    virtual void TakeDrawPacket(u_long128 *packet, unsigned int qwc);
    virtual void ClearAll();

    void ClearSaved()
    {
        for (int i = 0; i < 3; i++)
            _arrLastTex[i] = 0;
    }
    void Init();

    int _iGsBufToFill;                 // 0x4
    void *_arrGsBufs[3];               // 0x8
    zdb::CTexHandle *_arrLastTex[3];   // 0x14
    void *_pSavedPalette;              // 0x20
};

namespace {
zdb::CAssetLib *g_pSkbLib;
CDblBufTextureRelocator g_relocDblBuf;
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

/* CMenuState::Init's callees (their units'; only what is called) */
namespace zar {
class CZAR {
public:
    bool ReOpen(int dir, unsigned int mode);   /* 0x258B40 */
};
}

class CRdrArchive {
public:
    static zar::CZAR *AddArchive(const char *name, const char *path);   /* 0x32C1E0 */
};

/* the reader archive's directory number (OVL1 data) */
extern "C" int data_003F7020;


namespace zdb {
/* the camera's node (znode.h's CNode: only the placement) */
class CNode {
public:
    void SetPosition(float x, float y, float z);   /* 0x313A00 */
};
}

void ZuiInit();   /* 0x38D3C0 */
void HookupLib(zdb::CAssetLib *lib);
/* the count sound looked up (zui.cpp's UiLoadCountSound 0x369880, its row `__Fv`: the target loads
 * the menu into a0 for the test and the call -- the menu is its argument, which its body never
 * reads; spelled by the row's address until the row is renamed) */
extern "C" void func_00369880(CGameMenu *menu);
bool LoadWorld(const char *database);

struct _zrdr;
class CRdrFile;
_zrdr *zrdr_read(const char *name, const char *path, int flags);
_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);
char *zrdr_findstring(const _zrdr *rdr, const char *tag);
int zrdr_free(CRdrFile *file);

extern "C" {
void func_0030E340(void *net);
void func_00344090(const char *file, const char *bank);
void func_003448E0(const char *bank);
void func_0039D8C0();
bool func_001F53D0(CIOPManager::RebootPhase phase, CIOPManager::RebootMode mode, void *data);
}

/* a valve (zanim.h's CValve: only its creation and value) */
enum VALVE_TYPE { VALVE_ANY = 0, VALVE_PERM = 1, VALVE_TEMP = 2, VALVE_PERSIST = 3 };
enum VALVE_STATE { VALVE_SAME = 0, VALVE_ON = 1, VALVE_OFF = 2 };

class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);
    void MakeCallbacks(VALVE_STATE state);
    bool SetValue(int value)
    {
        VALVE_STATE state = VALVE_SAME;

        if (m_value != value)
            state = value == 0 ? VALVE_OFF : VALVE_ON;
        m_value = value;
        if (state != VALVE_SAME) {
            MakeCallbacks(state);
            return true;
        }
        return false;
    }

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
};

/* the network's byte at 0x109: whether its configuration is required (the name ours) */
extern "C" unsigned char data_0045A1C9;

/* the front end set up (OVL1 0x1F4860: SOCOM 1's, grown): skipped straight to the load state when
 * asked; the reader archive opened, the menu strings, the texture buffers, the "ui" world and its
 * camera, the UI, the keyboard and font libraries, the menu, the relocator, the sounds and banks,
 * the UI's ZAnim archive; the frame's sync set, the IOP's reboot callback registered and the
 * NETCNF_REQUIRED valve set from the network's flag. Byte-exact (round 35): the sound file read
 * twice, the second time into a reader of its own (rdr2, s3) with the selection defined after it
 * (s2) -- reusing the first reader's variable trades the three saved registers */
bool CMenuState::Init()
{
    func_0030E340(&theNetwork);
    CInput::Flush();
    if (m_skip) {
        theGame.Switch(&theGame.m_loadState, 0);
        return true;
    }

    zar::CZAR *zar = CRdrArchive::AddArchive("readerc.zar", "run/ui");

    if (zar)
        zar->ReOpen(data_003F7020, 1);
    GetStringTables()->Load("UIMn", GetLanguage());
    zdb::CTexManager::m_texmanager.doSetupBuffers();
    LoadWorld("ui");

    zdb::CWorld *world = zdb::CWorld::m_world;

    if (world) {
        zdb::CCamera *camera = world->m_camera;

        if (camera) {
            ((zdb::CNode *)camera)->SetPosition(320.0f, 0.0f, 320.0f);
            camera->GetLandmarkFarPlane(world);
            camera->Update(zdb::ZCAM_TYPE_0);
            thePipe.m_camera = camera;
            zdb::CVisual::m_camera = camera;
        }
    }
    ZuiInit();
    g_pSkbLib = zdb::CAssetMgr::GetLoadedLibRef("ui/assetlib/uisk");
    HookupLib(g_pSkbLib);
    zdb::CAssetMgr::GetLoadedLibRef("common/assetlib/font");
    m_menu = new CMenuStateMenu;
    g_relocDblBuf.Init();

    _zrdr *rdr = zrdr_read("uisounds.rdr", "data/common/dialog", 0);

    if (rdr) {
        _zrdr *sounds = zrdr_findtag(rdr, "SOUNDS");

        if (sounds) {
            TtySoundString = zrdr_findstring(sounds, "TELETYPE");
            BackSoundString = zrdr_findstring(sounds, "BACK");
        }
        zrdr_free((CRdrFile *)rdr);
    }
    func_00344090("sounds.rdr", "HUDUI");
    func_00344090("sounds.rdr", "SMUS");
    func_00344090("sounds.rdr", "UIVOICE");
    func_003448E0("UI");
    animfile = 0;
    animfile = ZAnim.OpenZAR("run/uizanim.zar");
    m_menu->Init((zdb::CTextureRelocMgr *)&g_relocDblBuf, animfile);

    _zrdr *rdr2 = zrdr_read("uisounds.rdr", "data/common/dialog", 0);
    const char *selection = 0;

    if (rdr2) {
        _zrdr *sounds = zrdr_findtag(rdr2, "SOUNDS");

        if (sounds) {
            TtySoundString = zrdr_findstring(sounds, "TELETYPE");
            BackSoundString = zrdr_findstring(sounds, "BACK");
            selection = zrdr_findstring(sounds, "SELECTION");
        }
    }
    SelectionSound = CSnd::GetSoundByName(selection ? selection : "");
    if (rdr2)
        zrdr_free((CRdrFile *)rdr2);
    if (m_menu)
        func_00369880(m_menu);
    thePipe.m_714 = 1;
    zSysFifoSetSync(true);
    zVid.m_7d = 0;
    zVid_Sync0();
    m_skip_swap = 0;
    func_0039D8C0();
    m_iopregh = theIOPManager.RegisterRebootCallback(func_001F53D0, 0);
    CValve::Create("NETCNF_REQUIRED", VALVE_TEMP)->SetValue(data_0045A1C9);
    return true;
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
    m_menu->TickMPEG();
    CInput::Tick(dT);
    CSnd::PostTick(dT, 0);
    func_00341250(dT);
    func_003654C0(m_menu, dT);
    CMessageQueue *queue = (CMessageQueue *)theMission.m_netQueue;
    CQueuedMessage msg;

    while (queue->HasMessages()) {
        queue->Dequeue(&msg);
        DisplayMessage(msg.text, 0, 0, 0, 0, 1, 0, msg.type);
    }
    thePipe.m_uiNode = m_menu->m_pBackplaneNode;
    thePipe.m_renderingUi = 1;
    thePipe.RenderUiNode(m_menu->m_pBackplaneNode);
    thePipe.m_renderingUi = 0;
    zdb::CTexManager::m_texmanager.m_buffer.data_[0]->Load(true);
    g_relocDblBuf.ClearSaved();
    if (zdb::CWorld::m_world) {
        thePipe.FIFO_Load(zdb::CTexManager::m_texmanager.m_buffer.data_[zdb::CTexManager::TexPhase[0]]);
        if (zdb::CWorld::m_world->m_camera)
            zdb::CWorld::m_world->m_camera->Update(zdb::ZCAM_TYPE_0);
        thePipe.RenderWorld(zdb::CWorld::m_world);
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
