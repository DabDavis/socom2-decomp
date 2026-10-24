/* st_core.cpp -- the core state: the mission's load, its tick, its teardown (SOCOM 1's
 * Apps/FTS/st_core.cpp, its DWARF's line order; OVL1 0x1EB070-0x1EDFFF). Defined in reverse of
 * their image order (-inline deferred). */

#include "fts_state.h"

/* unnamed yet (SOCOM 1's names in the comments) */
extern "C" {
void func_00356BE0(void *texbuffer, bool now);   /* zdb::CGSTexBuffer::Load */
void func_001F1820(float goal);                  /* CameraZoom::SetGoal */
void func_001F1870(void *seal);                  /* CameraZoom::AttachSeal */
void *func_002B3580();                           /* ftsGetPlayer */
void func_002A7D40(void *mission);               /* CMission::OnMissionStart */
void func_001FB420(CHUD *hud);                   /* CHUD::ReInit_Init */
void func_00356130(void *texmanager);            /* zdb::CTexManager::doSetupBuffers */
int func_001EA720();                             /* testLoad */
void func_00192BF8(int code);                    /* exit */
void func_00223290(void *state);                 /* CMPExitState::LoadLibs */
void func_002A6950(void *mission);
void func_003B17D0(int rate);                    /* CVideo::MaxFrameRate */
void func_003B1760(int a, int b, int c, float d);
void func_00351CB0();                            /* CValve::Reset */
void func_005B61A0();

extern unsigned char data_004A4511;
extern int data_0049E940;                        /* zdb::CTexManager::TexPhase's */
extern int data_0049E950;
extern unsigned char data_0045A0C1;              /* theNetwork's +9 (an online game) */
extern void *data_00408C10;                      /* the load thread's stack */
}

void zVid_Sync0();
void ftsNetworkGameUninit();
void DMEDisconnectFromGameServer();

class CZProjectileList {
public:
    void UnInit();
};

class CZMarkList {
public:
    void UnInit();
};

class CZWeapon {
public:
    static CZProjectileList *m_pProjectilelist;
};

class CEntity {
public:
    static bool PreTickAll(float dT, void *data);
};

/* the app's camera: its zdb camera at 0xB4 (SOCOM 1's at 0x34) */
class CAppCamera {
public:
    unsigned char _0[0xb4];
    zdb::CCamera *m_camera;   // 0xb4
};

const char *GetDatabase();

extern "C" {
CHUD *func_001FCD80(CHUD *hud);                  /* CHUD::CHUD */
void func_001FA360(CHUD *hud, zdb::CWorld *world);   /* CHUD::Init */
void func_0032C4F0();                            /* CRdrArchive::CloseAll */
void func_00356B90(void *texbuffer, int size);
void func_0031CED0(zdb::CWorld *world);
void func_0033B420(CPipe *pipe, zdb::CTexHandle *tex);

/* the tick tasks (SOCOM 1's in the comments, where it had them) */
bool func_005E7FD0(float dT, void *data);        /* CSealUnit::TickAll */
bool func_005CCCD0(float dT, void *data);        /* CSealCtrlAi::PreTick */
bool func_003D1F90(float dT, void *data);        /* CZWeapon::PreTick */
bool func_0050ECD0(float dT, void *data);        /* CAiEvent::Tick */
bool func_0029F430(float dT, void *data);        /* CEntity::TickAll */
bool func_0054F570(float dT, void *data);
bool func_0029F350(float dT, void *data);        /* CSealCtrlAi::PrePostTick's place */
bool func_0060BC40(float dT, void *data);
bool func_0029F1A0(float dT, void *data);        /* CEntity::PostTickAll */
bool func_003D1F50(float dT, void *data);        /* CZWeapon::PostTick */
bool func_00510570(float dT, void *data);        /* ai::TickPathPlanner */
bool func_0060B8F0(float dT, void *data);
bool func_005CCA30(float dT, void *data);        /* CSealCtrlAi::PostTick */
bool func_00341A60(float dT, void *data);        /* CSnd::PostTick */
bool func_00299C10(float dT, void *data);

extern CAppCamera *data_00415FF0;                /* appCamera */
extern zdb::CCamera *data_004B4810;              /* zdb::CVisual::m_camera */
extern unsigned char data_003DC3D0;              /* gopt's first byte */
extern int data_003E13F8;                        /* CVideo::m_defaultFrameRate */
extern int data_0049E934;                        /* zdb::CTexManager::TexPhase's */
extern int data_0049E938;
}

extern CHUD *theHUD;

extern "C" {
int func_001EA6E0();                             /* testUnload */
void func_001F1840(CHUD *hud);                   /* CameraZoom::AttachHud */
void func_001FCB70(CHUD **hud);                  /* CHUD::ReleaseHUD */
void func_002B3590(void *seal);                  /* ftsSetPlayer */
void func_003D3930();                            /* CZWeapon::DestroyProjectileList */
void func_003D37E0();                            /* CZWeapon::DestroyMarkList */
void func_0032C2B0(const char *name, const char *path);   /* CRdrArchive::RemoveArchive */

extern CZMarkList *data_004B5248;                /* CZWeapon::m_pMarklist */
}

/* zdb's task scheduler: a list of tasks (SOCOM 1's DWARF: 0x20 bytes) */
class CSched_Manager {
public:
    bool AddTask(const char *name, bool (*task)(float, void *), float priority, void *data);
    bool RemoveTask(const char *name, bool all);
    void ClearFreeList(bool all);

    unsigned char _0[0x20];
};

extern CSched_Manager zTaskScheduler;

extern "C" void func_00356080(TexBufferVector *texmanager, bool all);   /* zdb::CTexManager::DestroyBuffers */

static void LoadThreadEntry(void *arg);

/* the film's name let go of */
CCinematicState::~CCinematicState()
{
    if (m_moviename)
        _free(m_moviename, 0, 0);
    m_moviename = 0;
}

/* a pad's actuators stopped (the first pad's) */
void CCoreState::OnPush(int userdata)
{
    zSysFifoEnd();
    sceGsSyncPath(0, 0);
    CPad *pad = CInput::m_pads[0];

    pad->m_ActData[1] = 0;
    pad->m_ActData[0] = 0;
}

/* back on top: the two texture phases' buffers loaded again, and from a sub-state (1) the mission
 * reopened and restarted, the camera's zoom reset onto the player, the HUD set up again */
void CCoreState::OnPop(int userdata)
{
    func_00356BE0(data_0049E880.data_[data_0049E940], true);
    func_00356BE0(data_0049E880.data_[data_0049E950], true);
    zVid_Sync0();
    theMission.m_108 = 10.0f;
    if (userdata == 1) {
        func_001F1820(1.0f);
        theMission.ReOpen_Load();
        func_002A7D40(&theMission);
        func_001F1870(func_002B3580());
        func_001FB420(m_hud);
    }
    zVid_Sync0();
    m_skip_swap = 2;
}

CCoreState::CCoreState() : CGameState("CoreState")
{
    m_loading = false;
    m_loadstarted = false;
    m_skip_swap = 0;
    m_c0 = 1.0f;
    m_c4 = 1.0f / m_c0;
    m_c8 = m_c0;
    m_LoadThreadId = -1;
}

/* the base's destructor called in the body, so it runs twice: SOCOM 1's and SOCOM II's both inline
 * CGameState's teardown (vtable, name freed) two times over */
CCoreState::~CCoreState()
{
    CGameState::~CGameState();
}

/* the asset manager's handle for a texture by name (out of line: every caller calls it) */
#pragma auto_inline off
zdb::CTexHandle *zdb::CWorld::GetTexHandle(const char *name)
{
    return CAssetMgr::m_assets.GetTexHandle(name);
}

#pragma auto_inline reset

/* the mission set up: the world and its camera, the HUD, the player under the camera, the
 * reader archives, the tick tasks, the camera's far plane, room for 300 more nodes, the texture
 * buffers, the mission started, the pipe's noise texture, the GIF's path 3 opened, the frame rate
 * back to the default */
bool CCoreState::Init()
{
    m_world = zdb::CWorld::m_world;
    data_004B4810 = data_004887C0.m_camera = m_world->m_camera;
    CHUD *hud = (CHUD *)operator new(0x1acd0);

    if (hud)
        hud = func_001FCD80(hud);
    theHUD = m_hud = hud;
    if (data_003DC3D0) {
        CHUD &h = *m_hud;

        func_001FA360(&h, m_world);
        func_001F1840(&h);
    }
    func_001F1870(func_002B3580());
    func_0032C4F0();
    func_0032C2B0("readerm.zar", GetDatabase());
    zTaskScheduler.AddTask("UnitTick", func_005E7FD0, 0.8f, 0);
    zTaskScheduler.AddTask("ai_pre_tick", func_005CCCD0, 0.9f, 0);
    zTaskScheduler.AddTask("entity_pre_tick", CEntity::PreTickAll, 1.0f, 0);
    zTaskScheduler.AddTask("weapon_pre_tick", func_003D1F90, 3.0f, 0);
    zTaskScheduler.AddTask("CAiEventTick", func_0050ECD0, 4.5f, 0);
    zTaskScheduler.AddTask("entity_tick", func_0029F430, 4.6f, 0);
    zTaskScheduler.AddTask("body_collide_tick", func_0054F570, 4.65f, 0);
    zTaskScheduler.AddTask("entity_preposttick", func_0029F350, 4.7f, 0);
    zTaskScheduler.AddTask("target_pretick", func_0060BC40, 4.8f, 0);
    zTaskScheduler.AddTask("entity_post_tick", func_0029F1A0, 6.0f, 0);
    zTaskScheduler.AddTask("weapon_post_tick", func_003D1F50, 8.0f, 0);
    zTaskScheduler.AddTask("ai_route_planner", func_00510570, 10.5f, 0);
    zTaskScheduler.AddTask("target_post_tick", func_0060B8F0, 10.6f, 0);
    zTaskScheduler.AddTask("ai_posttick", func_005CCA30, 10.7f, 0);
    zTaskScheduler.AddTask("sound_post_tick", func_00341A60, 11.0f, 0);
    zTaskScheduler.AddTask("appCamera_tick", func_00299C10, 12.0f, 0);
    data_00415FF0->m_camera->GetLandmarkFarPlane(m_world);
    std::vector<zdb::CNode *> &nodes = m_world->m_nodes;
    unsigned int n = nodes.size() + 300;

    if (nodes.capacity() < n)
        nodes.reserve(n);
    func_00356B90(data_0049E880.data_[data_0049E934], 2048);
    func_00356B90(data_0049E880.data_[data_0049E938], 2048);
    func_0031CED0(m_world);
    func_00356BE0(data_0049E880.data_[data_0049E940], true);
    func_00356BE0(data_0049E880.data_[data_0049E950], true);
    func_002A7D40(&theMission);
    theMission.m_e8 = -1.0f;
    func_0033B420(&data_004887C0, zdb::CWorld::GetTexHandle("noise50.tif"));
    *(volatile unsigned int *)0x10003010 |= 4;
    zSysFifoEnd();
    data_004A4511 = 0;
    func_003B17D0(data_003E13F8);
    zVid_Sync0();
    return true;
}

/* torn down: the GIF's path 3 masked again, the frame rate freed, the mission unloaded, the
 * valves reset and the scheduler's free list cleared */
void CCoreState::UnInit()
{
    *(volatile unsigned int *)0x10003010 &= ~4;
    asm volatile {
        sync
    }
    func_003B17D0(-1);
    func_003B1760(0, 0, 0, 1.0f);
    data_004A4511 = 1;
    m_loading = false;
    m_loadstarted = false;
    auxUnload();
    func_00351CB0();
    zTaskScheduler.ClearFreeList(true);
    func_005B61A0();
}

/* the load thread: the sky's colour, the texture buffers, the load itself (exiting when it
 * fails), an online game's exit screens, then the mission's own */
static void LoadThreadEntry(void *arg)
{
    theGame.m_coreState.skyColor[0] = 190;
    theGame.m_coreState.skyColor[1] = 220;
    theGame.m_coreState.skyColor[2] = 250;
    func_00356130(&data_0049E880);
    if (!func_001EA720())
        func_00192BF8(0);
    if (data_0045A0C1)
        func_00223290(&theGame.m_MPexitState);
    theGame.m_coreState.m_loading = false;
    func_002A6950(&theMission);
    ExitDeleteThread();
}

/* the load thread started on a stack of its own (allocated once, its ends marked). Written so
 * it is 15 statements and Load would inline it; the original's had more (Load calls it) */
#pragma auto_inline off
void CCoreState::StartLoadThread()
{
    ThreadParam param;

    if (!data_00408C10)
        data_00408C10 = _memalign(16, 0x11000, 0, 0);
    memset(data_00408C10, 250, 2048);
    memset((char *)data_00408C10 + 0x10800, 250, 2048);
    memset((char *)data_00408C10 + 2048, 234, 0x10000);
    param.entry = LoadThreadEntry;
    param.option = 0;
    param.stackSize = 0x10000;
    param.initPriority = 6;
    param.gpReg = data_001DD2F0;
    param.stack = (char *)data_00408C10 + 2048;
    m_LoadThreadId = CreateThread(&param);
    if (m_LoadThreadId)
        StartThread(m_LoadThreadId, this);
}

#pragma auto_inline reset

/* the load thread's stack let go of (the name ours) */
void CCoreState::FreeLoadThread()
{
    m_LoadThreadId = -1;
    if (data_00408C10)
        _free(data_00408C10, 0, 0);
    data_00408C10 = 0;
}

/* the mission loaded: on its own thread, or here and now */
void CCoreState::Load(bool async)
{
    m_loading = true;
    m_loadstarted = true;
    zsys_FullAllocAndFree();
    if (async) {
        StartLoadThread();
        return;
    }
    skyColor[0] = 190;
    skyColor[1] = 220;
    skyColor[2] = 250;
    func_00356130(&data_0049E880);
    if (!func_001EA720())
        func_00192BF8(0);
    if (data_0045A0C1)
        func_00223290(&theGame.m_MPexitState);
    m_loading = false;
}

void CCoreState::Unload()
{
    m_loading = false;
    m_loadstarted = false;
    auxUnload();
}

/* the mission let go of: an online game's network, the tick tasks, the projectiles and the marks,
 * the loaded data (exiting if that fails), the world's texture buffers, the HUD, the player, the
 * camera's seal, the weapons' lists, the mission's reader archive, the scheduler's free list */
void CCoreState::auxUnload()
{
    if (data_0045A0C1 && theMission.m_18 == 4) {
        ftsNetworkGameUninit();
        DMEDisconnectFromGameServer();
    }
    zTaskScheduler.RemoveTask("CAiEventTick", false);
    zTaskScheduler.RemoveTask("UnitTick", false);
    zTaskScheduler.RemoveTask("ai_pre_tick", false);
    zTaskScheduler.RemoveTask("ai_posttick", false);
    zTaskScheduler.RemoveTask("entity_pre_tick", false);
    zTaskScheduler.RemoveTask("target_pretick", false);
    zTaskScheduler.RemoveTask("target_post_tick", false);
    zTaskScheduler.RemoveTask("weapon_pre_tick", false);
    zTaskScheduler.RemoveTask("ai_post_tick", false);
    zTaskScheduler.RemoveTask("ai_direg_tick", false);
    zTaskScheduler.RemoveTask("entity_post_tick", false);
    zTaskScheduler.RemoveTask("entity_tick", false);
    zTaskScheduler.RemoveTask("body_collide_tick", false);
    zTaskScheduler.RemoveTask("entity_preposttick", false);
    zTaskScheduler.RemoveTask("weapon_post_tick", false);
    zTaskScheduler.RemoveTask("ai_route_planner", false);
    zTaskScheduler.RemoveTask("sound_post_tick", false);
    zTaskScheduler.RemoveTask("light_tick", false);
    zTaskScheduler.RemoveTask("appCamera_tick", false);
    CZProjectileList *proj = CZWeapon::m_pProjectilelist;
    CZMarkList *marks = data_004B5248;

    if (proj)
        proj->UnInit();
    if (!func_001EA6E0())
        func_00192BF8(0);
    if (marks)
        marks->UnInit();
    m_world = 0;
    func_00356080(&data_0049E880, false);
    func_001F1840(0);
    func_001FCB70(&m_hud);
    func_002B3590(0);
    func_001F1870(0);
    func_003D3930();
    func_003D37E0();
    func_0032C2B0("readerm.zar", 0);
    zTaskScheduler.ClearFreeList(true);
}
